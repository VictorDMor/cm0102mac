"""Complete localization lookup contracts and productive ownership boundaries."""
import inspect
import hashlib
from compile_recomp import generate_page
from contextlib import closing
from copy import deepcopy
import json
from pathlib import Path
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, require_unique_semantic_replacements
from localization_lookup_audit import (
    CONTRACTS, FRAGMENTS, LOCALIZATION_LOOKUP_ENTRIES, TABLES, PADDING, WORDS,
    fingerprint, load_localization_lookup_tables, require_localization_lookup_bodies,
    require_localization_lookup_contract, require_localization_lookup_edges,
    require_localization_lookup_index, require_localization_lookup_padding,
    require_localization_lookup_tables, require_localization_lookup_words, require_localization_lookup_hooks, localization_lookup_hook,
)
from make_localization_lookup_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_complete_scope_and_exact901_prefix_with_four_owners(self):
        self.assertEqual(tuple(row[2] for row in CONTRACTS), (7, 2, 432))
        self.assertEqual(sum(row[2] for row in FRAGMENTS), 441)
        self.assertEqual(len(PADDING), 3)
        self.assertEqual(len(TABLES), 7)
        self.assertEqual(len(WORDS), 6)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 953)
        prefix = [{'entry': hex(e), 'module': m} for e, m in SEMANTIC_REPLACEMENTS[:901]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
                         'ad5345c3598b6b7d56d4ccba2e74d516c274d8c25a08a2affa059c24ad7728db')
        from localization_lookup_audit import LOCALIZATION_LOOKUP_ENTRIES
        from localization_format_audit import LOCALIZATION_FORMAT_ENTRIES
        expected = tuple((e, module+'_adapter') for module, entries in (
            ('localization_lookup',LOCALIZATION_LOOKUP_ENTRIES),('localization_format',LOCALIZATION_FORMAT_ENTRIES)) for e in entries)
        self.assertEqual(SEMANTIC_REPLACEMENTS[901:905], expected)
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry, _ in expected:
            with self.assertRaises(ValueError):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(entry,'duplicate')))
        self.assertNotIn(0x10727890, LOCALIZATION_LOOKUP_ENTRIES)
        self.assertNotIn(0x10728ac0, LOCALIZATION_LOOKUP_ENTRIES)

    def test_named_lookup_phases_without_original_execution(self):
        source = (ROOT / 'native/localization_lookup.cpp').read_text()
        for name in ('initialize_record', 'current_language', 'lookup',
                     'normalize_key', 'find_cached_record', 'find_file_record'):
            self.assertIn(name, source)
        for token in ('dispatch_page', 'run_until', 'switch (s.pc)', 'goto ', '_reference(s)',
                      'pcode', 'while(s.pc', 'memory.clear', 'memset'):
            self.assertNotIn(token, source)
        cmake = (ROOT / 'native/CMakeLists.txt').read_text()
        self.assertIn('add_executable(cm-localization-lookup-checks localization_lookup_checks.cpp', cmake)
        self.assertIn('localization_lookup_adapter.cpp', cmake)

    def test_external_interior_entrances_fail_closed(self):
        for start, end, *_ in CONTRACTS:
            require_localization_lookup_edges([(0, start), (start, end-4), (end-4, end)])
            for target in {start+4, end-4} - {start}:
                with self.assertRaises(ValueError):
                    require_localization_lookup_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_localization_lookup_index(db)
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
        require_localization_lookup_tables(self.read)
        self.assertEqual(len(load_localization_lookup_tables(INPUT)), 13)
        for slot, base, size, _ in TABLES:
            for address in (*range(slot, slot+4), base, base+size//2, base+size-1):
                with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                    require_localization_lookup_tables(self.mutated(address))

    def test_live_input_defaults_and_nonindexed_alignment_are_pinned(self):
        require_localization_lookup_padding(self.read)
        require_localization_lookup_words(self.read)
        self.assertFalse({a for a, _ in PADDING} & {r['address'] for r in self.records})
        for address, *_ in PADDING:
            with self.assertRaises(ValueError):
                require_localization_lookup_padding(self.mutated(address))
        for address, size, _ in WORDS:
            for byte in range(size):
                with self.assertRaises(ValueError):
                    require_localization_lookup_words(self.mutated(address+byte))

    def test_productive_hooks_precede_original_accounting(self):
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['localization_lookup_hooks']=True
        for page in sorted({f[1] for f in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page]
            source=generate_page(page,rows,{r['address'] for r in self.records},userops=self.userops,**disabled)
            require_localization_lookup_hooks(page,rows,source)
            for entry in LOCALIZATION_LOOKUP_ENTRIES:
                if entry>>12!=page:continue
                hook=localization_lookup_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_localization_lookup_hooks(page,rows,changed)

    def test_independent_reference_has_all_441_sites_and_no_productive_hooks(self):
        source = render_reference(self.records, self.userops)
        self.assertNotIn('try_modern_', source)
        self.assertNotIn('_impl<true,', source)
        for entry in LOCALIZATION_LOOKUP_ENTRIES:
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
                    require_localization_lookup_bodies(changed, complete=True)

    def test_missing_duplicate_reordered_and_absent_bodies_fail(self):
        for start, end, *_ in CONTRACTS:
            i = next(i for i, row in enumerate(self.records) if row['address'] == start)
            for changed in (self.records[:i]+self.records[i+1:], self.records+[self.records[i]],
                            list(reversed(self.records)), [row for row in self.records if not start <= row['address'] < end]):
                with self.assertRaises(ValueError):
                    require_localization_lookup_bodies(changed, complete=True)

    def test_every_fragment_is_independently_pinned(self):
        for start, page, count, expected in FRAGMENTS:
            end = next(end for root, end, *_ in CONTRACTS if root == start)
            rows = [r for r in self.records if start <= r['address'] < end and r['address'] >> 12 == page]
            self.assertEqual((len(rows), fingerprint(rows)), (count, expected))
            page_rows = [r for r in self.records if r['address'] >> 12 == page]
            require_localization_lookup_contract(page, page_rows, LOCALIZATION_LOOKUP_ENTRIES)
            with self.assertRaises(ValueError):
                require_localization_lookup_contract(page, [r for r in page_rows if r['address'] != rows[0]['address']], LOCALIZATION_LOOKUP_ENTRIES)


if __name__ == '__main__':
    unittest.main()
