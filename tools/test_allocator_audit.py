"""Allocator whole-body/page pins, destination hooks and progress-guard negatives."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import sqlite3
import unittest
from unittest.mock import patch

from allocator_audit import (CONTRACTS, ALLOCATOR_ENTRIES, INSTRUCTION_COUNTS,
    require_allocator_contract, require_allocator_bodies, require_allocator_edges,
    require_allocator_index, require_allocator_hooks)
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page, operation

INDEX = Path(__file__).resolve().parent.parent / '.local/recomp-input/instructions.sqlite'


class AllocatorManifestTests(unittest.TestCase):
    def test_exact_prior_100_and_27_ordered_entries(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 946)
        self.assertEqual(len({a for a, _ in SEMANTIC_REPLACEMENTS}), 946)
        self.assertEqual(SEMANTIC_REPLACEMENTS[100:127], tuple((e, 'allocator_adapter') for e in ALLOCATOR_ENTRIES))
        self.assertEqual(ALLOCATOR_ENTRIES, tuple(sorted(ALLOCATOR_ENTRIES)))
        self.assertEqual(len(CONTRACTS), 27)
        self.assertEqual(sum(INSTRUCTION_COUNTS), 1057)
        prior = [{'entry': f'0x{entry:x}', 'module': module} for entry, module in SEMANTIC_REPLACEMENTS[:100]]
        self.assertEqual(hashlib.sha256(json.dumps(prior, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         'dee0bb555af0657479f3defa0e8a2b763aabd5717e716a213d9c25d8c75004b3')


@unittest.skipUnless(INDEX.exists(), 'Private Mac export unavailable')
class AllocatorAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect(INDEX.resolve().as_uri()+'?mode=ro', uri=True)) as db:
            cls.records = [json.loads(row[0]) for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                (0x10774000, 0x10776000))]
        cls.pages = {page: [r for r in cls.records if r['address'] >> 12 == page] for page in (0x10774, 0x10775)}
        cls.codes = {page: generate_page(page, records, {r['address'] for r in records})
                     for page, records in cls.pages.items()}

    def test_complete_bodies_fragments_and_destination_hooks(self):
        require_allocator_bodies(self.records, complete=True)
        for (start, end, _), count in zip(CONTRACTS, INSTRUCTION_COUNTS):
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(len({r['address'] for r in body}), count)
            self.assertEqual(body[0]['address'], start)
            self.assertEqual(body[-1]['address'], end-4)
            self.assertEqual(body[-1]['assembly'], 'blr')
            for page in range(start >> 12, ((end-1) >> 12)+1):
                require_allocator_contract(page, self.pages[page])
            code = self.codes[start >> 12]
            fragment = code.split(f'i_{start:x}:', 1)[1].split(f'i_{start+4:x}:', 1)[0]
            hook = f'if (try_modern_allocator(s, 0x{start:x})) return true;'
            self.assertIn('#include "allocator_adapter.hpp"', code)
            self.assertIn(f'case 0x{start:x}: goto i_{start:x};', code)
            self.assertEqual(fragment.count(hook), 1)
            self.assertLess(fragment.index(hook), fragment.index(f's.step(0x{start:x});'))
            for kind in ('CALL', 'BRANCH'):
                self.assertEqual(operation([kind, None, ['ram', str(start), 4]], start-4, 0, start >> 12, {start}, 1), f'goto i_{start:x};')
                self.assertEqual(operation([kind, None, ['ram', str(start), 4]], start-4, 0, (start >> 12)+1, {start}, 1), f's.pc = 0x{start:x}; return true;')
            for kind in ('CALLIND', 'BRANCHIND'):
                self.assertEqual(operation([kind, None, ['register', '4200', 4]], start-4, 0, start >> 12, {start}, 1), 's.pc = std::uint32_t(s.r<4>(4200)); return true;')
        self.assertEqual(sum(0x10774f40 <= r['address'] < 0x10775000 for r in self.records), 48)
        self.assertEqual(sum(0x10775000 <= r['address'] < 0x107750e0 for r in self.records), 56)

    def test_changed_partial_duplicate_and_missing_fragments_reject(self):
        for start, end, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for victim in (start, start+4, end-4):
                partial = [r for r in body if r['address'] != victim]
                with self.assertRaisesRegex(ValueError, 'allocator contract changed'):
                    require_allocator_bodies(partial)
                page = victim >> 12
                with self.assertRaisesRegex(ValueError, 'allocator contract changed'):
                    require_allocator_contract(page, [r for r in partial if r['address'] >> 12 == page])
            for field, value in (('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1), ('address', start-4)):
                changed = deepcopy(body); changed[0][field] = value
                with self.assertRaisesRegex(ValueError, 'allocator contract changed'):
                    require_allocator_bodies(changed)
            with self.assertRaisesRegex(ValueError, 'allocator contract changed'):
                require_allocator_bodies(body + [deepcopy(body[0])])
            if start == 0x10774f40:
                for page in (0x10774, 0x10775):
                    with self.assertRaisesRegex(ValueError, 'allocator contract changed'):
                        require_allocator_bodies([r for r in body if r['address'] >> 12 == page])

    def test_missing_bodies_cannot_emit_full_manifest(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            for start, end, _ in CONTRACTS:
                db.execute('DELETE FROM instructions')
                db.executemany('INSERT INTO instructions VALUES (?,?)',
                    ((r['address'], json.dumps(r)) for r in self.records if not start <= r['address'] < end))
                with self.assertRaisesRegex(ValueError, 'Missing complete Mac allocator body'):
                    require_allocator_index(db)
            for page in (0x10774, 0x10775):
                db.execute('DELETE FROM instructions')
                db.executemany('INSERT INTO instructions VALUES (?,?)',
                    ((r['address'], json.dumps(r)) for r in self.records if r['address'] >> 12 != page))
                with self.assertRaises(ValueError): require_allocator_index(db)

    def test_all_edges_and_discriminating_interior_negatives(self):
        with closing(sqlite3.connect(INDEX.resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_allocator_index(db)
        for start, end, _ in CONTRACTS:
            for target in (start+4, end-4):
                with self.assertRaisesRegex(ValueError, 'External allocator interior edge'):
                    require_allocator_edges([(start-4, target)])
            require_allocator_edges([(start-4, start), (start, start+4)])

    def test_missing_duplicate_and_misplaced_hooks_reject(self):
        for start in ALLOCATOR_ENTRIES:
            page = start >> 12; code = self.codes[page]
            hook = f'if (try_modern_allocator(s, 0x{start:x})) return true;'
            for changed in (code.replace(hook, ''), code+hook,
                            code.replace(hook, '').replace(f's.step(0x{start:x});', f's.step(0x{start:x});\n'+hook)):
                with self.assertRaisesRegex(ValueError, 'allocator hook'):
                    require_allocator_hooks(page, self.pages[page], changed)
        for page, records in self.pages.items():
            code = generate_page(page, records, {r['address'] for r in records}, local_temporaries=False)
            require_allocator_hooks(page, records, code)

    def test_original_guard_still_has_its_independent_branch_audit(self):
        self.assertIn('next_pool_attempt(s.r<4>(100), divide_unsigned(s.r<4>(16), s.r<4>(0)))', self.codes[0x10774])
        for victim in (0x10774c00, 0x10774c10, 0x10774c24, 0x10774c34):
            records = deepcopy(self.pages[0x10774])
            next(r for r in records if r['address'] == victim)['assembly'] = 'changed'
            # Disable only the raw-body pin in this test to independently
            # exercise the unchanged historical progress-branch audit.
            with patch('compile_recomp.require_allocator_contract'):
                with self.assertRaisesRegex(ValueError, 'audited failed-allocation loop'):
                    generate_page(0x10774, records, {r['address'] for r in records})
        for entry in (0x10773fb0, 0x10773ff0, 0x10774060, 0x10775240, 0x10778d50, 0x10778d80):
            self.assertNotIn(entry, ALLOCATOR_ENTRIES)


if __name__ == '__main__':
    unittest.main()
