"""Complete pins, original-only reference and unchanged production boundary."""
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
from stdlib_core_audit import (CONTRACTS, STDLIB_CORE_ENTRIES, require_stdlib_core_bodies,
                               require_stdlib_core_edges, require_stdlib_core_index,
                               require_stdlib_core_contract, require_stdlib_core_hooks, stdlib_core_hook)
from make_stdlib_core_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_STDLIB_CORE_INPUT', ROOT / '.local/compiler-arithmetic-reconstruction/input'))


class ScopeTests(unittest.TestCase):
    def test_complete_four_entry_scope_and_production_preserved(self):
        self.assertEqual(len(CONTRACTS), 4)
        self.assertEqual(sum(n for _, _, n, _ in CONTRACTS), 90)
        self.assertEqual(STDLIB_CORE_ENTRIES, tuple(sorted(set(STDLIB_CORE_ENTRIES))))
        baseline = [{'entry': f'0x{a:x}', 'module': m} for a, m in SEMANTIC_REPLACEMENTS]
        self.assertEqual(len(baseline), 911)
        self.assertEqual(len({entry for entry, _ in SEMANTIC_REPLACEMENTS}), 911)
        self.assertEqual(hashlib.sha256(json.dumps(baseline[:264], sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '6e438c9dbfe73e5e4466f7ecb2375f14cd8e7ff61acc7838047b4dcb8826531c')
        self.assertEqual(SEMANTIC_REPLACEMENTS[264:268], tuple((a, 'stdlib_core_adapter') for a in STDLIB_CORE_ENTRIES))

    def test_only_complete_roots_are_exterior_entrances(self):
        for start, end, _, _ in CONTRACTS:
            require_stdlib_core_edges([(0, start), (start, end - 4)])
            for target in (start + 4, end - 4):
                with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                    require_stdlib_core_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private stdlib source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = [json.loads(r[0]) for start, end, _, _ in CONTRACTS for r in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_complete_index_and_all_interior_edges(self):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_stdlib_core_index(db)

    def test_each_contract_rejects_changed_fields(self):
        for entry in STDLIB_CORE_ENTRIES:
            index = next(n for n, r in enumerate(self.records) if r['address'] == entry)
            for key, value in (('address', entry + 1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                bad = deepcopy(self.records); bad[index][key] = value
                with self.subTest(entry=hex(entry), key=key), self.assertRaises(ValueError):
                    require_stdlib_core_bodies(bad, complete=True)

    def test_missing_duplicate_and_reordered_bodies_fail(self):
        for start, end, _, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for bad in (body[1:], body[:-1], body + [body[0]], list(reversed(body))):
                with self.assertRaises(ValueError): require_stdlib_core_bodies(bad)
            with self.assertRaises(ValueError):
                render_reference([r for r in self.records if not start <= r['address'] < end], self.userops)

    def test_reference_contains_all_90_labels_and_no_semantic_hook(self):
        code = render_reference(self.records, self.userops)
        labels = re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE)
        self.assertEqual(labels, [f'{r["address"]:x}' for r in self.records])
        self.assertEqual(len(set(labels)), 90)
        self.assertNotIn('i_107756bc:', code)
        for token in ('try_modern_', 'stdlib_core::', 'complete_context_transfer', 'bool page_10775('): self.assertNotIn(token, code)

    def test_hooks_precede_first_original_operation_at_each_destination(self):
        code = generate_page(0x10775, self.records, {r['address'] for r in self.records}, userops=self.userops)
        require_stdlib_core_hooks(0x10775, self.records, code)
        for entry in STDLIB_CORE_ENTRIES:
            hook = stdlib_core_hook(entry); step = f's.step(0x{entry:x});'
            for broken in (code.replace(hook, ''), code.replace(hook, hook + '\n' + hook),
                           code.replace(hook + '\n' + step, step + '\n' + hook)):
                with self.assertRaises(ValueError): require_stdlib_core_hooks(0x10775, self.records, broken)

    def test_selected_missing_whole_entry_cannot_claim_completion(self):
        for start, end, _, _ in CONTRACTS:
            bad = [r for r in self.records if not start <= r['address'] < end]
            with self.assertRaises(ValueError): require_stdlib_core_contract(0x10775, bad, STDLIB_CORE_ENTRIES)


if __name__ == '__main__':
    unittest.main()
