"""Certify FILE_SCREENS.CPP's entries, original boundaries and production owner.

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

import file_screens_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x100edf20, 0x100ee214, 189, '6ed851d3d6823972'),
    (0x100ee220, 0x100ee9c8, 490, '204222bd4303b7c5'),
    (0x100ee9d0, 0x100ef7c8, 894, '3c89863aaa8f87f6'),
    (0x100ef7d0, 0x100ef83c, 27, 'ae815f48cc750e51'),
    (0x100ef840, 0x100f052c, 826, '1d9a5bd9a09918c6'),
    (0x100f0530, 0x100f0598, 26, '37a0559e227afa48'),
    (0x100f05a0, 0x100f071c, 95, '86d8c8efed465413'),
    (0x100f0720, 0x100f08b4, 101, '916762ab3b7a7041'),
    (0x100f08c0, 0x100f0ba0, 184, '13241c9f54f9cd53'),
    (0x100f0ba0, 0x100f0d70, 116, '8ef23f7f63c945e4'),
    (0x100f0d70, 0x100f0ff8, 161, '69119503f920b7fb'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
PADDING = {0x100ee214, 0x100ee218, 0x100ee21c, 0x100ee9c8, 0x100ee9cc, 0x100ef7c8, 0x100ef7cc, 0x100ef83c, 0x100f052c,
           0x100f0598, 0x100f059c, 0x100f071c, 0x100f08b4, 0x100f08b8, 0x100f08bc, 0x100f0ff8, 0x100f0ffc}
UNREACHABLE_NOPS = {0x100f0474: 'b 0x100f0500', 0x100f0dbc: 'b 0x100f0e18'}
OWNED_CALLS = {(0x100ef20c, 0x100ef7d0)} | {(at, 0x100f05a0) for at in (
    0x100eeb14, 0x100eee60, 0x100eeea8, 0x100ef0c8, 0x100ef19c, 0x100ef258, 0x100ef374, 0x100ef3b8, 0x100ef49c,
    0x100ef5c4, 0x100ef6d8, 0x100ef74c)}
INCOMING_COUNTS = {0x100edf20: 49, 0x100ee220: 0, 0x100ee9d0: 0, 0x100ef7d0: 0, 0x100ef840: 0, 0x100f0530: 0,
                   0x100f05a0: 204, 0x100f0720: 2, 0x100f08c0: 0, 0x100f0ba0: 0, 0x100f0d70: 1}
# Transition vector -> (entry, TOC slot pointing at the vector, the slot's only readers).
VECTORS = {0x108b8a14: (0x100f0ba0, -0x55f4, {0x100f064c, 0x100f07e4}),
           0x108b8a1c: (0x100f08c0, -0x55f0, {0x100f0644, 0x100f07dc}),
           0x108b8a24: (0x100f0530, -0x55e4, {0x100ef7f8}),
           0x108b8a2c: (0x100ef840, -0x55e0, {0x100ef7e8}),
           0x108b8a34: (0x100ee9d0, -0x55dc, {0x100edfe8}),
           0x108b8a3c: (0x100ee220, -0x55d8, {0x100edfe0})}
# FINANCE.CPP's class methods, first in its code: only its array calls read them.
FINANCE_VECTORS = {0x108b8a44: (0x100f1010, -0x74a4, {0x100f8620, 0x100f874c}),
                   0x108b8a4c: (0x100f1000, -0x74a0, {0x100f8618})}
STRINGS = {0x0: b'FILE_SCREENS.CPP', 0x11: b'v%s %s.%s %d', 0x1e: b'Error', 0x24: b'File :', 0x2b: b'*', 0x2d: b'Cancel', 0x34: b'Info',
           0x39: b'Ok', 0x3c: b'Unable To Create', 0x84: b'Directory Not Found', 0x11a: b'File Is Blank', 0x148: b"File Doesn't Exist",
           0x182: b'general.dat', 0x18e: b'Cannot Read File Info', 0x1d0: b'Directory Is Blank', 0x208: b'File Exists',
           0x24f: b'Please Confirm', 0x27e: b'v%s %s.%s %d (%s)', 0x2d9: b'Load Game Error', 0x42d: b'Save Game Info', 0x43c: b'File Name',
           0x446: b'Last Saved', 0x4cc: b'Version', 0x4d4: b'Date', 0x4d9: b'Game Time', 0x564: b'Addictedness Rating',
           0x6e4: b'Foreground Leagues', 0x6f7: b'Background Leagues', 0x70a: b'Managers'}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private FILE_SCREENS export unavailable; inventory cannot be certified')


class FileScreensAuditTests(unittest.TestCase):
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

    def readers(self, db, slot):
        return {row[0] for row in db.execute('SELECT address FROM instructions WHERE body LIKE ?', (f'%,-{-slot:#x}(r2)%',))}

    def assert_production(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(audit.ROUTINES))
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in owners if at == entry], ['file_screens_adapter'])

    def assert_hook(self, code, entry):
        hook = audit.file_screens_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_exact_entries_original_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x100edf20, 0x100f1000))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.FILE_SCREENS_ENTRIES, ENTRIES)
        self.assertEqual(audit.UNREACHABLE_NOPS, frozenset(UNREACHABLE_NOPS))
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(self.records), 3109)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        self.assert_production(audit.rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_file_screens_index(db)

    def test_callers_and_callees_read_no_register_the_cpp_leaves_different(self):
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            self.assertEqual(audit.volatile_reads(db), [])

    def test_source_pool_and_neighbours_close_the_boundaries(self):
        pool = self.word(TOC - 0x55d4)
        self.assertEqual(pool, 0x107a25e0)
        for offset, text in STRINGS.items():
            self.assertEqual(self.original_bytes(pool + offset, len(text) + 1), text + b'\0')
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            readers = sorted(self.readers(db, -0x55d4))
            scouts = sorted(self.readers(db, -0x55fc))
            finance = sorted(self.readers(db, -0x55a0) | self.readers(db, -0x5598))
            before = json.loads(db.execute('SELECT body FROM instructions WHERE address<? ORDER BY address DESC LIMIT 1',
                                           (audit.MODULE[0],)).fetchone()[0])
        self.assertEqual(readers, [0x100edf38, 0x100ee244, 0x100ee9e8, 0x100ef868, 0x100f05b8, 0x100f0738, 0x100f08e0, 0x100f0da0])
        self.assertEqual(scouts[-1], 0x100edcf0)
        self.assertEqual(finance[0], 0x100f1078)
        self.assertEqual(before['assembly'], 'blr')
        # The first routine after the module is FINANCE's, not this source's.
        self.assertEqual(self.word(audit.MODULE[1]), 0x4e800020)

    def test_vectors_are_the_screen_callbacks_and_finance_owns_the_two_after_the_module(self):
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for vector, (entry, slot, readers) in {**VECTORS, **FINANCE_VECTORS}.items():
                self.assertEqual(self.word(vector), entry)
                self.assertEqual(self.word(vector + 4), TOC)
                self.assertEqual(self.word(TOC + slot), vector)
                self.assertEqual(self.readers(db, slot), readers)
        for _, (_, _, readers) in FINANCE_VECTORS.items():
            self.assertTrue(all(reader >= 0x100f8000 for reader in readers))
        self.assertFalse({entry for entry, _, _ in FINANCE_VECTORS.values()} & set(ENTRIES))

    def test_all_incoming_edges_are_calls_to_named_entries_with_no_external_fallthrough(self):
        counts = dict.fromkeys(ENTRIES, 0)
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
                        counts[target] += 1
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*audit.MODULE, *audit.MODULE)))
        self.assertEqual(counts, INCOMING_COUNTS)
        self.assertEqual(fallthroughs, [])

    def test_all_cross_entry_edges_are_the_direct_owned_calls(self):
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
                    self.assertNotIn(target, UNREACHABLE_NOPS)
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
                             [a for a in range(start, end, 4) if a not in UNREACHABLE_NOPS])
            self.assertEqual(records[-1]['assembly'], 'blr')
            addresses.update(range(start, end, 4))
        self.assertEqual(set(range(*audit.MODULE, 4)) - addresses, PADDING)
        for address in PADDING:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)
        for address, jump in UNREACHABLE_NOPS.items():
            self.assertEqual(self.word(address), 0x60000000)
            self.assertEqual(self.by_address[address - 4]['assembly'], jump)
            self.assertNotIn(address, self.by_address)

    def test_raw_pointer_census_finds_only_the_vectors(self):
        pointers = {}
        for start, contents in self.segments:
            for offset in range(0, len(contents) - 3, 4):
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if audit.MODULE[0] <= target < audit.MODULE[1]:
                    pointers[start + offset] = target
        self.assertEqual(pointers, {vector: entry for vector, (entry, _, _) in VECTORS.items()})

    def test_missing_export_and_stale_digest_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_file_screens_reference.py'),
                                     directory, str(Path(directory) / 'reference.cpp')],
                                    text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((Path(directory) / 'reference.cpp').exists())
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing FILE_SCREENS.CPP instructions'):
                audit.require_file_screens_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten())]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_file_screens_index(db)

    def test_duplicate_bad_boundary_and_missing_owner_are_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'FILE_SCREENS_ROUTINE(' in line)
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('0x100ee214, R3', '0x100ee218, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace(first, ''))
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all eleven complete'):
                    audit.require_file_screens_index(db, candidate)
        missing = [(at, owner) for at, owner in OWNER_REPLACEMENTS if at != ENTRIES[0]]
        with self.assertRaises(AssertionError):
            self.assert_production(audit.rewritten(), missing)

    def test_normal_hooks_precede_original_steps_and_detect_disabled_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_file_screens_hooks(page, part, normal)
            self.assertIn('#include "file_screens_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.file_screens_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_file_screens_hooks(page, part, broken)

    def test_generated_original_reference_has_every_label_and_no_rewrite(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_file_screens_reference.py'),
                                     str(EXPORT), str(target)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            code = target.read_text()
        self.assertNotIn('try_modern_', code)
        self.assertNotIn('file_screens::', code)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE),
                         [f'{r["address"]:x}' for r in self.records])


if __name__ == '__main__':
    unittest.main()
