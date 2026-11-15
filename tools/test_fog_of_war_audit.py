"""Certify fog_of_war.cpp's 24 entries, the unit's bounds and its production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import json
from pathlib import Path
import sqlite3
import tempfile
import unittest

import fog_of_war_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x106fd120, 0x106fd230, 68, 'c36d0b44ec63bc0c'), (0x106fd230, 0x106fd2b8, 34, 'e0d3b3eef963039a'),
    (0x106fd2c0, 0x106fd6c8, 257, '1c2c306f56aef6d9'), (0x106fd6d0, 0x106fd98c, 175, '887606c53536085f'),
    (0x106fd990, 0x106fd9e8, 22, '7b80f6bd37ff2038'), (0x106fd9f0, 0x106fda80, 36, '62fa5d522b0bd310'),
    (0x106fda80, 0x106fdafc, 31, '0af53ffb46c2398c'), (0x106fdb00, 0x106fdb30, 12, 'fcf70a3d07e477cb'),
    (0x106fdb30, 0x106fdc8c, 87, 'ecca8badad4adbd8'), (0x106fdc90, 0x106fdddc, 83, 'a715d4e5b13d3818'),
    (0x106fdde0, 0x106fde44, 25, '7ac91206d677035d'), (0x106fde50, 0x106fe000, 108, 'dae13f92bc715a37'),
    (0x106fe000, 0x106fe128, 74, '2d0036970716b841'), (0x106fe130, 0x106fe218, 58, '92f80b55cc2b2c8e'),
    (0x106fe220, 0x106fe27c, 23, '80395dd969cd5575'), (0x106fe280, 0x106fe3f0, 92, '30454f6655edbbe2'),
    (0x106fe3f0, 0x106fe700, 196, '21ff4ed0c4fe8b91'), (0x106fe700, 0x106fe7d0, 52, '4420a25b98ae6419'),
    (0x106fe7d0, 0x106fe818, 18, '6aba26af58909700'), (0x106fe820, 0x106fec6c, 275, 'd9ac3fd00eb3fcb3'),
    (0x106fec70, 0x106fef10, 168, 'c28179d9fc273405'), (0x106fef10, 0x106ff488, 350, '1829f66765b01d32'),
    (0x106ff490, 0x106ff4f0, 24, '08ba86404c6f0541'), (0x106ff4f0, 0x106ff550, 24, '09de2119bb5edf2d'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# Calls from outside the unit, per entry: 23 sites, every one a `bl` of an entry.
INCOMING = {0x106fd120: 2, 0x106fd230: 1, 0x106fd6d0: 1, 0x106fd990: 1, 0x106fd9f0: 1, 0x106fda80: 1,
            0x106fdb00: 8, 0x106fdb30: 1, 0x106fe280: 3, 0x106fec70: 4}
# Entries another routine of the unit calls (0x106fef10 also calls itself).
CROSS = {0x106fd2c0, 0x106fda80, 0x106fdc90, 0x106fdde0, 0x106fde50, 0x106fe000, 0x106fe130, 0x106fe220,
         0x106fe3f0, 0x106fe700, 0x106fe7d0, 0x106fe820, 0x106fec70, 0x106fef10, 0x106ff490, 0x106ff4f0}
# The unit's private TOC slots with (loads, first load, last load). No other code loads them.
PRIVATE_SLOTS = {
    '-0x405c': (4, 0x106fe728, 0x106fef2c),   # the double pool
    '-0x4058': (5, 0x106fd2dc, 0x106fe018),   # the string pool
}
# The neighbouring units' slots on either side: (loads, first, last), all outside the unit.
NEIGHBOUR_SLOTS = {'-0x4060': (1, 0x106fcbcc, 0x106fcbcc), '-0x4054': (2, 0x106ff560, 0x106ff6a0),
                   '-0x5c8c': (1, 0x106fc82c, 0x106fc82c), '-0x5c90': (1, 0x106fc834, 0x106fc834)}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private fog_of_war export unavailable; inventory cannot be certified')


class FogOfWarAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        cls.program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes())
                        for s in cls.program['segments'] if 'file' in s]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            cls.records = routine_records(db, *audit.MODULE)
        cls.by_address = {r['address']: r for r in cls.records}

    def original_bytes(self, address, count):
        for start, contents in self.segments:
            if start <= address and address + count <= start + len(contents):
                return contents[address - start:address - start + count]
        self.fail(f'Original bytes unavailable at {address:x}')

    def word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), 'big')

    def loads(self, slot):
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            found = [a for a, in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                            (f'%{slot}(r2)%',))]
        return len(found), found[0], found[-1]

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.MODULE, (0x106fd120, 0x106ff550))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.FOG_OF_WAR_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 24)
        self.assertEqual(len(self.records), 2292)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_fog_of_war_index(db, complete=False)

    def test_every_entry_is_routed_to_the_units_owner_alone(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        self.assertTrue(audit.COMPLETE)
        self.assertEqual(audit.rewritten(), [(a, b, pinned) for a, b, _, pinned in CONTRACTS])
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ['fog_of_war_adapter'])
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_fog_of_war_index(db)
            audit.require_fog_of_war_index(db, complete=True)

    def assert_hook(self, code, entry):
        hook = audit.fog_of_war_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_normal_hooks_precede_original_steps_and_detect_missing_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_fog_of_war_hooks(page, part, normal)
            self.assertIn('#include "fog_of_war_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.fog_of_war_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_fog_of_war_hooks(page, part, broken)

    def test_unit_bounds_are_its_private_toc_slots_and_its_neighbours(self):
        pool = self.word(TOC - 0x4058)
        self.assertEqual(self.original_bytes(pool, 0x42).split(b'\0')[:5],
                         [b'fog_of_war.cpp', b'v%s %s.%s %d', b'Error', b'fog.dat', b'Initialising game data'])
        doubles = self.word(TOC - 0x405c)
        self.assertEqual(doubles, pool + 0x44)
        self.assertEqual(self.original_bytes(doubles, 0x20).hex(),
                         '4021000000000000' '4330000000000000' '4330000080000000' '40efffe000000000')
        self.assertTrue(self.original_bytes(self.word(TOC - 0x4054), 0x1e).startswith(b'new_transfer_rule_screens.cpp'))
        self.assertTrue(self.original_bytes(self.word(TOC - 0x4060), 0x10).startswith(b'Please Confirm'))
        for slot, expected in PRIVATE_SLOTS.items():
            self.assertEqual(self.loads(slot), expected, slot)
            self.assertTrue(audit.MODULE[0] <= expected[1] <= expected[2] < audit.MODULE[1])
        for slot, expected in NEIGHBOUR_SLOTS.items():
            self.assertEqual(self.loads(slot), expected, slot)
            self.assertFalse(audit.MODULE[0] <= expected[1] < audit.MODULE[1])
            self.assertFalse(audit.MODULE[0] <= expected[2] < audit.MODULE[1])
        # Before: the unit's two routines at 0x106fcbb0 and 0x106fcee0 are reached through its vectors.
        self.assertEqual((self.word(self.word(TOC - 0x5c8c)), self.word(self.word(TOC - 0x5c90))), (0x106fcbb0, 0x106fcee0))

    def test_all_incoming_edges_are_calls_of_entries_and_no_external_fallthrough(self):
        incoming = {}
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    'WHERE f.value>=? AND f.value<? ORDER BY i.address', audit.MODULE):
                if audit.MODULE[0] <= address < audit.MODULE[1]:
                    continue
                record = json.loads(body)
                for target in record['flows']:
                    if audit.MODULE[0] <= target < audit.MODULE[1]:
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        self.assertIn(target, ENTRIES)
                        incoming[target] = incoming.get(target, 0) + 1
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*audit.MODULE, *audit.MODULE)))
        self.assertEqual(incoming, INCOMING)
        self.assertEqual(sum(incoming.values()), 23)
        self.assertEqual(fallthroughs, [])
        self.assertEqual(set(INCOMING) | CROSS, set(ENTRIES))

    def test_cross_entry_edges_are_calls_of_entries_and_no_computed_flow(self):
        cross = set()
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                self.assertNotIn('COMPUTED', record['flow'])
                targets = record['flows'] + ([record['fallthrough']] if record.get('fallthrough') is not None else [])
                for target in targets:
                    if not start <= target < end:
                        # No tail call and no branch out of a routine: only calls leave it.
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        if audit.MODULE[0] <= target < audit.MODULE[1]:
                            self.assertIn(target, ENTRIES)
                            cross.add(target)
                    elif record['flow'] == 'UNCONDITIONAL_CALL' and target == start:
                        cross.add(target)   # the random draw's call of itself
        self.assertEqual(cross, CROSS)
        self.assertEqual([s for s in self.program['symbols']
                          if audit.MODULE[0] <= s['address'] < audit.MODULE[1]], [])

    def test_every_instruction_and_unreachable_word_accounted(self):
        addresses = set()
        for start, end in audit.ROUTINES:
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual([r['address'] for r in records],
                             [a for a in range(start, end, 4) if a not in audit.UNREACHABLE])
            self.assertEqual(records[-1]['assembly'], 'blr')
            addresses.update(range(start, end, 4))
        targets = {t for r in self.records for t in r['flows']}
        for address in audit.UNREACHABLE:
            self.assertEqual(self.word(address), 0x60000000)
            self.assertNotIn(address, self.by_address)
            self.assertNotIn(address, targets)
            self.assertEqual(self.by_address[address - 4]['flow'], 'UNCONDITIONAL_JUMP')
        for address in set(range(*audit.MODULE, 4)) - addresses:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)

    def test_no_pointer_names_the_unit(self):
        code_end = self.segments[0][0] + len(self.segments[0][1])
        for start, contents in self.segments:
            for offset in range(0, len(contents) - 3, 4):
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if audit.MODULE[0] <= target < audit.MODULE[1]:
                    # No vector or jump table: any aligned word naming the unit is code.
                    self.assertLess(start + offset, code_end)

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[1]
            line = f'FOG_OF_WAR_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_fog_of_war_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_fog_of_war_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing fog_of_war.cpp instructions'):
                    audit.require_fog_of_war_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)


if __name__ == '__main__':
    unittest.main()
