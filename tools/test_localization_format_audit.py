"""Complete phrase formatting contracts and prepared-component boundaries."""
from contextlib import closing
from copy import deepcopy
import json
from pathlib import Path
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS
from localization_format_audit import (
    CONTRACTS, FRAGMENTS, LOCALIZATION_FORMAT_ENTRIES, TABLES, PADDING, WORDS,
    fingerprint, load_localization_format_tables, require_localization_format_bodies,
    require_localization_format_contract, require_localization_format_edges,
    require_localization_format_index, require_localization_format_padding,
    require_localization_format_tables, require_localization_format_words,
)
from make_localization_format_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_complete_scope_without_productive_ownership(self):
        self.assertEqual(tuple(row[2] for row in CONTRACTS), (730,))
        self.assertEqual(sum(row[2] for row in FRAGMENTS), 730)
        self.assertEqual(len(PADDING), 2)
        self.assertEqual(len(TABLES), 5)
        self.assertEqual(len(WORDS), 2)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 901)
        self.assertFalse(set(LOCALIZATION_FORMAT_ENTRIES) & {entry for entry, _ in SEMANTIC_REPLACEMENTS})
        self.assertNotIn(0x10728400, LOCALIZATION_FORMAT_ENTRIES)
        self.assertNotIn(0x10728ac0, LOCALIZATION_FORMAT_ENTRIES)

    def test_named_formatting_phases_without_original_execution(self):
        source = (ROOT / 'native/localization_format.cpp').read_text()
        for name in ('format_phrase', 'collect_bindings', 'split_arguments', 'render_translation',
                     'prepare_fallback', 'rewrite_fallback_markers', 'render_fallback'):
            self.assertIn(name, source)
        for token in ('dispatch_page', 'run_until', 'switch (s.pc)', 'goto ', '_reference(s)',
                      'pcode', 'while(s.pc', 'memory.clear', 'memset'):
            self.assertNotIn(token, source)
        cmake = (ROOT / 'native/CMakeLists.txt').read_text()
        self.assertIn('add_executable(cm-localization-format-checks localization_format.cpp', cmake)
        self.assertNotIn('target_sources(cm_aot PRIVATE localization_format', cmake)

    def test_external_interior_entrances_fail_closed(self):
        for start, end, *_ in CONTRACTS:
            require_localization_format_edges([(0, start), (start, end-4), (end-4, end)])
            for target in {start+4, end-4} - {start}:
                with self.assertRaises(ValueError):
                    require_localization_format_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_localization_format_index(db)
            cls.records = [json.loads(row[0]) for start, end, *_ in CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        program = json.loads((INPUT / 'program.json').read_text())
        cls.userops = program['userops']
        cls.segments = [(s['address'], (INPUT/s['file']).read_bytes() if s.get('file') else bytes(s['size']))
                        for s in program['segments']]

    @classmethod
    def read(cls, address, size):
        for base, data in cls.segments:
            if base <= address and address+size <= base+len(data):
                return data[address-base:address-base+size]
        raise ValueError('Missing original localization data')

    def mutated(self, address):
        def changed(a, n):
            data = bytearray(self.read(a, n))
            if a <= address < a+n:
                data[address-a] ^= 1
            return bytes(data)
        return changed

    def test_original_table_pointers_and_payload_boundaries_are_pinned(self):
        require_localization_format_tables(self.read)
        self.assertEqual(len(load_localization_format_tables(INPUT)), 7)
        for slot, base, size, _ in TABLES:
            for address in (*range(slot, slot+4), base, base+size//2, base+size-1):
                with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                    require_localization_format_tables(self.mutated(address))

    def test_live_input_defaults_and_nonindexed_alignment_are_pinned(self):
        require_localization_format_padding(self.read)
        require_localization_format_words(self.read)
        self.assertFalse({a for a, _ in PADDING} & {r['address'] for r in self.records})
        for address, *_ in PADDING:
            with self.assertRaises(ValueError):
                require_localization_format_padding(self.mutated(address))
        for address, size, _ in WORDS:
            for byte in range(size):
                with self.assertRaises(ValueError):
                    require_localization_format_words(self.mutated(address+byte))

    def test_independent_reference_has_all_730_sites_and_no_productive_hooks(self):
        source = render_reference(self.records, self.userops)
        self.assertNotIn('try_modern_', source)
        self.assertNotIn('_impl<true,', source)
        for entry in LOCALIZATION_FORMAT_ENTRIES:
            self.assertIn(f'case 0x{entry:x}:', source)
        for row in self.records:
            self.assertIn(f's.step(0x{row["address"]:x});', source)

    def test_modified_instruction_contract_fields_fail(self):
        for start, *_ in CONTRACTS:
            i = next(i for i, row in enumerate(self.records) if row['address'] == start)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records)
                changed[i][key] = value
                with self.subTest(start=hex(start), key=key), self.assertRaises(ValueError):
                    require_localization_format_bodies(changed, complete=True)

    def test_missing_duplicate_reordered_and_absent_bodies_fail(self):
        for start, end, *_ in CONTRACTS:
            i = next(i for i, row in enumerate(self.records) if row['address'] == start)
            for changed in (self.records[:i]+self.records[i+1:], self.records+[self.records[i]],
                            list(reversed(self.records)), [row for row in self.records if not start <= row['address'] < end]):
                with self.assertRaises(ValueError):
                    require_localization_format_bodies(changed, complete=True)

    def test_every_fragment_is_independently_pinned(self):
        for start, page, count, expected in FRAGMENTS:
            end = next(end for root, end, *_ in CONTRACTS if root == start)
            rows = [r for r in self.records if start <= r['address'] < end and r['address'] >> 12 == page]
            self.assertEqual((len(rows), fingerprint(rows)), (count, expected))
            page_rows = [r for r in self.records if r['address'] >> 12 == page]
            require_localization_format_contract(page, page_rows, LOCALIZATION_FORMAT_ENTRIES)
            with self.assertRaises(ValueError):
                require_localization_format_contract(page, [r for r in page_rows if r['address'] != rows[0]['address']], LOCALIZATION_FORMAT_ENTRIES)


if __name__ == '__main__':
    unittest.main()
