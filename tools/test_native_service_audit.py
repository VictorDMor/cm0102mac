"""Pinned facade boundaries, original references and additive production hooks."""
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
from native_service_audit import (CONTRACTS, NATIVE_SERVICE_ENTRIES, require_native_service_bodies,
                                 require_native_service_edges, require_native_service_index,
                                 require_native_service_contract, require_native_service_hooks, native_service_hook)
from make_native_service_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_NATIVE_SERVICE_INPUT', ROOT / '.local/compiler-arithmetic-reconstruction/input'))


class ScopeTests(unittest.TestCase):
    def test_complete_scope_and_preserved_536_entry_prefix(self):
        self.assertEqual(len(CONTRACTS), 3)
        self.assertEqual(sum(n for _, _, n, _ in CONTRACTS), 27)
        self.assertEqual(NATIVE_SERVICE_ENTRIES, tuple(sorted(set(NATIVE_SERVICE_ENTRIES))))
        baseline = [{'entry': f'0x{a:x}', 'module': m} for a, m in SEMANTIC_REPLACEMENTS]
        self.assertEqual(len(baseline), 704)
        self.assertEqual(len({a for a, _ in SEMANTIC_REPLACEMENTS}), 704)
        self.assertEqual(hashlib.sha256(json.dumps(baseline[:536], sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         'c4f5be3bde9d551349be581de51abab54a2c88d8f7a4ed6375f75b490aa2e2c4')
        self.assertEqual(SEMANTIC_REPLACEMENTS[536:539], tuple((a, 'native_service_adapter') for a in NATIVE_SERVICE_ENTRIES))

    def test_only_roots_are_exterior_entrances(self):
        for start, end, _, _ in CONTRACTS:
            require_native_service_edges([(0, start), (start, end - 4)])
            for target in range(start + 4, end, 4):
                with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                    require_native_service_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private facade source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = [json.loads(r[0]) for start, end, _, _ in CONTRACTS for r in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_complete_index_and_interior_edges(self):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_native_service_index(db)

    def test_every_pin_rejects_changed_fields(self):
        for entry in NATIVE_SERVICE_ENTRIES:
            index = next(n for n, r in enumerate(self.records) if r['address'] == entry)
            for key, value in (('address', entry + 1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                bad = deepcopy(self.records); bad[index][key] = value
                with self.subTest(entry=hex(entry), key=key), self.assertRaises(ValueError):
                    require_native_service_bodies(bad, complete=True)

    def test_missing_duplicate_reordered_or_wholly_absent_body_fails(self):
        for start, end, _, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for bad in (body[1:], body[:-1], body + [body[0]], list(reversed(body))):
                with self.assertRaises(ValueError): require_native_service_bodies(bad)
            with self.assertRaises(ValueError):
                render_reference([r for r in self.records if not start <= r['address'] < end], self.userops)
            with self.assertRaises(ValueError):
                require_native_service_contract(start >> 12, [], {start})
            with self.assertRaises(ValueError):
                require_native_service_contract(start >> 12, body[1:])

    def test_reference_has_exactly_27_original_labels(self):
        code = render_reference(self.records, self.userops)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE), [f'{r["address"]:x}' for r in self.records])
        for token in ('try_modern_', 'native_service::', 'complete_context_transfer', 'bool page_10770(', 'bool page_10778('):
            self.assertNotIn(token, code)

    def test_generated_hooks_precede_original_operations_at_every_root(self):
        for page in (0x10770, 0x10778):
            records = [r for r in self.records if r['address'] >> 12 == page]
            code = generate_page(page, records, {r['address'] for r in self.records}, userops=self.userops)
            require_native_service_hooks(page, records, code)
            for entry in (a for a in NATIVE_SERVICE_ENTRIES if a >> 12 == page):
                hook, step = native_service_hook(entry), f's.step(0x{entry:x});'
                for broken in (code.replace(hook, ''), code.replace(hook, hook + '\n' + hook),
                               code.replace(hook + '\n' + step, step + '\n' + hook)):
                    with self.assertRaises(ValueError): require_native_service_hooks(page, records, broken)


if __name__ == '__main__': unittest.main()
