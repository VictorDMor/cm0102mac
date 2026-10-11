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
from time_runtime_audit import (CONTRACTS, TIME_RUNTIME_ENTRIES, require_time_runtime_bodies,
                               require_time_runtime_edges, require_time_runtime_index,
                               require_time_runtime_contract, require_time_runtime_hooks, time_runtime_hook)
from make_time_runtime_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_TIME_RUNTIME_INPUT', ROOT / '.local/compiler-arithmetic-reconstruction/input'))


class ScopeTests(unittest.TestCase):
    def test_complete_eight_entry_scope_and_production_preserved(self):
        self.assertEqual(len(CONTRACTS), 8)
        self.assertEqual(sum(n for _, _, n, _ in CONTRACTS), 204)
        self.assertEqual(TIME_RUNTIME_ENTRIES, tuple(sorted(set(TIME_RUNTIME_ENTRIES))))
        baseline = [{'entry': f'0x{a:x}', 'module': m} for a, m in SEMANTIC_REPLACEMENTS]
        self.assertEqual(len(baseline), 559)
        self.assertEqual(len({entry for entry, _ in SEMANTIC_REPLACEMENTS}), 559)
        self.assertEqual(hashlib.sha256(json.dumps(baseline[:268], sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '97d4e7d8d5633d4efc9c771a720a4ddedfa347de67e7600c20ceca91ac20fbec')
        self.assertEqual(SEMANTIC_REPLACEMENTS[268:276], tuple((a, 'time_runtime_adapter') for a in TIME_RUNTIME_ENTRIES))

    def test_only_complete_roots_are_exterior_entrances(self):
        for start, end, _, _ in CONTRACTS:
            require_time_runtime_edges([(0, start), (start, end - 4)])
            for target in (start + 4, end - 4):
                with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                    require_time_runtime_edges([(0, target)])


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
            require_time_runtime_index(db)

    def test_each_contract_rejects_changed_fields(self):
        for entry in TIME_RUNTIME_ENTRIES:
            index = next(n for n, r in enumerate(self.records) if r['address'] == entry)
            for key, value in (('address', entry + 1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                bad = deepcopy(self.records); bad[index][key] = value
                with self.subTest(entry=hex(entry), key=key), self.assertRaises(ValueError):
                    require_time_runtime_bodies(bad, complete=True)

    def test_missing_duplicate_and_reordered_bodies_fail(self):
        for start, end, _, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for bad in (body[1:], body[:-1], body + [body[0]], list(reversed(body))):
                with self.assertRaises(ValueError): require_time_runtime_bodies(bad)
            with self.assertRaises(ValueError):
                render_reference([r for r in self.records if not start <= r['address'] < end], self.userops)

    def test_reference_contains_all_204_labels_and_no_semantic_hook(self):
        code = render_reference(self.records, self.userops)
        labels = re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE)
        self.assertEqual(labels, [f'{r["address"]:x}' for r in self.records])
        self.assertEqual(len(set(labels)), 204)
        self.assertNotIn('i_107756bc:', code)
        for token in ('try_modern_', 'time_runtime::', 'complete_context_transfer', 'bool page_1077f('): self.assertNotIn(token, code)

    def test_hooks_precede_first_original_operation_at_each_destination(self):
        code = generate_page(0x1077f, self.records, {r['address'] for r in self.records}, userops=self.userops)
        require_time_runtime_hooks(0x1077f, self.records, code)
        for entry in TIME_RUNTIME_ENTRIES:
            hook = time_runtime_hook(entry); step = f's.step(0x{entry:x});'
            for broken in (code.replace(hook, ''), code.replace(hook, hook + '\n' + hook),
                           code.replace(hook + '\n' + step, step + '\n' + hook)):
                with self.assertRaises(ValueError): require_time_runtime_hooks(0x1077f, self.records, broken)

    def test_selected_missing_whole_entry_cannot_claim_completion(self):
        for start, end, _, _ in CONTRACTS:
            bad = [r for r in self.records if not start <= r['address'] < end]
            with self.assertRaises(ValueError): require_time_runtime_contract(0x1077f, bad, TIME_RUNTIME_ENTRIES)


if __name__ == '__main__':
    unittest.main()
