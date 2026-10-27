"""Pin POL_CUP source boundaries, every entry and its production ownership."""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import pol_cup_audit
from pol_cup_audit import MODULE, ROUTINES, INCLUDES, rewritten, require_pol_cup_index, volatile_reads

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / ".local/current/export/instructions.sqlite"


@unittest.skipUnless(INPUT.exists(), "Private export unavailable")
class PolCupTests(unittest.TestCase):
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
            self.assertEqual([owner for at, owner in owners if at == entry], ["pol_cup_adapter"])

    def test_complete_original_inventory_and_production_ownership(self):
        self.assertEqual(len(ROUTINES), 6)
        self.assertEqual((ROUTINES[0][0], ROUTINES[-1][1]), MODULE)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(ROUTINES, ROUTINES[1:])))
        from compile_recomp import OWNER_REPLACEMENTS
        self.assert_production_inventory(rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(INPUT)) as db:
            require_pol_cup_index(db)
            self.assertEqual(db.execute("SELECT COUNT(*) FROM instructions WHERE address>=? AND address<?",
                                        MODULE).fetchone()[0], 1018)

    def test_source_pool_and_neighbor_boundaries(self):
        toc = 0x108bba90
        for slot, address, source in ((-0x40f0, 0x1089588c, "nir_lge_cup.cpp"),
                                      (-0x40ec, 0x108958e8, "pol_cup.cpp"),
                                      (-0x40e8, 0x10895924, "pol_lge_cup.cpp")):
            self.assertEqual(self.original_word(toc + slot), address)
            self.assertEqual(self.original_bytes(address, len(source) + 1), source.encode() + b"\0")
        with closing(sqlite3.connect(INPUT)) as db:
            pool_loads = [(address, json.loads(body)["assembly"]) for address, body in db.execute(
                "SELECT address,body FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x40ec(r2)%",))]
            self.assertEqual(pool_loads, [(0x106cd828, "lwz r31,-0x40ec(r2)"),
                                          (0x106cdbfc, "lwz r28,-0x40ec(r2)"),
                                          (0x106ce1b0, "lwz r31,-0x40ec(r2)"),
                                          (0x106ce584, "lwz r31,-0x40ec(r2)")])
            at = lambda address: json.loads(db.execute(
                "SELECT body FROM instructions WHERE address=?", (address,)).fetchone()[0])["assembly"]
            self.assertEqual(at(0x106cd804), "blr")
            self.assertEqual(at(MODULE[0]), "mfspr r0,LR")
            self.assertEqual(at(MODULE[1]), "mfspr r0,LR")
            self.assertEqual(at(MODULE[1] + 0x18), "lwz r31,-0x40e8(r2)")

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
        self.assertEqual(sorted(incoming), [(0x10353558, "UNCONDITIONAL_CALL", (0x106cd810,))])

    def test_transition_vectors_and_vtable_close_indirect_entry_inventory(self):
        pointers = {}
        for start, contents in self.segments:
            # Every byte position is considered, including unaligned pointers.
            for match in re.finditer(b"\x10\x6c", contents):
                offset = match.start()
                if offset + 4 <= len(contents):
                    target = int.from_bytes(contents[offset:offset + 4], "big")
                    if MODULE[0] <= target < MODULE[1]:
                        pointers[start + offset] = target
        expected = {0x108bcb9c: 0x106ce7c0, 0x108bcba4: 0x106ce570,
                    0x108bcbac: 0x106cdbe0, 0x108bcbb4: 0x106cdaf0}
        self.assertEqual(pointers, expected)
        for descriptor in expected:
            self.assertEqual(self.original_word(descriptor + 4), 0x108bba90)
        table = self.original_word(0x108bba90 - 0x5d0c)
        self.assertEqual(table, 0x108dfb44)
        for offset, descriptor in ((0x8, 0x108bcbb4), (0x10, 0x108bcba4),
                                   (0x44, 0x108bcbac), (0x94, 0x108bcb9c)):
            self.assertEqual(self.original_word(table + offset), descriptor)
        self.assertEqual({entry for entry, _ in ROUTINES} - set(pointers.values()),
                         {0x106cd810, 0x106ce180})

    def test_audit_rejects_stale_digest(self):
        routines = rewritten(drafts=True)
        stale = [(a, b, "0000000000000000") if index == 0 else (a, b, digest)
                 for index, (a, b, digest) in enumerate(routines)]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(pol_cup_audit, "rewritten", return_value=stale):
            with self.assertRaisesRegex(ValueError, "changed in the export"):
                require_pol_cup_index(db)

    def test_audit_rejects_duplicate_entry_and_disabled_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        with tempfile.TemporaryDirectory() as directory:
            duplicate = Path(directory) / "routines.inc"
            contents = INCLUDES.read_text()
            first = next(line for line in contents.splitlines() if "POL_CUP_ROUTINE(" in line)
            duplicate.write_text(contents + "\n" + first + "\n")
            with self.assertRaisesRegex(ValueError, "listed twice"):
                rewritten(duplicate)
            disabled = Path(directory) / "disabled.inc"
            disabled.write_text(contents.replace("POL_CUP_ROUTINE(", "POL_CUP_DRAFT(", 1))
            with self.assertRaises(AssertionError):
                self.assert_production_inventory(rewritten(disabled), OWNER_REPLACEMENTS)
        owners_without_constructor = [(entry, owner) for entry, owner in OWNER_REPLACEMENTS
                                      if entry != ROUTINES[0][0]]
        with self.assertRaises(AssertionError):
            self.assert_production_inventory(rewritten(), owners_without_constructor)

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
