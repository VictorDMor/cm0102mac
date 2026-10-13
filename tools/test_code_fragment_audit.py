"""Mac code-fragment body, fragment and interior-flow negatives."""
from contextlib import closing
from copy import deepcopy
from pathlib import Path
import json
import hashlib
import sqlite3
import unittest

from code_fragment_audit import (CONTRACTS, CODE_FRAGMENT_ENTRIES, INSTRUCTION_COUNTS,
    PAGE_FRAGMENTS, require_code_fragment_contract, require_code_fragment_bodies,
    require_code_fragment_edges, require_code_fragment_index, require_code_fragment_hooks)
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page

INDEX = Path(__file__).resolve().parent.parent / '.local/recomp-input/instructions.sqlite'


class ScopeTests(unittest.TestCase):
    def test_exact_coherent_metadata_scope(self):
        self.assertEqual(len(CONTRACTS), 5)
        self.assertEqual(CODE_FRAGMENT_ENTRIES, tuple(sorted(set(CODE_FRAGMENT_ENTRIES))))
        self.assertEqual(sum(INSTRUCTION_COUNTS), 301)
        self.assertEqual(PAGE_FRAGMENTS, {})
        for entry in (0x10771640, 0x10773710, 0x10773eb0, 0x10773f30, 0x10778d50, 0x10778d80):
            self.assertNotIn(entry, CODE_FRAGMENT_ENTRIES)

    def test_exact_prior_188_and_five_production_entries(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 663)
        self.assertEqual(len({entry for entry, _ in SEMANTIC_REPLACEMENTS}), 663)
        self.assertEqual(SEMANTIC_REPLACEMENTS[188:193], tuple((e, 'code_fragment_adapter') for e in CODE_FRAGMENT_ENTRIES))
        prior = [{'entry': f'0x{entry:x}', 'module': module} for entry, module in SEMANTIC_REPLACEMENTS[:188]]
        self.assertEqual(hashlib.sha256(json.dumps(prior, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '71685ebd95d1c0fc78ebeaa5ec1d9da103cc49027e1b75cca68860c93cc976af')


@unittest.skipUnless(INDEX.exists(), 'Private Mac export unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect(INDEX.resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = [json.loads(row[0]) for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                (CONTRACTS[0][0], CONTRACTS[-1][1]))]

    def test_all_complete_bodies_and_page_fragments(self):
        require_code_fragment_bodies(self.records, complete=True)
        for (start, end, _), count in zip(CONTRACTS, INSTRUCTION_COUNTS):
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(records), count)
            self.assertEqual(records[0]['address'], start)
            self.assertEqual(records[-1]['address'], end - 4)
            self.assertEqual(records[-1]['assembly'], 'blr')
            for page in range(start >> 12, ((end - 1) >> 12) + 1):
                require_code_fragment_contract(page, [r for r in self.records if r['address'] >> 12 == page])

    def test_changed_fields_missing_and_duplicate_instructions_reject(self):
        for start, end, _ in CONTRACTS:
            records = [r for r in self.records if start <= r['address'] < end]
            for key, value in (('address', start - 4), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(records); changed[0][key] = value
                with self.assertRaises(ValueError): require_code_fragment_bodies(changed)
            for victim in (start, start + 4, end - 4):
                changed = [r for r in records if r['address'] != victim]
                with self.assertRaises(ValueError): require_code_fragment_bodies(changed)
                with self.assertRaises(ValueError): require_code_fragment_contract(victim >> 12, [r for r in changed if r['address'] >> 12 == victim >> 12])
            with self.assertRaises(ValueError): require_code_fragment_bodies(records + [records[0]])

    def test_completely_missing_bodies_and_fragments_reject(self):
        for start, end, _ in CONTRACTS:
            with self.assertRaisesRegex(ValueError, 'Missing complete'):
                require_code_fragment_bodies([r for r in self.records if not start <= r['address'] < end], complete=True)
        for (start, page), _ in PAGE_FRAGMENTS.items():
            with self.assertRaises(ValueError):
                require_code_fragment_bodies([r for r in self.records if r['address'] >> 12 != page], complete=True)

    def test_actual_index_and_external_interior_flow_negatives(self):
        with closing(sqlite3.connect(INDEX.resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_code_fragment_index(db)
        for start, end, _ in CONTRACTS:
            for target in (start + 4, end - 4):
                with self.assertRaisesRegex(ValueError, 'External code_fragment interior edge'):
                    require_code_fragment_edges([(start - 4, target)])
            require_code_fragment_edges([(start - 4, start), (start, start + 4)])

    def test_destination_hooks_and_missing_duplicate_misplaced_negatives(self):
        for page in (0x10771,):
            records = [r for r in self.records if r['address'] >> 12 == page]
            for local in (True, False):
                code = generate_page(page, records, {r['address'] for r in records}, local_temporaries=local)
                require_code_fragment_hooks(page, records, code)
                self.assertIn('#include "code_fragment_adapter.hpp"', code)
                for entry in CODE_FRAGMENT_ENTRIES:
                    if entry >> 12 != page: continue
                    hook = f'if (try_modern_code_fragment(s, 0x{entry:x})) return true;'
                    step = f'counter.step(0x{entry:x});' if f'counter.step(0x{entry:x});' in code else f's.step(0x{entry:x});'
                    self.assertIn(f'case 0x{entry:x}: goto i_{entry:x};', code)
                    for bad in (code.replace(hook, ''), code + hook,
                                code.replace(hook, '').replace(step, step + '\n' + hook)):
                        with self.assertRaisesRegex(ValueError, 'code_fragment hook'):
                            require_code_fragment_hooks(page, records, bad)


if __name__ == '__main__':
    unittest.main()
