#!/usr/bin/env python3
"""Certify MANAGER_MANAGER.CPP's closed inventory against the private export.

The export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
from pathlib import Path
import re
import sqlite3
import unittest

import game_inventory as gi
import manager_manager_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
TOC_LOAD = re.compile(r"lwz r\d+,(-?0x[0-9a-f]+|-?\d+)\(r2\)$")
UNIT_SLOTS = (-0x5420, -0x5360)  # MANAGER_MANAGER.CPP's TOC slots, inclusive


class ManagerManagerAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (EXPORT / "instructions.sqlite").is_file():
            raise RuntimeError("Private export unavailable; the MANAGER_MANAGER.CPP inventory cannot be certified")
        cls.export = gi.Export(EXPORT)
        cls.vectors = gi.transition_vectors(cls.export)
        all_starts = gi.entries(cls.export, cls.vectors, gi.discovery_roots())
        cls.all_starts = all_starts
        # The case roots are jump-table cases (folded into their routine), named only by the root list.
        cls.starts = {a: r for a, r in all_starts.items() if a not in audit.CASE_ROOTS}
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
        self.assertEqual(len(audit.ROUTINES), 105)
        self.assertEqual(sum(n for _, _, n, _ in audit.CONTRACTS), 49690)

    def test_case_roots_are_cases(self):
        for root in audit.CASE_ROOTS:
            self.assertEqual(self.all_starts[root], {"root"}, f"{root:x}")
            host = next(e for e, end in audit.ROUTINES if e <= root < end)
            self.assertIn(host, (0x101b3c90, 0x101b83f0))
            self.assertIn(root, self.bodies[host][0])
            self.assertTrue(any(root in self.tables.get(a, []) for a in self.bodies[host][0]), f"{root:x}")

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

    def test_switch_tables_resolve(self):
        for entry, end in audit.ROUTINES:
            for address in range(entry, end, 4):
                if self.export.flow.get(address) == "COMPUTED_JUMP":
                    self.assertIn(address, self.tables, f"{address:x}")
                    for target in self.tables[address]:
                        self.assertIn(target, self.bodies[entry][0], f"{address:x} -> {target:x}")

    def test_flow_from_outside_only_reaches_entries(self):
        lo, hi = audit.MODULE
        entries = {entry for entry, _ in audit.ROUTINES}
        for address, target in self.export.target.items():
            if lo <= target < hi and not lo <= address < hi:
                self.assertIn(target, entries, f"{address:x} enters MANAGER_MANAGER.CPP at {target:x}")
        for address in self.vectors:
            if lo <= address < hi:
                self.assertIn(address, entries)

    def test_neighbours(self):
        lo, hi = audit.MODULE
        pools = gi.source_pools(self.export)
        self.assertEqual(pools[-0x5368], ["MANAGER_MANAGER.CPP"])
        self.assertEqual(pools[-0x5424], ["KEY_NATION.CPP"])
        self.assertEqual(pools[-0x5310], ["MANAGER_SCREENS.CPP"])
        # Every unit slot the module loads is MANAGER_MANAGER.CPP's.
        loaded = set()
        for entry, end in audit.ROUTINES:
            loaded |= {s for s in self.slots(entry, end) if -0x5500 < s < -0x5200}
        self.assertTrue(loaded)
        self.assertTrue(all(UNIT_SLOTS[0] <= s <= UNIT_SLOTS[1] for s in loaded), sorted(map(hex, loaded)))
        # After: MANAGER_SCREENS.CPP's first routine loads the slots next to its pool.
        self.assertIn(hi, self.starts)
        self.assertTrue({-0x5304, -0x5308, -0x530c} <= self.slots(hi, hi + 0x170))
        # Before: KEY_NATION.CPP's static initializer fills the table its routine 0x10193550 walks,
        # and the destructor 0x101935f0 sits in KEY_NATION.CPP's vector group.
        last_end = max(gi.body(self.export, 0x101935f0, self.starts, self.tables)[0]) + 4
        self.assertTrue(last_end <= lo < last_end + 16)  # then alignment padding
        self.assertFalse([e for e in self.starts if last_end <= e < lo])
        self.assertIn("lwz r3,-0x7b44(r2)", [self.export.assembly[a] for a in range(0x101935b0, 0x101935e0, 4)])
        self.assertEqual(self.export.assembly[0x10193550], "lwz r4,-0x7b44(r2)")
        group = [self.export.word(a) for a in range(0x108b8b5c, 0x108b8b84, 8)]
        self.assertEqual(group, [0x101935f0, 0x10193010, 0x101b8af0, 0x101b75c0, 0x101b6ff0])
        self.assertTrue(all(self.export.word(a) == gi.TOC for a in range(0x108b8b60, 0x108b8b84, 8)))
        self.assertTrue(self.slots(0x10193420, 0x101934f0) & {-0x5424})
        # 0x101c42c0: its only caller passes the receiver it passes to this unit's 0x101bb900.
        callers = [a for a, t in self.export.target.items() if t == 0x101c42c0]
        self.assertEqual(callers, [0x105d6d20])
        self.assertEqual(self.export.target[0x105d6d04], 0x101bb900)
        self.assertEqual(self.export.assembly[0x105d6cf4], "lwz r3,0x0(r27)")
        self.assertEqual(self.export.assembly[0x105d6d14], "lwz r3,0x0(r27)")

    def test_rewritten_digests(self):
        with closing(sqlite3.connect(EXPORT / "instructions.sqlite")) as db:
            for entry, end, _, pinned in audit.CONTRACTS:
                self.assertEqual(digest(routine_records(db, entry, end)), pinned, f"{entry:x}")
            audit.require_manager_manager_index(db)


if __name__ == "__main__":
    unittest.main()
