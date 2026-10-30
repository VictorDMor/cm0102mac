"""Close FIFA_RANKINGS original boundaries, targets and normal ownership."""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import fifa_rankings_audit as audit
from fifa_rankings_audit import MODULE, ROUTINES, rewritten, require_fifa_rankings_index

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / ".local/current/export/instructions.sqlite"


@unittest.skipUnless(INPUT.exists(), "Private export unavailable")
class FifaRankingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        program = json.loads((INPUT.parent / "program.json").read_text())
        cls.segments = [(s["address"], (INPUT.parent / s["file"]).read_bytes())
                        for s in program["segments"] if "file" in s]

    def original(self, address, size=4):
        for start, data in self.segments:
            if start <= address and address + size <= start + len(data):
                return data[address-start:address-start+size]
        self.fail(f"Missing original bytes at {address:x}")

    def word(self, address):
        return int.from_bytes(self.original(address), "big")

    def production(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(ROUTINES))
        for entry, _ in ROUTINES:
            self.assertEqual([owner for at, owner in owners if at == entry], ["fifa_rankings_adapter"])

    def test_complete_inventory_and_production(self):
        from compile_recomp import OWNER_REPLACEMENTS
        self.assertEqual(len(ROUTINES), 15)
        self.assertEqual((ROUTINES[0][0], ROUTINES[-1][1]), MODULE)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(ROUTINES, ROUTINES[1:])))
        self.production(rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(INPUT)) as db:
            require_fifa_rankings_index(db)
            self.assertEqual(db.execute("SELECT COUNT(*) FROM instructions WHERE address>=? AND address<?", MODULE).fetchone()[0], 1575)

    def test_source_pools_and_neighbors(self):
        toc = 0x108bba90
        for slot, address, name in ((-0x47b8, 0x10839e0c, b"CUP.CPP\0"),
                                    (-0x47b4, 0x1083a2dc, b"FIFA_RANKINGS.CPP\0"),
                                    (-0x4798, 0x1083a3d4, b"Insert CD\0")):
            self.assertEqual(self.word(toc + slot), address)
            self.assertEqual(self.original(address, len(name)), name)
        self.assertEqual(self.original(0x1083a3d4 + 0x37, 12), b"FIX_MAN.CPP\0")
        with closing(sqlite3.connect(INPUT)) as db:
            loads = [(a, json.loads(b)["assembly"]) for a, b in db.execute(
                "SELECT address,body FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x47b4(r2)%",))]
            self.assertEqual([a for a, _ in loads], [0x10486860, 0x10487038, 0x104871ec, 0x1048764c, 0x1048798c])
            at = lambda a: json.loads(db.execute("SELECT body FROM instructions WHERE address=?", (a,)).fetchone()[0])["assembly"]
            self.assertEqual(at(MODULE[0]-4), "blr")
            self.assertEqual(at(MODULE[0]), "mfspr r0,LR")
            self.assertEqual(at(0x10487fe0), "blr")
            self.assertEqual(at(MODULE[1]), "mfspr r0,LR")

    def test_all_incoming_and_cross_entry_flows(self):
        with closing(sqlite3.connect(INPUT)) as db:
            flows = []
            targets = set()
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    "WHERE f.value>=? AND f.value<?", MODULE):
                r = json.loads(body)
                for target in r["flows"]:
                    if not MODULE[0] <= target < MODULE[1]:
                        continue
                    if not MODULE[0] <= address < MODULE[1]:
                        flows.append((address, r["flow"], target)); targets.add(target)
                    else:
                        owner = next((a, b) for a, b in ROUTINES if a <= address < b)
                        if not owner[0] <= target < owner[1]:
                            self.assertEqual(r["flow"], "UNCONDITIONAL_CALL")
                            self.assertIn(target, dict(ROUTINES)); targets.add(target)
        self.assertEqual(sorted(flows), [(a, "UNCONDITIONAL_CALL", b) for a, b in
            ((0x10074730, 0x10487020), (0x102aaef8, 0x104866f0), (0x102aafa4, 0x10486790),
             (0x102ac0dc, 0x10486e40), (0x102ac3b4, 0x10486830), (0x102b9e30, 0x10487640), (0x103edc6c, 0x10487140))])
        self.assertEqual(targets | {0x10487fa0, 0x10487fb0}, set(dict(ROUTINES)))

    def test_all_initialized_pointers_and_callbacks(self):
        genuine = {}; instruction_coincidences = []
        for start, contents in self.segments:
            for match in re.finditer(b"\x10\x48", contents):
                offset = match.start()
                if offset+4 > len(contents): continue
                target = int.from_bytes(contents[offset:offset+4], "big")
                if not MODULE[0] <= target < MODULE[1]: continue
                at = start+offset
                if start == 0x10000000 and at % 4:
                    instruction_coincidences.append(at)
                else: genuine[at] = target
        self.assertEqual(len(instruction_coincidences), 77)
        with closing(sqlite3.connect(INPUT)) as db:
            for at in instruction_coincidences:
                # A discarded match must actually span indexed instructions,
                # rather than an unaligned literal in a constant/data pool.
                for word in (at & ~3, (at & ~3)+4):
                    self.assertIsNotNone(db.execute("SELECT address FROM instructions WHERE address=?", (word,)).fetchone())
        self.assertEqual(genuine, {0x108b9b04: 0x10487fb0, 0x108b9b0c: 0x10487fa0})
        for at in genuine: self.assertEqual(self.word(at+4), 0x108bba90)
        self.assertEqual(self.word(0x108bba90-0x6180), 0x108b9b04)
        self.assertEqual(self.word(0x108bba90-0x617c), 0x108b9b0c)

    def test_unindexed_words_are_skipped_padding(self):
        with closing(sqlite3.connect(INPUT)) as db:
            indexed = {a for a, in db.execute("SELECT address FROM instructions WHERE address>=? AND address<?", MODULE)}
        gaps = {a: self.word(a) for a in range(*MODULE, 4) if a not in indexed}
        self.assertEqual(len(gaps), 25)
        self.assertEqual({a for a, word in gaps.items() if word}, {0x104870d0, 0x10487854, 0x10487cdc, 0x10487d24})
        self.assertTrue(all(word in (0, 0x60000000) for word in gaps.values()))

    def test_scratch_dependency_audit(self):
        with closing(sqlite3.connect(INPUT)) as db:
            self.assertEqual(audit.volatile_reads(db), [])

    def test_reject_stale_digest(self):
        stale = [(a, b, "0"*16 if i == 0 else d) for i, (a, b, d) in enumerate(rewritten())]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(audit, "rewritten", return_value=stale):
            with self.assertRaisesRegex(ValueError, "changed in the export"):
                require_fifa_rankings_index(db)

    def test_reject_duplicate_disabled_and_wrong_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        text = audit.INCLUDES.read_text()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "routines.inc"
            path.write_text(text+text.splitlines()[0]+"\n")
            with self.assertRaisesRegex(ValueError, "listed twice"): rewritten(path)
            path.write_text(text.replace("FIFA_RANKINGS_ROUTINE(", "FIFA_RANKINGS_DRAFT(", 1))
            with self.assertRaises(AssertionError): self.production(rewritten(path), OWNER_REPLACEMENTS)
        with self.assertRaises(AssertionError):
            self.production(rewritten(), [(a, o) for a, o in OWNER_REPLACEMENTS if a != MODULE[0]])


if __name__ == "__main__": unittest.main()
