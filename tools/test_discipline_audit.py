#!/usr/bin/env python3
"""Certify DISCIPLINE.CPP's closed inventory against the private export.

The export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
from pathlib import Path
import re
import sqlite3
import unittest

import game_inventory as gi
import discipline_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
TOC_LOAD = re.compile(r"lwz r\d+,(-?0x[0-9a-f]+|-?\d+)\(r2\)$")
UNIT_SLOTS = (-0x5610, -0x5600)  # DISCIPLINE.CPP's TOC slots, inclusive


class DisciplineAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (EXPORT / "instructions.sqlite").is_file():
            raise RuntimeError("Private export unavailable; the DISCIPLINE.CPP inventory cannot be certified")
        cls.export = gi.Export(EXPORT)
        cls.vectors = gi.transition_vectors(cls.export)
        all_starts = gi.entries(cls.export, cls.vectors, gi.discovery_roots())
        cls.all_starts = all_starts
        cls.starts = dict(all_starts)
        cls.tables = {a: cases for a, flow in cls.export.flow.items() if flow == "COMPUTED_JUMP"
                      and (cases := gi.table_cases(cls.export.assembly, cls.export.word, a))}
        lo, hi = audit.MODULE
        cls.bodies = {entry: gi.body(cls.export, entry, cls.starts, cls.tables)
                      for entry in sorted(cls.starts) if lo <= entry < hi}

    def slots(self, entry, end):
        found = set()
        for address in range(entry, end, 4):
            if match := TOC_LOAD.match(self.export.assembly.get(address, "")):
                found.add(int(match.group(1), 0))
        return found

    def test_routines_are_the_derived_ones(self):
        derived = tuple((entry, max(b[0]) + 4, len(b[0])) for entry, b in self.bodies.items())
        self.assertEqual(derived, tuple((entry, end, n) for entry, end, n, _ in audit.CONTRACTS))
        self.assertEqual(len(audit.ROUTINES), 37)
        self.assertEqual(sum(n for _, _, n, _ in audit.CONTRACTS), 11122)

    def test_bodies_stay_inside_and_cover_the_module(self):
        lo, hi = audit.MODULE
        covered = set()
        for entry, end in audit.ROUTINES:
            instructions, _, tails, unresolved, missing = self.bodies[entry]
            self.assertEqual((tails, unresolved, missing), ([], [], []), f"{entry:x}")
            self.assertTrue(all(entry <= a < end for a in instructions), f"{entry:x} leaves its range")
            inside = set(range(entry, end, 4))
            self.assertEqual(inside - instructions, audit.UNREACHABLE & inside, f"{entry:x}")
            covered |= instructions
        code = next(data for start, data in self.export.segments if start == gi.CODE_START)
        for address in audit.UNREACHABLE:
            word = int.from_bytes(code[address - gi.CODE_START:address - gi.CODE_START + 4], "big")
            self.assertTrue(word == 0x60000000 or word >> 26 == 18, f"{address:x} holds {word:08x}")
            self.assertNotIn(address, self.export.flow)
        for address in range(lo, hi, 4):
            if address in covered or address in audit.UNREACHABLE:
                continue
            self.assertNotIn(address, self.export.flow, f"{address:x} is code outside every routine")
            self.assertEqual(code[address - gi.CODE_START:address - gi.CODE_START + 4], b"\0\0\0\0",
                             f"{address:x} is not padding")

    def test_no_switch_tables(self):
        for entry, end in audit.ROUTINES:
            for address in range(entry, end, 4):
                self.assertNotEqual(self.export.flow.get(address), "COMPUTED_JUMP", f"{address:x}")

    def test_flow_from_outside_only_reaches_entries(self):
        lo, hi = audit.MODULE
        entries = {entry for entry, _ in audit.ROUTINES}
        for address, target in self.export.target.items():
            if lo <= target < hi and not lo <= address < hi:
                self.assertIn(target, entries, f"{address:x} enters DISCIPLINE.CPP at {target:x}")
        for address in self.vectors:
            if lo <= address < hi:
                self.assertIn(address, entries)

    def test_neighbours(self):
        lo, hi = audit.MODULE
        pools = gi.source_pools(self.export)
        self.assertEqual(pools[-0x5608], ["DISCIPLINE.CPP"])
        self.assertEqual(pools[-0x5618], ["DATABASE.CPP"])
        self.assertEqual(pools[-0x55d4], ["FILE_SCREENS.CPP"])
        # Every unit slot the module loads is DISCIPLINE.CPP's.
        loaded = set()
        for entry, end in audit.ROUTINES:
            loaded |= {s for s in self.slots(entry, end) if -0x5700 < s < -0x5500}
        self.assertEqual(loaded, {-0x5610, -0x560c, -0x5608, -0x5604, -0x5600})
        # The conversion constants at -0x5610 end this unit's literal data: the next unit's texts follow.
        self.assertEqual(self.export.word(gi.TOC - 0x5610), 0x1079fcfc)
        self.assertEqual(self.export.word(gi.TOC - 0x55fc), 0x1079fd0c)
        self.assertEqual(self.slots(0x100e8780, 0x100e8d10) & set(range(-0x5700, -0x5500)), {-0x5610})
        self.assertEqual(self.slots(0x100e8d10, 0x100e9288) & set(range(-0x5700, -0x5500)), {-0x5610})
        # The comparator 0x100e9290: its only vector is this unit's TOC -0x560c, between DATABASE.CPP's
        # vector group and FILE_SCREENS.CPP's.
        self.assertEqual([a for a in self.vectors if lo <= a < hi], [0x100e9290])
        self.assertEqual(self.export.word(gi.TOC - 0x560c), 0x108b8a0c)
        self.assertEqual([self.export.word(a) for a in (0x108b8a04, 0x108b8a0c, 0x108b8a14)], [0x100cde70, 0x100e9290, 0x100f0ba0])
        # Before: DATABASE.CPP's audited range ends here.
        import database_audit
        self.assertEqual(database_audit.MODULE[1], lo)
        # After: 0x100e92d0 loads no unit slot, only SHORTLIST_MANAGER.CPP calls it, and it shares no
        # call with this unit; the unnamed unit's routines after it load -0x55fc/-0x55f8.
        self.assertIn(hi, self.starts)
        after = gi.body(self.export, hi, self.starts, {})[0]
        self.assertFalse(self.slots(hi, max(after) + 4) & set(range(-0x5700, -0x5500)))
        callers = [a for a, t in self.export.target.items() if t == hi]
        self.assertEqual(callers, [0x10360cfc])
        entries = {entry for entry, _ in audit.ROUTINES}
        self.assertFalse({self.export.target.get(a) for a in after} & entries)
        self.assertEqual(self.slots(0x100e94a0, 0x100e9a18) & set(range(-0x5700, -0x5500)), {-0x55f8})

    def test_rewritten_digests(self):
        with closing(sqlite3.connect(EXPORT / "instructions.sqlite")) as db:
            for entry, end, _, pinned in audit.CONTRACTS:
                self.assertEqual(digest(routine_records(db, entry, end)), pinned, f"{entry:x}")
            audit.require_discipline_index(db)


if __name__ == "__main__":
    unittest.main()
