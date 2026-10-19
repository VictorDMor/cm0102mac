"""Complete context/score selection boundaries."""
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
from make_match_selection_support_reference import render_reference
from match_selection_support_audit import (
    CONTRACTS, FRAGMENTS, MATCH_SELECTION_SUPPORT_ENTRIES, require_match_selection_support_bodies,
    require_match_selection_support_contract, require_match_selection_support_edges,
    require_match_selection_support_index, require_match_selection_support_hooks, match_selection_support_hook,
)

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class ScopeTests(unittest.TestCase):
    def test_complete_three_root_integration_preserves923_prefix(self):
        self.assertEqual(tuple(v[2] for v in CONTRACTS), (257, 93, 50))
        self.assertEqual(sum(v[2] for v in FRAGMENTS), 400)
        self.assertEqual(MATCH_SELECTION_SUPPORT_ENTRIES, (0x101e1640, 0x101e1a50, 0x102875f0))
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 958)
        self.assertEqual(SEMANTIC_REPLACEMENTS[923:926], tuple((e, 'match_selection_support_adapter') for e in MATCH_SELECTION_SUPPORT_ENTRIES))
        prefix = [{'entry': hex(e), 'module': m} for e, m in SEMANTIC_REPLACEMENTS[:923]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '60ffe80b3969b723352005c8af3ac2a46646e1e01ce1c27ba73fd15eecb4952c')

    def test_duplicate_owners_fail(self):
        for entry in MATCH_SELECTION_SUPPORT_ENTRIES:
            with self.assertRaises(ValueError):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS, (entry, 'duplicate')))

    def test_external_interior_edges_fail_closed(self):
        for start, end, *_ in CONTRACTS:
            require_match_selection_support_edges([(0, start), (start, end - 4), (end - 4, end)])
            for target in (start + 4, end - 4):
                with self.subTest(root=hex(start), target=hex(target)), self.assertRaises(ValueError):
                    require_match_selection_support_edges([(0, target)])

    def test_named_complete_bodies_have_no_original_dispatch(self):
        for name in ('context', 'mode', 'score'):
            source = (ROOT / f'native/match_selection_support_{name}.cpp').read_text()
            for forbidden in ('match_selection_support_reference', 'dispatch_page', 'run_until', 'pcode', 'goto ', 'switch (', 's.pc'):
                self.assertNotIn(forbidden, source)
        cmake = (ROOT / 'native/CMakeLists.txt').read_text()
        self.assertIn('add_executable(cm-match-selection-support-checks match_selection_support_checks.cpp', cmake)


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_match_selection_support_index(db)
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
                    require_match_selection_support_bodies(changed, complete=True)

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
        self.assertEqual(re.findall(r'match_selection_support_step_seen\(s, 0x([0-9a-f]+)\)', source), expected)
        for forbidden in ('try_modern_', 'complete_context_transfer'):
            self.assertNotIn(forbidden, source)

    def test_page_fragments_reject_partial_input(self):
        selected = {v['address']: v for v in self.records}
        for _, page, *_ in FRAGMENTS:
            rows = [v for v in self.records if v['address'] >> 12 == page]
            require_match_selection_support_contract(page, rows, selected)
            with self.subTest(page=hex(page)), self.assertRaises(ValueError):
                require_match_selection_support_contract(page, rows[1:], selected)

    def test_hooks_precede_accounting_and_reject_missing_duplicate_or_late_hook(self):
        disabled = {n: False for n in inspect.signature(generate_page).parameters if n.endswith('_hooks') or n == 'guest_context_transfers'}
        disabled['match_selection_support_hooks'] = True
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows = [v for v in self.records if v['address'] >> 12 == page]
            code = generate_page(page, rows, {v['address'] for v in self.records}, userops=self.userops, **disabled)
            require_match_selection_support_hooks(page, rows, code)
            for entry in MATCH_SELECTION_SUPPORT_ENTRIES:
                if entry >> 12 != page: continue
                hook = match_selection_support_hook(entry); step = f's.step(0x{entry:x});'
                for changed in (code.replace(hook, '', 1), code.replace(hook, hook + '\n' + hook, 1), code.replace(hook, '', 1).replace(step, step + '\n' + hook, 1)):
                    with self.assertRaises(ValueError): require_match_selection_support_hooks(page, rows, changed)

    def test_index_extrema_include_complete_block(self):
        # Scan both extrema, including injected external interior entrances.
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            db.executemany('INSERT INTO instructions VALUES(?, ?)', [(v['address'], json.dumps(v)) for v in self.records])
            for target in (min(v[0] for v in CONTRACTS) + 4, max(v[1] for v in CONTRACTS) - 4):
                record = dict(address=0, assembly='external', ops=[], flows=[target], fallthrough=None)
                db.execute('INSERT OR REPLACE INTO instructions VALUES(0, ?)', (json.dumps(record),))
                with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                    require_match_selection_support_index(db)


if __name__ == '__main__':
    unittest.main()
