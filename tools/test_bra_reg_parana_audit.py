"""Certify bra_reg_parana.cpp's entries, original boundary and production owner.

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

import bra_reg_parana_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
CONTRACTS = (
    (0x106f6a80, 0x106f6db8, 0xce, 'e1dcadd75e1abefe'),
    (0x106f6dc0, 0x106f6eb4, 0x3c, '30d73cb1f157ca2d'),
    (0x106f6ec0, 0x106f7ba8, 0x33a, '389793ab8794424f'),
    (0x106f7bb0, 0x106f7cf4, 0x51, '3be7f417825dd8c3'),
    (0x106f7d00, 0x106f7dc4, 0x31, 'c4708d7d139af57a'),
    (0x106f7dd0, 0x106f8094, 0xb0, 'f9b46f7c60d82834'),
    (0x106f80a0, 0x106f815c, 0x2f, 'fb7e098d797451dc'),
    (0x106f8160, 0x106f83d8, 0x9e, 'ea5389cb11341a75'),
    (0x106f83e0, 0x106f8540, 0x57, '54faf80ce5411dc6'),
    (0x106f8540, 0x106f871c, 0x77, '6d23d901aea5cbf0'),
    (0x106f8720, 0x106f8814, 0x3d, '274962f9e639f678'),
    (0x106f8820, 0x106f89cc, 0x6b, '674286e615677c0f'),
    (0x106f89d0, 0x106f8a5c, 0x23, '54f3bd0d0221d7bb'),
    (0x106f8a60, 0x106f8b50, 0x3b, '02fefddf5675e9ec'),
    (0x106f8b50, 0x106f8d08, 0x6d, 'fcb706fbde0de12c'),
    (0x106f8d10, 0x106f8d30, 0x8, '4ab917a2b6aa561a'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
PADDING = {0x106f6db8, 0x106f6dbc, 0x106f6eb4, 0x106f6eb8, 0x106f6ebc, 0x106f7ba8, 0x106f7bac, 0x106f7cf4, 0x106f7cf8, 0x106f7cfc, 0x106f7dc4, 0x106f7dc8, 0x106f7dcc, 0x106f8094, 0x106f8098, 0x106f809c, 0x106f815c, 0x106f83d8, 0x106f83dc, 0x106f871c, 0x106f8814, 0x106f8818, 0x106f881c, 0x106f89cc, 0x106f8a5c, 0x106f8d08, 0x106f8d0c}
UNREACHABLE_NOPS = {0x106f6e1c: 'b 0x106f6e4c', 0x106f7e38: 'b 0x106f7e68', 0x106f8510: 'b 0x106f852c', 0x106f8ac0: 'b 0x106f8ad4', 0x106f8bf8: 'b 0x106f8c14'}
INCOMING = ((0x10350a18, 0x106f6a80),)
OWNED_CALLS = {(0x106f6bd0, 0x106f7bb0), (0x106f7f30, 0x106f7bb0), (0x106f8140, 0x106f8160), (0x106f8ad0, 0x106f8b50)}
TRANSITION_VECTORS = {0x108bd23c: 0x106f8d10, 0x108bd244: 0x106f8a60, 0x108bd24c: 0x106f89d0, 0x108bd254: 0x106f8820, 0x108bd25c: 0x106f8720, 0x108bd264: 0x106f8540, 0x108bd26c: 0x106f83e0, 0x108bd274: 0x106f80a0, 0x108bd27c: 0x106f7dd0, 0x108bd284: 0x106f7d00, 0x108bd28c: 0x106f6ec0, 0x108bd294: 0x106f6dc0}
VTABLE_SLOTS = {0x8: 0x108bd294, 0xc: 0x108bd244, 0x10: 0x108bd27c, 0x30: 0x108bd274, 0x38: 0x108bd25c, 0x44: 0x108bd28c, 0x4c: 0x108bd26c, 0x50: 0x108bd264, 0x64: 0x108bd254, 0x70: 0x108bd24c, 0x94: 0x108bd284, 0x98: 0x108bd23c}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private BRA_REG_PARANA export unavailable; inventory cannot be certified')


class BraRegParanaAuditTests(unittest.TestCase):
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
            self.assertEqual([owner for at, owner in owners if at == entry], ['bra_reg_parana_adapter'])

    def assert_hook(self, code, entry):
        hook = audit.bra_reg_parana_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_exact_entries_original_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x106f6a80, 0x106f8d30))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.BRA_REG_PARANA_ENTRIES, ENTRIES)
        self.assertEqual(audit.UNREACHABLE_NOPS, frozenset(UNREACHABLE_NOPS))
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(self.records), 2188)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        self.assert_production(audit.rewritten(), OWNER_REPLACEMENTS)
        self.assertFalse(set(ENTRIES) & {entry for entry, _ in SEMANTIC_REPLACEMENTS})
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_bra_reg_parana_index(db)

    def test_source_pool_vtable_and_neighbours_close_the_boundaries(self):
        toc = 0x108bba90
        self.assertEqual(self.word(toc - 0x406c), 0x10896d30)
        self.assertEqual(self.original_bytes(0x10896d30, 0x20), b'bra_reg_parana.cpp\0v%s %s.%s %d\0')
        self.assertEqual(self.original_bytes(0x10896d50, 6), b'Error\0')
        vtable = self.word(toc - 0x5c9c)
        self.assertEqual(vtable, 0x108e0c58)
        for slot, vector in VTABLE_SLOTS.items():
            self.assertEqual(self.word(vtable + slot), vector)
        for vector, entry in TRANSITION_VECTORS.items():
            self.assertEqual((self.word(vector), self.word(vector + 4)), (entry, toc))
            self.assertIn(entry, ENTRIES)
        self.assertLess(self.word(0x108bd234), audit.MODULE[0])
        self.assertGreaterEqual(self.word(0x108bd29c), audit.MODULE[1])
        expected = {0x106f6a60: 'mfspr r0,LR', 0x106f6a7c: 'blr',
                    0x106f6a80: 'mfspr r0,LR', 0x106f8d2c: 'blr',
                    0x106f8d30: 'mfspr r0,LR', 0x106f8d48: 'lwz r31,-0x4068(r2)'}
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, assembly in expected.items():
                actual = json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                                (address,)).fetchone()[0])
                self.assertEqual(actual['assembly'], assembly)
            pool = [row[0] for row in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                 ('%-0x406c(r2)%',))]
        self.assertEqual(pool, [0x106f6a98, 0x106f6ed4, 0x106f7bcc, 0x106f7de8,
                                0x106f80a8, 0x106f8178, 0x106f8558, 0x106f8834, 0x106f8b60])
        # These are four literal halfwords in the TOC, not pointers to another module's pool.
        self.assertEqual(self.original_bytes(toc - 0x3620, 8), bytes.fromhex('0001000001000000'))

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

    def test_all_cross_entry_edges_are_the_four_direct_owned_calls(self):
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

    def test_external_dependency_list_is_exact(self):
        dependencies = {target for record in self.records
                        if record['flow'] == 'UNCONDITIONAL_CALL'
                        for target in record['flows']
                        if not audit.MODULE[0] <= target < audit.MODULE[1]}
        self.assertEqual(tuple(sorted(dependencies)), audit.EXTERNAL_DEPENDENCIES)
        self.assertEqual(len(dependencies), 45)
        # Every indirect method call is lowered through the original ABI thunk.
        self.assertIn(0x10781320, dependencies)
        self.assertFalse(any('COMPUTED' in record['flow'] for record in self.records))

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
            for match in re.finditer(b'\x10\x6f', contents):
                offset = match.start()
                if offset + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if audit.MODULE[0] <= target < audit.MODULE[1]:
                    pointers[start + offset] = target
        # Word-aligned pointers are exactly the vectors. Unaligned hits are bytes
        # inside instruction encodings and tables, which nothing loads as a word.
        self.assertEqual({at: to for at, to in pointers.items() if at % 4 == 0}, TRANSITION_VECTORS)
        self.assertEqual(sorted(at for at in pointers if at % 4), [0x101ad55b])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, assembly in ((0x101ad558, 'b 0x101ad868'),
                                      (0x101ad55c, 'xoris r0,r28,0x8000')):
                record = json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                                (address,)).fetchone()[0])
                self.assertEqual(record['assembly'], assembly)

    def test_missing_export_and_stale_digest_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_bra_reg_parana_reference.py'),
                                     directory, str(Path(directory) / 'reference.cpp')],
                                    text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((Path(directory) / 'reference.cpp').exists())
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing bra_reg_parana.cpp instructions'):
                audit.require_bra_reg_parana_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten(drafts=True))]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_bra_reg_parana_index(db)

    def test_duplicate_bad_boundary_draft_and_missing_owner_are_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'BRA_REG_PARANA_ROUTINE(' in line)
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('0x106f6db8, R3', '0x106f6dbc, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('BRA_REG_PARANA_ROUTINE(', 'BRA_REG_PARANA_DRAFT(', 1))
            self.assertEqual(len(audit.rewritten(candidate, drafts=True)), 16)
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all sixteen complete'):
                    audit.require_bra_reg_parana_index(db, candidate)
            candidate.write_text(contents.replace(first, ''))
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all sixteen complete'):
                    audit.require_bra_reg_parana_index(db, candidate)
        missing = [(at, owner) for at, owner in OWNER_REPLACEMENTS if at != ENTRIES[0]]
        with self.assertRaises(AssertionError):
            self.assert_production(audit.rewritten(), missing)

    def test_normal_hooks_precede_original_steps_and_detect_disabled_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_bra_reg_parana_hooks(page, part, normal)
            self.assertIn('#include "bra_reg_parana_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.bra_reg_parana_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_bra_reg_parana_hooks(page, part, broken)

    def test_generated_original_reference_has_every_label_and_no_rewrite(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_bra_reg_parana_reference.py'),
                                     str(EXPORT), str(target)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            code = target.read_text()
        self.assertNotIn('try_modern_', code)
        observed = re.findall(r'bra_reg_parana::detail::observe_entry\(c, (0x[0-9a-f]+)\);', code)
        self.assertEqual([int(at, 16) for at in observed], list(ENTRIES))
        without_observer = re.sub(r'bra_reg_parana::detail::observe_entry\(c, 0x[0-9a-f]+\);', '', code)
        self.assertNotIn('bra_reg_parana::', without_observer)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE),
                         [f'{r["address"]:x}' for r in self.records])


if __name__ == '__main__':
    unittest.main()
