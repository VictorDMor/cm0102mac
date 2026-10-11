"""Complete compiler arithmetic body pins and isolated original reference checks."""
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
from compiler_arithmetic_audit import (CONTRACTS, COMPILER_ARITHMETIC_ENTRIES,
    require_compiler_arithmetic_bodies, require_compiler_arithmetic_edges,
    require_compiler_arithmetic_index, require_compiler_arithmetic_hooks, compiler_arithmetic_hook)
from make_compiler_arithmetic_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_COMPILER_ARITHMETIC_INPUT', ROOT / '.local/exception-runtime-reconstruction/input'))
INDEX = INPUT / 'instructions.sqlite'


class ScopeTests(unittest.TestCase):
    def test_complete_scope_and_preserved_production_baseline(self):
        self.assertEqual(len(CONTRACTS), 8)
        self.assertEqual(sum(count for _, _, count, _ in CONTRACTS), 412)
        self.assertEqual(COMPILER_ARITHMETIC_ENTRIES, tuple(sorted(set(COMPILER_ARITHMETIC_ENTRIES))))
        baseline = [{'entry': f'0x{entry:x}', 'module': module} for entry, module in SEMANTIC_REPLACEMENTS[:226]]
        self.assertEqual(len(baseline), 226)
        self.assertEqual(hashlib.sha256(json.dumps(baseline, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         'cd2bf157adfb6e441959706bdfc12f8f4998aa07683b390e03c1ca31d1cfad0c')
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 536)
        self.assertEqual(len({entry for entry, _ in SEMANTIC_REPLACEMENTS}), 536)
        self.assertEqual(SEMANTIC_REPLACEMENTS[226:234], tuple((entry, 'compiler_arithmetic_adapter') for entry in COMPILER_ARITHMETIC_ENTRIES))
        for excluded in (0x10773710, 0x107737e0, 0x10773830, 0x10773eb0, 0x10773f30, 0x10773fb0):
            self.assertNotIn(excluded, COMPILER_ARITHMETIC_ENTRIES)

    def test_external_interior_edges_reject_for_every_entry(self):
        for start, end, _, _ in CONTRACTS:
            require_compiler_arithmetic_edges([(start - 4, start), (start, end - 4)])
            for target in (start + 4, end - 4):
                with self.subTest(entry=hex(start), target=hex(target)), self.assertRaisesRegex(ValueError, 'External compiler arithmetic interior'):
                    require_compiler_arithmetic_edges([(start - 4, target)])


@unittest.skipUnless(INDEX.exists(), 'Private original compiler arithmetic export unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect(INDEX.resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_actual_complete_index(self):
        with closing(sqlite3.connect(INDEX.resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_compiler_arithmetic_index(db)

    def test_every_body_rejects_changed_missing_duplicate_and_reordered_instructions(self):
        require_compiler_arithmetic_bodies(self.records, complete=True)
        for start, end, _, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for key, value in (('address', start + 1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                mutation = deepcopy(body); mutation[0][key] = value
                with self.subTest(entry=hex(start), key=key), self.assertRaises(ValueError):
                    require_compiler_arithmetic_bodies(mutation)
            for mutation in (body[1:], body[:-1], body + [body[0]], list(reversed(body))):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_compiler_arithmetic_bodies(mutation)
            with self.assertRaisesRegex(ValueError, 'Missing complete'):
                require_compiler_arithmetic_bodies([r for r in self.records if not start <= r['address'] < end], complete=True)

    def test_isolated_reference_retains_all_412_labels_and_always_uses_original_body(self):
        code = render_reference(self.records, self.userops)
        labels = re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE)
        self.assertEqual(labels, [f'{r["address"]:x}' for r in self.records])
        self.assertEqual(len(set(labels)), 412)
        self.assertIn('bool compiler_arithmetic_reference(State& s)', code)
        self.assertIn('compiler_arithmetic_reference_impl<false, true>', code)
        self.assertIn('compiler_arithmetic_reference_impl<false, false>', code)
        self.assertNotIn('compiler_arithmetic_reference_impl<true,', code)
        self.assertNotIn('page_10773_impl', code)
        self.assertNotIn('try_modern_exception_', code)
        self.assertNotIn('compiler_arithmetic::', code)
        self.assertNotIn('try_modern_compiler_arithmetic', code)

    def test_every_destination_hook_owns_counter_synchronization_before_original_step(self):
        code = generate_page(0x10773, self.records, {r['address'] for r in self.records}, userops=self.userops)
        require_compiler_arithmetic_hooks(0x10773, self.records, code)
        self.assertNotIn('try_fast_unsigned_division(s)', code)
        for entry in COMPILER_ARITHMETIC_ENTRIES:
            hook, step = compiler_arithmetic_hook(entry), f'counter.step(0x{entry:x});'
            for bad in (code.replace(hook, '', 1), code.replace(hook, hook + hook, 1),
                        code.replace(hook, '', 1).replace(step, step + hook, 1),
                        code.replace(hook, f'if (try_modern_compiler_arithmetic(s, 0x{entry:x})) return true;', 1)):
                with self.subTest(entry=hex(entry)), self.assertRaises(ValueError):
                    require_compiler_arithmetic_hooks(0x10773, self.records, bad)
        with self.assertRaisesRegex(ValueError, 'bypasses'):
            require_compiler_arithmetic_hooks(0x10773, self.records, code + 'try_fast_unsigned_division(s)')
        original = generate_page(0x10773, self.records, {r['address'] for r in self.records},
                                userops=self.userops, compiler_arithmetic_hooks=False)
        self.assertNotIn('try_modern_compiler_arithmetic', original)
        self.assertIn('try_fast_unsigned_division(s)', original)

    def test_reference_cannot_silently_omit_an_entry(self):
        for start, end, _, _ in CONTRACTS:
            with self.subTest(entry=hex(start)), self.assertRaisesRegex(ValueError, 'Missing complete'):
                render_reference([r for r in self.records if not start <= r['address'] < end], self.userops)


if __name__ == '__main__':
    unittest.main()
