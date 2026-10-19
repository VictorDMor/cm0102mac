"""Complete match/general range sampler components and original boundaries."""
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
from make_range_sampling_reference import render_reference
from range_sampling_audit import (
    CONTRACTS, FRAGMENTS, RANGE_SAMPLING_ENTRIES, require_range_sampling_bodies,
    require_range_sampling_contract, require_range_sampling_edges,
    require_range_sampling_index, require_range_sampling_hooks, range_sampling_hook,
)

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class ScopeTests(unittest.TestCase):
    def test_complete_two_root_integration_preserves909_prefix(self):
        self.assertEqual(tuple(v[2] for v in CONTRACTS), (753, 350))
        self.assertEqual(sum(v[2] for v in FRAGMENTS), 1103)
        self.assertEqual(RANGE_SAMPLING_ENTRIES, (0x10226520, 0x107124e0))
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 953)
        self.assertEqual(SEMANTIC_REPLACEMENTS[909:911], tuple((e, 'range_sampling_adapter') for e in RANGE_SAMPLING_ENTRIES))
        prefix = [{'entry': hex(e), 'module': m} for e, m in SEMANTIC_REPLACEMENTS[:909]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         'b5ac42f401f843ef9a5863056657e4a1d0a6c4415e1837c0eec8d55cfe608276')

    def test_duplicate_owners_fail(self):
        for entry in RANGE_SAMPLING_ENTRIES:
            with self.assertRaises(ValueError):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS, (entry, 'duplicate')))

    def test_external_interior_edges_fail_closed(self):
        for start, end, *_ in CONTRACTS:
            require_range_sampling_edges([(0, start), (start, end - 4), (end - 4, end)])
            for target in (start + 4, end - 4):
                with self.subTest(root=hex(start), target=hex(target)), self.assertRaises(ValueError):
                    require_range_sampling_edges([(0, target)])

    def test_named_complete_bodies_have_no_original_dispatch(self):
        for name in ('match', 'general'):
            source = (ROOT / f'native/range_sampling_{name}.cpp').read_text()
            for forbidden in ('range_sampling_reference', 'dispatch_page', 'run_until', 'pcode', 'goto ', 'switch (', 'while ('):
                self.assertNotIn(forbidden, source)
            for operation in ('advance', 'scaled', 'signed', 'first', 'second', 'third'):
                self.assertIn(operation + '_', source)
        cmake = (ROOT / 'native/CMakeLists.txt').read_text()
        self.assertIn('add_executable(cm-range-sampling-checks range_sampling_checks.cpp', cmake)


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_range_sampling_index(db)
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
                    require_range_sampling_bodies(changed, complete=True)

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
        self.assertEqual(re.findall(r'range_sampling_step_seen\(s, 0x([0-9a-f]+)\)', source), expected)
        for forbidden in ('try_modern_', 'complete_context_transfer'):
            self.assertNotIn(forbidden, source)

    def test_page_fragments_reject_partial_input(self):
        selected = {v['address']: v for v in self.records}
        for _, page, *_ in FRAGMENTS:
            rows = [v for v in self.records if v['address'] >> 12 == page]
            require_range_sampling_contract(page, rows, selected)
            with self.subTest(page=hex(page)), self.assertRaises(ValueError):
                require_range_sampling_contract(page, rows[1:], selected)

    def test_hooks_precede_accounting_and_fail_on_missing_duplicate_or_late_hook(self):
        disabled = {n: False for n in inspect.signature(generate_page).parameters if n.endswith('_hooks') or n == 'guest_context_transfers'}
        disabled['range_sampling_hooks'] = True
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows = [v for v in self.records if v['address'] >> 12 == page]
            code = generate_page(page, rows, {v['address'] for v in self.records}, userops=self.userops, **disabled)
            require_range_sampling_hooks(page, rows, code)
            for entry in RANGE_SAMPLING_ENTRIES:
                if entry >> 12 != page: continue
                hook = range_sampling_hook(entry); step = f's.step(0x{entry:x});'
                for changed in (code.replace(hook, '', 1), code.replace(hook, hook + '\n' + hook, 1), code.replace(hook, '', 1).replace(step, step + '\n' + hook, 1)):
                    with self.assertRaises(ValueError): require_range_sampling_hooks(page, rows, changed)

    def test_explicit_branch_mutations_are_pinned_to_original_conditionals(self):
        source = (ROOT / 'native/range_sampling_checks.cpp').read_text()
        line = next(v for v in source.splitlines() if 'constexpr std::array branch_sites' in v)
        actual = [int(v, 16) for v in re.findall(r'0x([0-9a-f]+)u', line)]
        expected = [v['address'] for v in self.records if v['assembly'].split()[0] in ('bne', 'ble', 'bgt', 'bge')]
        self.assertEqual(actual, expected)


if __name__ == '__main__':
    unittest.main()
