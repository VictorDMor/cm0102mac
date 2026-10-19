"""Complete original contracts and prepared ownership for staff contract queries."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import unittest
from unittest.mock import patch
from compile_recomp import SEMANTIC_REPLACEMENTS, require_unique_semantic_replacements, generate_page
from contract_query_audit import (CONTRACTS, FRAGMENTS, CONTRACT_QUERY_ENTRIES,
    require_contract_query_bodies, require_contract_query_contract,
    require_contract_query_edges, require_contract_query_index, require_contract_query_hooks, contract_query_hook)
from make_contract_query_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_exact_accepted_prefix_and_complete_candidate_owner(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 923)
        self.assertEqual(CONTRACT_QUERY_ENTRIES, (0x10082f20, 0x10082f90, 0x10083110))
        prefix = [{'entry': f'0x{entry:x}', 'module': owner} for entry, owner in SEMANTIC_REPLACEMENTS[:756]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '227fca872a4b07455474488cab1d1d46728cd5bed1cbc2aa82e71ca0b3cb98f9')
        self.assertEqual(SEMANTIC_REPLACEMENTS[756:759], tuple((entry, 'contract_query_adapter') for entry in CONTRACT_QUERY_ENTRIES))
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry in CONTRACT_QUERY_ENTRIES:
            with self.subTest(entry=hex(entry)), self.assertRaisesRegex(ValueError, 'Duplicate semantic replacement'):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS, (entry, 'other_owner')))

    def test_complete_boundaries_and_external_interior_edges(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS), 145)
        self.assertEqual(sum(row[2] for row in FRAGMENTS), 145)
        for start, end, *_ in CONTRACTS:
            require_contract_query_edges([(0, start), (start, end-4), (end-4, end)])
            for target in (start+4, end-4):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_contract_query_edges([(0, target)])

    def test_named_queries_do_not_replay_original_bodies(self):
        source = (ROOT / 'native/contract_queries.cpp').read_text()
        for forbidden in ('contract_query_reference', 'dispatch_page', 'run_until', 'pcode', 'i_1008'):
            self.assertNotIn(forbidden, source)
        for name in ('find_primary', 'find_for_club', 'find_secondary', 'select_club_contract'):
            self.assertIn(name, source)


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_contract_query_index(db)
            cls.records = [json.loads(row[0]) for start, end, *_ in CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_changed_original_fields_are_rejected(self):
        for start, page, *_ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page and row['address'] >= start)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records); changed[index][key] = value
                with self.subTest(entry=hex(start), page=hex(page), key=key), self.assertRaises(ValueError):
                    require_contract_query_bodies(changed, complete=True)

    def test_missing_duplicate_reordered_and_absent_bodies_fail(self):
        for start, end, *_ in CONTRACTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] == start)
            for changed in (self.records[:index]+self.records[index+1:], self.records+[self.records[index]],
                            list(reversed(self.records)), [row for row in self.records if not start <= row['address'] < end]):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    render_reference(changed, self.userops)

    def test_partial_pages_fail(self):
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows = [row for row in self.records if row['address'] >> 12 == page]
            selected = {row['address'] for row in self.records}
            require_contract_query_contract(page, rows, selected)
            for start, end, *_ in CONTRACTS:
                if start >> 12 <= page <= (end-1) >> 12:
                    with self.assertRaises(ValueError):
                        require_contract_query_contract(page, [row for row in rows if not start <= row['address'] < end], {start})

    def test_productive_hooks_precede_original_accounting(self):
        import inspect
        disabled = {name: False for name in inspect.signature(generate_page).parameters
                    if name.endswith('_hooks') or name == 'guest_context_transfers'}
        disabled['contract_query_hooks'] = True
        for page in sorted({r['address'] >> 12 for r in self.records}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            selected = {r['address'] for r in self.records}
            code = generate_page(page, part, selected, userops=self.userops, **disabled)
            require_contract_query_hooks(page, part, code)
            for entry in CONTRACT_QUERY_ENTRIES:
                if entry >> 12 != page: continue
                hook = contract_query_hook(entry)
                step = f's.step(0x{entry:x});'
                for changed in (code.replace(hook, '', 1),
                                code.replace(hook, '', 1).replace(step, step+'\n'+hook, 1),
                                code.replace(hook, hook+'\n'+hook, 1)):
                    with self.subTest(entry=hex(entry)), self.assertRaises(ValueError):
                        require_contract_query_hooks(page, part, changed)

    def test_reference_retains_all_steps_without_semantic_hooks(self):
        source = render_reference(self.records, self.userops)
        expected = [f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', source, re.MULTILINE), expected)
        self.assertEqual(re.findall(r'contract_query_step_seen\(s, 0x([0-9a-f]+)\)', source), expected)
        self.assertNotIn('try_modern_', source)
        self.assertNotIn('complete_context_transfer', source)

    def test_reversed_contract_index_rejects_highest_interior_edge(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            db.executemany('INSERT INTO instructions VALUES (?,?)',
                           [(row['address'], json.dumps(row)) for row in self.records])
            edge = {'address': 0, 'flows': [CONTRACT_QUERY_ENTRIES[-1]+4], 'fallthrough': None}
            db.execute('INSERT INTO instructions VALUES (?,?)', (0, json.dumps(edge)))
            with patch('contract_query_audit.CONTRACTS', tuple(reversed(CONTRACTS))):
                with self.assertRaisesRegex(ValueError, 'External contract-query interior edge'):
                    require_contract_query_index(db)


if __name__ == '__main__':
    unittest.main()
