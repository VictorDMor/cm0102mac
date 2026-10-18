"""Pinned tree-mutation boundaries, original references and additive production hooks."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from tree_mutation_audit import (CONTRACTS, TREE_MUTATION_ENTRIES, require_tree_mutation_bodies,
                                 require_tree_mutation_edges, require_tree_mutation_index,
                                 require_tree_mutation_contract, require_tree_mutation_hooks, tree_mutation_hook)
from make_tree_mutation_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_TREE_MUTATION_INPUT', ROOT / '.local/compiler-arithmetic-reconstruction/input'))


class ScopeTests(unittest.TestCase):
    def test_complete_scope_and_preserved_570_entry_prefix(self):
        self.assertEqual(len(CONTRACTS), 3)
        self.assertEqual(sum(n for _, _, n, _ in CONTRACTS), 285)
        self.assertEqual(TREE_MUTATION_ENTRIES, tuple(sorted(set(TREE_MUTATION_ENTRIES))))
        baseline = [{'entry': f'0x{a:x}', 'module': m} for a, m in SEMANTIC_REPLACEMENTS]
        self.assertEqual(len(baseline), 831)
        self.assertEqual(len({a for a, _ in SEMANTIC_REPLACEMENTS}), 831)
        self.assertEqual(hashlib.sha256(json.dumps(baseline[:570], sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '7c46bda5a1297c19d312785558d5b5dbec2beff2c1c06fd9e11a1aa5cbbb7256')
        self.assertEqual(SEMANTIC_REPLACEMENTS[570:573], tuple((a, 'tree_mutation_adapter') for a in TREE_MUTATION_ENTRIES))

    def test_only_roots_are_exterior_entrances(self):
        for start, end, _, _ in CONTRACTS:
            require_tree_mutation_edges([(0, start), (start, end - 4)])
            for target in range(start + 4, end, 4):
                with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                    require_tree_mutation_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private tree-mutation source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = [json.loads(r[0]) for start, end, _, _ in CONTRACTS for r in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_complete_index_and_interior_edges(self):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_tree_mutation_index(db)

    def test_every_pin_rejects_changed_fields(self):
        for entry in TREE_MUTATION_ENTRIES:
            index = next(n for n, r in enumerate(self.records) if r['address'] == entry)
            for key, value in (('address', entry + 1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                bad = deepcopy(self.records); bad[index][key] = value
                with self.subTest(entry=hex(entry), key=key), self.assertRaises(ValueError):
                    require_tree_mutation_bodies(bad, complete=True)

    def test_missing_duplicate_reordered_or_wholly_absent_body_fails(self):
        for start, end, _, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for bad in (body[1:], body[:-1], body + [body[0]], list(reversed(body))):
                with self.assertRaises(ValueError): require_tree_mutation_bodies(bad)
            with self.assertRaises(ValueError):
                render_reference([r for r in self.records if not start <= r['address'] < end], self.userops)
            with self.assertRaises(ValueError):
                require_tree_mutation_contract(start >> 12, [], {start})
            with self.assertRaises(ValueError):
                require_tree_mutation_contract(start >> 12, body[1:])

    def test_reference_has_exactly_285_original_labels(self):
        code = render_reference(self.records, self.userops)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE), [f'{r["address"]:x}' for r in self.records])
        for token in ('try_modern_', 'tree_mutation::', 'complete_context_transfer', 'bool page_10000(', 'bool page_1076e('):
            self.assertNotIn(token, code)

    def test_recursive_calls_publish_the_original_links_and_exit_to_dispatch(self):
        code = render_reference(self.records, self.userops)
        recursive = 0
        for start, end, _, _ in CONTRACTS:
            for record in self.records:
                if start <= record['address'] < end and record['assembly'] == f'bl 0x{start:x}':
                    fragment = code.split(f'i_{record["address"]:x}:', 1)
                    fragment = fragment[1].split('\n}', 1)[0]
                    self.assertIn(f's.wr<4>(4128, UINT64_C({record["address"] + 4}));', fragment)
                    self.assertIn(f's.pc = 0x{start:x}; return true;', fragment)
                    self.assertNotIn(f'goto i_{start:x};', fragment)
                    recursive += 1
        self.assertEqual(recursive, 18)

    def test_generated_hooks_precede_original_operations_at_every_root(self):
        for page in tuple(sorted({start >> 12 for start, _, _, _ in CONTRACTS})):
            records = [r for r in self.records if r['address'] >> 12 == page]
            code = generate_page(page, records, {r['address'] for r in self.records}, userops=self.userops)
            require_tree_mutation_hooks(page, records, code)
            for entry in (a for a in TREE_MUTATION_ENTRIES if a >> 12 == page):
                hook, step = tree_mutation_hook(entry), f's.step(0x{entry:x});'
                for broken in (code.replace(hook, ''), code.replace(hook, hook + '\n' + hook),
                               code.replace(hook + '\n' + step, step + '\n' + hook)):
                    with self.assertRaises(ValueError): require_tree_mutation_hooks(page, records, broken)


if __name__ == '__main__': unittest.main()
