"""Certify UTILS.CPP's entries, original boundaries and production owner.

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

import utils_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x10745e00, 0x10745fc0, 112, '5090374b83cbc463'),
    (0x10745fc0, 0x107460b4, 61, 'ad92a48c3975c2b8'),
    (0x107460c0, 0x10746278, 110, '062d5719a9b0ad4c'),
    (0x10746280, 0x10746300, 32, '785e3b9bc794036e'),
    (0x10746300, 0x1074633c, 15, '9ace0a8ab2f5504f'),
    (0x10746340, 0x1074646c, 75, '78fdde694e830206'),
    (0x10746470, 0x107464d0, 24, '0eb192122bdbfe03'),
    (0x107464d0, 0x107465a0, 52, '8d7735c35d03316b'),
    (0x107465a0, 0x107465ec, 19, '66ffc86d937c9b11'),
    (0x107465f0, 0x10746760, 91, '9f25d129a6830115'),
    (0x10746760, 0x10746780, 8, '5ed5f809c63beaa1'),
    (0x10746780, 0x107467a0, 8, '940f7892825cc1fa'),
    (0x107467a0, 0x10746a2c, 163, '37034241c6ffa6fd'),
    (0x10746a30, 0x10746a70, 16, 'b57f7612827c56d0'),
    (0x10746a70, 0x10746e80, 260, '1f68b0fc070ae3dd'),
    (0x10746e80, 0x10746f78, 60, '31541ef9ea7479ee'),
    (0x10746f80, 0x10747074, 60, '2bf1d90077652a5e'),
    (0x10747080, 0x107470ac, 11, '0697b370fcdd9ab3'),
    (0x107470b0, 0x107471cc, 71, '599234d9b7991d90'),
    (0x107471d0, 0x1074725c, 35, '4fc64548751ddd36'),
    (0x10747260, 0x107473b8, 86, '8b108e6eb60549db'),
    (0x107473c0, 0x10747494, 52, 'ea074f570319158d'),
    (0x107474a0, 0x107475ac, 67, 'ce904fdfb25f92e6'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
RNG_ENTRY = 0x107474a0
HOOKED = tuple(a for a in ENTRIES if a != RNG_ENTRY)
PADDING = {0x107460b4, 0x107460b8, 0x107460bc, 0x10746278, 0x1074627c, 0x1074633c, 0x1074646c, 0x107465ec,
           0x10746a2c, 0x10746f78, 0x10746f7c, 0x10747074, 0x10747078, 0x1074707c, 0x107470ac, 0x107471cc,
           0x1074725c, 0x107473b8, 0x107473bc, 0x10747494, 0x10747498, 0x1074749c, 0x107475ac}
UNREACHABLE_NOPS = {0x107466f8: 'b 0x1074674c', 0x10746f04: 'b 0x10746f5c', 0x10746f34: 'b 0x10746f5c',
                    0x10746fd4: 'b 0x10746fe4', 0x1074740c: 'b 0x1074743c'}
OWNED_CALLS = {(0x107462a4, 0x107460c0), (0x107462b8, 0x107473c0), (0x10747294, 0x107460c0), (0x107472a4, 0x107473c0)}
INCOMING_COUNTS = {0x10745e00: 25, 0x10745fc0: 2, 0x107460c0: 1, 0x10746280: 249, 0x10746300: 13, 0x10746340: 2,
                   0x10746470: 22, 0x107464d0: 28, 0x107465a0: 20, 0x107465f0: 87, 0x10746760: 11, 0x10746780: 2,
                   0x107467a0: 44, 0x10746a30: 30, 0x10746a70: 28, 0x10746e80: 2, 0x10746f80: 15, 0x10747080: 2,
                   0x107470b0: 2, 0x107471d0: 1, 0x10747260: 4, 0x107473c0: 0, 0x107474a0: 4}
# Jump tables: (address, TOC slot, labels by index).
LOWER_TABLE = (0x108e3080, -0x3e44, [0x10746b80 + 0x10 * i for i in range(21)])
UPPER_TABLE = (0x108e302c, -0x3e48, [0x10746d00 + 0x10 * i for i in range(21)])
LINE_TABLE = (0x108e30d4, -0x3e4c, [{9: 0x10747014, 10: 0x10746ffc, 13: 0x10747014, 32: 0x10747014, 35: 0x10746fd0}
                                    .get(c, 0x1074701c) for c in range(9, 36)])
COMPUTED = {0x10746b7c, 0x10746cfc, 0x10746fcc}
STRINGS = {0x0: b'UTILS.CPP', 0xa: b'v%s %s.%s %d', 0x17: b'Error', 0x1d: b'-ignoreenv', 0x28: b'SI_DATA', 0x30: b'data',
           0x35: b'CM3_DATA', 0x3e: b'CM3_TEMP', 0x47: b'temp', 0x4c: b'CM3_DEBUG', 0x56: b'debug', 0x5c: b'CM3_PREFS',
           0x66: b'prefs', 0x6c: b'CM3_TACTICS', 0x78: b'CM3_TRAINING', 0x85: b'training', 0x8e: b'CM3_HISTORY',
           0x9a: b'history', 0xa2: b'CM3_PICTURES', 0xaf: b'pictures', 0xb8: b'CM3_SOUNDS', 0xc3: b'sounds',
           0xca: b'CM3_HELP', 0xd3: b'help', 0xd8: b'CM3_QSTART', 0xe3: b'quickstarts', 0xef: b'', 0xf0: b'1er', 0xf4: b'%d',
           0xf7: b'1\xe8re', 0xfc: b'%d\xe8me', 0x102: b'<%d - number>th', 0x112: b'<%d - number>st',
           0x122: b'<%d - number>nd', 0x132: b'<%d - number>rd', 0x142: b'zero', 0x1c4: b'twenty', 0x1cb: b'Zero',
           0x24d: b'Twenty', 0x254: b'rb', 0x257: b'wb', 0x25a: b'', 0x25b: b'Random Number seed: %d'}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private UTILS export unavailable; inventory cannot be certified')


class UtilsAuditTests(unittest.TestCase):
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
        for entry in HOOKED:
            self.assertEqual([owner for at, owner in owners if at == entry], ['utils_adapter'])
        self.assertEqual([owner for at, owner in owners if at == RNG_ENTRY], ['rng_core_adapter'])

    def assert_hook(self, code, entry):
        hook = audit.utils_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_exact_entries_original_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x10745e00, 0x107475b0))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.UTILS_ENTRIES, ENTRIES)
        self.assertEqual(audit.UTILS_HOOKED_ENTRIES, HOOKED)
        self.assertEqual(audit.UNREACHABLE_NOPS, frozenset(UNREACHABLE_NOPS))
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(self.records), 1488)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        self.assert_production(audit.rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_utils_index(db)

    def test_rng_initializer_runs_this_module_and_keeps_no_register_copy(self):
        adapter = (ROOT / 'native/rng_core_adapter.cpp').read_text()
        self.assertIn('utils::initialize_random(c, c.r(3));', adapter)
        self.assertIn('(index == 2 && utils_declining())', adapter)
        self.assertNotIn('initialize_pool', adapter)
        from rng_core_audit import RNG_CORE_ENTRIES
        self.assertIn(RNG_ENTRY, RNG_CORE_ENTRIES)

    def test_callers_and_callees_read_no_register_the_cpp_leaves_different(self):
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            self.assertEqual(audit.volatile_reads(db), [])

    def test_usable_paths_handle_stubs_cannot_return_null(self):
        from compile_recomp import OWNER_REPLACEMENTS
        self.assertEqual((self.word(0x10739870), self.word(0x10739874)), (0x3860001e, 0x4e800020))
        self.assertEqual(self.word(0x10739880), 0x4e800020)
        self.assertFalse({0x10739870, 0x10739880} & {entry for entry, _ in OWNER_REPLACEMENTS})
        self.assertEqual(self.by_address[0x10746438]['assembly'], 'beq 0x10746448')

    def test_source_pool_tables_and_neighbours_close_the_boundaries(self):
        pool = self.word(TOC - 0x3e34)
        self.assertEqual(pool, 0x1089f770)
        for offset, text in STRINGS.items():
            self.assertEqual(self.original_bytes(pool + offset, len(text) + 1), text + b'\0')
        # The next source's pool follows, as ZIPDIR.CPP's code follows this module's.
        self.assertEqual(self.word(TOC - 0x3e30), 0x1089f9e4)
        self.assertEqual(self.original_bytes(0x1089f9e4, 11), b'ZIPDIR.CPP\0')
        self.assertEqual(self.original_bytes(pool - 4, 4), b'udp\0')
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            readers = [row[0] for row in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                    ('%,-0x3e34(r2)%',))]
            zipdir = [row[0] for row in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                   ('%,-0x3e30(r2)%',))]
            before = json.loads(db.execute('SELECT body FROM instructions WHERE address=?', (0x10745df0,)).fetchone()[0])
            first = json.loads(db.execute('SELECT body FROM instructions WHERE address=?', (0x107475b0,)).fetchone()[0])
        self.assertEqual(readers[0], 0x10745e0c)
        self.assertEqual(readers[-1], 0x107474e4)
        self.assertTrue(all(audit.MODULE[0] <= at < audit.MODULE[1] for at in readers))
        self.assertEqual(zipdir[0], 0x107475c8)
        self.assertEqual(before['assembly'], 'blr')
        self.assertEqual(first['assembly'], 'mfspr r0,LR')
        # Every routine between the first and last pool reader is in the module's
        # span: none reads another source's pool slot.
        for start, end in audit.ROUTINES:
            for record in self.records:
                if start <= record['address'] < end:
                    self.assertNotIn(',-0x3e30(r2)', record['assembly'])
                    self.assertNotIn(',-0x3e2c(r2)', record['assembly'])

    def test_jump_tables_cover_every_computed_jump(self):
        for address, slot, labels in (LOWER_TABLE, UPPER_TABLE, LINE_TABLE):
            self.assertEqual(self.word(TOC + slot), address)
            self.assertEqual([self.word(address + i * 4) for i in range(len(labels))], labels)
            for label in labels:
                self.assertIn(label, self.by_address)
        self.assertEqual({r['address'] for r in self.records if 'COMPUTED' in r['flow']}, COMPUTED)

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
                if 'COMPUTED' in record['flow']:
                    self.assertIn(record['address'], COMPUTED)
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

    def test_raw_pointer_census_finds_only_the_jump_tables(self):
        pointers = {}
        for start, contents in self.segments:
            for prefix in (b'\x10\x74\x5e', b'\x10\x74\x5f', b'\x10\x74\x60', b'\x10\x74\x61', b'\x10\x74\x62', b'\x10\x74\x63',
                           b'\x10\x74\x64', b'\x10\x74\x65', b'\x10\x74\x66', b'\x10\x74\x67', b'\x10\x74\x68', b'\x10\x74\x69',
                           b'\x10\x74\x6a', b'\x10\x74\x6b', b'\x10\x74\x6c', b'\x10\x74\x6d', b'\x10\x74\x6e', b'\x10\x74\x6f',
                           b'\x10\x74\x70', b'\x10\x74\x71', b'\x10\x74\x72', b'\x10\x74\x73', b'\x10\x74\x74', b'\x10\x74\x75'):
                for match in re.finditer(re.escape(prefix), contents):
                    offset = match.start()
                    if offset + 4 > len(contents):
                        continue
                    target = int.from_bytes(contents[offset:offset + 4], 'big')
                    if audit.MODULE[0] <= target < audit.MODULE[1]:
                        pointers[start + offset] = target
        expected = {}
        for address, _, labels in (LOWER_TABLE, UPPER_TABLE, LINE_TABLE):
            expected.update({address + i * 4: label for i, label in enumerate(labels)})
        self.assertEqual({at: to for at, to in pointers.items() if at % 4 == 0}, expected)

    def test_missing_export_and_stale_digest_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_utils_reference.py'),
                                     directory, str(Path(directory) / 'reference.cpp')],
                                    text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((Path(directory) / 'reference.cpp').exists())
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing UTILS.CPP instructions'):
                audit.require_utils_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten(drafts=True))]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_utils_index(db)

    def test_duplicate_bad_boundary_draft_and_missing_owner_are_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'UTILS_ROUTINE(' in line)
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('0x10745fc0, R3', '0x10745fc4, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('UTILS_ROUTINE(', 'UTILS_DRAFT(', 1))
            self.assertEqual(len(audit.rewritten(candidate, drafts=True)), 23)
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all twenty-three complete'):
                    audit.require_utils_index(db, candidate)
            candidate.write_text(contents.replace(first, ''))
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all twenty-three complete'):
                    audit.require_utils_index(db, candidate)
        missing = [(at, owner) for at, owner in OWNER_REPLACEMENTS if at != ENTRIES[0]]
        with self.assertRaises(AssertionError):
            self.assert_production(audit.rewritten(), missing)

    def test_normal_hooks_precede_original_steps_and_detect_disabled_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_utils_hooks(page, part, normal)
            self.assertIn('#include "utils_adapter.hpp"', normal)
            for entry in HOOKED:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.utils_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_utils_hooks(page, part, broken)
            if page == RNG_ENTRY >> 12:
                fragment = normal.split(f'i_{RNG_ENTRY:x}:', 1)[-1].split(f'i_{RNG_ENTRY + 4:x}:', 1)[0]
                self.assertIn(f'try_modern_rng_core(s, 0x{RNG_ENTRY:x})', fragment)

    def test_generated_original_reference_has_every_label_and_no_rewrite(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_utils_reference.py'),
                                     str(EXPORT), str(target)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            code = target.read_text()
        self.assertNotIn('try_modern_', code)
        self.assertNotIn('utils::', code)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE),
                         [f'{r["address"]:x}' for r in self.records])


if __name__ == '__main__':
    unittest.main()
