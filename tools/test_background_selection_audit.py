"""Background selector contracts and hooks at every generated destination."""
from contextlib import closing
from copy import deepcopy
import json
from pathlib import Path
import sqlite3
import unittest

from background_selection_audit import (
    BACKGROUND_SELECTION_ENTRIES, BACKGROUND_SELECTION_ACCESS_INSTRUCTIONS,
    CONTRACTS, require_background_selection_contract)
from calendar_audit import DATE_SUBSYSTEM_ENTRIES
from collection_lookup_audit import COLLECTION_LOOKUP_ENTRIES
from compile_recomp import (CALENDAR_ADAPTER_ENTRIES, SEMANTIC_REPLACEMENTS,
                            HOST_OBSERVED_INSTRUCTIONS, generate_page, operation)
from rng_core_audit import RNG_CORE_ENTRIES

INDEX = Path(__file__).resolve().parent.parent / '.local/current/export/instructions.sqlite'


class BackgroundManifestTests(unittest.TestCase):
    def test_71_entries_preserve_existing_modules(self):
        existing = (
            tuple((entry, 'calendar_adapter') for entry in (
                0x103317f0, 0x10331860, 0x10715b20, 0x10717890,
                0x10717b20, 0x10718130, 0x107182f0, 0x10718360)) +
            tuple((entry, 'date_subsystem_adapter') for entry in DATE_SUBSYSTEM_ENTRIES) +
            tuple((entry, 'rng_core_adapter') for entry in RNG_CORE_ENTRIES))
        self.assertEqual(len(existing), 37)
        self.assertEqual(SEMANTIC_REPLACEMENTS[:37], existing)
        self.assertEqual(SEMANTIC_REPLACEMENTS[37:41], tuple(
            (entry, 'background_selection_adapter') for entry in BACKGROUND_SELECTION_ENTRIES))
        self.assertEqual(SEMANTIC_REPLACEMENTS[41:46], tuple(
            (entry, 'collection_lookup_adapter') for entry in COLLECTION_LOOKUP_ENTRIES))
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 963)
        self.assertEqual(len({entry for entry, _ in SEMANTIC_REPLACEMENTS}), 963)
        self.assertEqual(len(CALENDAR_ADAPTER_ENTRIES) + len(DATE_SUBSYSTEM_ENTRIES), 34)


@unittest.skipUnless(INDEX.exists(), 'Private Mac export unavailable')
class BackgroundAuditTests(unittest.TestCase):
    def records(self, page):
        with closing(sqlite3.connect(INDEX.resolve().as_uri() + '?mode=ro', uri=True)) as db:
            return [json.loads(row[0]) for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                (page << 12, (page + 1) << 12))]

    def test_complete_contracts_and_destination_hooks(self):
        self.assertEqual(BACKGROUND_SELECTION_ENTRIES,
                         (0x107122e0, 0x10712a60, 0x10713960, 0x107139c0))
        for start, end, count in (
                (0x107122e0, 0x107124e0, 128), (0x10712a60, 0x10712c4c, 123),
                (0x10713960, 0x107139b4, 21), (0x107139c0, 0x10713a74, 45)):
            records = self.records(start >> 12)
            self.assertEqual(sum(start <= r['address'] < end for r in records), count)
            require_background_selection_contract(start >> 12, records)
            for local_temporaries in (False, True):
                code = generate_page(start >> 12, records, {r['address'] for r in records},
                                     local_temporaries=local_temporaries)
                self.assertIn('#include "background_selection_adapter.hpp"', code)
                self.assertIn(f'case 0x{start:x}: goto i_{start:x};', code)
                fragment = code.split(f'i_{start:x}:', 1)[1].split(f'i_{start + 4:x}:', 1)[0]
                hook = f'if (try_modern_background_selection(s, 0x{start:x})) return true;'
                self.assertLess(fragment.index(hook), fragment.index(f's.step(0x{start:x});'))
                self.assertEqual(fragment.count(hook), 1)
                # Keeping the original first operation supports a private
                # independent comparison mode when the hook declines an entry.
                self.assertIn('s.wr<4>(0, s.r<4>(4128));', fragment)
            for kind in ('CALL', 'BRANCH'):
                self.assertEqual(operation([kind, None, ['ram', str(start), 4]],
                    start - 4, 0, start >> 12, {start}, 1), f'goto i_{start:x};')
                self.assertEqual(operation([kind, None, ['ram', str(start), 4]],
                    start - 4, 0, (start >> 12) + 1, {start}, 1),
                    f's.pc = 0x{start:x}; return true;')
            for kind in ('CALLIND', 'BRANCHIND'):
                self.assertEqual(operation([kind, None, ['register', '4200', 4]],
                    start - 4, 0, start >> 12, {start}, 1),
                    's.pc = std::uint32_t(s.r<4>(4200)); return true;')

    def test_memory_access_and_getter_fault_pcs_are_exact(self):
        accesses = set()
        for page in {start >> 12 for start, _, _ in CONTRACTS}:
            records = self.records(page)
            accesses.update(r['address'] for r in records
                            if any(start <= r['address'] < end for start, end, _ in CONTRACTS)
                            and any(op[0] in ('LOAD', 'STORE') for op in r['ops']))
            code = generate_page(page, records, {r['address'] for r in records}, block_accounting=True)
            for address in BACKGROUND_SELECTION_ACCESS_INSTRUCTIONS:
                if address >> 12 == page:
                    fragment = code.split(f'i_{address:x}:', 1)[1].split(f'i_{address + 4:x}:', 1)[0]
                    self.assertIn(f's.step(0x{address:x});\n', fragment)
        # Both samplers sign-extend the count to 16 bits before their guards.
        # The legacy FP blocks require a count outside [-65535, 65535], so
        # their 28 memory instructions cannot reach the adapter's access path.
        legacy_fp_accesses = {address for address in accesses if any(
            0x10712380 + offset <= address < 0x10712458 + offset for offset in (0, 0x788))}
        self.assertEqual(len(legacy_fp_accesses), 28)
        self.assertEqual(accesses - legacy_fp_accesses, BACKGROUND_SELECTION_ACCESS_INSTRUCTIONS)
        self.assertTrue((BACKGROUND_SELECTION_ACCESS_INSTRUCTIONS |
                         {0x100d4ea0, 0x100d53d8, 0x100d5084}) <= HOST_OBSERVED_INSTRUCTIONS)

    def test_changed_or_incomplete_bodies_reject(self):
        for start, end, _ in CONTRACTS:
            page = start >> 12
            records = self.records(page)
            for victim in (start, start + 4, end - 4):
                incomplete = [r for r in records if r['address'] != victim]
                with self.assertRaisesRegex(ValueError, 'background selection contract changed'):
                    generate_page(page, incomplete, {r['address'] for r in incomplete})
            fragment = [r for r in records if r['address'] == start]
            with self.assertRaisesRegex(ValueError, 'background selection contract changed'):
                require_background_selection_contract(page, fragment)
            for field, value in (('assembly', 'changed'), ('ops', []), ('flows', [-1]),
                                 ('fallthrough', -1)):
                changed = deepcopy(records)
                next(r for r in changed if r['address'] == start)[field] = value
                with self.assertRaisesRegex(ValueError, 'background selection contract changed'):
                    generate_page(page, changed, {r['address'] for r in changed})
            unrelated = [r for r in records if all(
                not begin <= r['address'] < limit for begin, limit, _ in CONTRACTS)]
            require_background_selection_contract(page, unrelated)

    def test_sampler_wrappers_accessor_calls_and_null_guards_remain_generated(self):
        selector_code = generate_page(0x10712, self.records(0x10712),
                                     {r['address'] for r in self.records(0x10712)})
        self.assertIn('i_107124e0:', selector_code)
        self.assertNotIn('try_modern_background_selection(s, 0x107124e0)', selector_code)
        self.assertIn('s.pc = 0x1074df60; return true;', selector_code)
        self.assertIn('s.pc = 0x1074dd40; return true;', selector_code)
        self.assertIn('s.pc = 0x1077b770; return true;', selector_code)
        records = self.records(0x10713)
        wrapper_code = generate_page(0x10713, records, {r['address'] for r in records})
        self.assertIn('s.pc = 0x1074df90; return true;', wrapper_code)
        for address in (0x10713a80, 0x10713ae0, 0x10713b30, 0x10713c10):
            self.assertNotIn(f'try_modern_background_selection(s, 0x{address:x})', wrapper_code)
        for address, getter in ((0x10713a9c, 0x100d4df0), (0x10713aec, 0x100d5310),
                                (0x10713b3c, 0x100d4fd0)):
            fragment = wrapper_code.split(f'i_{address:x}:', 1)[1].split(f'i_{address + 4:x}:', 1)[0]
            self.assertIn('if (s.r<4>(12) == 0) {', fragment)
            self.assertIn('s.wr<4>(2440, s.r<4>(8));', fragment)
            self.assertIn(f's.wr<4>(4128, 0x{address + 4:x});', fragment)
            self.assertIn(f'goto i_{address + 4:x};', fragment)
            self.assertIn(f's.pc = 0x{getter:x}; return true;', fragment)
            changed = deepcopy(records)
            next(r for r in changed if r['address'] == address)['ops'] = []
            with self.assertRaisesRegex(ValueError, 'Background call site no longer matches'):
                generate_page(0x10713, changed, {r['address'] for r in changed})


if __name__ == '__main__':
    unittest.main()
