"""Close DISPUTE.CPP original boundaries, entries, data pointers and production routes."""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import dispute_audit
from dispute_audit import MODULE, ROUTINES, INCLUDES, rewritten, require_dispute_index, volatile_reads

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/current/export/instructions.sqlite'
INTERNAL_CALLS = [
    (0x1070bd30, 0x1070be60), (0x1070c46c, 0x1070c7f0),
    (0x1070c4d4, 0x1070c7f0), (0x1070c630, 0x1070d650),
    (0x1070c6d8, 0x1070d650), (0x1070c6e4, 0x1070c8b0),
    (0x1070cd5c, 0x1070d0d0), (0x1070cec0, 0x1070d0d0),
    (0x1070d0b8, 0x1070d650), (0x1070d4ac, 0x1070d0d0),
]
INCOMING_CALLS = [
    (0x1010bb10, 0x1070c8f0), (0x1010d9f4, 0x1070c8f0),
    (0x10119970, 0x1070c510), (0x101224b8, 0x1070c3f0),
    (0x101224d8, 0x1070c7f0), (0x101224ec, 0x1070c710),
    (0x10122508, 0x1070c7f0), (0x1012254c, 0x1070c7f0),
    (0x1012256c, 0x1070d650), (0x101a0c90, 0x1070c3f0),
    (0x102c5f10, 0x1070d820), (0x102d8cfc, 0x1070c7f0),
    (0x102e4708, 0x1070c7f0), (0x10345414, 0x1070bd00),
    (0x1034a1d8, 0x1070bd00), (0x1034d1e8, 0x1070c040),
    (0x1034e0b8, 0x1070bde0), (0x1039b188, 0x1070c3f0),
    (0x1039b1a0, 0x1070c710), (0x103a2dc8, 0x1070c3f0),
    (0x103a31c8, 0x1070c3f0), (0x103a35a0, 0x1070c3f0),
    (0x1067b5a0, 0x1070c8f0), (0x1067b5bc, 0x1070c8f0),
    (0x1067b88c, 0x1070c8f0), (0x1067b8a8, 0x1070c8f0),
]


class DisputeAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not INPUT.exists():
            raise RuntimeError('Private DISPUTE export unavailable; inventory cannot be certified')
        cls.program = json.loads((INPUT.parent / 'program.json').read_text())
        cls.segments = [(s['address'], (INPUT.parent / s['file']).read_bytes())
                        for s in cls.program['segments'] if 'file' in s]

    def original_bytes(self, address, size):
        for start, contents in self.segments:
            if start <= address and address + size <= start + len(contents):
                return contents[address - start:address - start + size]
        self.fail(f'Original bytes unavailable at {address:x}')

    def original_word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), 'big')

    def assert_production_inventory(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(ROUTINES))
        for entry, _ in ROUTINES:
            self.assertEqual([owner for at, owner in owners if at == entry], ['dispute_adapter'])

    def test_exact_inventory_digests_and_production_ownership(self):
        self.assertEqual(len(ROUTINES), 13)
        self.assertEqual((ROUTINES[0][0], ROUTINES[-1][1]), MODULE)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(ROUTINES, ROUTINES[1:])))
        from compile_recomp import OWNER_REPLACEMENTS
        self.assert_production_inventory(rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(INPUT)) as db:
            require_dispute_index(db)
            self.assertEqual(db.execute('SELECT COUNT(*) FROM instructions WHERE address>=? AND address<?',
                                        MODULE).fetchone()[0], 2063)

    def test_source_pool_and_neighbor_boundaries(self):
        toc = 0x108bba90
        for slot, address, source in ((-0x400c, 0x10898d14, b'rb_asia.cpp\0'),
                                      (-0x4004, 0x10898d34, b'dispute.dat\0dispute.cpp\0'),
                                      (-0x4000, 0x1089966c, b'tur_second_b.cpp\0')):
            self.assertEqual(self.original_word(toc + slot), address)
            self.assertEqual(self.original_bytes(address, len(source)), source)
        self.assertEqual(self.original_word(toc - 0x4008), 0x1089964c)
        # The numeric pool contains one, one hundred, 65535 and the signed-bias
        # conversion double. It belongs to the same source unit as its strings.
        self.assertEqual(self.original_bytes(0x1089964c, 32).hex(),
                         '3ff0000000000000405900000000000040efffe0000000004330000080000000')
        with closing(sqlite3.connect(INPUT)) as db:
            pool_loads = [(a, json.loads(b)['assembly']) for a, b in db.execute(
                'SELECT address,body FROM instructions WHERE body LIKE ? ORDER BY address',
                ('%-0x4004(r2)%',))]
            self.assertEqual(pool_loads, [(0x1070beb8, 'lwz r4,-0x4004(r2)'),
                                         (0x1070c058, 'lwz r31,-0x4004(r2)'),
                                         (0x1070d840, 'lwz r25,-0x4004(r2)')])
            numeric_loads = [(a, json.loads(b)['assembly']) for a, b in db.execute(
                'SELECT address,body FROM instructions WHERE body LIKE ? ORDER BY address',
                ('%-0x4008(r2)%',))]
            self.assertEqual(numeric_loads, [(0x1070c8f8, 'lwz r21,-0x4008(r2)'),
                                            (0x1070d0dc, 'lwz r25,-0x4008(r2)')])
            at = lambda address: json.loads(db.execute(
                'SELECT body FROM instructions WHERE address=?', (address,)).fetchone()[0])['assembly']
            self.assertEqual(at(0x1070bcf8), 'blr')
            self.assertEqual(at(MODULE[0]), 'mfspr r0,LR')
            self.assertEqual(at(MODULE[1]), 'mfspr r0,LR')
            self.assertEqual(at(MODULE[1] + 0x18), 'lwz r31,-0x4000(r2)')

    def test_every_instruction_and_unindexed_word_is_accounted_for(self):
        gaps = {0x1070be60: {0x1070bff4}, 0x1070c040: {0x1070c340},
                0x1070c710: {0x1070c784}, 0x1070c7f0: {0x1070c854},
                0x1070d820: {0x1070d95c}}
        with closing(sqlite3.connect(INPUT)) as db:
            for entry, end in ROUTINES:
                with self.subTest(entry=hex(entry)):
                    records = [json.loads(body) for body, in db.execute(
                        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                        (entry, end))]
                    self.assertEqual(records[0]['assembly'], 'mfspr r0,LR')
                    self.assertEqual(records[-1]['assembly'], 'blr')
                    self.assertEqual(sum(r['assembly'] == 'mfspr r0,LR' for r in records), 1)
                    indexed = {r['address'] for r in records}
                    missing = set(range(entry, records[-1]['address'] + 4, 4)) - indexed
                    self.assertEqual(missing, gaps.get(entry, set()))
                    for address in missing:
                        self.assertEqual(self.original_word(address),
                                         0x480000a0 if address == 0x1070d95c else 0x60000000)
                        predecessor = next(r for r in records if r['address'] == address - 4)
                        self.assertEqual(predecessor['flow'], 'UNCONDITIONAL_JUMP')
                        self.assertIsNone(predecessor.get('fallthrough'))
                        self.assertFalse(any(address in r['flows'] for r in records))
                    last = records[-1]['address'] + 4
                    self.assertEqual(self.original_bytes(last, end - last), bytes(end - last))
                    self.assertLess(end - last, 16)

    def test_whole_export_incoming_flows_have_no_interior_resumes(self):
        with closing(sqlite3.connect(INPUT)) as db:
            incoming = []
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    'WHERE f.value>=? AND f.value<?', MODULE):
                if MODULE[0] <= address < MODULE[1]:
                    continue
                record = json.loads(body)
                self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                incoming.extend((address, target) for target in record['flows'])
        self.assertEqual(sorted(incoming), INCOMING_CALLS)

    def test_internal_cross_entry_flows_and_no_computed_jumps(self):
        crossing = []
        with closing(sqlite3.connect(INPUT)) as db:
            for entry, end in ROUTINES:
                for body, in db.execute('SELECT body FROM instructions WHERE address>=? AND address<?',
                                       (entry, end)):
                    record = json.loads(body)
                    self.assertNotIn('COMPUTED', record['flow'])
                    for target in record['flows']:
                        if MODULE[0] <= target < MODULE[1]:
                            if record['flow'] == 'UNCONDITIONAL_CALL':
                                crossing.append((record['address'], target))
                            elif not entry <= target < end:
                                self.fail(f'Unexpected cross-entry flow at {record["address"]:x}')
        self.assertEqual(sorted(crossing), INTERNAL_CALLS)

    def test_all_byte_pointer_scan_excludes_traceback_false_positive(self):
        pointers = {}
        for start, contents in self.segments:
            for match in re.finditer(b'\x10\x70', contents):
                offset = match.start()
                if offset + 4 <= len(contents):
                    target = int.from_bytes(contents[offset:offset + 4], 'big')
                    if MODULE[0] <= target < MODULE[1]:
                        pointers[start + offset] = target
        self.assertEqual(pointers, {0x108a2e29: 0x1070d000})
        # This is one byte into a code-relative traceback-table word, followed
        # by its function size and table offset. Adjacent records establish
        # the encoding, and the relocated entries are outside DISPUTE.
        with closing(sqlite3.connect(INPUT)) as db:
            for address, relative, size, table_offset in (
                    (0x108a2e1c, 0x00106b00, 0x5cc, 0xb444),
                    (0x108a2e28, 0x001070d0, 0x6c4, 0xb456),
                    (0x108a2e34, 0x001077a0, 0x6d4, 0xb468)):
                self.assertEqual([self.original_word(address + i * 4) for i in range(3)],
                                 [relative, size, table_offset])
                entry = 0x10000000 + relative
                record = json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                               (entry,)).fetchone()[0])
                self.assertEqual(record['assembly'], 'mfspr r0,LR')
                self.assertFalse(MODULE[0] <= entry < MODULE[1])

    def test_audit_rejects_stale_digest(self):
        routines = rewritten(drafts=True)
        self.assertEqual(len(routines), len(ROUTINES))
        stale = [(a, b, '0000000000000000') if index == 0 else (a, b, digest)
                 for index, (a, b, digest) in enumerate(routines)]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(dispute_audit, 'rewritten', return_value=stale):
            with self.assertRaisesRegex(ValueError, 'changed in the export'):
                require_dispute_index(db)

    def test_audit_rejects_duplicate_disabled_and_missing_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        with tempfile.TemporaryDirectory() as directory:
            duplicate = Path(directory) / 'routines.inc'
            contents = INCLUDES.read_text()
            first = next(line for line in contents.splitlines() if 'DISPUTE_ROUTINE(' in line)
            duplicate.write_text(contents + '\n' + first + '\n')
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                rewritten(duplicate)
            disabled = Path(directory) / 'disabled.inc'
            disabled.write_text(contents.replace('DISPUTE_ROUTINE(', 'DISPUTE_DRAFT(', 1))
            with self.assertRaises(AssertionError):
                self.assert_production_inventory(rewritten(disabled), OWNER_REPLACEMENTS)
        missing = [(entry, owner) for entry, owner in OWNER_REPLACEMENTS if entry != ROUTINES[0][0]]
        with self.assertRaises(AssertionError):
            self.assert_production_inventory(rewritten(), missing)

    def test_default_generation_guards_all_entries_and_reference_bypasses(self):
        from compile_recomp import generate_page
        with closing(sqlite3.connect(INPUT)) as db:
            records = [json.loads(body) for body, in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', MODULE)]
        selected = {r['address'] for r in records}
        for page in sorted({entry >> 12 for entry, _ in ROUTINES}):
            part = [r for r in records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected)
            for entry, _ in ROUTINES:
                if entry >> 12 == page:
                    self.assertIn(f'if (try_modern_dispute(s, 0x{entry:x})) return true;', normal)
            self.assertIn('#include "dispute_adapter.hpp"', normal)
            reference = generate_page(page, part, selected, dispute_hooks=False)
            self.assertNotIn('try_modern_dispute', reference)

    def test_no_unmodelled_scratch_register_dependencies(self):
        with closing(sqlite3.connect(INPUT)) as db:
            self.assertEqual(volatile_reads(db), [])


if __name__ == '__main__':
    unittest.main()
