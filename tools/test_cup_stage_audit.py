"""Certify CUP_STAGE's original inventory, boundaries and hook contracts.

The private export is required evidence. Every entry must remain in production;
a missing, stale or draft registration fails the completed-module contract.
"""
from contextlib import closing
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import cup_stage_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
CONTRACTS = (
    (0x104bc3c0, 0x104bc748, 226, '1cc2c1fc657c994e'),
    (0x104bc750, 0x104bc9a8, 150, '2a73a24301a6726c'),
    (0x104bc9b0, 0x104bca88, 54, 'c6efef90686d646a'),
    (0x104bca90, 0x104bcb20, 36, '307ffd7eddca4592'),
    (0x104bcb20, 0x104bd550, 652, '4b7b80f93a8ee8f0'),
    (0x104bd550, 0x104bd8b8, 217, '8d4fa297f4bfc618'),
    (0x104bd8c0, 0x104be7dc, 962, '789abc96f9ee6717'),
    (0x104be7e0, 0x104bed58, 350, '823d8b389f5c20e2'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
PADDING = {0x104bc748, 0x104bc74c, 0x104bc9a8, 0x104bc9ac, 0x104bca88,
           0x104bca8c, 0x104bd8b8, 0x104bd8bc, 0x104be7dc}
NOP_TARGETS = {
    0x104bd7c4: 0x104bd7f0, 0x104bd9fc: 0x104be7c8,
    0x104bda34: 0x104bda4c, 0x104bdfc8: 0x104be3c8,
    0x104be2c0: 0x104be3c8, 0x104be48c: 0x104be5bc,
}
OWNED_CALLS = {(0x104bc5e8, 0x104bca90), (0x104bdee0, 0x104be7e0),
               (0x104be1dc, 0x104be7e0), (0x104bebbc, 0x104be7e0)}
VECTORS = {0x108b9e04: 0x104bd8c0, 0x108b9e0c: 0x104bc9b0}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private CUP_STAGE export unavailable; inventory cannot be certified')


class CupStageAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        cls.program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes())
                        for s in cls.program['segments'] if 'file' in s]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            cls.records = routine_records(db, *audit.MODULE)
        cls.by_address = {r['address']: r for r in cls.records}

    def raw(self, address, size):
        for start, contents in self.segments:
            if start <= address and address + size <= start + len(contents):
                return contents[address - start:address - start + size]
        self.fail(f'Original bytes unavailable at {address:x}')

    def word(self, address):
        return int.from_bytes(self.raw(address, 4), 'big')

    def test_exact_entries_ends_counts_digests_and_registration(self):
        expected = tuple((a, b, digest) for a, b, _, digest in CONTRACTS)
        self.assertEqual(audit.MODULE, (0x104bc3c0, 0x104bed58))
        self.assertEqual(audit.CONTRACTS, expected)
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(tuple(audit.rewritten()), expected)
        self.assertEqual(tuple(audit.rewritten(drafts=True)), expected)
        self.assertEqual(audit.CUP_STAGE_ENTRIES, ENTRIES)
        self.assertEqual(len(self.records), 2647)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_cup_stage_index(db)

    def test_source_numeric_pools_toc_and_neighboring_modules(self):
        toc = 0x108bba90
        for slot, address in ((-0x4674, 0x1083c328), (-0x4678, 0x1083c324),
                              (-0x467c, 0x1083c34c), (-0x4680, 0x1083bc00),
                              (-0x4670, 0x1083c35c), (-0x613c, 0x108d7de8)):
            self.assertEqual(self.word(toc + slot), address)
        self.assertEqual(self.raw(0x1083c328, 36), b'CUP_STAGE.CPP\0v%s %s.%s %d\0Error\0\0\0\0')
        self.assertEqual(self.word(0x1083c324), 0xffffffff)
        self.assertEqual(self.raw(0x1083c34c, 16), bytes.fromhex('40efffe0000000004330000080000000'))
        self.assertEqual(self.raw(0x1083bc00, 13), b'BRA_CUP.CPP\0v')
        self.assertEqual(self.raw(0x1083c35c, 13), b'DEN_CUP.CPP\0v')
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            expected = {0x104bc3bc: 'blr', 0x104bc3c0: 'mfspr r0,LR',
                        0x104bed54: 'blr', 0x104bed60: 'mfspr r0,LR',
                        0x104bed78: 'lwz r31,-0x4670(r2)'}
            for address, assembly in expected.items():
                record = json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                               (address,)).fetchone()[0])
                self.assertEqual(record['assembly'], assembly)
            source_uses = [a for a, in db.execute(
                'SELECT address FROM instructions WHERE body LIKE ? ORDER BY address', ('%-0x4674(r2)%',))]
            self.assertEqual(len(source_uses), 19)
            self.assertTrue(all(audit.MODULE[0] <= a < audit.MODULE[1] for a in source_uses))
            self.assertEqual(source_uses[0], 0x104bc3d8)
            self.assertEqual(source_uses[-1], 0x104be594)
        self.assertEqual(self.raw(0x104bed58, 8), b'\0' * 8)

    def test_all_incoming_edges_are_calls_to_named_entries_without_fallthrough(self):
        incoming = []
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
                        incoming.append((address, target))
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*audit.MODULE, *audit.MODULE)))
        self.assertEqual(fallthroughs, [])
        self.assertEqual(len(incoming), 106)
        self.assertEqual(hashlib.sha256(json.dumps(incoming).encode()).hexdigest(),
                         '803c9dce0fe9d9a98c7b7f189f37109414ed375c6d7fc83046b09f24248e37a6')
        self.assertEqual({target for _, target in incoming},
                         {0x104bc3c0, 0x104bc750, 0x104bcb20, 0x104bd550})

    def test_cross_entry_edges_have_no_shared_tail_or_interior_resume(self):
        cross, calls = set(), set()
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                self.assertNotIn('COMPUTED', record['flow'])
                targets = record['flows'] + ([record['fallthrough']]
                                              if record.get('fallthrough') is not None else [])
                for target in targets:
                    if record["flow"] == "UNCONDITIONAL_CALL" and target in ENTRIES:
                        calls.add((record["address"], target))
                    self.assertNotIn(target, PADDING | set(NOP_TARGETS))
                    if audit.MODULE[0] <= target < audit.MODULE[1] and not start <= target < end:
                        self.assertIn(target, ENTRIES)
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        cross.add((record['address'], target))
        self.assertEqual(calls, OWNED_CALLS)
        self.assertEqual(cross, OWNED_CALLS - {(0x104bebbc, 0x104be7e0)})
        self.assertEqual([s for s in self.program['symbols']
                          if audit.MODULE[0] <= s['address'] < audit.MODULE[1]], [])

    def test_every_instruction_zero_padding_and_skipped_alignment_nop_accounted(self):
        addresses = set()
        self.assertEqual(audit.UNREACHABLE_NOPS, frozenset(NOP_TARGETS))
        for start, end in audit.ROUTINES:
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual([r['address'] for r in records],
                             [a for a in range(start, end, 4) if a not in NOP_TARGETS])
            self.assertEqual(records[-1]['assembly'], 'blr')
            addresses.update(range(start, end, 4))
        self.assertEqual(set(range(*audit.MODULE, 4)) - addresses, PADDING)
        for address in PADDING:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)
        for address, target in NOP_TARGETS.items():
            self.assertEqual(self.word(address), 0x60000000)
            self.assertEqual(self.by_address[address - 4]['assembly'], f'b 0x{target:x}')
            self.assertNotIn(address, self.by_address)

    def test_every_byte_pointer_scan_and_canonical_transition_vectors(self):
        aligned, crossing = {}, []
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for start, contents in self.segments:
                for match in re.finditer(b'\x10\x4b', contents):
                    offset = match.start()
                    if offset + 4 > len(contents):
                        continue
                    target = int.from_bytes(contents[offset:offset + 4], 'big')
                    if not audit.MODULE[0] <= target < audit.MODULE[1]:
                        continue
                    address = start + offset
                    if address % 4 == 0:
                        aligned[address] = target
                        continue
                    # These are overlapping immediate fields across two indexed
                    # instructions in executable code, not aligned CFM vectors.
                    crossing.append((address, target))
                    self.assertEqual(start, 0x10000000)
                    for instruction in (address & ~3, (address & ~3) + 4):
                        self.assertIsNotNone(db.execute('SELECT 1 FROM instructions WHERE address=?',
                                                       (instruction,)).fetchone())
        self.assertEqual(aligned, VECTORS)
        self.assertEqual(len(crossing), 86)
        self.assertEqual(hashlib.sha256(json.dumps(crossing).encode()).hexdigest(),
                         'eb51bc3b8f100774f95fd94cecd33d5dbf56772c45525964672f4cf25acfe9ef')
        for vector, entry in VECTORS.items():
            self.assertEqual((self.word(vector), self.word(vector + 4)), (entry, 0x108bba90))
        self.assertEqual(self.word(0x108d7de8 + 0x9c), 0x108b9e04)
        self.assertEqual(self.word(0x108d7de8 + 8), 0x108b9e0c)
        self.assertEqual(self.word(0x108b9e14), 0x104bfcf0)
        self.assertEqual(self.word(0x108b9dfc), 0x104ba6e0)

    def test_missing_export_missing_instruction_and_stale_digest_fail(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_cup_stage_reference.py'),
                                     directory, str(target)], text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(target.exists())
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing CUP_STAGE.CPP instructions'):
                audit.require_cup_stage_index(db)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as source:
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                db.executemany('INSERT INTO instructions VALUES (?,?)', source.execute(
                    'SELECT address,body FROM instructions WHERE address>=? AND address<?', audit.MODULE))
                body = json.loads(db.execute('SELECT body FROM instructions WHERE address=?', (ENTRIES[0],)).fetchone()[0])
                body['assembly'] += ' stale'
                db.execute('UPDATE instructions SET body=? WHERE address=?', (json.dumps(body), ENTRIES[0]))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_cup_stage_index(db)

    def test_duplicate_bad_boundary_missing_and_stale_registrations_fail(self):
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'CUP_STAGE_' in line)
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(contents.replace('0x104bc748, R3', '0x104bc750, R3'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                for broken in (contents.replace(first, ''), contents.replace('c6efef90686d646a', '0000000000000000'),
                               contents.replace('CUP_STAGE_ROUTINE(', 'CUP_STAGE_DRAFT(', 1)):
                    candidate.write_text(broken)
                    with self.assertRaisesRegex(ValueError, 'all eight complete production'):
                        audit.require_cup_stage_index(db, candidate)

    def test_hook_contract_detects_missing_duplicate_and_late_hooks(self):
        with patch.object(audit, 'CUP_STAGE_ENTRIES', ENTRIES):
            for entry in ENTRIES:
                record = self.by_address[entry]
                hook = audit.cup_stage_hook(entry)
                step = f's.step(0x{entry:x});'
                prefix = f'i_{entry:x}:\n'
                suffix = f'\ni_{entry+4:x}:\n'
                audit.require_cup_stage_hooks(entry >> 12, [record], prefix + hook + '\n' + step + suffix)
                for fragment in (step, hook + '\n' + hook + '\n' + step, step + '\n' + hook):
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_cup_stage_hooks(entry >> 12, [record], prefix + fragment + suffix)

    def test_registered_production_entries_have_exclusive_owners_and_normal_hooks(self):
        from compile_recomp import CUP_STAGE_ENTRIES, OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS, generate_page
        self.assertEqual(CUP_STAGE_ENTRIES, ENTRIES)
        self.assertEqual(CUP_STAGE_ENTRIES, audit.CUP_STAGE_ENTRIES)
        # No older semantic/builder family owned any of these entries.
        self.assertFalse(set(ENTRIES) & {entry for entry, _ in SEMANTIC_REPLACEMENTS})
        for entry in CUP_STAGE_ENTRIES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ['cup_stage_adapter'])
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_cup_stage_hooks(page, part, normal)
            disabled = generate_page(page, part, selected, userops=self.program['userops'], cup_stage_hooks=False)
            self.assertNotIn('try_modern_cup_stage', disabled)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                hook = audit.cup_stage_hook(entry)
                self.assertEqual(normal.count(hook), int(entry in CUP_STAGE_ENTRIES))
                if entry in CUP_STAGE_ENTRIES:
                    self.assertIn('#include "cup_stage_adapter.hpp"', normal)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_cup_stage_hooks(page, part, normal.replace(hook, ''))

    def test_original_reference_has_all_labels_without_replacement_hooks(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_cup_stage_reference.py'),
                                     str(EXPORT), str(target)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            code = target.read_text()
        self.assertNotIn('try_modern_', code)
        self.assertNotIn('cup_stage::', code)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE),
                         [f'{r["address"]:x}' for r in self.records])


if __name__ == '__main__':
    unittest.main()
