"""Certify FINANCE.CPP's 58 entries and the unit's bounds.

The private export is required evidence, rather than a reason to skip tests.
The C++ of this unit is not written yet: no entry has an owner, and that test
turns over at delivery.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest

import finance_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x100f1000, 0x100f1004, 1, '2fa8501b41f265dc'), (0x100f1010, 0x100f104c, 15, 'bff18379b66e752d'),
    (0x100f1050, 0x100f20fc, 1067, '99ce452fc80e4f12'), (0x100f2100, 0x100f2678, 350, '97ce6bc5aa0d4b7a'),
    (0x100f2680, 0x100f478c, 2115, '9855fa5c2a993402'), (0x100f4790, 0x100f4f14, 480, '1358cc3f6b54d192'),
    (0x100f4f20, 0x100f4f80, 24, 'a6ce7617df977de7'), (0x100f4f80, 0x100f55a8, 394, '7a4a511c518ccd68'),
    (0x100f55b0, 0x100f5730, 96, 'caab3e59e3a4473c'), (0x100f5730, 0x100f5768, 14, 'f74acbbb74e8205a'),
    (0x100f5770, 0x100f7050, 1590, 'df0ab9ff3c5de9c8'), (0x100f7050, 0x100f709c, 19, 'aeee709928528bdb'),
    (0x100f70a0, 0x100f70d8, 14, '14de89901c37ef30'), (0x100f70e0, 0x100f712c, 19, '9e6f70a557f1b5ff'),
    (0x100f7130, 0x100f7168, 14, 'ca41dc1bfedfc5c1'), (0x100f7170, 0x100f71a8, 14, '02879ddaaa5b8f30'),
    (0x100f71b0, 0x100f71e8, 14, 'f954dad24ecb3a70'), (0x100f71f0, 0x100f7228, 14, '42b8a34bc450169f'),
    (0x100f7230, 0x100f7268, 14, 'e472ac15d7cefd84'), (0x100f7270, 0x100f7ce4, 668, 'f32e5b9fe36b53be'),
    (0x100f7cf0, 0x100f84d4, 505, '6ec70f6f99bf4e84'), (0x100f84e0, 0x100f8714, 141, 'ae530b3f030731e2'),
    (0x100f8720, 0x100f8780, 24, '72ca3f97243b9cb3'), (0x100f8780, 0x100f95d8, 917, 'ddc3ee3efaf88e50'),
    (0x100f95e0, 0x100f9afc, 326, '550058a60d7fcef9'), (0x100f9b00, 0x100f9c44, 81, '3913ddde3d1026b2'),
    (0x100f9c50, 0x100f9d80, 76, '6f71c2250e3d43ae'), (0x100f9d80, 0x100f9f3c, 111, 'bc7c0bb02bb4aa64'),
    (0x100f9f40, 0x100fa1e8, 170, '4e10db2305eb4cb3'), (0x100fa1f0, 0x100fa6d8, 314, 'f3e974bd0427deb3'),
    (0x100fa6e0, 0x100fcfc4, 2612, '9615b997c1267b61'), (0x100fcfd0, 0x100fd194, 113, 'aa84143ae3afb66c'),
    (0x100fd1a0, 0x100fd210, 28, '01b61568fb11a604'), (0x100fd210, 0x100fe638, 1290, '555c3980eaa39bac'),
    (0x100fe640, 0x100fefa0, 600, '88deef72427a9901'), (0x100fefa0, 0x100ffeb4, 965, '6ac2379d82d953d5'),
    (0x100ffec0, 0x10100694, 501, '684e5e4f8c114426'), (0x101006a0, 0x101013f8, 854, '8de4771a6e45c3e0'),
    (0x10101400, 0x10101fe8, 761, 'd32091a86107e89c'), (0x10101ff0, 0x101020b8, 50, '2cb8fd18c68969ad'),
    (0x101020c0, 0x10102590, 308, 'c3586d90879733b4'), (0x10102590, 0x10102654, 49, '5134eb846a3f25ba'),
    (0x10102660, 0x10102818, 110, 'ee40846966db79fd'), (0x10102820, 0x10102908, 58, 'fa2411c56ae5c9fe'),
    (0x10102910, 0x10102a6c, 86, 'ec558a223fbab3e1'), (0x10102a70, 0x10102b44, 53, '30d1402525260db5'),
    (0x10102b50, 0x10102c90, 80, 'c55fa7691ec358b7'), (0x10102c90, 0x10102d1c, 35, '311d974ae14648a3'),
    (0x10102d20, 0x10102e00, 56, 'bda693d8c4a54425'), (0x10102e00, 0x10102e94, 37, '079b4d2f9227d4f7'),
    (0x10102ea0, 0x101030c0, 136, '71b25322dfc8b0ea'), (0x101030c0, 0x10103178, 46, '778e2f415be7874a'),
    (0x10103180, 0x10103268, 58, 'b70367e37a082a1f'), (0x10103270, 0x101032d8, 26, '040d04c98ce0c736'),
    (0x101032e0, 0x10103374, 37, '9887122dd0033276'), (0x10103380, 0x10103420, 40, '7a8700f11b80c6dc'),
    (0x10103420, 0x1010350c, 59, '23d03bcd3c25d73f'), (0x10103510, 0x101035f4, 57, 'c0f10612b3a323c2'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# Calls from outside the unit, per entry: 332 sites, every one a `bl` of an entry.
INCOMING = {0x100f2680: 24, 0x100f4790: 5, 0x100f4f20: 2, 0x100f4f80: 7, 0x100f55b0: 32, 0x100f5730: 1,
            0x100f5770: 31, 0x100f7050: 5, 0x100f70a0: 5, 0x100f70e0: 4, 0x100f7130: 2, 0x100f7170: 1,
            0x100f71b0: 3, 0x100f71f0: 25, 0x100f7230: 1, 0x100f7cf0: 3, 0x100f84e0: 2, 0x100f8720: 1,
            0x100f8780: 1, 0x100f95e0: 1, 0x100f9c50: 1, 0x100fa6e0: 2, 0x100fcfd0: 1, 0x100fd1a0: 1,
            0x101020c0: 1, 0x10102660: 1, 0x10102910: 1, 0x10102b50: 1, 0x10102d20: 1, 0x10102ea0: 1,
            0x10103180: 1, 0x101032e0: 1, 0x10103420: 1, 0x10103510: 162}
# Entries another routine of the unit calls.
CROSS = {0x100f1050, 0x100f2100, 0x100f2680, 0x100f4f80, 0x100f55b0, 0x100f5770, 0x100f7270, 0x100f9b00,
         0x100f9d80, 0x100f9f40, 0x100fa1f0, 0x100fd210, 0x100fe640, 0x100fefa0, 0x100ffec0, 0x101006a0,
         0x10101400, 0x10101ff0, 0x10102590, 0x10102820, 0x10102a70, 0x10102c90, 0x10102e00, 0x101030c0,
         0x10103270, 0x10103380, 0x10103510}
# The unit's private TOC slots with (loads, first load, last load). No other code loads them.
PRIVATE_SLOTS = {
    '-0x5598': (20, 0x100f1084, 0x10103518),   # the string pool
    '-0x559c': (5, 0x100f1190, 0x100f76f0),    # floats
    '-0x55a0': (20, 0x100f1078, 0x101020d0),   # doubles
    '-0x55a4': (1, 0x100f1884, 0x100f1884),    # data tables
    '-0x55a8': (1, 0x100f27b0, 0x100f27b0),
    '-0x55ac': (1, 0x100f2874, 0x100f2874),
    '-0x55b0': (1, 0x100f26a4, 0x100f26a4),
    '-0x55b4': (1, 0x100f269c, 0x100f269c),
    '-0x55b8': (1, 0x100f309c, 0x100f309c),
    '-0x55bc': (1, 0x100f3110, 0x100f3110),
    '-0x55c0': (1, 0x100f752c, 0x100f752c),
    '-0x55c4': (1, 0x100f84f8, 0x100f84f8),    # two scrambled names
    '-0x55c8': (1, 0x100f84f0, 0x100f84f0),
    '-0x55cc': (1, 0x101022dc, 0x101022dc),    # jump tables
    '-0x55d0': (1, 0x10102f98, 0x10102f98),
    '-0x74a0': (1, 0x100f8618, 0x100f8618),    # the element constructor's vector
    '-0x74a4': (2, 0x100f8620, 0x100f874c),    # the element destructor's vector
}
# The neighbouring units' slots on either side of the run: (loads, first, last), all outside the unit.
NEIGHBOUR_SLOTS = {'-0x55d4': (8, 0x100edf38, 0x100f0da0), '-0x5594': (1, 0x10106560, 0x10106560)}
# The two jump tables: TOC slot -> (the `bctr`, the routine every case stays in).
JUMP_TABLES = {-0x55cc: (0x101022ec, 0x101020c0), -0x55d0: (0x10102fa8, 0x10102ea0)}
# The two pointers to the unit: transition vectors (code address, then the TOC) of the array
# element's constructor and destructor, each loaded through its own TOC slot.
VECTORS = {0x108b8a4c: (0x100f1000, -0x74a0), 0x108b8a44: (0x100f1010, -0x74a4)}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private FINANCE export unavailable; inventory cannot be certified')


class FinanceAuditTests(unittest.TestCase):
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
        self.assertEqual(audit.MODULE, (0x100f1000, 0x10103600))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.FINANCE_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 58)
        self.assertEqual(len(self.records), 18706)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_finance_index(db, complete=False)

    def test_no_entry_has_an_owner_yet(self):
        # Turns over at delivery: every entry then has `finance_adapter` alone, and
        # require_finance_index runs with complete=True.
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if audit.MODULE[0] <= at < audit.MODULE[1]], [])
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if audit.MODULE[0] <= at < audit.MODULE[1]], [])
        self.assertEqual(audit.rewritten(drafts=True), [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with self.assertRaisesRegex(ValueError, 'all 58 complete'):
                audit.require_finance_index(db, complete=True)

    def test_unit_bounds_are_its_private_toc_slots_and_its_neighbours(self):
        pool = self.word(TOC - 0x5598)
        self.assertEqual(self.original_bytes(pool, 0x20).split(b'\0')[:3], [b'FINANCE.CPP', b'v%s %s.%s %d', b'Error'])
        # FILE_SCREENS.CPP's pool is the slot before the run.
        self.assertIn(b'FILE_SCREENS.CPP\0', self.original_bytes(self.word(TOC - 0x55d4), 0x200))

        def loads(slot):
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                found = [a for a, in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                (f'%{slot}(r2)%',))]
            return len(found), found[0], found[-1]
        for slot, expected in PRIVATE_SLOTS.items():
            self.assertEqual(loads(slot), expected, slot)
            self.assertTrue(audit.MODULE[0] <= expected[1] <= expected[2] < audit.MODULE[1])
        # The run is whole: every slot from -0x55d0 to -0x5598, and the two vectors.
        self.assertEqual({int(slot, 16) for slot in PRIVATE_SLOTS}, set(range(-0x55d0, -0x5594, 4)) | {-0x74a4, -0x74a0})
        for slot, expected in NEIGHBOUR_SLOTS.items():
            self.assertEqual(loads(slot), expected, slot)
            self.assertFalse(audit.MODULE[0] <= expected[1] < audit.MODULE[1])
            self.assertFalse(audit.MODULE[0] <= expected[2] < audit.MODULE[1])
        # Before: FILE_SCREENS.CPP's audit ends where this unit starts. After: the next routine is
        # past the unit's last pool user and loads FIND_SCREENS.CPP's slots.
        import file_screens_audit
        self.assertEqual(file_screens_audit.MODULE[1], audit.MODULE[0])
        self.assertEqual(self.routine_of(0x100f1078), ENTRIES[2])
        self.assertEqual(self.routine_of(0x10103518), ENTRIES[-1])
        self.assertIn(b'FIND_SCREENS.CPP\0', self.original_bytes(self.word(TOC - 0x5574), 0x200))
        # The two routines before the first pool user are reached only through their vectors, which
        # the array allocation in 0x100f84e0 and the release in 0x100f8720 pass to the runtime.
        self.assertEqual(self.by_address[0x100f8618]['assembly'], 'lwz r4,-0x74a0(r2)')
        self.assertEqual(self.by_address[0x100f8620]['assembly'], 'lwz r5,-0x74a4(r2)')
        self.assertEqual(self.by_address[0x100f862c]['assembly'], 'bl 0x10773150')
        self.assertEqual(self.by_address[0x100f874c]['assembly'], 'lwz r4,-0x74a4(r2)')
        self.assertEqual(self.by_address[0x100f8750]['assembly'], 'bl 0x107734a0')
        self.assertEqual({self.routine_of(a) for a in (0x100f8618, 0x100f8620, 0x100f874c)}, {0x100f84e0, 0x100f8720})

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
        self.assertEqual(sum(incoming.values()), 332)
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
        self.assertEqual(computed, dict(JUMP_TABLES.values()))
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
        for address in audit.UNREACHABLE_NOPS:
            self.assertEqual(self.word(address), 0x60000000)
            self.assertNotIn(address, self.by_address)
        # A forward branch right after an unconditional branch, and nothing branches to it.
        targets = {t for r in self.records for t in r['flows']}
        for address in audit.UNREACHABLE_BRANCHES:
            self.assertEqual(self.word(address) & 0xffff0003, 0x48000000)
            self.assertEqual(self.by_address[address - 4]['flow'], 'UNCONDITIONAL_JUMP')
            self.assertNotIn(address, self.by_address)
            self.assertNotIn(address, targets)
        for address in set(range(*audit.MODULE, 4)) - addresses:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)

    def test_pointers_to_the_unit_are_two_vectors_and_its_own_jump_tables(self):
        code_end = self.segments[0][0] + len(self.segments[0][1])
        tables = sorted((self.word(TOC + slot), routine) for slot, (_, routine) in JUMP_TABLES.items())
        cases, seen = 0, set()
        for start, contents in self.segments:
            for match in re.finditer(b'\x10[\x0f\x10]', contents):
                at = start + match.start()
                if match.start() + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[match.start():match.start() + 4], 'big')
                if not audit.MODULE[0] <= target < audit.MODULE[1]:
                    continue
                if at in VECTORS:
                    entry, slot = VECTORS[at]
                    self.assertEqual((target, self.word(at + 4), self.word(TOC + slot)), (entry, TOC, at))
                    seen.add(at)
                elif at % 4:
                    # Bytes of instructions or data that read as an address: never a word-aligned pointer.
                    self.assertNotIn(target, ENTRIES)
                else:
                    # A jump-table case: it belongs to the nearest table at or below it and stays in
                    # the routine that loads that table.
                    self.assertGreaterEqual(at, code_end)
                    base, routine = max(t for t in tables if t[0] <= at)
                    self.assertEqual(self.routine_of(target), routine)
                    self.assertNotEqual(self.word(at + 4), TOC)
                    cases += 1
        self.assertEqual(seen, set(VECTORS))
        self.assertEqual(cases, 17)

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[1]
            line = f'FINANCE_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_finance_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_finance_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing FINANCE.CPP instructions'):
                    audit.require_finance_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)


if __name__ == '__main__':
    unittest.main()
