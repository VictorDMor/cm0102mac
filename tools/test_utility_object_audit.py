"""Pinned utility-object boundaries, original references and additive production hooks."""
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
from utility_object_audit import (CONTRACTS, UTILITY_OBJECT_ENTRIES, require_utility_object_bodies,
                                 require_utility_object_edges, require_utility_object_index,
                                 require_utility_object_contract, require_utility_object_hooks, utility_object_hook)
from make_utility_object_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_UTILITY_OBJECT_INPUT', ROOT / '.local/compiler-arithmetic-reconstruction/input'))


class ScopeTests(unittest.TestCase):
    def test_complete_scope_and_preserved_559_entry_prefix(self):
        self.assertEqual(len(CONTRACTS), 11)
        self.assertEqual(sum(n for _, _, n, _ in CONTRACTS), 239)
        self.assertEqual(UTILITY_OBJECT_ENTRIES, tuple(sorted(set(UTILITY_OBJECT_ENTRIES))))
        baseline = [{'entry': f'0x{a:x}', 'module': m} for a, m in SEMANTIC_REPLACEMENTS]
        self.assertEqual(len(baseline), 590)
        self.assertEqual(len({a for a, _ in SEMANTIC_REPLACEMENTS}), 590)
        self.assertEqual(hashlib.sha256(json.dumps(baseline[:559], sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '2d2d5079bbf41b2456c5d6c68aa30c2a4f1207d739c6c659ec1b3f2b88c9fc25')
        self.assertEqual(SEMANTIC_REPLACEMENTS[559:570], tuple((a, 'utility_object_adapter') for a in UTILITY_OBJECT_ENTRIES))

    def test_only_roots_are_exterior_entrances(self):
        for start, end, _, _ in CONTRACTS:
            require_utility_object_edges([(0, start), (start, end - 4)])
            for target in range(start + 4, end, 4):
                with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                    require_utility_object_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private utility-object source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = [json.loads(r[0]) for start, end, _, _ in CONTRACTS for r in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_complete_index_and_interior_edges(self):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_utility_object_index(db)

    def test_every_pin_rejects_changed_fields(self):
        for entry in UTILITY_OBJECT_ENTRIES:
            index = next(n for n, r in enumerate(self.records) if r['address'] == entry)
            for key, value in (('address', entry + 1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                bad = deepcopy(self.records); bad[index][key] = value
                with self.subTest(entry=hex(entry), key=key), self.assertRaises(ValueError):
                    require_utility_object_bodies(bad, complete=True)

    def test_missing_duplicate_reordered_or_wholly_absent_body_fails(self):
        for start, end, _, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for bad in (body[1:], body[:-1], body + [body[0]], list(reversed(body))):
                with self.assertRaises(ValueError): require_utility_object_bodies(bad)
            with self.assertRaises(ValueError):
                render_reference([r for r in self.records if not start <= r['address'] < end], self.userops)
            with self.assertRaises(ValueError):
                require_utility_object_contract(start >> 12, [], {start})
            with self.assertRaises(ValueError):
                require_utility_object_contract(start >> 12, body[1:])

    def test_reference_has_exactly_239_original_labels(self):
        code = render_reference(self.records, self.userops)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE), [f'{r["address"]:x}' for r in self.records])
        for token in ('try_modern_', 'utility_object::', 'complete_context_transfer', 'bool page_10000(', 'bool page_1076e('):
            self.assertNotIn(token, code)

    def test_generated_hooks_precede_original_operations_at_every_root(self):
        for page in tuple(sorted({start >> 12 for start, _, _, _ in CONTRACTS})):
            records = [r for r in self.records if r['address'] >> 12 == page]
            code = generate_page(page, records, {r['address'] for r in self.records}, userops=self.userops)
            require_utility_object_hooks(page, records, code)
            for entry in (a for a in UTILITY_OBJECT_ENTRIES if a >> 12 == page):
                hook, step = utility_object_hook(entry), f's.step(0x{entry:x});'
                for broken in (code.replace(hook, ''), code.replace(hook, hook + '\n' + hook),
                               code.replace(hook + '\n' + step, step + '\n' + hook)):
                    with self.assertRaises(ValueError): require_utility_object_hooks(page, records, broken)


if __name__ == '__main__': unittest.main()
