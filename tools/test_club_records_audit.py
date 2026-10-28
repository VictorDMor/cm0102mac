"""Audit original CLUB_RECORDS boundaries, entries, digests and hook ownership."""
from contextlib import closing
import json
from pathlib import Path
import sqlite3
import struct
import tempfile
import unittest

from club_records_audit import MODULE, ROUTINES, INCLUDES, rewritten, require_club_records_index

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / ".local/current/export"


@unittest.skipUnless((INPUT / "instructions.sqlite").exists(), "Private original export unavailable")
class ClubRecordsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        metadata = json.loads((INPUT / "program.json").read_text())
        cls.segments = [(s["address"], (INPUT / s["file"]).read_bytes())
                        for s in metadata["segments"] if "file" in s]

    def original_bytes(self, address, size):
        for start, data in self.segments:
            if start <= address and address + size <= start + len(data):
                return data[address-start:address-start+size]
        self.fail(f"Original bytes unavailable at {address:x}")

    def word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), "big")

    def test_source_pools_and_neighbor_boundaries(self):
        toc = 0x108bba90
        for slot, offset, name in ((-0x59c8, 0x58, b"CLUB_HISTORY.CPP\0"),
                                   (-0x599c, 0x59, b"CLUB_RECORDS.CPP\0"),
                                   (-0x57f4, 0, b"CLUB_SCREENS.CPP\0")):
            self.assertEqual(self.original_bytes(self.word(toc+slot)+offset, len(name)), name)
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            loads = [a for a, in db.execute(
                "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x599c(r2)%",))]
            self.assertEqual((loads[0], loads[-1], len(loads)), (0x1000e6d0, 0x1001cb48, 31))
            at = lambda a: json.loads(db.execute("SELECT body FROM instructions WHERE address=?", (a,)).fetchone()[0])["assembly"]
            self.assertEqual(at(0x1000e680), "lwz r0,0x0(r3)")
            self.assertEqual(at(MODULE[0]), "mfspr r0,LR")
            self.assertEqual(at(MODULE[1]), "mfspr r0,LR")
            self.assertEqual(at(MODULE[1]+0x10), "lwz r22,-0x57f4(r2)")

    def test_inventory_partition_and_reference_digests(self):
        self.assertEqual(len(ROUTINES), 92)
        self.assertEqual((ROUTINES[0][0], ROUTINES[-1][1]), MODULE)
        self.assertTrue(all(a[1] == b[0] for a,b in zip(ROUTINES, ROUTINES[1:])))
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            require_club_records_index(db)
            self.assertEqual(db.execute("SELECT COUNT(*) FROM instructions WHERE address>=? AND address<?", MODULE).fetchone()[0], 15473)
            for entry,end in ROUTINES:
                records = [json.loads(b) for b, in db.execute(
                    "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address", (entry,end))]
                self.assertEqual(records[0]["address"], entry)
                self.assertEqual(records[-1]["assembly"], "blr")
                last = records[-1]["address"] + 4
                self.assertLess(end-last, 16)
                self.assertEqual(self.original_bytes(last,end-last), bytes(end-last))
                indexed = {r["address"] for r in records}
                for missing in set(range(entry,last,4))-indexed:
                    self.assertEqual(self.word(missing), 0x60000000)
                    before = next(r for r in records if r["address"] == missing-4)
                    self.assertIsNone(before.get("fallthrough"))
                    self.assertFalse(any(missing in r["flows"] for r in records))

    def test_incoming_calls_and_cross_entry_flows(self):
        entries = {a for a,_ in ROUTINES}
        reached = set()
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            for a,b in db.execute("SELECT address,body FROM instructions WHERE body LIKE '%CALL%' OR body LIKE '%JUMP%'"):
                record = json.loads(b)
                for target in record["flows"]:
                    if not MODULE[0] <= target < MODULE[1]:
                        continue
                    if record["flow"].endswith("CALL") or not MODULE[0] <= a < MODULE[1]:
                        self.assertIn(target, entries)
                        reached.add(target)
                    else:
                        source = next(bounds for bounds in ROUTINES if bounds[0] <= a < bounds[1])
                        self.assertTrue(source[0] <= target < source[1], f"cross-entry tail/resume {a:x}->{target:x}")
        self.assertEqual(entries-reached, {0x1001daa0})

    def test_transition_vectors_and_jump_tables(self):
        tvs = []
        for start,data in self.segments:
            for offset in range(0,len(data)-7,4):
                target,toc = struct.unpack_from(">II", data, offset)
                if MODULE[0] <= target < MODULE[1] and toc == 0x108bba90:
                    tvs.append((start+offset,target))
        self.assertEqual(tvs, [(0x108b862c,0x100163b0),(0x108b8864,0x1001daa0)])
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            indexed = {a for a, in db.execute("SELECT address FROM instructions WHERE address>=? AND address<?", MODULE)}
            for slot,count in ((-0x59a8,33),(-0x59ac,33),(-0x59b0,33),(-0x59b4,33),(-0x59b8,33),(-0x59bc,33)):
                table = self.word(0x108bba90+slot)
                for index in range(count):
                    self.assertIn(self.word(table+index*4), indexed)

    def assert_production_inventory(self,entries,owners):
        self.assertEqual([(a,b) for a,b,_ in entries],list(ROUTINES))
        for entry,_,_ in entries:
            self.assertEqual([owner for address,owner in owners if address==entry], ["club_records_adapter"])

    def test_registered_entries_have_unique_production_owners(self):
        from compile_recomp import OWNER_REPLACEMENTS
        self.assert_production_inventory(rewritten(),OWNER_REPLACEMENTS)

    def test_missing_or_disabled_owner_is_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/"disabled.inc"
            path.write_text(INCLUDES.read_text().replace("CLUB_RECORDS_ROUTINE(","CLUB_RECORDS_DRAFT(",1))
            with self.assertRaises(AssertionError):self.assert_production_inventory(rewritten(path),OWNER_REPLACEMENTS)
        with self.assertRaises(AssertionError):self.assert_production_inventory(rewritten(),[(entry,owner) for entry,owner in OWNER_REPLACEMENTS if entry!=ROUTINES[0][0]])

    def test_default_guards_and_hook_free_reference(self):
        from compile_recomp import generate_page
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            records=[json.loads(body) for body, in db.execute("SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address",MODULE)]
        selected={r["address"] for r in records}
        for page in sorted({entry>>12 for entry,_ in ROUTINES}):
            part=[r for r in records if r["address"]>>12==page]
            normal=generate_page(page,part,selected)
            self.assertIn('#include "club_records_adapter.hpp"',normal)
            for entry,_ in ROUTINES:
                if entry>>12==page:self.assertIn(f"if (try_modern_club_records(s, 0x{entry:x})) return true;",normal)
            self.assertNotIn("try_modern_club_records",generate_page(page,part,selected,club_records_hooks=False))

    def test_scratch_dependencies_and_rng_word_are_accounted_for(self):
        from club_records_audit import volatile_reads
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:self.assertEqual(volatile_reads(db),[])
        # This lone non-TV code-looking value is two random uint16 samples.
        self.assertEqual(self.word(0x108eccf4),0x10016c28)
        self.assertEqual((0x108eccf4-self.word(0x108bba90-0x7b8c))//2,19930)
        self.assertNotEqual(self.word(0x108eccf8),0x108bba90)

    def test_domain_sources_do_not_use_instruction_builders_or_original_bodies(self):
        for path in (ROOT / "native").glob("club_records*.cpp"):
            if path.name in {"club_records_adapter.cpp","club_records_checks.cpp"}:continue
            for forbidden in ("SemanticGuestCall","run_original_body","club_records_reference"):
                self.assertNotIn(forbidden,path.read_text(),path.name)

    def test_duplicate_entry_and_stale_digest_are_rejected(self):
        line = INCLUDES.read_text().splitlines()[0]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/"routines.inc"
            path.write_text(line+"\n"+line+"\n")
            with self.assertRaisesRegex(ValueError,"twice"):
                rewritten(path)
        from unittest.mock import patch
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            with patch("club_records_audit.rewritten",return_value=[(ROUTINES[0][0],ROUTINES[0][1],"bad")]):
                with self.assertRaisesRegex(ValueError,"changed in the export"):
                    require_club_records_index(db)


if __name__ == "__main__":
    unittest.main()
