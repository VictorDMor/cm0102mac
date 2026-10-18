"""Whole ball-action boundary and independent component-reference safeguards."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import re
import sqlite3
import unittest

from ball_action_audit import ENTRY, END, COUNT, FRAGMENTS, TABLES, load_ball_action, load_ball_action_tables, require_ball_action_body, require_ball_action_edges, require_ball_action_tables, require_ball_action_contract, require_ball_action_index, require_ball_action_hooks, ball_action_hook
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from make_ball_action_reference import COMPONENTS, render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_BALL_ACTION_INPUT', ROOT / '.local/compiler-arithmetic-reconstruction/input'))


class ScopeTests(unittest.TestCase):
    def test_complete_scope_is_not_the_truncated_inventory_extent(self):
        self.assertEqual((ENTRY, END, COUNT), (0x1023cd20, 0x10244c58, 8142))
        self.assertEqual(END - ENTRY, COUNT * 4)
        self.assertEqual(sum(count for _, count, _ in FRAGMENTS), COUNT)
        self.assertGreater(END, ENTRY + 30564)

    def test_only_the_complete_root_extends_the_prior_manifest(self):
        manifest = [{'entry': f'0x{entry:x}', 'module': module} for entry, module in SEMANTIC_REPLACEMENTS]
        self.assertEqual(len(manifest), 871)
        self.assertEqual(hashlib.sha256(json.dumps(manifest[:677], sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         'bf8519e9fb7ff93e6741076af5a6d27074007dfe67a4d22ebac6c95c0b244822')
        self.assertEqual(manifest[677:678], [{'entry': '0x1023cd20', 'module': 'ball_action_adapter'}])
        self.assertEqual(len({row['entry'] for row in manifest}), 871)
        for source in (ROOT / 'native').glob('ball_action*.cpp'):
            if 'checks' in source.name or source.name == 'ball_action_adapter.cpp':
                continue
            for forbidden in ('_reference(', 'dispatch_page', 'pcode', 'i_102'):
                self.assertNotIn(forbidden, source.read_text(), source.name)

    def test_coordinator_and_adapter_own_the_complete_root(self):
        source = (ROOT / 'native/ball_action_coordinator.cpp').read_text()
        self.assertIn('GuestNativeScope scope(state,entry)', source)
        self.assertIn('scope.frame(std::uint32_t(state.r<4>(4)))', source)
        self.assertIn('while(state.pc!=caller)', source)
        self.assertIn('return_action_frame(state,dependencies,observer)', source)
        adapter = (ROOT / 'native/ball_action_adapter.cpp').read_text()
        self.assertIn('ball_action::act(s, nullptr, d.observer)', adapter)
        self.assertIn('d.original_body == entry', adapter)
        self.assertIn('match_pc_profile_enabled()', adapter)
        for address in (ENTRY + 4, END - 4):
            with self.assertRaises(ValueError):
                ball_action_hook(address)

    def test_only_the_complete_root_is_an_external_entrance(self):
        require_ball_action_edges([(0, ENTRY), (ENTRY, END - 4), (END - 4, END)])
        for target in (ENTRY + 4, *(page << 12 for page, *_ in FRAGMENTS[1:]), END - 4):
            with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                require_ball_action_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = load_ball_action(db)
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']
        cls.tables = load_ball_action_tables(INPUT)

    def test_productive_hook_precedes_the_original_step_and_references_disable_it(self):
        selected = {row['address'] for row in self.records}
        for page, *_ in FRAGMENTS:
            part = [row for row in self.records if row['address'] >> 12 == page]
            code = generate_page(page, part, selected, userops=self.userops)
            require_ball_action_hooks(page, part, code)
            self.assertEqual(code.count(ball_action_hook(ENTRY)), int(page == ENTRY >> 12))
            require_ball_action_contract(page, part, selected)
            if page == ENTRY >> 12:
                for changed in (code.replace(ball_action_hook(ENTRY), ''), code.replace(ball_action_hook(ENTRY), ball_action_hook(ENTRY) * 2)):
                    with self.assertRaises(ValueError):
                        require_ball_action_hooks(page, part, changed)
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_ball_action_index(db)

    def test_every_source_field_is_pinned_on_every_fragment(self):
        for page, *_ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = list(self.records)
                changed[index] = deepcopy(changed[index])
                changed[index][key] = value
                with self.subTest(page=hex(page), key=key), self.assertRaises(ValueError):
                    require_ball_action_body(changed)

    def test_missing_duplicate_reordered_and_truncated_bodies_fail(self):
        for changed in (self.records[:-1], self.records + [self.records[-1]], list(reversed(self.records)), self.records[:7641]):
            with self.assertRaises(ValueError):
                render_reference(changed, self.userops, 'preparation')
        for page, *_ in FRAGMENTS:
            with self.subTest(page=hex(page)), self.assertRaises(ValueError):
                require_ball_action_body([row for row in self.records if row['address'] >> 12 != page])

    def test_component_references_retain_all_labels_and_checkpoints_without_hooks(self):
        for component, (start, end) in COMPONENTS.items():
            with self.subTest(component=component):
                code = render_reference(self.records, self.userops, component)
                expected = [f'{address:x}' for address in range(start, end, 4)]
                self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE), expected)
                self.assertEqual(re.findall(r'ball_action_step_seen\(s, 0x([0-9a-f]+)\)', code), expected)
                for forbidden in ('try_modern_', 'complete_context_transfer', 'bool page_10000(', 'bool page_1076e('):
                    self.assertNotIn(forbidden, code)

    def test_five_live_tables_match_the_original_computed_branch_sites(self):
        sites = [row['address'] for row in self.records if row['assembly'] == 'bctr']
        self.assertEqual(sites, [site for site, *_ in TABLES])
        self.assertEqual([len(self.tables[site]) for site in sites], [47, 57, 47, 10, 47])
        self.assertEqual([len(set(self.tables[site])) for site in sites], [12, 12, 12, 3, 7])

    def test_every_original_table_pointer_and_word_is_pinned(self):
        words = {}
        for site, slot, table, count, _ in TABLES:
            words[slot] = table
            words.update({table + n * 4: value for n, value in enumerate(self.tables[site])})
            self.assertEqual(len(self.tables[site]), count)
        require_ball_action_tables(words.__getitem__)
        for address in words:
            changed = dict(words)
            changed[address] ^= 1
            with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                require_ball_action_tables(changed.__getitem__)


if __name__ == '__main__':
    unittest.main()
