"""Whole original movement scope and independent reference fidelity."""
from contextlib import closing
from copy import deepcopy
import hashlib
import inspect
import json
import os
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from player_movement_audit import ENTRY, END, COUNT, FRAGMENTS, TABLES, load_player_movement, load_player_movement_tables, require_player_movement_body, require_player_movement_edges, require_player_movement_tables, require_player_movement_contract, require_player_movement_hooks
from make_player_movement_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_PLAYER_MOVEMENT_INPUT', ROOT / '.local/compiler-arithmetic-reconstruction/input'))


class ScopeTests(unittest.TestCase):
    def test_complete_scope_appends_one_production_root(self):
        self.assertEqual(sum(count for _, count, _ in FRAGMENTS), COUNT)
        self.assertEqual((ENTRY, END, COUNT), (0x1022e7e0, 0x10235da4, 7537))
        self.assertEqual(SEMANTIC_REPLACEMENTS[648], (ENTRY, "player_movement_adapter"))
        manifest = [{'entry': f'0x{entry:x}', 'module': module} for entry, module in SEMANTIC_REPLACEMENTS]
        self.assertEqual(len(manifest), 815)
        self.assertEqual(hashlib.sha256(json.dumps(manifest[:643], sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '3f290aa60e8a00b40e6bceba6d3abfbe86e6a32413fa267d7ec7c2e07a7c1398')

    def test_previous_manifest_is_preserved(self):
        previous=[{'entry':f'0x{entry:x}','module':module} for entry,module in SEMANTIC_REPLACEMENTS[:648]]
        self.assertEqual(hashlib.sha256(json.dumps(previous,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
                         'ad73754c9c759159ac6dc22a40d7392bd94d25283af8bf4c1c323f0c721f65f0')

    def test_production_does_not_replay_its_body(self):
        sources=list((ROOT/'native').glob('player_movement*.cpp'))
        sources=[source for source in sources if not any(word in source.name for word in ('checks','characterization','adapter'))]
        self.assertTrue(sources)
        for source in sources:
            text=source.read_text()
            for forbidden in ('player_movement_reference','dispatch_page','pcode','i_102'):
                self.assertNotIn(forbidden,text,source.name)
        coordinator=(ROOT/'native/player_movement_coordinator.cpp').read_text()
        self.assertIn('if(state.pc<entry || state.pc>=end)',coordinator)
        self.assertIn('GuestNativeScope scope(state,entry)',coordinator)

    def test_only_the_root_is_an_allowed_external_entrance(self):
        require_player_movement_edges([(0, ENTRY), (ENTRY, END - 4), (END - 4, END)])
        for target in (ENTRY + 4, *(page << 12 for page, *_ in FRAGMENTS[1:]), END - 4):
            with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                require_player_movement_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = load_player_movement(db)
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']
        cls.tables = load_player_movement_tables(INPUT)
        cls.code = render_reference(cls.records, cls.userops)

    def test_every_source_field_is_pinned_on_every_fragment(self):
        for page, *_ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records)
                changed[index][key] = value
                with self.subTest(page=hex(page), key=key), self.assertRaises(ValueError):
                    require_player_movement_body(changed)

    def test_missing_duplicate_reordered_absent_and_unrelated_bodies_fail(self):
        for page, *_ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page)
            for changed in (self.records[:index] + self.records[index + 1:], self.records + [self.records[index]],
                            list(reversed(self.records)), [row for row in self.records if row['address'] >> 12 != page]):
                with self.subTest(page=hex(page)), self.assertRaises(ValueError):
                    render_reference(changed, self.userops)
        unrelated = deepcopy(self.records[-1]); unrelated['address'] = END
        with self.assertRaises(ValueError): render_reference(self.records + [unrelated], self.userops)

    def test_reference_retains_every_label_and_checkpoint_without_hooks(self):
        expected = [f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', self.code, re.MULTILINE), expected)
        self.assertEqual(re.findall(r'player_movement_reference_step_seen\(s, 0x([0-9a-f]+)\)', self.code), expected)
        for page, *_ in FRAGMENTS: self.assertIn(f'bool player_movement_reference_{page:x}(State& s)', self.code)
        for forbidden in ('try_modern_', 'complete_context_transfer', 'bool page_10000(', 'bool page_1076e('):
            self.assertNotIn(forbidden, self.code)

    def test_every_direct_dependency_target_is_preserved_before_accounting(self):
        expected = [(f'{row["address"]:x}', row['assembly'].split()[1][2:])
                    for row in self.records if row['assembly'].startswith('bl ')]
        self.assertEqual(re.findall(r'player_movement_reference_call_seen\(s, 0x([0-9a-f]+), 0x([0-9a-f]+)\)', self.code), expected)
        self.assertEqual(len(expected), 337)
        self.assertEqual(len({target for _, target in expected}), 55)
        for pc, target in expected:
            self.assertIn(f'player_movement_reference_call_seen(s, 0x{pc}, 0x{target}); s.step(0x{pc});', self.code)

    def test_computed_jumps_read_the_live_ctr_and_all_original_table_words(self):
        sites = [row['address'] for row in self.records if row['assembly'] == 'bctr']
        self.assertEqual(sites, [site for site, *_ in TABLES])
        for site, *_ in TABLES:
            self.assertIn(f'player_movement_reference_jump_seen(s, 0x{site:x}, std::uint32_t(s.r<4>(4132))); s.step(0x{site:x});', self.code)
        self.assertEqual([len(self.tables[site]) for site in sites], [47, 47, 11])
        self.assertEqual([len(set(self.tables[site])) for site in sites], [12, 12, 3])

    def test_complete_production_hook_and_fragment_contracts(self):
        for page,*_ in FRAGMENTS:
            rows=[row for row in self.records if row['address']>>12==page]
            disabled={key:False for key in inspect.signature(generate_page).parameters if key.endswith('_hooks')}
            disabled['player_movement_hooks']=True
            code=generate_page(page,rows,{row['address'] for row in self.records},userops=self.userops,**disabled)
            require_player_movement_contract(page,rows,{ENTRY})
            require_player_movement_hooks(page,rows,code)
            if page==ENTRY>>12:
                hook=f'if (try_modern_player_movement(s, 0x{ENTRY:x})) return true;'
                with self.assertRaises(ValueError):require_player_movement_hooks(page,rows,code.replace(hook,''))
            else:self.assertNotIn('try_modern_player_movement',code)
            with self.assertRaises(ValueError):require_player_movement_contract(page,rows[:-1],{row['address'] for row in rows})

    def test_table_pointer_or_any_word_mutation_is_rejected(self):
        words = {}
        for site, slot, table, count, _ in TABLES:
            words[slot] = table
            words.update({table + n * 4: value for n, value in enumerate(self.tables[site])})
            self.assertEqual(len(self.tables[site]), count)
        require_player_movement_tables(words.__getitem__)
        for address in words:
            changed = dict(words); changed[address] ^= 1
            with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                require_player_movement_tables(changed.__getitem__)


if __name__ == '__main__':
    unittest.main()
