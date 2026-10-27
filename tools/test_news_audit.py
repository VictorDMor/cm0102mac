"""Pin NEWS's original scope and validate the implemented subset honestly."""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import news_audit
from news_audit import MODULE, ROUTINES, INCLUDES, rewritten, require_news_index
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / ".local/current/export/instructions.sqlite"


class RegistrationTests(unittest.TestCase):
    def registration(self, stage="DRAFT", entry=0x102bdf90, end=0x102be040):
        return f'NEWS_{stage}({entry:#x}, {end:#x}, R3, "03d1d1c8ff24e128", hook)\n'

    def test_partial_and_empty_lists_do_not_claim_completion(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "routines.inc"
            path.write_text("")
            self.assertEqual(rewritten(path), [])
            path.write_text(self.registration())
            self.assertEqual(rewritten(path), [])
            self.assertEqual(rewritten(path, drafts=True),
                             [(0x102bdf90, 0x102be040, "03d1d1c8ff24e128")])

    def test_duplicate_and_wrong_boundary_are_rejected_including_drafts(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "routines.inc"
            path.write_text(self.registration("ROUTINE") + self.registration())
            with self.assertRaisesRegex(ValueError, "listed twice"):
                rewritten(path)
            path.write_text(self.registration(end=0x102be044))
            with self.assertRaisesRegex(ValueError, "not a routine of NEWS.CPP"):
                rewritten(path)
            path.write_text(self.registration(entry=0x102bdf94))
            with self.assertRaisesRegex(ValueError, "not a routine of NEWS.CPP"):
                rewritten(path, drafts=True)

    def test_missing_explicit_group_is_not_silently_ignored(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "routines.inc"
            path.write_text('#include "news_routines_missing.inc"\n')
            with self.assertRaises(FileNotFoundError):
                rewritten(path)


@unittest.skipUnless(INPUT.exists(), "Private NEWS reference unavailable; source audit not run")
class NewsOriginalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        program = json.loads((INPUT.parent / "program.json").read_text())
        cls.segments = [(segment["address"], (INPUT.parent / segment["file"]).read_bytes())
                        for segment in program["segments"] if "file" in segment]

    def original_bytes(self, address, size):
        for start, contents in self.segments:
            if start <= address and address + size <= start + len(contents):
                return contents[address - start:address - start + size]
        self.fail(f"Original bytes unavailable at {address:x}")

    def original_word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), "big")

    def test_original_inventory_and_zero_alignment_gaps(self):
        self.assertEqual(len(ROUTINES), 33)
        self.assertEqual((ROUTINES[0][0], ROUTINES[-1][1]), MODULE)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(ROUTINES, ROUTINES[1:])))
        with closing(sqlite3.connect(INPUT)) as db:
            records = routine_records(db, *MODULE)
            self.assertEqual(len(records), 16819)
            self.assertEqual(digest(records), "37535b5bfc1b7d12")
            self.assertEqual([record["address"] for record in records
                              if record["assembly"] == "mfspr r0,LR"],
                             [entry for entry, _ in ROUTINES])
            addresses = {record["address"] for record in records}
            gaps = [address for address in range(*MODULE, 4) if address not in addresses]
            self.assertEqual(len(gaps), 45)
            self.assertTrue(all(self.original_word(address) == 0 for address in gaps))
            require_news_index(db)

    def test_assertion_pool_and_neighboring_module_boundaries(self):
        toc = 0x108bba90
        for slot, address, source in ((-0x5120, 0x107f0efc, "NATIONAL_TEAMS_SCREENS.CPP"),
                                      (-0x5100, 0x107f14bc, "NEWS.CPP"),
                                      (-0x50f0, 0x107f54c8, "NEWS_SCREENS.CPP")):
            self.assertEqual(self.original_word(toc + slot), address)
            self.assertEqual(self.original_bytes(address, len(source) + 1), source.encode() + b"\0")
        with closing(sqlite3.connect(INPUT)) as db:
            loads = [address for address, in db.execute(
                "SELECT address FROM instructions WHERE body LIKE '%-0x5100(r2)%' ORDER BY address")]
            self.assertEqual(loads, [0x102be090, 0x102c6b58, 0x102c86dc, 0x102ca468,
                                    0x102cb348, 0x102cb4b8, 0x102cb578, 0x102cb648,
                                    0x102cb808, 0x102cb94c, 0x102cbc30, 0x102cc038,
                                    0x102cc14c, 0x102cc2a4, 0x102cc424, 0x102cc5d4,
                                    0x102cc754, 0x102cc8fc, 0x102cc9ec, 0x102ccb38,
                                    0x102ccddc, 0x102cd0d8, 0x102cd458, 0x102cd624,
                                    0x102cd8c8, 0x102cda78, 0x102cdc98, 0x102cde3c,
                                    0x102ce32c])
            at = lambda address: json.loads(db.execute(
                "SELECT body FROM instructions WHERE address=?", (address,)).fetchone()[0])["assembly"]
            self.assertEqual(at(0x102bdf8c), "blr")
            self.assertEqual(at(MODULE[0]), "mfspr r0,LR")
            self.assertEqual(at(0x102ce704), "blr")
            self.assertEqual(at(MODULE[1]), "mfspr r0,LR")
            self.assertEqual(at(MODULE[1] + 0x18), "lwz r30,-0x50f0(r2)")

    def test_direct_calls_close_entries_and_no_external_interior_resumes(self):
        entries = {entry for entry, _ in ROUTINES}
        direct_targets, external_count, direct_count = set(), 0, 0
        with closing(sqlite3.connect(INPUT)) as db:
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    "WHERE f.value>=? AND f.value<?", MODULE):
                record = json.loads(body)
                inside = MODULE[0] <= address < MODULE[1]
                targets = {target for target in record["flows"] if MODULE[0] <= target < MODULE[1]}
                if record["flow"] == "UNCONDITIONAL_CALL":
                    direct_targets.update(targets)
                    direct_count += 1
                    self.assertTrue(targets <= entries)
                if not inside:
                    external_count += 1
                    self.assertEqual(record["flow"], "UNCONDITIONAL_CALL")
                    self.assertTrue(targets <= entries)
                elif record["flow"] != "UNCONDITIONAL_CALL":
                    owner = next((start, end) for start, end in ROUTINES if start <= address < end)
                    self.assertTrue(all(owner[0] <= target < owner[1] for target in targets))
        self.assertEqual(direct_targets, entries)
        self.assertEqual(direct_count, 5273)
        self.assertEqual(external_count, 5188)

    def test_pointer_inventory_is_only_a_local_switch_table(self):
        entries = {entry for entry, _ in ROUTINES}
        aligned = {}
        descriptors = {}
        for start, contents in self.segments:
            for offset in range(0, len(contents) - 3, 4):
                value = int.from_bytes(contents[offset:offset + 4], "big")
                if MODULE[0] <= value < MODULE[1]:
                    aligned[start + offset] = value
            # Entry/TOC descriptors can be unaligned; search every byte offset.
            for match in re.finditer(b"\x10[\x2b\x2c]", contents):
                offset = match.start()
                target = int.from_bytes(contents[offset:offset + 4], "big")
                if target in entries:
                    descriptors[start + offset] = int.from_bytes(contents[offset + 4:offset + 8], "big")
        self.assertEqual(descriptors, {})
        self.assertEqual(set(aligned), set(range(0x108ce84c, 0x108ce92c, 4)))
        self.assertEqual(set(aligned.values()), {0x102c4ee8, 0x102c4f08})
        self.assertEqual(self.original_word(0x108bba90 - 0x5108), 0x108ce84c)
        with closing(sqlite3.connect(INPUT)) as db:
            records = routine_records(db, *MODULE)
        computed = [(record["address"], record["assembly"]) for record in records
                    if record["flow"] in ("COMPUTED_CALL", "COMPUTED_JUMP")]
        self.assertEqual(computed, [(0x102c4ee4, "bctr")])

    def test_stale_registered_reference_is_rejected(self):
        # Works before any implementation is registered; absence is no pass.
        stale = [(0x102bdf90, 0x102be040, "0000000000000000")]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(news_audit, "rewritten", return_value=stale):
            with self.assertRaisesRegex(ValueError, "changed in the export"):
                require_news_index(db)

    def test_registered_production_subset_has_exactly_one_generator_owner(self):
        # Check only completed registrations. Drafts/unimplemented entries are
        # intentionally outside this assertion; this is not a closure claim.
        from compile_recomp import OWNER_REPLACEMENTS
        for entry, _, _ in rewritten():
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ["news_adapter"])


if __name__ == "__main__":
    unittest.main()
