"""Pin INTER_AMER_CUP source boundaries, every entry and its production ownership."""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import inter_amer_cup_audit
from inter_amer_cup_audit import MODULE, ROUTINES, INCLUDES, rewritten, require_inter_amer_cup_index, volatile_reads

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / ".local/current/export/instructions.sqlite"


@unittest.skipUnless(INPUT.exists(), "Private export unavailable")
class InterAmerCupTests(unittest.TestCase):
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
            self.assertEqual([owner for at, owner in owners if at == entry], ["inter_amer_cup_adapter"])

    def test_complete_original_inventory_and_production_ownership(self):
        self.assertEqual(MODULE, (0x106ebdd0, 0x106eca50))
        self.assertEqual(ROUTINES, (
            (0x106ebdd0, 0x106ec0b0), (0x106ec0b0, 0x106ec1a0),
            (0x106ec1a0, 0x106ec330), (0x106ec330, 0x106ec7b0),
            (0x106ec7b0, 0x106eca00), (0x106eca00, 0x106eca50),
        ))
        self.assertEqual((ROUTINES[0][0], ROUTINES[-1][1]), MODULE)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(ROUTINES, ROUTINES[1:])))
        from compile_recomp import OWNER_REPLACEMENTS
        self.assert_production_inventory(rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(INPUT)) as db:
            require_inter_amer_cup_index(db)
            self.assertEqual(db.execute("SELECT COUNT(*) FROM instructions WHERE address>=? AND address<?",
                                        MODULE).fetchone()[0], 794)

    def test_source_pool_and_neighbor_boundaries(self):
        toc = 0x108bba90
        for slot, address, source in ((-0x4094, 0x108963b4, "oceania_nations.cpp"),
                                      (-0x4090, 0x10896650, "inter_amer_cup.cpp"),
                                      (-0x408c, 0x10896678, "oceania_club_champ.cpp")):
            self.assertEqual(self.original_word(toc + slot), address)
            self.assertEqual(self.original_bytes(address, len(source) + 1), source.encode() + b"\0")
        with closing(sqlite3.connect(INPUT)) as db:
            pool_loads = [(address, json.loads(body)["assembly"]) for address, body in db.execute(
                "SELECT address,body FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x4090(r2)%",))]
            self.assertEqual(pool_loads, [
                (0x106ebde8, "lwz r31,-0x4090(r2)"),
                (0x106ec1a8, "lwz r31,-0x4090(r2)"),
                (0x106ec364, "lwz r30,-0x4090(r2)"),
                (0x106ec7c4, "lwz r31,-0x4090(r2)"),
            ])
            at = lambda address: json.loads(db.execute(
                "SELECT body FROM instructions WHERE address=?", (address,)).fetchone()[0])["assembly"]
            self.assertEqual(at(0x106ebdc4), "blr")
            self.assertEqual(at(MODULE[0]), "mfspr r0,LR")
            self.assertEqual(at(MODULE[1]), "mfspr r0,LR")
            self.assertEqual(at(MODULE[1] + 0x8), "lwz r31,-0x408c(r2)")

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
                    expected = {0x106ec7b0: {0x106ec824}}.get(entry, set())
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
            (0x106ebeac, "UNCONDITIONAL_CALL", 0x106ec330),
            (0x106ec898, "UNCONDITIONAL_CALL", 0x106ec330),
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
        self.assertEqual(sorted(incoming), [(0x1034f34c, "UNCONDITIONAL_CALL", (0x106ebdd0,))])

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
            0x108bd044: 0x106eca00,
            0x108bd04c: 0x106ec7b0,
            0x108bd054: 0x106ec1a0,
            0x108bd05c: 0x106ec0b0,
        }
        self.assertEqual(pointers, expected)
        for descriptor in expected:
            self.assertEqual(self.original_word(descriptor + 4), 0x108bba90)
        table = self.original_word(0x108bba90 - 0x5cc0)
        self.assertEqual(table, 0x108e0804)
        for offset, descriptor in (
            (0x08, 0x108bd05c),
            (0x10, 0x108bd04c),
            (0x44, 0x108bd054),
            (0x94, 0x108bd044),
        ):
            self.assertEqual(self.original_word(table + offset), descriptor)
        self.assertEqual({entry for entry, _ in ROUTINES} - set(pointers.values()),
                         {0x106ebdd0, 0x106ec330})

    def test_control_flow_closes_without_interior_resumes_or_switch_tables(self):
        with closing(sqlite3.connect(INPUT)) as db:
            records = [json.loads(body) for body, in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address", MODULE)]
        indexed = {record["address"] for record in records}
        for record in records:
            self.assertNotIn(record["assembly"], ("bctr", "bctrl"))
            self.assertNotIn(record["flow"], ("COMPUTED_JUMP", "COMPUTED_CALL"))
            for target in record["flows"]:
                if MODULE[0] <= target < MODULE[1]:
                    self.assertIn(target, indexed)
                else:
                    self.assertEqual(record["flow"], "UNCONDITIONAL_CALL")
        self.assertEqual([symbol for symbol in self.program["symbols"]
                          if MODULE[0] <= symbol["address"] < MODULE[1]], [])

    def test_audit_rejects_stale_digest(self):
        routines = rewritten(drafts=True)
        stale = [(a, b, "0000000000000000") if index == 0 else (a, b, digest)
                 for index, (a, b, digest) in enumerate(routines)]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(inter_amer_cup_audit, "rewritten", return_value=stale):
            with self.assertRaisesRegex(ValueError, "changed in the export"):
                require_inter_amer_cup_index(db)

    def test_audit_rejects_duplicate_entry_and_disabled_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        with tempfile.TemporaryDirectory() as directory:
            duplicate = Path(directory) / "routines.inc"
            contents = INCLUDES.read_text()
            first = next(line for line in contents.splitlines() if "INTER_AMER_CUP_ROUTINE(" in line)
            duplicate.write_text(contents + "\n" + first + "\n")
            with self.assertRaisesRegex(ValueError, "listed twice"):
                rewritten(duplicate)
            disabled = Path(directory) / "disabled.inc"
            disabled.write_text(contents.replace("INTER_AMER_CUP_ROUTINE(", "INTER_AMER_CUP_DRAFT(", 1))
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
                    self.assertIn(f"if (try_modern_inter_amer_cup(s, 0x{entry:x})) return true;", normal)
            self.assertIn('#include "inter_amer_cup_adapter.hpp"', normal)
            reference = generate_page(page, part, selected, inter_amer_cup_hooks=False)
            self.assertNotIn("try_modern_inter_amer_cup", reference)

    def test_no_unmodelled_scratch_register_dependencies(self):
        with closing(sqlite3.connect(INPUT)) as db:
            self.assertEqual(volatile_reads(db), [])

    def test_virtual_dispatch_explicitly_loads_original_descriptor(self):
        with closing(sqlite3.connect(INPUT)) as db:
            records = [json.loads(body) for body, in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address", MODULE)]
        calls = [r for r in records if r["assembly"] == "bl 0x10781320"]
        self.assertEqual(len(calls), 8)
        for call in calls:
            loads = [r for r in records if call["address"] - 24 <= r["address"] < call["address"]
                     and r["assembly"].startswith("lwz r12,")]
            self.assertTrue(loads)
            self.assertRegex(loads[-1]["assembly"], r"lwz r12,0x[0-9a-f]+\(r12\)")


if __name__ == "__main__":
    unittest.main()
