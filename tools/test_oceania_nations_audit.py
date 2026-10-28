"""Pin OCEANIA_NATIONS source boundaries, every entry and its production ownership."""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import oceania_nations_audit
from oceania_nations_audit import MODULE, ROUTINES, INCLUDES, rewritten, require_oceania_nations_index, volatile_reads

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / ".local/current/export/instructions.sqlite"


@unittest.skipUnless(INPUT.exists(), "Private export unavailable")
class OceaniaNationsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.program = json.loads((INPUT.parent / "program.json").read_text())
        cls.segments = [(segment["address"], (INPUT.parent / segment["file"]).read_bytes())
                        for segment in cls.program["segments"] if "file" in segment]

    def original_bytes(self, address, size):
        for start, contents in self.segments:
            if start <= address and address + size <= start + len(contents):
                return contents[address - start:address - start + size]
        self.fail(f"Original bytes unavailable at {address:x}")

    def original_word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), "big")

    def assert_production_inventory(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(ROUTINES))
        for entry, _ in ROUTINES:
            self.assertEqual([owner for at, owner in owners if at == entry], ["oceania_nations_adapter"])

    def test_complete_original_inventory_and_production_ownership(self):
        self.assertEqual(len(ROUTINES), 15)
        self.assertEqual((ROUTINES[0][0], ROUTINES[-1][1]), MODULE)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(ROUTINES, ROUTINES[1:])))
        from compile_recomp import OWNER_REPLACEMENTS
        self.assert_production_inventory(rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(INPUT)) as db:
            require_oceania_nations_index(db)
            self.assertEqual(db.execute("SELECT COUNT(*) FROM instructions WHERE address>=? AND address<?",
                                        MODULE).fetchone()[0], 2315)

    def test_source_pool_and_neighbor_boundaries(self):
        toc = 0x108bba90
        for slot, address, source in ((-0x4098, 0x1089610c, "fifa_confed.cpp"),
                                      (-0x4094, 0x108963b4, "oceania_nations.cpp"),
                                      (-0x4090, 0x10896650, "inter_amer_cup.cpp")):
            self.assertEqual(self.original_word(toc + slot), address)
            self.assertEqual(self.original_bytes(address, len(source) + 1), source.encode() + b"\0")
        with closing(sqlite3.connect(INPUT)) as db:
            pool_loads = [(address, json.loads(body)["assembly"]) for address, body in db.execute(
                "SELECT address,body FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x4094(r2)%",))]
            self.assertEqual(pool_loads, [
                (0x106e9958, "lwz r31,-0x4094(r2)"),
                (0x106e9cc4, "lwz r31,-0x4094(r2)"),
                (0x106ea798, "lwz r4,-0x4094(r2)"),
                (0x106eadb0, "lwz r31,-0x4094(r2)"),
                (0x106eaee8, "lwz r31,-0x4094(r2)"),
                (0x106eafb8, "lwz r31,-0x4094(r2)"),
                (0x106eb1e8, "lwz r31,-0x4094(r2)"),
                (0x106eb43c, "lwz r21,-0x4094(r2)"),
                (0x106eb7b8, "lwz r31,-0x4094(r2)"),
                (0x106ebb24, "lwz r29,-0x4094(r2)"),
            ])
            at = lambda address: json.loads(db.execute(
                "SELECT body FROM instructions WHERE address=?", (address,)).fetchone()[0])["assembly"]
            self.assertEqual(at(0x106e9930), "blr")
            self.assertEqual(at(MODULE[0]), "mfspr r0,LR")
            self.assertEqual(at(MODULE[1]), "mfspr r0,LR")
            self.assertEqual(at(MODULE[1] + 0x18), "lwz r31,-0x4090(r2)")

    def test_each_entry_ends_in_return_and_unindexed_words_are_alignment(self):
        with closing(sqlite3.connect(INPUT)) as db:
            for entry, end in ROUTINES:
                with self.subTest(entry=hex(entry)):
                    records = [json.loads(body) for body, in db.execute(
                        "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address",
                        (entry, end))]
                    self.assertEqual(records[0]["assembly"], "mfspr r0,LR")
                    self.assertEqual(records[-1]["assembly"], "blr")
                    indexed = {r["address"] for r in records}
                    missing = set(range(entry, records[-1]["address"] + 4, 4)) - indexed
                    expected = {0x106e9ba0: {0x106e9bfc}, 0x106eb400: {0x106eb4d0}}.get(entry, set())
                    self.assertEqual(missing, expected)
                    for address in missing:
                        self.assertEqual(self.original_word(address), 0x60000000)  # unreachable nop
                        predecessor = next(r for r in records if r["address"] == address - 4)
                        self.assertEqual(predecessor["flow"], "UNCONDITIONAL_JUMP")
                        self.assertIsNone(predecessor.get("fallthrough"))
                    self.assertFalse(any(target in missing for r in records for target in r["flows"]))
                    last = records[-1]["address"] + 4
                    self.assertEqual(self.original_bytes(last, end - last), bytes(end - last))
                    self.assertTrue(end - last < 16)

    def test_internal_cross_entry_flows_are_only_calls_to_private_helpers(self):
        cross_entry = []
        with closing(sqlite3.connect(INPUT)) as db:
            for entry, end in ROUTINES:
                for body, in db.execute(
                        "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address",
                        (entry, end)):
                    record = json.loads(body)
                    for target in record["flows"]:
                        if MODULE[0] <= target < MODULE[1] and not entry <= target < end:
                            cross_entry.append((record["address"], record["flow"], target))
        self.assertEqual(cross_entry, [
            (0x106eaf80, "UNCONDITIONAL_CALL", 0x106eb1d0),
            (0x106eb8ac, "UNCONDITIONAL_CALL", 0x106eb400),
            (0x106ebac0, "UNCONDITIONAL_CALL", 0x106eafa0),
        ])

    def test_incoming_flows_have_no_extra_entries_or_interior_resumes(self):
        with closing(sqlite3.connect(INPUT)) as db:
            incoming = []
            # JSON filtering keeps the full export out of Python's memory.
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    "WHERE f.value>=? AND f.value<?", MODULE):
                if MODULE[0] <= address < MODULE[1]:
                    continue
                record = json.loads(body)
                incoming.append((address, record["flow"], tuple(record["flows"])))
        self.assertEqual(sorted(incoming), [(0x1034f044, "UNCONDITIONAL_CALL", (0x106e9940,))])

    def test_transition_vectors_and_vtable_close_indirect_entry_inventory(self):
        pointers = {}
        for start, contents in self.segments:
            # Every byte position is considered, including unaligned pointers.
            for match in re.finditer(b"\x10\x6e", contents):
                offset = match.start()
                if offset + 4 <= len(contents):
                    target = int.from_bytes(contents[offset:offset + 4], "big")
                    if MODULE[0] <= target < MODULE[1]:
                        pointers[start + offset] = target
        expected = {
            0x108bcfec: 0x106ebcd0,
            0x108bcff4: 0x106ebb10,
            0x108bcffc: 0x106eb7a0,
            0x108bd004: 0x106eaee0,
            0x108bd00c: 0x106eada0,
            0x108bd014: 0x106eac00,
            0x108bd01c: 0x106eabc0,
            0x108bd024: 0x106eaaf0,
            0x108bd02c: 0x106ea780,
            0x108bd034: 0x106e9cb0,
            0x108bd03c: 0x106e9ba0,
        }
        self.assertEqual(pointers, expected)
        for descriptor in expected:
            self.assertEqual(self.original_word(descriptor + 4), 0x108bba90)
        table = self.original_word(0x108bba90 - 0x5cc4)
        self.assertEqual(table, 0x108e0748)
        for offset, descriptor in (
            (0x8, 0x108bd03c),
            (0x10, 0x108bd00c),
            (0x1c, 0x108bcffc),
            (0x30, 0x108bd004),
            (0x38, 0x108bd01c),
            (0x44, 0x108bd034),
            (0x4c, 0x108bd014),
            (0x50, 0x108bd02c),
            (0x64, 0x108bcff4),
            (0x70, 0x108bcfec),
            (0x94, 0x108bd024),
        ):
            self.assertEqual(self.original_word(table + offset), descriptor)
        self.assertEqual({entry for entry, _ in ROUTINES} - set(pointers.values()),
                         {0x106e9940, 0x106eafa0, 0x106eb1d0, 0x106eb400})

    def test_audit_rejects_stale_digest(self):
        routines = rewritten(drafts=True)
        stale = [(a, b, "0000000000000000") if index == 0 else (a, b, digest)
                 for index, (a, b, digest) in enumerate(routines)]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(oceania_nations_audit, "rewritten", return_value=stale):
            with self.assertRaisesRegex(ValueError, "changed in the export"):
                require_oceania_nations_index(db)

    def test_audit_rejects_duplicate_entry_and_disabled_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        with tempfile.TemporaryDirectory() as directory:
            duplicate = Path(directory) / "routines.inc"
            contents = INCLUDES.read_text()
            first = next(line for line in contents.splitlines() if "OCEANIA_NATIONS_ROUTINE(" in line)
            duplicate.write_text(contents + "\n" + first + "\n")
            with self.assertRaisesRegex(ValueError, "listed twice"):
                rewritten(duplicate)
            disabled = Path(directory) / "disabled.inc"
            disabled.write_text(contents.replace("OCEANIA_NATIONS_ROUTINE(", "OCEANIA_NATIONS_DRAFT(", 1))
            with self.assertRaises(AssertionError):
                self.assert_production_inventory(rewritten(disabled), OWNER_REPLACEMENTS)
        owners_without_constructor = [(entry, owner) for entry, owner in OWNER_REPLACEMENTS
                                      if entry != ROUTINES[0][0]]
        with self.assertRaises(AssertionError):
            self.assert_production_inventory(rewritten(), owners_without_constructor)

    def test_default_generation_guards_every_entry_and_reference_bypasses_it(self):
        from compile_recomp import generate_page
        with closing(sqlite3.connect(INPUT)) as db:
            records = [json.loads(body) for body, in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address", MODULE)]
        selected = {r["address"] for r in records}
        for page in sorted({entry >> 12 for entry, _ in ROUTINES}):
            part = [r for r in records if r["address"] >> 12 == page]
            normal = generate_page(page, part, selected)
            for entry, _ in ROUTINES:
                if entry >> 12 == page:
                    self.assertIn(f"if (try_modern_oceania_nations(s, 0x{entry:x})) return true;", normal)
            self.assertIn('#include "oceania_nations_adapter.hpp"', normal)
            reference = generate_page(page, part, selected, oceania_nations_hooks=False)
            self.assertNotIn("try_modern_oceania_nations", reference)

    def test_no_unmodelled_scratch_register_dependencies(self):
        with closing(sqlite3.connect(INPUT)) as db:
            self.assertEqual(volatile_reads(db), [])

    def test_virtual_dispatch_explicitly_loads_original_descriptor(self):
        with closing(sqlite3.connect(INPUT)) as db:
            records = [json.loads(body) for body, in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address", MODULE)]
        calls = [r for r in records if r["assembly"] == "bl 0x10781320"]
        self.assertEqual(len(calls), 10)
        for call in calls:
            loads = [r for r in records if call["address"] - 24 <= r["address"] < call["address"]
                     and r["assembly"].startswith("lwz r12,")]
            self.assertTrue(loads)
            self.assertRegex(loads[-1]["assembly"], r"lwz r12,0x[0-9a-f]+\(r12\)")


if __name__ == "__main__":
    unittest.main()
