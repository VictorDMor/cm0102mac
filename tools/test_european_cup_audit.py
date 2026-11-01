"""Certify european_cup.cpp's entries, original boundary and production owner.

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

import european_cup_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
CONTRACTS = (
    (0x104e9fb0, 0x104ea300, 212, 'c0060d62e81c90cc'),
    (0x104ea300, 0x104ea44c, 82, 'fa07aaf1dfbf7be5'),
    (0x104ea450, 0x104ea618, 114, 'cf155ef767203efd'),
    (0x104ea620, 0x104eb288, 794, '6fd824fcee5aae35'),
    (0x104eb290, 0x104eb764, 309, '3d670e1107952ccd'),
    (0x104eb770, 0x104eb900, 100, '216138f85147fc43'),
    (0x104eb900, 0x104eb934, 13, '42c60aef766098a2'),
    (0x104eb940, 0x104ebc00, 176, '8f489bfda01f4217'),
    (0x104ebc00, 0x104ebf30, 203, '71140ff109a54f08'),
    (0x104ebf30, 0x104ebf80, 20, '79c5ecf84e5bf161'),
    (0x104ebf80, 0x104ec20c, 163, 'c8cc5ee9278eda74'),
    (0x104ec210, 0x104ec2f8, 58, '4fff5554af093178'),
    (0x104ec300, 0x104ed110, 895, '1d965b4c104a67bf'),
    (0x104ed110, 0x104ed688, 350, '755a42dbfad888d3'),
    (0x104ed690, 0x104ee2d4, 778, '4c37cc1a8eb791b8'),
    (0x104ee2e0, 0x104ee7f0, 322, '9a7f9366a37ea034'),
    (0x104ee7f0, 0x104eeac4, 180, '80f7b8e2f7bf2f63'),
    (0x104eead0, 0x104eee20, 211, 'fd0f3e964faa7338'),
    (0x104eee20, 0x104ef0fc, 183, '2fb2d648f4126e7b'),
    (0x104ef100, 0x104ef130, 12, '3efe2888c92fec40'),
    (0x104ef130, 0x104ef30c, 119, '56ef0940ee3161e0'),
    (0x104ef310, 0x104ef370, 24, 'd84893be206a73a4'),
    (0x104ef370, 0x104ef378, 2, '1b5aee182a765fe6'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
PADDING = {0x104ea44c, 0x104ea618, 0x104ea61c, 0x104eb288, 0x104eb28c, 0x104eb764, 0x104eb768, 0x104eb76c, 0x104eb934, 0x104eb938, 0x104eb93c, 0x104ec20c, 0x104ec2f8, 0x104ec2fc, 0x104ed688, 0x104ed68c, 0x104ee2d4, 0x104ee2d8, 0x104ee2dc, 0x104eeac4, 0x104eeac8, 0x104eeacc, 0x104ef0fc, 0x104ef30c, 0x104ef378, 0x104ef37c}
UNREACHABLE_NOPS = {0x104ea3a4: 'b 0x104ea3d4', 0x104ebcd8: 'b 0x104ebd08', 0x104ec658: 'b 0x104ec668', 0x104ecb64: 'b 0x104ecb74', 0x104ecbf8: 'b 0x104ecc94', 0x104eccb0: 'b 0x104ecd84', 0x104ed05c: 'b 0x104ed0e0', 0x104ed810: 'b 0x104ed844', 0x104ed868: 'b 0x104ed89c', 0x104eda70: 'b 0x104edb2c', 0x104edb20: 'b 0x104edb38', 0x104edc90: 'b 0x104edcf8', 0x104edcf0: 'b 0x104edd04', 0x104ede74: 'b 0x104edebc', 0x104ee438: 'b 0x104ee48c', 0x104ee4ac: 'b 0x104ee500', 0x104ee974: 'b 0x104ee9ac', 0x104eebd0: 'b 0x104eebec'}
INCOMING = ((0x1034f550, 0x104e9fb0),)
OWNED_CALLS = {(0x104ea0fc, 0x104ebf80), (0x104ea104, 0x104eb770), (0x104ebd64, 0x104ebf80), (0x104ebdc8, 0x104eb770),
               (0x104ec2c8, 0x104ec300), (0x104ec2d4, 0x104ed690), (0x104ec2dc, 0x104ee2e0), (0x104ec9a4, 0x104ed110)}
TRANSITION_VECTORS = {0x108ba4ac: 0x104ef370, 0x108ba4b4: 0x104ef130, 0x108ba4bc: 0x104eee20, 0x108ba4c4: 0x104eead0,
                      0x108ba4cc: 0x104ee7f0, 0x108ba4d4: 0x104ef310, 0x108ba4dc: 0x104ef100, 0x108ba4e4: 0x104ec210,
                      0x108ba4ec: 0x104ebf30, 0x108ba4f4: 0x104ebc00, 0x108ba4fc: 0x104eb940, 0x108ba504: 0x104eb900,
                      0x108ba50c: 0x104eb290, 0x108ba514: 0x104ea620, 0x108ba51c: 0x104ea450, 0x108ba524: 0x104ea300}
VTABLE_SLOTS = {0x08: 0x108ba524, 0x0c: 0x108ba51c, 0x10: 0x108ba4f4, 0x30: 0x108ba4e4, 0x38: 0x108ba504, 0x40: 0x108ba4b4,
                0x44: 0x108ba514, 0x4c: 0x108ba4fc, 0x50: 0x108ba50c, 0x60: 0x108ba4cc, 0x64: 0x108ba4c4, 0x68: 0x108ba4ac,
                0x70: 0x108ba4bc, 0x94: 0x108ba4ec}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private EUROPEAN_CUP export unavailable; inventory cannot be certified')


class EuropeanCupAuditTests(unittest.TestCase):
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
            self.assertEqual([owner for at, owner in owners if at == entry], ['european_cup_adapter'])

    def assert_hook(self, code, entry):
        hook = audit.european_cup_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_exact_entries_original_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x104e9fb0, 0x104ef380))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.EUROPEAN_CUP_ENTRIES, ENTRIES)
        self.assertEqual(audit.UNREACHABLE_NOPS, frozenset(UNREACHABLE_NOPS))
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(self.records), 5320)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        self.assert_production(audit.rewritten(), OWNER_REPLACEMENTS)
        self.assertFalse(set(ENTRIES) & {entry for entry, _ in SEMANTIC_REPLACEMENTS})
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_european_cup_index(db)

    def test_source_pool_vtable_and_neighbours_close_the_boundaries(self):
        toc = 0x108bba90
        self.assertEqual(self.word(toc - 0x45c8), 0x1083ca94)
        self.assertEqual(self.word(toc - 0x45cc), 0x1083cd68)
        self.assertEqual(self.original_bytes(0x1083ca94, 0x24), b'EUROPEAN_CUP.CPP\0v%s %s.%s %d\0Error\0')
        self.assertEqual(self.original_bytes(0x1083cd78, 0x12), b'EUR_SUPER_CUP.CPP\0')
        vtable = self.word(toc - 0x6094)
        self.assertEqual(vtable, 0x108d94b0)
        for slot, vector in VTABLE_SLOTS.items():
            self.assertEqual(self.word(vtable + slot), vector)
        for vector, entry in TRANSITION_VECTORS.items():
            self.assertEqual((self.word(vector), self.word(vector + 4)), (entry, toc))
            self.assertIn(entry, ENTRIES)
        self.assertEqual((self.word(toc - 0x6098), self.word(toc - 0x609c)), (0x108ba4dc, 0x108ba4d4))
        # The neighbouring modules' vectors flank these in the data segment.
        self.assertLess(self.word(0x108ba4a4), audit.MODULE[0])
        self.assertGreaterEqual(self.word(0x108ba52c), audit.MODULE[1])
        expected = {0x104e9fa0: 'blr', 0x104e9fb0: 'mfspr r0,LR', 0x104ef374: 'blr', 0x104ef380: 'mfspr r0,LR',
                    0x104ef398: 'lwz r31,-0x45c4(r2)'}
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, assembly in expected.items():
                actual = json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                                (address,)).fetchone()[0])
                self.assertEqual(actual['assembly'], assembly)
            pool = [row[0] for row in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                 ('%-0x45c8(r2)%',))]
            doubles = [row[0] for row in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                    ('%-0x45cc(r2)%',))]
        self.assertEqual(pool, [0x104e9fc8, 0x104ea638, 0x104eb2b8, 0x104eb780, 0x104ebc14, 0x104ebfa0, 0x104ec218, 0x104ec364, 0x104ec380, 0x104ec3a0, 0x104ec558, 0x104ec574, 0x104ec594, 0x104ec68c, 0x104ec6a8, 0x104ec6c8, 0x104ecf24, 0x104ecf40, 0x104ecf60, 0x104ecfe4, 0x104ed000, 0x104ed020, 0x104ed6f8, 0x104ed714, 0x104ed734, 0x104ed914, 0x104ed930, 0x104ed950, 0x104ed988, 0x104ed9a4, 0x104ed9c4, 0x104ee0c0, 0x104ee0dc, 0x104ee0fc, 0x104ee194, 0x104ee1b0, 0x104ee1d0, 0x104ee310, 0x104ee808, 0x104eeae8, 0x104ef144])
        self.assertEqual(doubles, [0x104ec30c, 0x104ed11c])

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
            for match in re.finditer(b'\x10\x4e', contents):
                offset = match.start()
                if offset + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if audit.MODULE[0] <= target < audit.MODULE[1]:
                    pointers[start + offset] = target
        # Word-aligned pointers are exactly the vectors. Unaligned hits are bytes
        # inside instruction encodings and tables, which nothing loads as a word.
        # 0x108f8f94 holds a table value with an odd target: not an instruction address.
        self.assertEqual({at: to for at, to in pointers.items() if at % 4 == 0},
                         {**TRANSITION_VECTORS, 0x108f8f94: 0x104ee879})
        self.assertEqual(sorted(at for at in pointers if at % 4),
                         [])

    def test_missing_export_and_stale_digest_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_european_cup_reference.py'),
                                     directory, str(Path(directory) / 'reference.cpp')],
                                    text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((Path(directory) / 'reference.cpp').exists())
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing european_cup.cpp instructions'):
                audit.require_european_cup_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten(drafts=True))]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_european_cup_index(db)

    def test_duplicate_bad_boundary_draft_and_missing_owner_are_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'EUROPEAN_CUP_ROUTINE(' in line)
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('0x104ea300, R3', '0x104ea2fc, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('EUROPEAN_CUP_ROUTINE(', 'EUROPEAN_CUP_DRAFT(', 1))
            self.assertEqual(len(audit.rewritten(candidate, drafts=True)), 23)
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all twenty-three complete'):
                    audit.require_european_cup_index(db, candidate)
            candidate.write_text(contents.replace(first, ''))
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all twenty-three complete'):
                    audit.require_european_cup_index(db, candidate)
        missing = [(at, owner) for at, owner in OWNER_REPLACEMENTS if at != ENTRIES[0]]
        with self.assertRaises(AssertionError):
            self.assert_production(audit.rewritten(), missing)

    def test_normal_hooks_precede_original_steps_and_detect_disabled_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_european_cup_hooks(page, part, normal)
            self.assertIn('#include "european_cup_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.european_cup_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_european_cup_hooks(page, part, broken)

    def test_generated_original_reference_has_every_label_and_no_rewrite(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_european_cup_reference.py'),
                                     str(EXPORT), str(target)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            code = target.read_text()
        self.assertNotIn('try_modern_', code)
        self.assertNotIn('european_cup::', code)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE),
                         [f'{r["address"]:x}' for r in self.records])


if __name__ == '__main__':
    unittest.main()
