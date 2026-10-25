"""The registers the C++ PLAYER_STATS.CPP routines do not reproduce are never read."""
from contextlib import closing
import json
import os
from pathlib import Path
import sqlite3
import unittest
from player_stats_audit import CALLBACK_SITES, MODULE, ROUTINES, volatile_reads

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get("CM_PLAYER_STATS_INPUT", ROOT / ".local/current/export"))


@unittest.skipUnless((INPUT / "instructions.sqlite").exists(), "Private export unavailable")
class ProgramTests(unittest.TestCase):
    def test_no_code_around_player_stats_reads_what_the_cpp_leaves_different(self):
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            self.assertEqual(volatile_reads(db), [])

    def test_callback_sites_call_the_pointer_glue(self):
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            for site in CALLBACK_SITES:
                body = json.loads(db.execute("SELECT body FROM instructions WHERE address = ?", (site,)).fetchone()[0])
                self.assertEqual(body["assembly"], "bl 0x10781320")

    def test_routines_tile_the_module(self):
        self.assertEqual(ROUTINES[0][0], MODULE[0])
        self.assertEqual(ROUTINES[-1][1], MODULE[1])
        for (_, end), (entry, _) in zip(ROUTINES, ROUTINES[1:]):
            self.assertEqual(end, entry)


if __name__ == "__main__":
    unittest.main()
