#!/usr/bin/env python3
"""Certify DATABASE.CPP's closed inventory against the private export.

The export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
from pathlib import Path
import re
import sqlite3
import unittest

import contract_screens_audit
import database_audit as audit
import engine_initialization_audit
import game_inventory as gi

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
TOC_LOAD = re.compile(r"lwz r\d+,(-?0x[0-9a-f]+|-?\d+)\(r2\)$")


class DatabaseAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (EXPORT / "instructions.sqlite").is_file():
            raise RuntimeError("Private export unavailable; the DATABASE.CPP inventory cannot be certified")
        cls.export = gi.Export(EXPORT)
        vectors = gi.transition_vectors(cls.export)
        cls.starts = gi.entries(cls.export, vectors, gi.discovery_roots())
        cls.tables = {a: cases for a, flow in cls.export.flow.items() if flow == "COMPUTED_JUMP"
                      and (cases := gi.table_cases(cls.export.assembly, cls.export.word, a))}
        lo, hi = audit.MODULE
        cls.bodies = {entry: gi.body(cls.export, entry, cls.starts, cls.tables)
                      for entry in sorted(cls.starts) if lo <= entry < hi}

    def test_routines_are_the_derived_ones(self):
        derived = tuple((entry, max(b[0]) + 4) for entry, b in self.bodies.items())
        self.assertEqual(derived, audit.ROUTINES)
        self.assertEqual(len(audit.ROUTINES), 189)

    def test_bodies_stay_inside_and_cover_the_module(self):
        lo, hi = audit.MODULE
        covered = set()
        for entry, end in audit.ROUTINES:
            instructions, _, _, unresolved, missing = self.bodies[entry]
            self.assertEqual((unresolved, missing), ([], []), f"{entry:x}")
            self.assertTrue(all(entry <= a < end for a in instructions), f"{entry:x} leaves its range")
            inside = {a for a in range(entry, end, 4)}
            self.assertEqual(inside - instructions, audit.UNREACHABLE & inside, f"{entry:x}")
            covered |= instructions
        code = next(data for start, data in self.export.segments if start == gi.CODE_START)
        for address in range(lo, hi, 4):
            if address in covered or address in audit.UNREACHABLE:
                continue
            self.assertNotIn(address, self.export.flow, f"{address:x} is code outside every routine")
            self.assertEqual(code[address - gi.CODE_START:address - gi.CODE_START + 4], b"\0\0\0\0",
                             f"{address:x} is not padding")

    def test_flow_from_outside_only_reaches_entries(self):
        lo, hi = audit.MODULE
        entries = {entry for entry, _ in audit.ROUTINES}
        for address, target in self.export.target.items():
            if lo <= target < hi and not lo <= address < hi:
                self.assertIn(target, entries, f"{address:x} enters DATABASE.CPP at {target:x}")
        for address in gi.transition_vectors(self.export):
            if lo <= address < hi:
                self.assertIn(address, entries)

    def test_neighbours(self):
        lo, hi = audit.MODULE
        self.assertEqual(contract_screens_audit.MODULE[1], lo)
        pools = gi.source_pools(self.export)

        def sources(entry, end):
            found = set()
            for address in range(entry, end, 4):
                match = TOC_LOAD.match(self.export.assembly.get(address, ""))
                if match and len(pools.get(int(match.group(1), 0), ())) == 1:
                    found.add(pools[int(match.group(1), 0)][0])
            return found

        named = set()
        for entry, end in audit.ROUTINES:
            named |= sources(entry, end)
        self.assertEqual(named, {"DATABASE.CPP"})
        next_end = max(gi.body(self.export, hi, self.starts, self.tables)[0]) + 4
        self.assertEqual(sources(hi, next_end), {"DISCIPLINE.CPP"})
        initializers = {entry: end for entry, end, _, _ in engine_initialization_audit.CONTRACTS}
        self.assertEqual(initializers.get(audit.INITIALIZER), dict(audit.ROUTINES)[audit.INITIALIZER])
        self.assertEqual(audit.ROUTINES[-1][0], audit.INITIALIZER)

    def test_rewritten_digests(self):
        with closing(sqlite3.connect(EXPORT / "instructions.sqlite")) as db:
            audit.require_database_index(db)


if __name__ == "__main__":
    unittest.main()
