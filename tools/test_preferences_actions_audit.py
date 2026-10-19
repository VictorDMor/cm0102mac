"""Whole preference-session opening/action contracts, with productive hooks."""
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
from preferences_actions_audit import (
    CONTRACTS, FRAGMENTS, PREFERENCES_ACTIONS_ENTRIES, TABLES, PADDING,
    load_preferences_actions_tables, require_preferences_actions_tables,
    require_preferences_actions_padding, require_preferences_actions_bodies,
    require_preferences_actions_contract, require_preferences_actions_edges,
    require_preferences_actions_index, fingerprint, require_preferences_actions_hooks, preferences_actions_hook)
from make_preferences_actions_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_exact_897_prefix_and_four_productive_ui_owners(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 909)
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

    def test_whole_session_action_scope_and_interior_edge_rejection(self):
        self.assertEqual(tuple(row[2] for row in CONTRACTS), (41, 66, 264))
        self.assertEqual(sum(row[2] for row in FRAGMENTS), 371)
        self.assertEqual(len(PADDING), 5)
        self.assertEqual(tuple(row[2] for row in TABLES), (4, 4, 4, 8, 8))
        self.assertNotIn(0x1033d030, PREFERENCES_ACTIONS_ENTRIES)
        self.assertNotIn(0x1033fc60, PREFERENCES_ACTIONS_ENTRIES)
        for start, end, *_ in CONTRACTS:
            require_preferences_actions_edges([(0, start), (start, end-4), (end-4, end)])
            for target in {start+4, end-4} - {start}:
                with self.assertRaises(ValueError):
                    require_preferences_actions_edges([(0, target)])

    def test_named_session_and_action_stages_without_replay(self):
        source = ''.join((ROOT / 'native' / name).read_text() for name in (
            'preferences_actions_opening.cpp', 'preferences_actions_dispatch.cpp'))
        for token in ('dispatch_page', 'run_until', 'switch (s.pc)', 'goto ', '_reference(s)',
                      'pcode', 'while(s.pc', 'memory.clear', 'memset'):
            self.assertNotIn(token, source)
        for token in ('open_with_mode', 'open_from_game_state', 'dispatch_action',
                      'select_action', 'action_is', 'event_value'):
            self.assertIn(token, source)
        self.assertIn('preferences_actions_adapter.cpp', (ROOT / 'native/CMakeLists.txt').read_text())


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_preferences_actions_index(db)
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
        raise ValueError('Missing original preference action data')

    def test_every_callback_pointer_and_initial_cell_byte_is_pinned(self):
        require_preferences_actions_tables(self.read)
        self.assertEqual(len(load_preferences_actions_tables(INPUT)), 5)
        for slot, base, size, _ in TABLES:
            for address in [*range(slot, slot+4), *range(base, base+size)]:
                def changed(a, n):
                    data = bytearray(self.read(a, n))
                    if a <= address < a+n:
                        data[address-a] ^= 1
                    return bytes(data)
                with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                    require_preferences_actions_tables(changed)

    def test_alignment_is_pinned_and_outside_instruction_claim(self):
        require_preferences_actions_padding(self.read)
        self.assertFalse({a for a, _ in PADDING} & {r['address'] for r in self.records})
        for address, _ in PADDING:
            def changed(a, n):
                data = bytearray(self.read(a, n))
                if a <= address < a+n:
                    data[address-a] ^= 1
                return bytes(data)
            with self.assertRaises(ValueError):
                require_preferences_actions_padding(changed)

    def test_auto_save_menu_context_without_claiming_builder_reconstruction(self):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            rows = [json.loads(row[0]) for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (0x1033d030, 0x1033f840))]
        self.assertEqual(len(rows), 2562)
        self.assertEqual(fingerprint(rows), '6759f7e3fde7f16410c58b0f9be5b8a347b0487589028212c1f8f1009149636e')
        bank = int.from_bytes(self.read(0x108bba90-0x4e18, 4), 'big')
        by_address = {row['address']: row['assembly'] for row in rows}
        for label_pc, offset, value_pc, value, label in (
                (0x1033eab0, 0xabc, 0x1033ead4, 7, 'Every Week'),
                (0x1033eb60, 0xac7, 0x1033eb84, 14, 'Every Two Weeks'),
                (0x1033ec10, 0xad7, 0x1033ec34, 30, 'Every Month'),
                (0x1033ecc0, 0xae3, 0x1033ece4, 90, 'Every Three Months'),
                (0x1033ed70, 0xaf6, 0x1033ed94, 180, 'Every Six Months'),
                (0x1033ee20, 0xb07, 0x1033ee44, 360, 'Every Year')):
            self.assertEqual(self.read(bank+offset, len(label)+1), label.encode()+b'\0')
            self.assertEqual(by_address[label_pc], f'addi r4,r30,0x{offset:x}')
            self.assertEqual(by_address[value_pc-4], 'li r3,0x2d')
            self.assertEqual(by_address[value_pc], f'li r0,0x{value:x}')

    def test_productive_hooks_precede_original_accounting(self):
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['preferences_actions_hooks']=True
        for page in sorted({f[1] for f in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page]
            source=generate_page(page,rows,{r['address'] for r in self.records},userops=self.userops,**disabled)
            require_preferences_actions_hooks(page,rows,source)
            for entry in PREFERENCES_ACTIONS_ENTRIES:
                if entry>>12!=page:continue
                hook=preferences_actions_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_preferences_actions_hooks(page,rows,changed)

    def test_complete_reference_with_every_productive_hook_disabled(self):
        source = render_reference(self.records, self.userops)
        self.assertNotIn('try_modern_', source)
        self.assertNotIn('_impl<true,', source)
        for entry in PREFERENCES_ACTIONS_ENTRIES:
            self.assertIn(f'case 0x{entry:x}:', source)
        for row in self.records:
            self.assertIn(f's.step(0x{row["address"]:x});', source)

    def test_changed_instruction_fields_fail(self):
        for start, *_ in CONTRACTS:
            i = next(i for i, row in enumerate(self.records) if row['address'] == start)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records)
                changed[i][key] = value
                with self.subTest(start=hex(start), key=key), self.assertRaises(ValueError):
                    require_preferences_actions_bodies(changed, complete=True)

    def test_missing_duplicate_reordered_or_absent_bodies_fail(self):
        for start, end, *_ in CONTRACTS:
            i = next(i for i, row in enumerate(self.records) if row['address'] == start)
            for changed in (self.records[:i]+self.records[i+1:], self.records+[self.records[i]],
                            list(reversed(self.records)), [row for row in self.records if not start <= row['address'] < end]):
                with self.subTest(start=hex(start)), self.assertRaises(ValueError):
                    render_reference(changed, self.userops)

    def test_partial_page_omissions_fail(self):
        for start, page, *_ in FRAGMENTS:
            rows = [r for r in self.records if r['address'] >> 12 == page]
            first = min(r['address'] for r in rows if r['address'] >= max(start, page << 12))
            require_preferences_actions_contract(page, rows, {r['address'] for r in self.records})
            with self.assertRaises(ValueError):
                require_preferences_actions_contract(page, [r for r in rows if r['address'] != first], {start})


if __name__ == '__main__':
    unittest.main()
