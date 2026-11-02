"""Certify ENG_FIRST's closed inventory, reference and production routing.

Private original evidence is mandatory: missing exports fail rather than skip.
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

import eng_first_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
PRIVATE = ROOT / '.local/rewrites/eng_first'
CONTRACTS = (
    (0x105267c0, 0x10526aec, 203, 'f26fa18090eaa657'),
    (0x10526af0, 0x10526bf4, 64, '7d4302379c5656f0'),
    (0x10526c00, 0x10528c60, 2072, '74c1595191bf24b0'),
    (0x10528c60, 0x10528e4c, 122, '01ceb389d62607c3'),
    (0x10528e50, 0x10528f1c, 51, 'c54e75fc7110a0f6'),
    (0x10528f20, 0x105291b8, 165, 'b4f18ca865945bf7'),
    (0x105291c0, 0x1052927c, 47, '39adac2f0a2599bc'),
    (0x10529280, 0x10529464, 121, '6e6f5e721cf2581f'),
    (0x10529470, 0x105295ec, 94, '497009c0a9651c53'),
    (0x105295f0, 0x105296c8, 54, '01e61cfaacb4903e'),
    (0x105296d0, 0x10529834, 89, 'c91b710e66b70b50'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
PADDING = {0x10526aec, 0x10526bf4, 0x10526bf8, 0x10526bfc,
           0x10528e4c, 0x10528f1c, 0x105291b8, 0x105291bc,
           0x1052927c, 0x10529464, 0x10529468, 0x1052946c,
           0x105295ec, 0x105296c8, 0x105296cc,
           0x10529834, 0x10529838, 0x1052983c}
NOPS = {0x10526b4c, 0x10528d18, 0x10528f88, 0x10529570}
OWNED_CALLS = {(0x10526904, 0x10528c60), (0x1052904c, 0x10528c60),
               (0x10529260, 0x10529280)}
VECTORS = {0x108bac44: 0x105296d0, 0x108bac4c: 0x105295f0,
           0x108bac54: 0x10529470, 0x108bac5c: 0x105291c0,
           0x108bac64: 0x10528f20, 0x108bac6c: 0x10528e50,
           0x108bac74: 0x10526c00, 0x108bac7c: 0x10526af0}
LABELS = (0x105277d4, 0x10527908, 0x10527a3c, 0x10527b70,
          0x10527ca4, 0x10527dd8, 0x10527f0c)
SWITCH = {0x108da6b0 + 4 * i: label for i, label in enumerate(LABELS)}
FALSE_POINTERS = {0x105f9bd6: 0x10529001, 0x105f9c52: 0x10529061,
                  0x108e3fca: 0x105296a3}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file() or not (path / 'program.json').is_file():
        raise RuntimeError('Private ENG_FIRST export unavailable; inventory cannot be certified')


class EngFirstAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        PRIVATE.mkdir(parents=True, exist_ok=True)
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
            self.assertEqual([owner for at, owner in owners if at == entry], ['eng_first_adapter'])

    def record_at(self, db, address):
        row = db.execute('SELECT body FROM instructions WHERE address=?', (address,)).fetchone()
        self.assertIsNotNone(row, f'Missing original instruction {address:x}')
        return json.loads(row[0])

    def test_exact_entries_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x105267c0, 0x10529840))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.ENG_FIRST_ENTRIES, ENTRIES)
        self.assertEqual(audit.UNREACHABLE_NOPS, frozenset(NOPS))
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(self.records), 3082)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        self.assert_production(audit.rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_eng_first_index(db)

    def test_source_pools_and_neighbors_close_boundary(self):
        toc = 0x108bba90
        for slot, address, source in ((-0x4530, 0x1083e91c, b'ENG_CONF.CPP\0'),
                                      (-0x4528, 0x1083e93c, b'ENG_FIRST.CPP\0'),
                                      (-0x451c, 0x1083e990, b'ENG_PRM.CPP\0')):
            self.assertEqual(self.word(toc + slot), address)
            self.assertEqual(self.original_bytes(address, len(source)), source)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, assembly in {
                0x105267b4: 'blr', 0x105267c0: 'mfspr r0,LR',
                0x10529830: 'blr', 0x10529840: 'mfspr r0,LR',
                0x10529858: 'lwz r31,-0x451c(r2)',
            }.items():
                self.assertEqual(self.record_at(db, address)['assembly'], assembly)
            self.assertEqual(list(db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                             ('%-0x4528(r2)%',))),
                             [(0x105267d8,), (0x10526c14,), (0x10528c80,), (0x10528f38,),
                              (0x105291c8,), (0x10529294,), (0x10529644,)])

    def test_incoming_edges_are_constructor_call_without_interior_resumes(self):
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
        self.assertEqual(incoming, [(0x103512bc, 0x105267c0)])
        self.assertEqual(fallthroughs, [])

    def test_cross_entry_flows_are_three_owned_calls_and_one_calendar_switch(self):
        cross, computed, external = set(), [], []
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                if 'COMPUTED' in record['flow']:
                    computed.append((record['address'], record['flow']))
                targets = record['flows'] + ([record['fallthrough']]
                                             if record.get('fallthrough') is not None else [])
                for target in targets:
                    self.assertNotIn(target, PADDING | NOPS)
                    if audit.MODULE[0] <= target < audit.MODULE[1]:
                        self.assertIn(target, self.by_address)
                        if not start <= target < end:
                            self.assertIn(target, ENTRIES)
                            self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                            cross.add((record['address'], target))
                    else:
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        external.append((record['address'], target))
        self.assertEqual(cross, OWNED_CALLS)
        self.assertEqual(computed, [(0x105277d0, 'COMPUTED_JUMP')])
        self.assertEqual((len(external), len({target for _, target in external})), (327, 38))
        self.assertEqual([s for s in self.program['symbols']
                          if audit.MODULE[0] <= s['address'] < audit.MODULE[1]], [])

    def test_all_body_words_padding_and_unreachable_nops_accounted(self):
        covered = set()
        for start, end in audit.ROUTINES:
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual([r['address'] for r in records],
                             [a for a in range(start, end, 4) if a not in NOPS])
            self.assertEqual(records[0]['assembly'], 'mfspr r0,LR')
            self.assertEqual(records[-1]['assembly'], 'blr')
            covered.update(range(start, end, 4))
        self.assertEqual(set(range(*audit.MODULE, 4)) - covered, PADDING)
        for address in PADDING:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)
        for address in NOPS:
            self.assertEqual(self.word(address), 0x60000000)
            self.assertNotIn(address, self.by_address)
            predecessor = self.by_address[address - 4]
            self.assertEqual(predecessor['flow'], 'UNCONDITIONAL_JUMP')
            self.assertIsNone(predecessor.get('fallthrough'))

    def test_vectors_vtable_and_bounded_switch_close_indirect_entries(self):
        toc = 0x108bba90
        self.assertEqual(self.word(toc - 0x6024), 0x108da6cc)
        for descriptor, entry in VECTORS.items():
            self.assertEqual((self.word(descriptor), self.word(descriptor + 4)), (entry, toc))
        for offset, descriptor in ((0x08, 0x108bac7c), (0x10, 0x108bac64),
                                   (0x30, 0x108bac5c), (0x44, 0x108bac74),
                                   (0x4c, 0x108bac54), (0x50, 0x108bac4c),
                                   (0x70, 0x108bac44), (0x94, 0x108bac6c)):
            self.assertEqual(self.word(0x108da6cc + offset), descriptor)
        self.assertEqual(set(ENTRIES) - set(VECTORS.values()), {0x105267c0, 0x10528c60, 0x10529280})
        self.assertEqual(self.word(toc - 0x452c), 0x108da6b0)
        for address, target in SWITCH.items():
            self.assertEqual(self.word(address), target)
            self.assertIn(target, self.by_address)
            self.assertNotIn(target, ENTRIES)
        self.assertEqual(self.by_address[0x105277b4]['assembly'], 'subi r0,r3,0xd')
        self.assertEqual(self.by_address[0x105277b8]['assembly'], 'cmplwi r0,0x6')
        self.assertEqual(self.by_address[0x105277bc]['flows'], [0x10528040])
        self.assertEqual(self.by_address[0x105277d0]['assembly'], 'bctr')

    def test_every_byte_pointer_census_classifies_only_vectors_labels_and_data_patterns(self):
        pointers = {}
        for start, contents in self.segments:
            for match in re.finditer(b'\x10\x52', contents):
                offset = match.start()
                if offset + 4 <= len(contents):
                    target = int.from_bytes(contents[offset:offset + 4], 'big')
                    if audit.MODULE[0] <= target < audit.MODULE[1]:
                        pointers[start + offset] = target
        self.assertEqual(pointers, VECTORS | SWITCH | FALSE_POINTERS)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address in (0x105f9bd6, 0x105f9c52):
                self.assertEqual(address % 4, 2)
                self.assertTrue(self.record_at(db, address & ~3)['assembly'].startswith('addi '))
                self.assertTrue(self.record_at(db, (address + 3) & ~3)['assembly'].startswith('stw '))
        self.assertEqual(self.word(0x108bba90 - 0x7b8c), 0x108e3140)
        self.assertTrue(0x108e3140 <= 0x108e3fca < 0x108fb7e0)
        self.assertNotEqual(self.word(0x108e3fca + 4), 0x108bba90)

    def test_missing_export_and_stale_or_incomplete_reference_fail(self):
        with tempfile.TemporaryDirectory(dir=PRIVATE) as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_eng_first_reference.py'),
                                     directory, str(target)], text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(target.exists())
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing ENG_FIRST.CPP instructions'):
                audit.require_eng_first_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten())]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_eng_first_index(db)

    def test_duplicate_bad_boundary_draft_and_missing_owner_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'ENG_FIRST_ROUTINE(' in line)
        with tempfile.TemporaryDirectory(dir=PRIVATE) as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents + '\n' + first.replace('ENG_FIRST_ROUTINE(', 'ENG_FIRST_DRAFT('))
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'listed twice'):
                    audit.require_eng_first_index(db, candidate)
            candidate.write_text(contents.replace('0x10526aec, R3', '0x10526af0, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            for changed in (contents.replace('ENG_FIRST_ROUTINE(', 'ENG_FIRST_DRAFT(', 1),
                            contents.replace(first, '')):
                candidate.write_text(changed)
                with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                    with self.assertRaisesRegex(ValueError, 'all eleven complete'):
                        audit.require_eng_first_index(db, candidate)
                with self.assertRaises(AssertionError):
                    self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
        missing = [(entry, owner) for entry, owner in OWNER_REPLACEMENTS if entry != ENTRIES[0]]
        with self.assertRaises(AssertionError):
            self.assert_production(audit.rewritten(), missing)

    def test_generated_default_hooks_precede_original_steps_with_negative_controls(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_eng_first_hooks(page, part, normal)
            self.assertIn('#include "eng_first_adapter.hpp"', normal)
            original = generate_page(page, part, selected, userops=self.program['userops'], eng_first_hooks=False)
            self.assertNotIn('try_modern_eng_first', original)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                hook, step = audit.eng_first_hook(entry), f's.step(0x{entry:x});'
                fragment = normal.split(f'i_{entry:x}:', 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
                self.assertLess(fragment.index(hook), fragment.index(step))
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_eng_first_hooks(page, part, broken)

    def test_hook_free_reference_retains_every_original_label(self):
        with tempfile.TemporaryDirectory(dir=PRIVATE) as directory:
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_eng_first_reference.py'),
                                     str(EXPORT), str(target)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            code = target.read_text()
        self.assertNotIn('try_modern_', code)
        self.assertNotIn('eng_first::', code)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE),
                         [f'{r["address"]:x}' for r in self.records])

    def test_internal_calls_preserve_checker_parent_selection_and_original_override(self):
        code = (ROOT / 'native/eng_first_adapter.cpp').read_text()
        direct = code.split('namespace eng_first {', 1)[1]
        self.assertIn('declining || (routine_checker &&', direct)
        self.assertIn('!routine_check_child() || !routine_checker->wants(entry)', direct)
        self.assertIn('c.call(entry, back)', direct)
        self.assertIn('c.link(back)', direct)
        self.assertIn('routine->hook(c)', direct)
        self.assertLess(direct.index('c.link(back)'), direct.index('routine->hook(c)'))
        self.assertIn('CM_ENG_FIRST_ORIGINAL', code)
        for path in (ROOT / 'native').glob('eng_first*.cpp'):
            if path.name.endswith('_checks.cpp'):
                continue
            self.assertNotIn('SemanticGuestCall', path.read_text())


if __name__ == '__main__':
    unittest.main()
