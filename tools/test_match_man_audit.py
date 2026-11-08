#!/usr/bin/env python3
"""Certify MATCH_MAN.CPP's closed inventory against the private export.

The export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import hashlib
from pathlib import Path
import re
import sqlite3
import unittest

import game_inventory as gi
import match_eng_audit
import match_man_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
TOC_LOAD = re.compile(r"lwz r\d+,(-?0x[0-9a-f]+|-?\d+)\(r2\)$")
UNIT_SLOTS = (-0x5294, -0x5274)  # MATCH_MAN.CPP's TOC slots, inclusive


class MatchManAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (EXPORT / "instructions.sqlite").is_file():
            raise RuntimeError("Private export unavailable; the MATCH_MAN.CPP inventory cannot be certified")
        cls.export = gi.Export(EXPORT)
        cls.vectors = gi.transition_vectors(cls.export)
        cls.starts = gi.entries(cls.export, cls.vectors, gi.discovery_roots())
        cls.tables = {a: cases for a, flow in cls.export.flow.items() if flow == "COMPUTED_JUMP"
                      and (cases := gi.table_cases(cls.export.assembly, cls.export.word, a))}
        for site, _, table, count, _ in audit.SWITCH_TABLES:
            cls.tables[site] = [cls.export.word(table + 4 * n) for n in range(count)]
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
        self.assertEqual(len(audit.ROUTINES), 23)
        self.assertEqual(sum(n for _, _, n, _ in audit.CONTRACTS), 21056)

    def test_bodies_stay_inside_and_cover_the_module(self):
        lo, hi = audit.MODULE
        covered = set()
        for entry, end in audit.ROUTINES:
            instructions, _, _, unresolved, missing = self.bodies[entry]
            self.assertEqual((unresolved, missing), ([], []), f"{entry:x}")
            self.assertTrue(all(entry <= a < end for a in instructions), f"{entry:x} leaves its range")
            inside = set(range(entry, end, 4))
            self.assertEqual(inside - instructions, audit.UNREACHABLE & inside, f"{entry:x}")
            covered |= instructions
        code = next(data for start, data in self.export.segments if start == gi.CODE_START)
        for address in audit.UNREACHABLE:
            word = int.from_bytes(code[address - gi.CODE_START:address - gi.CODE_START + 4], "big")
            # An alignment nop or a leftover branch.
            self.assertTrue(word == 0x60000000 or word >> 26 == 18, f"{address:x} holds {word:08x}")
            self.assertNotIn(address, self.export.flow)
            previous = self.export.assembly[address - 4]
            self.assertTrue(previous.startswith("b 0x"), f"{address:x} follows {previous}")
        for address in range(lo, hi, 4):
            if address in covered or address in audit.UNREACHABLE:
                continue
            self.assertNotIn(address, self.export.flow, f"{address:x} is code outside every routine")
            self.assertEqual(code[address - gi.CODE_START:address - gi.CODE_START + 4], b"\0\0\0\0",
                             f"{address:x} is not padding")

    def test_switch_tables(self):
        computed = {a for entry, end in audit.ROUTINES for a in range(entry, end, 4)
                    if self.export.flow.get(a) == "COMPUTED_JUMP"}
        self.assertEqual(len(computed), 4)
        pinned_sites = {site for site, *_ in audit.SWITCH_TABLES}
        # The others follow the pattern check_jump_tables.py resolves.
        for site in computed - pinned_sites:
            self.assertTrue(gi.table_cases(self.export.assembly, self.export.word, site), f"{site:x}")
        for site, slot, table, count, pinned in audit.SWITCH_TABLES:
            self.assertIsNone(gi.table_cases(self.export.assembly, self.export.word, site) or None)
            self.assertEqual(self.export.assembly[site], "bctr")
            self.assertTrue(self.export.assembly[site - 4].startswith("mtspr CTR"))
            self.assertEqual(self.export.word((gi.TOC + slot) & 0xFFFFFFFF), table)
            prefix = [self.export.assembly.get(a, "") for a in range(site - 64, site)]
            self.assertIn(f"lwz r3,-0x{-slot:x}(r2)", prefix)
            bound = [line for line in prefix if line.startswith("cmplwi")]
            self.assertEqual(bound, [f"cmplwi r0,0x{count - 1:x}"])
            data = b"".join(self.export.word(table + 4 * n).to_bytes(4, "big") for n in range(count))
            self.assertEqual(hashlib.sha256(data).hexdigest(), pinned)
        for site in computed:
            entry = next(e for e, end in audit.ROUTINES if e <= site < end)
            for target in self.tables[site]:
                self.assertIn(target, self.bodies[entry][0], f"{site:x} -> {target:x}")
                self.assertNotIn(target, self.starts)

    def test_flow_from_outside_only_reaches_entries(self):
        lo, hi = audit.MODULE
        entries = {entry for entry, _ in audit.ROUTINES}
        for address, target in self.export.target.items():
            if lo <= target < hi and not lo <= address < hi:
                self.assertIn(target, entries, f"{address:x} enters MATCH_MAN.CPP at {target:x}")
        for address in self.vectors:
            if lo <= address < hi:
                self.assertIn(address, entries)
        for entry, (_, _, tails, _, _) in self.bodies.items():
            self.assertEqual(tails, [], f"{entry:x} tail-calls")

    def test_neighbours(self):
        lo, hi = audit.MODULE
        pools = gi.source_pools(self.export)
        self.assertEqual(pools[UNIT_SLOTS[1]], ["MATCH_MAN.CPP"])
        self.assertEqual(pools[-0x52a0], ["MATCH_EVENTS.CPP"])
        self.assertEqual(pools[-0x5270], ["MATCH_OFFICIAL.CPP"])
        # Every unit slot the module loads is MATCH_MAN.CPP's.
        unit = range(-0x52a0, -0x5260, 4)
        loaded = set()
        for entry, end in audit.ROUTINES:
            loaded |= self.slots(entry, end) & set(unit)
        self.assertTrue(loaded)
        self.assertTrue(all(UNIT_SLOTS[0] <= slot <= UNIT_SLOTS[1] for slot in loaded), sorted(loaded))
        # The last routine before: MATCH_EVENTS.CPP's; the first after: MATCH_OFFICIAL.CPP's
        # constructors, then routines loading MATCH_OFFICIAL.CPP's slots.
        before = max(e for e in self.starts if e < lo)
        self.assertEqual(max(gi.body(self.export, before, self.starts, self.tables)[0]) + 4, lo)
        self.assertEqual([self.export.assembly[a] for a in range(hi, hi + 16, 4)],
                         ["li r0,0x0", "stw r0,0x0(r3)", "stw r0,0x4(r3)", "blr"])
        constructor = 0x1021b020
        self.assertIn(constructor, self.starts)
        self.assertEqual(self.export.assembly[constructor + 0x28], "stw r4,0x0(r3)")
        self.assertEqual(self.export.assembly[constructor + 0x2c], "stw r27,0x4(r3)")
        official = next(e for e in sorted(self.starts) if e > constructor)
        self.assertTrue(self.slots(official, max(gi.body(self.export, official, self.starts, self.tables)[0]) + 4)
                        & set(range(-0x5270, -0x5260, 4)))
        self.assertEqual(match_eng_audit.MODULE[1] <= lo, True)
        # The function-pointer vectors: MATCH_ENG.CPP's group, MATCH_MAN.CPP's four, MATCH_PL.CPP's.
        group = [self.export.word(a) for a in range(0x108b8c34, 0x108b8c64, 8)]
        self.assertEqual(group, [0x101d91b0, 0x1020ee00, 0x1020ea00, 0x10206cb0, 0x102065e0, 0x102270f0])
        self.assertTrue(all(self.export.word(a) == gi.TOC for a in range(0x108b8c38, 0x108b8c64, 8)))

    def test_rewritten_digests(self):
        with closing(sqlite3.connect(EXPORT / "instructions.sqlite")) as db:
            for entry, end, _, pinned in audit.CONTRACTS:
                self.assertEqual(digest(routine_records(db, entry, end)), pinned, f"{entry:x}")
            audit.require_match_man_index(db)


if __name__ == "__main__":
    unittest.main()
