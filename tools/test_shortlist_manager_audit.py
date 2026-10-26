"""The registers the C++ SHORTLIST_MANAGER.CPP routines do not reproduce are never read."""
from contextlib import closing
import json
import os
from pathlib import Path
import sqlite3
import unittest
from shortlist_manager_audit import CALLBACK_SITES, MODULE, ROUTINES, volatile_reads

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get("CM_SHORTLIST_MANAGER_INPUT", ROOT / ".local/current/export"))


@unittest.skipUnless((INPUT / "instructions.sqlite").exists(), "Private export unavailable")
class ProgramTests(unittest.TestCase):
    def test_no_code_around_shortlist_manager_reads_what_the_cpp_leaves_different(self):
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            self.assertEqual(volatile_reads(db), [])

    def test_callback_sites_call_the_pointer_glue(self):
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            for site in CALLBACK_SITES:
                body = json.loads(db.execute("SELECT body FROM instructions WHERE address = ?", (site,)).fetchone()[0])
                self.assertEqual(body["assembly"], "bl 0x10781320")

    def test_module_bounds_are_its_string_pool_users(self):
        # The pool slot (TOC -0x4dd8) is loaded only inside the module, and
        # SETUP.CPP's (-0x4e18) and SIMULATED_STATS.CPP's (-0x4dd0) only outside.
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            def users(offset):
                pattern = f"%,-0x{-offset:x}(r2)"
                return [a for (a,) in db.execute(
                    "SELECT address FROM instructions WHERE json_extract(body, '$.assembly') LIKE ?", (pattern,))]
            low, high = MODULE
            self.assertTrue(users(-0x4dd8) and all(low <= a < high for a in users(-0x4dd8)))
            for other in (-0x4e18, -0x4dd0):
                self.assertTrue(all(not low <= a < high for a in users(other)))

    def test_routines_tile_the_module(self):
        self.assertEqual(ROUTINES[0][0], MODULE[0])
        self.assertEqual(ROUTINES[-1][1], MODULE[1])
        for (_, end), (entry, _) in zip(ROUTINES, ROUTINES[1:]):
            self.assertEqual(end, entry)


if __name__ == "__main__":
    unittest.main()
