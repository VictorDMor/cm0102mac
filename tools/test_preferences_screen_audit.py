"""Complete preferences presentation builder contracts, with productive hooks."""
import inspect
from compile_recomp import generate_page
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, require_unique_semantic_replacements
from preferences_actions_audit import PREFERENCES_ACTIONS_ENTRIES
from preferences_screen_audit import (
    CONTRACTS, FRAGMENTS, PREFERENCES_SCREEN_ENTRIES, TABLES, LANGUAGE_NAMES, WORDS, PADDING,
    load_preferences_screen_tables, require_preferences_screen_tables, require_preferences_screen_words,
    require_preferences_screen_padding, require_preferences_screen_bodies,
    require_preferences_screen_contract, require_preferences_screen_edges, require_preferences_screen_index, require_preferences_screen_hooks, preferences_screen_hook)
from make_preferences_screen_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_exact_897_prefix_and_four_productive_ui_owners(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 949)
        manifest = [{'entry': hex(e), 'module': m} for e, m in SEMANTIC_REPLACEMENTS[:897]]
        self.assertEqual(hashlib.sha256(json.dumps(manifest, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '0977c8a7fdc1b5f94cc74d9a6e641cc62f466e49d1ae3335a7c85ed81d05a8aa')
        from preferences_actions_audit import PREFERENCES_ACTIONS_ENTRIES
        from preferences_screen_audit import PREFERENCES_SCREEN_ENTRIES
        expected = tuple((e, module+'_adapter') for module, entries in (
            ('preferences_actions', PREFERENCES_ACTIONS_ENTRIES), ('preferences_screen', PREFERENCES_SCREEN_ENTRIES)) for e in entries)
        self.assertEqual(SEMANTIC_REPLACEMENTS[897:901], expected)
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry, _ in expected:
            with self.assertRaises(ValueError):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS, (entry, 'duplicate')))

    def test_whole_builder_scope_and_reject_external_interior_entries(self):
        self.assertEqual(tuple(row[2] for row in CONTRACTS), (2562,))
        self.assertEqual(tuple(row[2] for row in FRAGMENTS), (1010, 1024, 528))
        self.assertEqual(PREFERENCES_SCREEN_ENTRIES, (0x1033d030,))
        self.assertEqual(len(TABLES), 14)
        self.assertEqual(len(LANGUAGE_NAMES), 11)
        self.assertEqual(tuple(size for _, size, _ in WORDS), (1, 2, 2))
        self.assertEqual(tuple(word for _, word in PADDING), ('60000000', '60000000'))
        start, end, *_ = CONTRACTS[0]
        require_preferences_screen_edges([(0, start), (start, end-4), (end-4, end)])
        for target in (start+4, end-4, 0x1033e000, 0x1033f000):
            with self.assertRaises(ValueError):
                require_preferences_screen_edges([(0, target)])

    def test_named_stages_and_structured_loops_without_original_replay(self):
        source = ''.join((ROOT/'native'/name).read_text() for name in (
            'preferences_screen.cpp', 'preferences_screen_lists.cpp', 'preferences_screen_options.cpp'))
        for token in ('dispatch_page', 'run_until', 'switch (s.pc)', 'goto ', '_reference(s)',
                      'pcode', 'while(s.pc', 'memory.clear', 'memset'):
            self.assertNotIn(token, source)
        for name in ('initialize_resources', 'capture_session', 'create_settings_frame', 'create_options_grid',
                     'build_language_selector', 'build_currency_selector', 'build_font_selector',
                     'build_wage_choices', 'build_screen_choices', 'build_background_change_choices',
                     'build_sound_choices', 'build_database_choices', 'build_match_storage_choices',
                     'build_autosave_choices', 'build_compression_choices', 'build_safe_save_choices',
                     'build_flashing_choices', 'build_background_match_choices', 'build_print_choices',
                     'return_to_caller', 'count_languages', 'append_language_rows', 'append_currency_rows',
                     'append_video_rows', 'append_inactive_choice'):
            self.assertIn(name, source)
        self.assertEqual(source.count('while(true)'), 4)
        self.assertIn('preferences_screen_adapter.cpp', (ROOT/'native/CMakeLists.txt').read_text())


@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_preferences_screen_index(db)
            start, end, *_ = CONTRACTS[0]
            cls.records = [json.loads(row[0]) for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        program = json.loads((INPUT/'program.json').read_text())
        cls.userops = program['userops']
        cls.segments = [(s['address'], (INPUT/s['file']).read_bytes() if s.get('file') else bytes(s['size']))
                        for s in program['segments']]

    @classmethod
    def read(cls, address, size):
        for base, data in cls.segments:
            if base <= address and address+size <= base+len(data):
                return data[address-base:address-base+size]
        raise ValueError('Missing original presentation data')

    def test_every_table_pointer_bank_and_language_name_byte_is_pinned(self):
        require_preferences_screen_tables(self.read)
        self.assertEqual(len(load_preferences_screen_tables(INPUT)), 28)
        regions = [(slot, 4) for slot, _, _, _ in TABLES]
        regions += [(base, size) for _, base, size, _ in TABLES]
        regions += [(address, size) for address, size, _ in LANGUAGE_NAMES]
        for base, size in regions:
            for address in range(base, base+size):
                def changed(a, n):
                    value = bytearray(self.read(a, n))
                    if a <= address < a+n:
                        value[address-a] ^= 1
                    return bytes(value)
                with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                    require_preferences_screen_tables(changed)

    def test_lazy_color_initial_words_are_pinned(self):
        require_preferences_screen_words(self.read)
        for base, size, _ in WORDS:
            for address in range(base, base+size):
                def changed(a, n):
                    value = bytearray(self.read(a, n))
                    if a <= address < a+n:
                        value[address-a] ^= 1
                    return bytes(value)
                with self.assertRaises(ValueError):
                    require_preferences_screen_words(changed)

    def test_nonindexed_loop_alignment_is_pinned(self):
        require_preferences_screen_padding(self.read)
        self.assertFalse({a for a, _ in PADDING} & {row['address'] for row in self.records})
        for address, _ in PADDING:
            def changed(a, n):
                value = bytearray(self.read(a, n))
                if a <= address < a+n:
                    value[address-a] ^= 1
                return bytes(value)
            with self.assertRaises(ValueError):
                require_preferences_screen_padding(changed)

    def test_productive_hooks_precede_original_accounting(self):
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['preferences_screen_hooks']=True
        for page in sorted({f[1] for f in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page]
            source=generate_page(page,rows,{r['address'] for r in self.records},userops=self.userops,**disabled)
            require_preferences_screen_hooks(page,rows,source)
            for entry in PREFERENCES_SCREEN_ENTRIES:
                if entry>>12!=page:continue
                hook=preferences_screen_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_preferences_screen_hooks(page,rows,changed)

    def test_complete_reference_keeps_all_sites_and_disables_every_hook(self):
        source = render_reference(self.records, self.userops)
        self.assertNotIn('try_modern_', source)
        self.assertNotIn('_impl<true,', source)
        self.assertIn('case 0x1033d030:', source)
        for row in self.records:
            self.assertIn(f's.step(0x{row["address"]:x});', source)

    def test_changed_instruction_fields_fail_in_every_fragment(self):
        for _, page, *_ in FRAGMENTS:
            i = next(i for i, row in enumerate(self.records) if row['address'] >> 12 == page)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records)
                changed[i][key] = value
                with self.subTest(page=hex(page), key=key), self.assertRaises(ValueError):
                    require_preferences_screen_bodies(changed, complete=True)

    def test_missing_duplicate_reordered_or_absent_source_fails(self):
        for changed in (self.records[1:], self.records+[self.records[0]], list(reversed(self.records)), []):
            with self.assertRaises(ValueError):
                render_reference(changed, self.userops)

    def test_partial_page_omissions_fail(self):
        for start, page, *_ in FRAGMENTS:
            rows = [row for row in self.records if row['address'] >> 12 == page]
            require_preferences_screen_contract(page, rows, {row['address'] for row in self.records})
            with self.assertRaises(ValueError):
                require_preferences_screen_contract(page, rows[1:], {start})


if __name__ == '__main__':
    unittest.main()
