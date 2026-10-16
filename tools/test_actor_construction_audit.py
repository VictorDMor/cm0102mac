"""Pinned actor-construction boundaries, original references and additive production hooks."""
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
from actor_construction_audit import (CONTRACTS, FRAGMENTS, ACTOR_CONSTRUCTION_ENTRIES, require_actor_construction_bodies,
                                 require_actor_construction_edges, require_actor_construction_index,
                                 require_actor_construction_contract, require_actor_construction_hooks, actor_construction_hook)
from make_actor_construction_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_ACTOR_CONSTRUCTION_INPUT', ROOT / '.local/compiler-arithmetic-reconstruction/input'))


class ScopeTests(unittest.TestCase):
    def test_complete_scope_and_preserved_604_entry_prefix(self):
        self.assertEqual(len(CONTRACTS), 2)
        self.assertEqual(sum(n for _, _, n, _ in CONTRACTS), 2778)
        self.assertEqual(ACTOR_CONSTRUCTION_ENTRIES, tuple(sorted(set(ACTOR_CONSTRUCTION_ENTRIES))))
        baseline = [{'entry': f'0x{a:x}', 'module': m} for a, m in SEMANTIC_REPLACEMENTS]
        self.assertEqual(len(baseline), 682)
        self.assertEqual(len({a for a, _ in SEMANTIC_REPLACEMENTS}), 682)
        self.assertEqual(hashlib.sha256(json.dumps(baseline[:604], sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '1e4935c4e56744dd4024373da1a601be905d5bd79540ea70241b93ac830163b6')
        self.assertEqual(SEMANTIC_REPLACEMENTS[604:606], tuple((a, 'actor_construction_adapter') for a in ACTOR_CONSTRUCTION_ENTRIES))

    def test_only_roots_are_exterior_entrances(self):
        for start, end, _, _ in CONTRACTS:
            require_actor_construction_edges([(0, start), (start, end - 4)])
            for target in range(start + 4, end, 4):
                with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                    require_actor_construction_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private actor-construction source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = [json.loads(r[0]) for start, end, _, _ in CONTRACTS for r in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_complete_index_and_interior_edges(self):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_actor_construction_index(db)

    def test_every_pin_rejects_changed_fields(self):
        for entry in ACTOR_CONSTRUCTION_ENTRIES:
            index = next(n for n, r in enumerate(self.records) if r['address'] == entry)
            for key, value in (('address', entry + 1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                bad = deepcopy(self.records); bad[index][key] = value
                with self.subTest(entry=hex(entry), key=key), self.assertRaises(ValueError):
                    require_actor_construction_bodies(bad, complete=True)

    def test_missing_duplicate_reordered_or_wholly_absent_body_fails(self):
        for start, end, _, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for bad in (body[1:], body[:-1], body + [body[0]], list(reversed(body))):
                with self.assertRaises(ValueError): require_actor_construction_bodies(bad)
            with self.assertRaises(ValueError):
                render_reference([r for r in self.records if not start <= r['address'] < end], self.userops)
            with self.assertRaises(ValueError):
                require_actor_construction_contract(start >> 12, [], {start})
            with self.assertRaises(ValueError):
                require_actor_construction_contract(start >> 12, body[1:])
        for start, page, count, _ in FRAGMENTS:
            end = next(end for entry, end, _, _ in CONTRACTS if entry == start)
            part = [r for r in self.records if start <= r['address'] < end and r['address'] >> 12 == page]
            self.assertEqual(len(part), count)
            require_actor_construction_contract(page, part)
            for bad in (part[1:], part[:-1], part + [part[0]], list(reversed(part))):
                with self.subTest(page=hex(page)), self.assertRaises(ValueError):
                    require_actor_construction_contract(page, bad)

    def test_reference_has_exactly_2778_original_labels(self):
        code = render_reference(self.records, self.userops)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE), [f'{r["address"]:x}' for r in self.records])
        for entry, page, _, _ in FRAGMENTS:
            self.assertIn(f'bool actor_construction_reference_{entry:x}_{page:x}(State& s)', code)
        for token in ('try_modern_', 'actor_construction::', 'complete_context_transfer', 'bool page_10000(', 'bool page_1076e('):
            self.assertNotIn(token, code)

    def test_generated_hooks_precede_original_operations_at_every_root(self):
        for page in tuple(sorted({f[1] for f in FRAGMENTS})):
            records = [r for r in self.records if r['address'] >> 12 == page]
            code = generate_page(page, records, {r['address'] for r in self.records}, userops=self.userops)
            require_actor_construction_hooks(page, records, code)
            for entry in (a for a in ACTOR_CONSTRUCTION_ENTRIES if a >> 12 == page):
                hook = actor_construction_hook(entry)
                step = f'counter.step(0x{entry:x});' if f'counter.step(0x{entry:x});' in code else f's.step(0x{entry:x});'
                for broken in (code.replace(hook, ''), code.replace(hook, hook + '\n' + hook),
                               code.replace(hook + '\n' + step, step + '\n' + hook)):
                    with self.assertRaises(ValueError): require_actor_construction_hooks(page, records, broken)


if __name__ == '__main__': unittest.main()
