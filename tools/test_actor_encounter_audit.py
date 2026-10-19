"""Complete encounter response and score boundaries."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import unittest

from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page, require_unique_semantic_replacements
import inspect
from make_actor_encounter_reference import render_reference
from actor_encounter_audit import (
    CONTRACTS, FRAGMENTS, ACTOR_ENCOUNTER_ENTRIES, require_actor_encounter_bodies,
    require_actor_encounter_contract, require_actor_encounter_edges,
    require_actor_encounter_index, require_actor_encounter_hooks, actor_encounter_hook,
)

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class ScopeTests(unittest.TestCase):
    def test_four_complete_roots_preserve926_prefix(self):
        self.assertEqual(tuple(v[2] for v in CONTRACTS), (35, 44, 1000, 730))
        self.assertEqual(sum(v[2] for v in FRAGMENTS), 1809)
        self.assertEqual(ACTOR_ENCOUNTER_ENTRIES, (0x101d6860, 0x102865a0, 0x10286650, 0x102876c0))
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 953)
        self.assertEqual(SEMANTIC_REPLACEMENTS[926:930], tuple((e, 'actor_encounter_adapter') for e in ACTOR_ENCOUNTER_ENTRIES))
        prefix = [{'entry': hex(e), 'module': m} for e, m in SEMANTIC_REPLACEMENTS[:926]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '65cdd04dcfe25b5ce5290f9cf2962889e08615e3b4d02e99a02309d1c2e895d1')

    def test_duplicate_owners_fail(self):
        for entry in ACTOR_ENCOUNTER_ENTRIES:
            with self.assertRaises(ValueError):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS, (entry, 'duplicate')))

    def test_external_interior_edges_fail_closed(self):
        for start, end, *_ in CONTRACTS:
            require_actor_encounter_edges([(0, start), (start, end - 4), (end - 4, end)])
            for target in (start + 4, end - 4):
                with self.subTest(root=hex(start), target=hex(target)), self.assertRaises(ValueError):
                    require_actor_encounter_edges([(0, target)])

    def test_named_complete_bodies_have_no_original_dispatch(self):
        for name in ('team_scaling', 'actor_scaling', 'response', 'peer'):
            source = (ROOT / f'native/actor_encounter_{name}.cpp').read_text()
            for forbidden in ('actor_encounter_reference', 'dispatch_page', 'run_until', 'pcode', 'goto ', 'switch (', 's.pc'):
                self.assertNotIn(forbidden, source)
        cmake = (ROOT / 'native/CMakeLists.txt').read_text()
        self.assertIn('add_executable(cm-actor-encounter-checks actor_encounter_checks.cpp', cmake)


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_actor_encounter_index(db)
            cls.records = [json.loads(row[0]) for start, end, *_ in CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_source_fields_are_pinned_for_each_fragment(self):
        for entry, page, *_ in FRAGMENTS:
            end = next(v[1] for v in CONTRACTS if v[0] == entry)
            index = next(i for i, row in enumerate(self.records) if entry <= row['address'] < end and row['address'] >> 12 == page)
            for field, value in [('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)]:
                changed = deepcopy(self.records)
                changed[index][field] = value
                with self.subTest(root=hex(entry), page=hex(page), field=field), self.assertRaises(ValueError):
                    require_actor_encounter_bodies(changed, complete=True)

    def test_absent_missing_reordered_and_duplicate_bodies_fail(self):
        for start, end, *_ in CONTRACTS:
            index = next(i for i, row in enumerate(self.records) if row['address'] == start)
            for changed in (self.records[:index] + self.records[index + 1:], self.records + [self.records[index]],
                            list(reversed(self.records)), [v for v in self.records if not start <= v['address'] < end]):
                with self.subTest(root=hex(start)), self.assertRaises(ValueError):
                    render_reference(changed, self.userops)

    def test_every_original_instruction_remains_in_independent_reference(self):
        source = render_reference(self.records, self.userops)
        expected = [f'{v["address"]:x}' for v in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', source, re.M), expected)
        self.assertEqual(re.findall(r'actor_encounter_step_seen\(s, 0x([0-9a-f]+)\)', source), expected)
        for forbidden in ('try_modern_', 'complete_context_transfer'):
            self.assertNotIn(forbidden, source)

    def test_page_fragments_reject_partial_input(self):
        selected = {v['address']: v for v in self.records}
        for _, page, *_ in FRAGMENTS:
            rows = [v for v in self.records if v['address'] >> 12 == page]
            require_actor_encounter_contract(page, rows, selected)
            with self.subTest(page=hex(page)), self.assertRaises(ValueError):
                require_actor_encounter_contract(page, rows[1:], selected)

    def test_hooks_precede_accounting_and_reject_missing_duplicate_or_late_hook(self):
        disabled = {n: False for n in inspect.signature(generate_page).parameters if n.endswith('_hooks') or n == 'guest_context_transfers'}
        disabled['actor_encounter_hooks'] = True
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows = [v for v in self.records if v['address'] >> 12 == page]
            code = generate_page(page, rows, {v['address'] for v in self.records}, userops=self.userops, **disabled)
            require_actor_encounter_hooks(page, rows, code)
            for entry in ACTOR_ENCOUNTER_ENTRIES:
                if entry >> 12 != page: continue
                hook = actor_encounter_hook(entry); step = f's.step(0x{entry:x});'
                for changed in (code.replace(hook, '', 1), code.replace(hook, hook + '\n' + hook, 1), code.replace(hook, '', 1).replace(step, step + '\n' + hook, 1)):
                    with self.assertRaises(ValueError): require_actor_encounter_hooks(page, rows, changed)

    def test_index_extrema_include_complete_block(self):
        # Scan both extrema, including injected external interior entrances.
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            db.executemany('INSERT INTO instructions VALUES(?, ?)', [(v['address'], json.dumps(v)) for v in self.records])
            for target in (min(v[0] for v in CONTRACTS) + 4, max(v[1] for v in CONTRACTS) - 4):
                record = dict(address=0, assembly='external', ops=[], flows=[target], fallthrough=None)
                db.execute('INSERT OR REPLACE INTO instructions VALUES(0, ?)', (json.dumps(record),))
                with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                    require_actor_encounter_index(db)


if __name__ == '__main__':
    unittest.main()
