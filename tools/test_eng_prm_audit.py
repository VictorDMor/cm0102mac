"""Pin the complete ENG_PRM inventory, production owners and ABI dependencies."""
from contextlib import closing
import json
from pathlib import Path
import sqlite3
import unittest

from eng_prm_audit import MODULE, ROUTINES, rewritten, require_eng_prm_index, volatile_reads

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / ".local/current/export/instructions.sqlite"


@unittest.skipUnless(INPUT.exists(), "Private export unavailable")
class EngPrmTests(unittest.TestCase):
    def test_complete_original_inventory_and_production_ownership(self):
        self.assertEqual(len(ROUTINES), 9)
        self.assertEqual((ROUTINES[0][0], ROUTINES[-1][1]), MODULE)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(ROUTINES, ROUTINES[1:])))
        self.assertEqual([(a, b) for a, b, _ in rewritten()], list(ROUTINES))
        with closing(sqlite3.connect(INPUT)) as db:
            require_eng_prm_index(db)
            self.assertEqual(db.execute("SELECT COUNT(*) FROM instructions WHERE address>=? AND address<?",
                                        MODULE).fetchone()[0], 3155)
        from compile_recomp import OWNER_REPLACEMENTS
        for entry, _ in ROUTINES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ["eng_prm_adapter"])

    def test_no_unmodelled_scratch_register_dependencies(self):
        with closing(sqlite3.connect(INPUT)) as db:
            self.assertEqual(volatile_reads(db), [])

    def test_virtual_dispatch_explicitly_loads_original_descriptor(self):
        with closing(sqlite3.connect(INPUT)) as db:
            records = [json.loads(body) for body, in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address", MODULE)]
        calls = [r for r in records if r["assembly"] == "bl 0x10781320"]
        self.assertEqual(len(calls), 16)
        for call in calls:
            loads = [r for r in records if call["address"] - 24 <= r["address"] < call["address"]
                     and r["assembly"].startswith("lwz r12,")]
            self.assertTrue(loads)
            self.assertRegex(loads[-1]["assembly"], r"lwz r12,0x[0-9a-f]+\(r12\)")


if __name__ == "__main__":
    unittest.main()
