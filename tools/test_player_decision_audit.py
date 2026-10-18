"""Whole original decision scope and independent reference fidelity."""
from contextlib import closing
from copy import deepcopy
import json
import os
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from player_decision_audit import ENTRY, END, COUNT, FRAGMENTS, load_player_decision, require_player_decision_body, require_player_decision_edges, require_player_decision_contract, require_player_decision_hooks, player_decision_hook
from make_player_decision_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_PLAYER_DECISION_INPUT', ROOT / '.local/compiler-arithmetic-reconstruction/input'))


class ScopeTests(unittest.TestCase):
    def test_complete_original_scope_and_preserved_642_entry_prefix(self):
        self.assertEqual(sum(count for _, count, _ in FRAGMENTS), COUNT)
        self.assertEqual((ENTRY, END, COUNT), (0x102836a0, 0x10285748, 2090))
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 735)
        self.assertEqual(SEMANTIC_REPLACEMENTS[642:643], ((ENTRY, 'player_decision_adapter'),))
        import hashlib
        prior = [{'entry': f'0x{entry:x}', 'module': module} for entry, module in SEMANTIC_REPLACEMENTS[:642]]
        self.assertEqual(hashlib.sha256(json.dumps(prior, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         'f2efaafb37166e008c7ed153d56bff3b63f9a8d80e872fe247500410d36ca6e0')

    def test_only_root_is_an_allowed_external_entrance(self):
        require_player_decision_edges([(0, ENTRY), (ENTRY, END - 4), (END - 4, END)])
        for target in (ENTRY + 4, 0x10284000, 0x10285000, END - 4):
            with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                require_player_decision_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = load_player_decision(db)
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_every_source_field_is_pinned_on_every_fragment(self):
        for page, _, _ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records)
                changed[index][key] = value
                with self.subTest(page=hex(page), key=key), self.assertRaises(ValueError):
                    require_player_decision_body(changed)

    def test_missing_duplicate_reordered_and_absent_fragments_fail(self):
        for page, _, _ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page)
            for changed in (self.records[:index] + self.records[index + 1:],
                            self.records + [self.records[index]],
                            list(reversed(self.records)),
                            [row for row in self.records if row['address'] >> 12 != page]):
                with self.subTest(page=hex(page)), self.assertRaises(ValueError):
                    render_reference(changed, self.userops)

    def test_independent_reference_keeps_every_original_label_and_checkpoint(self):
        code = render_reference(self.records, self.userops)
        expected = [f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE), expected)
        self.assertEqual(re.findall(r'player_decision_reference_step_seen\(s, 0x([0-9a-f]+)\)', code), expected)
        for page, _, _ in FRAGMENTS:
            self.assertIn(f'bool player_decision_reference_{page:x}(State& s)', code)
        for forbidden in ('try_modern_', 'complete_context_transfer', 'bool page_10000(', 'bool page_1076e('):
            self.assertNotIn(forbidden, code)

    def test_every_production_fragment_and_hook_is_pinned(self):
        for page, _, _ in FRAGMENTS:
            part = [row for row in self.records if row['address'] >> 12 == page]
            require_player_decision_contract(page, part)
            for broken in (part[1:], part[:-1], part + [part[0]], list(reversed(part))):
                with self.assertRaises(ValueError): require_player_decision_contract(page, broken)
            code = generate_page(page, part, {row['address'] for row in self.records}, userops=self.userops)
            require_player_decision_hooks(page, part, code)
            if page == ENTRY >> 12:
                hook = player_decision_hook(ENTRY)
                step = f'counter.step(0x{ENTRY:x});' if f'counter.step(0x{ENTRY:x});' in code else f's.step(0x{ENTRY:x});'
                for broken in (code.replace(hook, ''), code.replace(hook, hook + '\n' + hook), code.replace(hook + '\n' + step, step + '\n' + hook)):
                    with self.assertRaises(ValueError): require_player_decision_hooks(page, part, broken)
                with self.assertRaises(ValueError): require_player_decision_contract(page, [], {ENTRY})

    def test_reference_rejects_unrelated_instructions(self):
        unrelated = deepcopy(self.records[-1])
        unrelated['address'] = END
        with self.assertRaises(ValueError):
            render_reference(self.records + [unrelated], self.userops)

    def test_call_probes_preserve_original_targets_and_precede_accounting(self):
        code = render_reference(self.records, self.userops)
        expected = [(f'{row["address"]:x}', row['assembly'].split()[1][2:])
                    for row in self.records if row['assembly'].startswith('bl ')]
        self.assertEqual(re.findall(r'player_decision_reference_call_seen\(s, 0x([0-9a-f]+), 0x([0-9a-f]+)\)', code), expected)
        self.assertEqual(len(expected), 87)
        for pc, target in expected:
            self.assertIn(f'player_decision_reference_call_seen(s, 0x{pc}, 0x{target}); s.step(0x{pc});', code)


if __name__ == '__main__':
    unittest.main()
