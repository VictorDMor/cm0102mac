"""Run with the private original instructions.sqlite; no original data is embedded."""
from copy import deepcopy
import json
from pathlib import Path
import sqlite3
import sys
import unittest

from match_speed_audit import ANCHORS, CONTRACTS, require_match_speed_context


class MatchSpeedAuditTests(unittest.TestCase):
    fixtures = {}

    @classmethod
    def setUpClass(cls):
        if not cls.fixtures:
            raise unittest.SkipTest("Pass the private original instructions.sqlite")

    def check(self, first, rows, selected=None):
        require_match_speed_context(first >> 12, rows,
                                    {r["address"] for r in rows} if selected is None else selected)

    def test_original(self):
        for first, rows in self.fixtures.items():
            self.check(first, rows)

    def test_incomplete_selected_contract(self):
        for first, rows in self.fixtures.items():
            selected = {r["address"] for r in rows}
            for index in range(len(rows)):
                with self.subTest(address=hex(rows[index]["address"])):
                    with self.assertRaises(ValueError):
                        self.check(first, rows[:index] + rows[index + 1:], selected)
                    with self.assertRaises(ValueError):
                        self.check(first, rows, selected - {rows[index]["address"]})

    def test_duplicate_instructions(self):
        for first, rows in self.fixtures.items():
            for row in rows:
                with self.assertRaises(ValueError):
                    self.check(first, rows + [row])

    def test_changed_instruction_and_pcode(self):
        for first, rows in self.fixtures.items():
            for index in range(len(rows)):
                for key, value in (("assembly", "nop"), ("size", 8),
                                   ("flow", "COMPUTED_JUMP"), ("fallthrough", 0),
                                   ("flows", [0]), ("ops", [])):
                    if rows[index].get(key) == value:
                        continue
                    changed = deepcopy(rows)
                    changed[index][key] = value
                    with self.assertRaises(ValueError):
                        self.check(first, changed)

    def test_unrelated_partial_export(self):
        for (first, rows), anchor in zip(self.fixtures.items(), ANCHORS):
            self.check(first, [r for r in rows if r["address"] != anchor])


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    database = Path(sys.argv.pop()).resolve().as_uri() + "?mode=ro"
    with sqlite3.connect(database, uri=True) as connection:
        for first, last in CONTRACTS:
            MatchSpeedAuditTests.fixtures[first] = [json.loads(row[0]) for row in connection.execute(
                "SELECT body FROM instructions WHERE address BETWEEN ? AND ? ORDER BY address", (first, last))]
    unittest.main()
