"""Certify DISPLAY.CPP's 32 entries, the unit's bounds and its production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest

import display_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x10718b40, 0x10718ba0, 24, '6c6ece9764c24dc1'), (0x10718ba0, 0x10718d74, 117, 'e203b1a7b326ad9c'),
    (0x10718d80, 0x10718dd0, 20, '0660a1be6f62c50f'), (0x10718dd0, 0x10718e20, 20, '8fad5d4341ae36c2'),
    (0x10718e20, 0x10718ee4, 49, '95e5885bb050fe5b'), (0x10718ef0, 0x10718f30, 16, '9ef0566e41ac61ac'),
    (0x10718f30, 0x10718f60, 12, '3f47e384855cd16f'), (0x10718f60, 0x10718fbc, 23, 'ec0698d85e4324c6'),
    (0x10718fc0, 0x10719348, 226, 'c97ea3d9da3f7fb0'), (0x10719350, 0x10719554, 129, '7164351f24c27e3d'),
    (0x10719560, 0x10719740, 120, 'ea042437e387bfe7'), (0x10719740, 0x107198c0, 96, '108da3ce38a23b23'),
    (0x107198c0, 0x1071a0f4, 525, '15d7a931ef141b86'), (0x1071a100, 0x1071a1bc, 47, '5528e66647ee23d7'),
    (0x1071a1c0, 0x1071a27c, 47, 'e28dbf666411cec6'), (0x1071a280, 0x1071aa10, 481, '597178760b770d3e'),
    (0x1071aa10, 0x1071add0, 240, 'a2d8f4e6b9b707d8'), (0x1071add0, 0x1071afb8, 122, 'fc2027eb23fa30e2'),
    (0x1071afc0, 0x1071b514, 340, '579d27d3142cef28'), (0x1071b520, 0x1071bcf8, 502, '28539945da46cfe5'),
    (0x1071bd00, 0x1071c070, 220, 'cbe2fd14cb76ac9c'), (0x1071c070, 0x1071c188, 70, '1c964f2bca7b0702'),
    (0x1071c190, 0x1071c1d0, 16, '47c3f6eb1c1cc994'), (0x1071c1d0, 0x1071c2cc, 63, '6b1764be35f1c1ed'),
    (0x1071c2d0, 0x1071de80, 1770, '59c2e76b86f03d4b'), (0x1071de80, 0x10723650, 5618, '71d88348d3337988'),
    (0x10723650, 0x10723844, 119, 'cd2cd57daca44b5d'), (0x10723850, 0x10723a24, 117, '4a6472e69885c2e1'),
    (0x10723a30, 0x10723ca4, 157, 'd0685b9a0111325c'), (0x10723cb0, 0x10725770, 1712, '4f280e23da78f67d'),
    (0x10725770, 0x10725998, 137, 'e62e5ce4a25db12a'), (0x107259a0, 0x107259fc, 23, '4c92c257100a21b8'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# Calls from outside the unit, per entry: every one a `bl` of an entry.
INCOMING = {0x10718b40: 2, 0x10718ba0: 2, 0x10718e20: 1, 0x10718ef0: 1, 0x10718f30: 1, 0x10718f60: 1, 0x10718fc0: 2, 0x10719350: 3, 0x10719560: 5, 0x10719740: 961, 0x107198c0: 543, 0x1071c070: 1, 0x1071c1d0: 1, 0x1071de80: 37, 0x10723a30: 1, 0x10723cb0: 1, 0x107259a0: 1}
# Entries another routine of the unit calls.
CROSS = {0x10718ef0, 0x10718fc0, 0x10719560, 0x107198c0, 0x1071a100, 0x1071a1c0, 0x1071a280, 0x1071aa10, 0x1071add0, 0x1071afc0, 0x1071b520, 0x1071bd00, 0x1071c190, 0x1071c2d0, 0x10723650, 0x10723850, 0x10725770}
# The unit's private TOC slots with (loads, first load, last load). No other code loads them.
PRIVATE_SLOTS = {
    '-0x3f9c': (1, 0x107259dc, 0x107259dc),
    '-0x3f98': (1, 0x107259c4, 0x107259c4),
    '-0x3f94': (1, 0x10721e30, 0x10721e30),
    '-0x3f90': (1, 0x1071df48, 0x1071df48),
    '-0x3f8c': (18, 0x10718e30, 0x10725780),
    '-0x35b8': (3, 0x107257bc, 0x10725808),
    '-0x35b4': (2, 0x1072578c, 0x107257a4),
    '-0x35b0': (3, 0x107257a0, 0x107257e0),
    '-0x5c28': (1, 0x107259ac, 0x107259ac),
    '-0x5c08': (4, 0x10719574, 0x107259d0),
    '-0x5bfc': (8, 0x10718e28, 0x1071ff24),
    '-0x5bf8': (1, 0x10718d38, 0x10718d38),
    '-0x5bf4': (1, 0x10718d24, 0x10718d24),
}
# The neighbouring units' slots on either side of the runs: (loads, first, last), all outside the unit.
NEIGHBOUR_SLOTS = {
    '-0x3fa0': (17, 0x10715708, 0x107189e8),
    '-0x3f88': (3, 0x10725a0c, 0x107268d8),
    '-0x5c30': (2, 0x10715120, 0x10725ef8),
    '-0x5be8': (1, 0x10725ec8, 0x10725ec8),
}
# The two pointers to the unit outside its jump tables: transition vectors (code address, then the
# TOC) of the display table's two element destructors, each loaded through its own TOC slot.
VECTORS = {0x108bd5c4: (0x10718d80, -0x5bf8), 0x108bd5cc: (0x10718dd0, -0x5bf4)}
# The table destructor 0x10718ba0 also has a transition vector, which no TOC slot or data word names
# (it is called directly).
UNNAMED_VECTORS = {0x108bd5bc: 0x10718ba0}
# The jump tables: TOC slot -> (the `bctr`, the routine every case stays in, the number of cases).
JUMP_TABLES = {-0x3f90: (0x1071df58, 0x1071de80, 27), -0x3f94: (0x10721e40, 0x1071de80, 11)}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private DISPLAY export unavailable; inventory cannot be certified')


class DisplayAuditTests(unittest.TestCase):
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

    def routine_of(self, address):
        return next((start for start, end in audit.ROUTINES if start <= address < end), None)

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.MODULE, (0x10718b40, 0x10725a00))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.DISPLAY_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 32)
        self.assertEqual(len(self.records), 13178)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_display_index(db, complete=False)

    def test_every_entry_is_routed_to_the_units_owner_alone(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        # Every entry is routed (no draft left), and nothing else owns one.
        self.assertTrue(audit.COMPLETE)
        self.assertEqual(audit.rewritten(), [(a, b, pinned) for a, b, _, pinned in CONTRACTS])
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ['display_adapter'])
        # The three older builder owners stay listed but are taken over: not routed, their hooks removed.
        from compile_recomp import DISPLAY_TAKEOVERS
        self.assertEqual(sorted((at, owner) for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES),
                         [(0x10718b40, 'aggregate_object_adapter'), (0x10718ba0, 'aggregate_object_adapter'),
                          (0x107259a0, 'engine_initialization_adapter')])
        self.assertEqual(DISPLAY_TAKEOVERS, frozenset(audit.TAKEOVERS))
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_display_index(db)
            audit.require_display_index(db, complete=True)

    def assert_hook(self, code, entry):
        from turbo_pages import page_hook
        hook = page_hook(entry >> 12, entry, f'try_modern_display(s, 0x{entry:x})')
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        steps = [t for t in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if t in fragment]
        self.assertEqual(len(steps), 1)
        self.assertLess(fragment.index(hook), fragment.index(steps[0]))

    def test_normal_hooks_precede_original_steps_and_detect_missing_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_display_hooks(page, part, normal)
            self.assertIn('#include "display_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                from turbo_pages import page_hook
                hook = page_hook(page, entry, f'try_modern_display(s, 0x{entry:x})')
                step = next(t for t in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if t in normal)
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_display_hooks(page, part, broken)

    def test_unit_bounds_are_its_private_toc_slots_and_its_neighbours(self):
        pool = self.word(TOC - 0x3f8c)
        self.assertEqual(self.original_bytes(pool, 0x30).split(b'\0')[:3], [b'DISPLAY.CPP', b'v%s %s.%s %d', b'Error'])
        # DATE.CPP's pool is the code slot before the run, GUIO.CPP's (it starts "     %s") the one after.
        self.assertEqual(self.original_bytes(self.word(TOC - 0x3fa0), 9), b'DATE.CPP\0')
        self.assertIn(b'GUIO.CPP\0', self.original_bytes(self.word(TOC - 0x3f88), 0x20))

        def loads(slot):
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                found = [a for a, in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                (f'%{slot}(r2)%',))]
            return len(found), found[0], found[-1]
        for slot, expected in PRIVATE_SLOTS.items():
            self.assertEqual(loads(slot), expected, slot)
            self.assertTrue(audit.MODULE[0] <= expected[1] <= expected[2] < audit.MODULE[1])
        for slot, expected in NEIGHBOUR_SLOTS.items():
            self.assertEqual(loads(slot), expected, slot)
            self.assertFalse(audit.MODULE[0] <= expected[1] < audit.MODULE[1])
            self.assertFalse(audit.MODULE[0] <= expected[2] < audit.MODULE[1])
        # DATE.CPP's audit ends where this unit starts; GUIO.CPP's first routine follows the last one.
        import date_audit
        self.assertEqual(date_audit.MODULE[1], 0x10718b38)
        self.assertEqual(self.routine_of(0x1071df48), 0x1071de80)
        self.assertEqual(self.routine_of(0x107259dc), ENTRIES[-1])

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
        self.assertEqual(sum(incoming.values()), 1564)
        self.assertEqual(fallthroughs, [])
        # Every entry is reached: by an outside call, a call inside the unit, or a vector.
        vectored = {target for target, _ in VECTORS.values()}
        self.assertEqual(set(INCOMING) | CROSS | vectored, set(ENTRIES))
        self.assertEqual(vectored & (set(INCOMING) | CROSS), set())

    def test_cross_entry_edges_are_calls_of_entries_and_computed_flow_is_the_two_tables(self):
        cross, computed = set(), {}
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                if 'COMPUTED' in record['flow']:
                    self.assertEqual((record['assembly'], record['flow']), ('bctr', 'COMPUTED_JUMP'))
                    computed[record['address']] = start
                targets = record['flows'] + ([record['fallthrough']] if record.get('fallthrough') is not None else [])
                for target in targets:
                    if not start <= target < end:
                        # No tail call and no branch out of a routine: only calls leave it.
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        if audit.MODULE[0] <= target < audit.MODULE[1]:
                            self.assertIn(target, ENTRIES)
                            cross.add(target)
        self.assertEqual(cross, CROSS)
        self.assertEqual(computed, {bctr: routine for _, (bctr, routine, _) in JUMP_TABLES.items()})
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

    def test_pointers_to_the_unit_are_two_vectors_and_its_own_jump_tables(self):
        code_end = self.segments[0][0] + len(self.segments[0][1])
        tables = {self.word(TOC + slot): (routine, count) for slot, (_, routine, count) in JUMP_TABLES.items()}
        cases = {table: [self.word(table + 4 * n) for n in range(count)] for table, (_, count) in tables.items()}
        for table, targets in cases.items():
            self.assertTrue(all(self.routine_of(t) == tables[table][0] for t in targets))
        case_words = {table + 4 * n for table, targets in cases.items() for n in range(len(targets))}
        seen = set()
        for start, contents in self.segments:
            for match in re.finditer(b'\x10[\x71\x72]', contents):
                at = start + match.start()
                if match.start() + 4 > len(contents) or at % 4:
                    continue
                target = int.from_bytes(contents[match.start():match.start() + 4], 'big')
                if not audit.MODULE[0] <= target < audit.MODULE[1]:
                    continue
                if at in UNNAMED_VECTORS:
                    self.assertEqual((target, self.word(at + 4)), (UNNAMED_VECTORS[at], TOC))
                    for begin, data in self.segments:
                        self.assertNotIn(at.to_bytes(4, 'big'), [data[i:i + 4] for i in range(0, len(data) - 3, 4)])
                    seen.add(at)
                elif at in VECTORS:
                    entry, slot = VECTORS[at]
                    self.assertEqual((target, self.word(at + 4), self.word(TOC + slot)), (entry, TOC, at))
                    seen.add(at)
                elif at >= code_end:
                    self.assertIn(at, case_words)
        self.assertEqual(seen, set(VECTORS) | set(UNNAMED_VECTORS))
        self.assertEqual(sum(len(t) for t in cases.values()), 38)

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[1]
            line = f'DISPLAY_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_display_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_display_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing DISPLAY.CPP instructions'):
                    audit.require_display_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)


if __name__ == '__main__':
    unittest.main()
