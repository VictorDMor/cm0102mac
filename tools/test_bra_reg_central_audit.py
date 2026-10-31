"""Certify bra_reg_central.cpp's entries, original boundary and production owner.

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

import bra_reg_central_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
CONTRACTS = (
    (0x106d73f0, 0x106d7728, 206, '6e638e6787aa40af'),
    (0x106d7730, 0x106d7824, 60, '941a5c92f6e3d1da'),
    (0x106d7830, 0x106d835c, 715, 'd3e6efa3e7dd9b90'),
    (0x106d8360, 0x106d84a4, 81, '44101a367f5191b9'),
    (0x106d84b0, 0x106d8568, 46, '76dbb97f04e392ac'),
    (0x106d8570, 0x106d8800, 163, '15c12eca554e7f4e'),
    (0x106d8800, 0x106d88bc, 47, 'f6e2bc3d8cffde4e'),
    (0x106d88c0, 0x106d8ae8, 138, 'd4fc0a84a037cdf2'),
    (0x106d8af0, 0x106d8c48, 84, 'ffbbe3842dd9e506'),
    (0x106d8c50, 0x106d8e2c, 119, '7420ea87805cd760'),
    (0x106d8e30, 0x106d8e68, 14, '72b148fbd6f0f09d'),
    (0x106d8e70, 0x106d90b8, 146, 'aa12a55420fdc979'),
    (0x106d90c0, 0x106d914c, 35, 'fa27d995b67ae892'),
    (0x106d9150, 0x106d9170, 8, 'f0a9fd69ab4455d8'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
PADDING = {0x106d7728, 0x106d772c, 0x106d7824, 0x106d7828, 0x106d782c, 0x106d835c,
           0x106d84a4, 0x106d84a8, 0x106d84ac, 0x106d8568, 0x106d856c, 0x106d88bc,
           0x106d8ae8, 0x106d8aec, 0x106d8c48, 0x106d8c4c, 0x106d8e2c, 0x106d8e68,
           0x106d8e6c, 0x106d90b8, 0x106d90bc, 0x106d914c}
UNREACHABLE_NOPS = {0x106d778c: 'b 0x106d77bc', 0x106d85d8: 'b 0x106d8608',
                    0x106d8b74: 'b 0x106d8c24', 0x106d8c18: 'b 0x106d8c34'}
INCOMING = ((0x103508e0, 0x106d73f0),)
OWNED_CALLS = {(0x106d7540, 0x106d8360), (0x106d869c, 0x106d8360), (0x106d88a0, 0x106d88c0)}
TRANSITION_VECTORS = {0x108bcd24: 0x106d9150, 0x108bcd2c: 0x106d90c0, 0x108bcd34: 0x106d8e70,
                      0x108bcd3c: 0x106d8e30, 0x108bcd44: 0x106d8c50, 0x108bcd4c: 0x106d8af0,
                      0x108bcd54: 0x106d8800, 0x108bcd5c: 0x106d8570, 0x108bcd64: 0x106d84b0,
                      0x108bcd6c: 0x106d7830, 0x108bcd74: 0x106d7730}
VTABLE_SLOTS = {0x08: 0x108bcd74, 0x10: 0x108bcd5c, 0x30: 0x108bcd54, 0x38: 0x108bcd3c, 0x44: 0x108bcd6c,
                0x4c: 0x108bcd4c, 0x50: 0x108bcd44, 0x64: 0x108bcd34, 0x70: 0x108bcd2c, 0x94: 0x108bcd64,
                0x98: 0x108bcd24}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private BRA_REG_CENTRAL export unavailable; inventory cannot be certified')


class BraRegCentralAuditTests(unittest.TestCase):
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
            self.assertEqual([owner for at, owner in owners if at == entry], ['bra_reg_central_adapter'])

    def assert_hook(self, code, entry):
        hook = audit.bra_reg_central_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_exact_entries_original_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x106d73f0, 0x106d9170))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.BRA_REG_CENTRAL_ENTRIES, ENTRIES)
        self.assertEqual(audit.UNREACHABLE_NOPS, frozenset(UNREACHABLE_NOPS))
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(self.records), 1862)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        self.assert_production(audit.rewritten(), OWNER_REPLACEMENTS)
        self.assertFalse(set(ENTRIES) & {entry for entry, _ in SEMANTIC_REPLACEMENTS})
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_bra_reg_central_index(db)

    def test_source_pool_vtable_and_neighbours_close_the_boundaries(self):
        toc = 0x108bba90
        self.assertEqual(self.word(toc - 0x40c8), 0x10895aa8)
        self.assertEqual(self.word(toc - 0x40c4), 0x10895ab0)
        self.assertEqual(self.original_bytes(0x10895aa8, 8), bytes.fromhex('0007020506010403'))
        self.assertEqual(self.original_bytes(0x10895ab0, 0x21), b'bra_reg_central.cpp\0v%s %s.%s %d\0')
        self.assertEqual(self.original_bytes(0x10895ad1, 6), b'Error\0')
        vtable = self.word(toc - 0x5ce8)
        self.assertEqual(vtable, 0x108e00ac)
        for slot, vector in VTABLE_SLOTS.items():
            self.assertEqual(self.word(vtable + slot), vector)
        for vector, entry in TRANSITION_VECTORS.items():
            self.assertEqual((self.word(vector), self.word(vector + 4)), (entry, toc))
            self.assertIn(entry, ENTRIES)
        # The neighbouring modules' vectors flank these in the data segment.
        self.assertLess(self.word(0x108bcd1c), audit.MODULE[0])
        self.assertGreaterEqual(self.word(0x108bcd7c), audit.MODULE[1])
        expected = {0x106d7300: 'mfspr r0,LR', 0x106d73e0: 'blr', 0x106d73f0: 'mfspr r0,LR',
                    0x106d916c: 'blr', 0x106d9170: 'mfspr r0,LR', 0x106d9188: 'lwz r31,-0x40c0(r2)'}
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, assembly in expected.items():
                actual = json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                                (address,)).fetchone()[0])
                self.assertEqual(actual['assembly'], assembly)
            pool = [row[0] for row in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                 ('%-0x40c4(r2)%',))]
            bracket = [row[0] for row in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                    ('%-0x40c8(r2)%',))]
        self.assertEqual(pool, [0x106d7408, 0x106d7844, 0x106d837c, 0x106d8588, 0x106d8808,
                                0x106d88dc, 0x106d8c68, 0x106d8e84])
        self.assertEqual(bracket, [0x106d88cc])

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

    def test_raw_pointer_census_at_every_byte_finds_only_the_transition_vectors(self):
        pointers = {}
        for start, contents in self.segments:
            for match in re.finditer(b'\x10\x6d', contents):
                offset = match.start()
                if offset + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if audit.MODULE[0] <= target < audit.MODULE[1]:
                    pointers[start + offset] = target
        # Word-aligned pointers are exactly the vectors. Unaligned hits are bytes
        # inside instruction encodings and tables, which nothing loads as a word.
        self.assertEqual({at: to for at, to in pointers.items() if at % 4 == 0}, TRANSITION_VECTORS)
        self.assertEqual(sorted(at for at in pointers if at % 4),
                         [0x100ead76, 0x101c8af6, 0x10318af6, 0x104ea6b6, 0x106a70c6, 0x106ca6b6, 0x108e82cf])

    def test_missing_export_and_stale_digest_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_bra_reg_central_reference.py'),
                                     directory, str(Path(directory) / 'reference.cpp')],
                                    text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((Path(directory) / 'reference.cpp').exists())
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing bra_reg_central.cpp instructions'):
                audit.require_bra_reg_central_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten(drafts=True))]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_bra_reg_central_index(db)

    def test_duplicate_bad_boundary_draft_and_missing_owner_are_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'BRA_REG_CENTRAL_ROUTINE(' in line)
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('0x106d7728, R3', '0x106d772c, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('BRA_REG_CENTRAL_ROUTINE(', 'BRA_REG_CENTRAL_DRAFT(', 1))
            self.assertEqual(len(audit.rewritten(candidate, drafts=True)), 14)
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all fourteen complete'):
                    audit.require_bra_reg_central_index(db, candidate)
            candidate.write_text(contents.replace(first, ''))
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all fourteen complete'):
                    audit.require_bra_reg_central_index(db, candidate)
        missing = [(at, owner) for at, owner in OWNER_REPLACEMENTS if at != ENTRIES[0]]
        with self.assertRaises(AssertionError):
            self.assert_production(audit.rewritten(), missing)

    def test_normal_hooks_precede_original_steps_and_detect_disabled_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_bra_reg_central_hooks(page, part, normal)
            self.assertIn('#include "bra_reg_central_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.bra_reg_central_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_bra_reg_central_hooks(page, part, broken)

    def test_generated_original_reference_has_every_label_and_no_rewrite(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_bra_reg_central_reference.py'),
                                     str(EXPORT), str(target)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            code = target.read_text()
        self.assertNotIn('try_modern_', code)
        self.assertNotIn('bra_reg_central::', code)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE),
                         [f'{r["address"]:x}' for r in self.records])


if __name__ == '__main__':
    unittest.main()
