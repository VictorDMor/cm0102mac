"""Certify SIPRINT's printing entries, original boundary and production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import siprint_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
CONTRACTS = (
    (0x1074c960, 0x1074c99c, 15, '9400a531850de1bb'),
    (0x1074c9a0, 0x1074ca50, 44, '311922aa5d5cec75'),
    (0x1074ca50, 0x1074ca54, 1, 'b7d4781665bcdc06'),
    (0x1074ca60, 0x1074cb44, 57, 'a747a8d1dea36829'),
    (0x1074cb50, 0x1074cc18, 50, '7b05a159376abae3'),
    (0x1074cc20, 0x1074cc2c, 3, 'e433ee6914088aba'),
    (0x1074cc30, 0x1074cd8c, 87, '2e52c98c2107ffdd'),
    (0x1074cd90, 0x1074d33c, 362, 'cc540034c545a300'),
    (0x1074d340, 0x1074d37c, 15, 'ceab8a99cc31c0ea'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
PADDING = {0x1074c99c, 0x1074ca54, 0x1074ca58, 0x1074ca5c,
           0x1074cb44, 0x1074cb48, 0x1074cb4c, 0x1074cc18,
           0x1074cc1c, 0x1074cc2c, 0x1074cd8c, 0x1074d33c}
UNREACHABLE_NOP = 0x1074d170
INCOMING = (
    (0x10000050, 0x1074d340), (0x1013f630, 0x1074cc20),
    (0x10723388, 0x1074cd90), (0x107233b0, 0x1074cd90),
    (0x1076f324, 0x1074c9a0), (0x1076f758, 0x1074ca50),
    (0x1076f84c, 0x1074ca50), (0x107709cc, 0x1074cc30),
)
OWNED_CALLS = {(0x1074c9e4, 0x1074ca60), (0x1074c9f8, 0x1074cb50)}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private SIPRINT export unavailable; inventory cannot be certified')


class SiprintAuditTests(unittest.TestCase):
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

    def assert_production(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(audit.ROUTINES))
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in owners if at == entry], ['siprint_adapter'])

    def assert_hook(self, code, entry):
        hook = audit.siprint_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_exact_entries_original_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS, SIPRINT_TAKEOVERS
        self.assertEqual(audit.MODULE, (0x1074c960, 0x1074d37c))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.SIPRINT_ENTRIES, ENTRIES)
        self.assertEqual(audit.UNREACHABLE_NOPS, frozenset({UNREACHABLE_NOP}))
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(self.records), 634)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        self.assert_production(audit.rewritten(), OWNER_REPLACEMENTS)
        self.assertEqual(SIPRINT_TAKEOVERS, frozenset({0x1074d340}))
        self.assertIn((0x1074d340, 'engine_initialization_adapter'), SEMANTIC_REPLACEMENTS)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_siprint_index(db)

    def test_source_numeric_pools_printer_singleton_and_destructor_close_boundaries(self):
        toc = 0x108bba90
        for slot, address in ((-0x3e14, 0x108a0124), (-0x3e18, 0x108a015c),
                              (-0x7408, 0x10bee468), (-0x3e1c, 0x10bee478),
                              (-0x5b94, 0x108bd634)):
            self.assertEqual(self.word(toc + slot), address)
        pool = b'\x0ePrinter Format\x07CourierSIPRINT.CPP\0'
        self.assertEqual(self.original_bytes(0x108a0124, len(pool)), pool)
        self.assertEqual(self.original_bytes(0x108a015c, 8), bytes.fromhex('4330000080000000'))
        self.assertEqual((self.word(0x108bd634), self.word(0x108bd638)), (ENTRIES[0], toc))
        self.assertEqual(self.word(0x108bd63c), 0x1074d450)
        expected = {
            0x1074c954: 'blr', 0x1074c960: 'mfspr r0,LR',
            0x1074d344: 'lwz r3,-0x7408(r2)', 0x1074d350: 'lwz r4,-0x5b94(r2)',
            0x1074d358: 'lwz r5,-0x3e1c(r2)', 0x1074d378: 'blr',
            0x1074d380: 'mfspr r0,LR', 0x1074f1fc: 'bl 0x1074d380',
            0x1074f268: 'bl 0x1074d450', 0x1074f27c: 'bl 0x1074d450',
            0x1074f29c: 'bl 0x1074f2c0',
        }
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, assembly in expected.items():
                actual = json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                                (address,)).fetchone()[0])
                self.assertEqual(actual['assembly'], assembly)
            self.assertEqual(list(db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                             ('%-0x3e14(r2)%',))),
                             [(0x1074cbd4,), (0x1074cd10,), (0x1074cda0,)])

    def test_all_incoming_edges_have_named_entry_and_no_external_fallthrough(self):
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
                        incoming.append((address, target))
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*audit.MODULE, *audit.MODULE)))
        self.assertEqual(tuple(incoming), INCOMING)
        self.assertEqual(fallthroughs, [])

    def test_all_cross_entry_edges_are_the_two_direct_owned_calls(self):
        cross = set()
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                self.assertNotIn('COMPUTED', record['flow'])
                targets = record['flows'] + ([record['fallthrough']]
                                              if record.get('fallthrough') is not None else [])
                for target in targets:
                    self.assertNotIn(target, PADDING)
                    self.assertNotEqual(target, UNREACHABLE_NOP)
                    if audit.MODULE[0] <= target < audit.MODULE[1] and not start <= target < end:
                        self.assertIn(target, ENTRIES)
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        cross.add((record['address'], target))
        self.assertEqual(cross, OWNED_CALLS)
        self.assertEqual([s for s in self.program['symbols']
                          if audit.MODULE[0] <= s['address'] < audit.MODULE[1]], [])

    def test_every_instruction_padding_and_unreachable_interior_nop_accounted(self):
        addresses = set()
        for start, end in audit.ROUTINES:
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual([r['address'] for r in records],
                             [a for a in range(start, end, 4) if a != UNREACHABLE_NOP])
            self.assertEqual(records[-1]['assembly'], 'blr')
            addresses.update(range(start, end, 4))
        self.assertEqual(set(range(*audit.MODULE, 4)) - addresses, PADDING)
        for address in PADDING:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)
        self.assertEqual(self.word(UNREACHABLE_NOP), 0x60000000)
        self.assertEqual(self.by_address[UNREACHABLE_NOP - 4]['assembly'], 'b 0x1074d240')
        self.assertNotIn(UNREACHABLE_NOP, self.by_address)

    def test_raw_pointer_census_at_every_byte_finds_only_destructor_vector(self):
        pointers = {}
        for start, contents in self.segments:
            for match in re.finditer(b'\x10\x74', contents):
                offset = match.start()
                if offset + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if audit.MODULE[0] <= target < audit.MODULE[1]:
                    pointers[start + offset] = target
        self.assertEqual(pointers, {0x108bd634: 0x1074c960})

    def test_missing_export_and_stale_digest_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_siprint_reference.py'),
                                     directory, str(Path(directory) / 'reference.cpp')],
                                    text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((Path(directory) / 'reference.cpp').exists())
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing SIPRINT.CPP instructions'):
                audit.require_siprint_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten(drafts=True))]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_siprint_index(db)

    def test_duplicate_bad_boundary_draft_and_missing_owner_are_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'SIPRINT_ROUTINE(' in line)
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('0x1074c99c, R3', '0x1074c9a0, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('SIPRINT_ROUTINE(', 'SIPRINT_DRAFT(', 1))
            self.assertEqual(len(audit.rewritten(candidate, drafts=True)), 9)
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all nine complete'):
                    audit.require_siprint_index(db, candidate)
            candidate.write_text(contents.replace(first, ''))
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all nine complete'):
                    audit.require_siprint_index(db, candidate)
        missing = [(at, owner) for at, owner in OWNER_REPLACEMENTS if at != ENTRIES[0]]
        with self.assertRaises(AssertionError):
            self.assert_production(audit.rewritten(), missing)

    def test_normal_hooks_precede_original_steps_and_detect_disabled_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_siprint_hooks(page, part, normal)
            self.assertIn('#include "siprint_adapter.hpp"', normal)
            self.assertNotIn('try_modern_engine_initialization', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.siprint_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_siprint_hooks(page, part, broken)

    def test_generated_original_reference_has_every_label_and_no_rewrite(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_siprint_reference.py'),
                                     str(EXPORT), str(target)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            code = target.read_text()
        self.assertNotIn('try_modern_', code)
        self.assertNotIn('siprint::', code)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE),
                         [f'{r["address"]:x}' for r in self.records])


if __name__ == '__main__':
    unittest.main()
