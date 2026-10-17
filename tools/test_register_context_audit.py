"""Complete pins and entrances for the Mac register-context component block."""
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
from register_context_audit import (ROOT_CONTRACTS, ENTRY_CONTRACTS, REGISTER_CONTEXT_ENTRIES,
                                   require_register_context_bodies, require_register_context_edges,
                                   require_register_context_index, require_register_context_hooks,
                                   register_context_hook, CAPTURE_OBSERVATIONS)
from make_register_context_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_REGISTER_CONTEXT_INPUT', ROOT / '.local/compiler-arithmetic-reconstruction/input'))


class ScopeTests(unittest.TestCase):
    def test_four_complete_bodies_thirty_entries_and_unchanged_production(self):
        self.assertEqual(len(ROOT_CONTRACTS), 4)
        self.assertEqual(sum(n for _, _, n, _ in ROOT_CONTRACTS), 91)
        self.assertEqual(len(ENTRY_CONTRACTS), 30)
        self.assertEqual(sum(n for _, _, n, _ in ENTRY_CONTRACTS), 299)
        self.assertEqual(REGISTER_CONTEXT_ENTRIES, tuple(sorted(set(REGISTER_CONTEXT_ENTRIES))))
        baseline = [{'entry': f'0x{a:x}', 'module': module} for a, module in SEMANTIC_REPLACEMENTS]
        self.assertEqual(len(baseline), 707)
        self.assertEqual(hashlib.sha256(json.dumps(baseline[:234], sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '9f3d57d5077117883869f644578eb0d91f972527febf4b742ab5598f54f043b0')
        self.assertEqual(SEMANTIC_REPLACEMENTS[234:264], tuple((a, 'register_context_adapter') for a in REGISTER_CONTEXT_ENTRIES))

    def test_valid_suffix_entrances_and_forbidden_interior_edges(self):
        for entry in REGISTER_CONTEXT_ENTRIES:
            require_register_context_edges([(0, entry)])
        for start, end, _, _ in ROOT_CONTRACTS:
            require_register_context_edges([(start, end - 4)])
            for target in (end - 4, start + 1, end - 1):
                with self.subTest(target=hex(target)), self.assertRaisesRegex(ValueError, 'External register-context interior'):
                    require_register_context_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private register-context source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = [json.loads(row[0]) for start, end, _, _ in ROOT_CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_complete_index_and_all_external_edges(self):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_register_context_index(db)

    def test_every_entry_rejects_changed_first_instruction(self):
        require_register_context_bodies(self.records, complete=True)
        for entry in REGISTER_CONTEXT_ENTRIES:
            index = next(n for n, r in enumerate(self.records) if r['address'] == entry)
            for key, value in (('address', entry + 1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                mutation = deepcopy(self.records); mutation[index][key] = value
                with self.subTest(entry=hex(entry), key=key), self.assertRaises(ValueError):
                    require_register_context_bodies(mutation, complete=True)

    def test_missing_duplicate_and_reordered_bodies_fail(self):
        for start, end, _, _ in ROOT_CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for bad in (body[1:], body[:-1], body + [body[0]], list(reversed(body))):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_register_context_bodies(bad)
            with self.assertRaisesRegex(ValueError, 'Missing complete'):
                require_register_context_bodies([r for r in self.records if not start <= r['address'] < end], complete=True)

    def test_isolated_reference_contains_all_91_labels_and_no_native_dispatch(self):
        code = render_reference(self.records, self.userops)
        labels = re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE)
        self.assertEqual(labels, [f'{r["address"]:x}' for r in self.records])
        self.assertEqual(len(set(labels)), 91)
        for token in ('register_context_reference_impl<false, true>', 'register_context_reference_impl<false, false>'):
            self.assertIn(token, code)
        for token in ('page_10773_impl', 'register_context_reference_impl<true,', 'try_modern_', 'register_context::', 'complete_context_transfer'):
            self.assertNotIn(token, code)

    def test_reference_cannot_omit_a_shared_tail_or_nonlocal_body(self):
        for start, end, _, _ in ROOT_CONTRACTS:
            with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                render_reference([r for r in self.records if not start <= r['address'] < end], self.userops)

    def test_production_hooks_precede_accounting_and_capture_observations_follow_it(self):
        code = generate_page(0x10773, self.records, {r['address'] for r in self.records}, userops=self.userops)
        require_register_context_hooks(0x10773, self.records, code)
        for entry in REGISTER_CONTEXT_ENTRIES:
            hook = register_context_hook(entry)
            for bad in (code.replace(hook, ''), code.replace(hook, hook + '\n' + hook),
                        code.replace(hook, '').replace(f'counter.step(0x{entry:x});', f'counter.step(0x{entry:x});\n{hook}')):
                with self.subTest(entry=hex(entry)), self.assertRaises(ValueError):
                    require_register_context_hooks(0x10773, self.records, bad)
        for address, hook in CAPTURE_OBSERVATIONS.items():
            for bad in (code.replace(hook, ''), code.replace(hook, '').replace(
                    f'counter.step(0x{address:x});', f'{hook}\ncounter.step(0x{address:x});')):
                with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                    require_register_context_hooks(0x10773, self.records, bad)


if __name__ == '__main__':
    unittest.main()
