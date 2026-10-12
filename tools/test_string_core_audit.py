"""Mac string contracts, destination hooks and preserved generated callers."""
from contextlib import closing
from copy import deepcopy
import json
from pathlib import Path
import sqlite3
import unittest

from string_core_audit import (
    STRING_CORE_ENTRIES, CONTRACTS, INSTRUCTION_COUNTS,
    require_string_core_contract)
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page, operation

INDEX = Path(__file__).resolve().parent.parent / '.local/recomp-input/instructions.sqlite'


class StringManifestTests(unittest.TestCase):
    def test_fourteen_entries_extend_existing_46(self):
        self.assertEqual(len(STRING_CORE_ENTRIES), 14)
        self.assertEqual(sum(INSTRUCTION_COUNTS), 295)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 642)
        self.assertEqual(SEMANTIC_REPLACEMENTS[46:60], tuple(
            (entry, 'string_core_adapter') for entry in STRING_CORE_ENTRIES))
        self.assertTrue(all(module != 'string_core_adapter'
                            for _, module in SEMANTIC_REPLACEMENTS[:46]))


@unittest.skipUnless(INDEX.exists(), 'Private Mac export unavailable')
class StringAuditTests(unittest.TestCase):
    def records(self, page):
        with closing(sqlite3.connect(INDEX.resolve().as_uri() + '?mode=ro', uri=True)) as db:
            return [json.loads(row[0]) for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                (page << 12, (page + 1) << 12))]

    def test_complete_contracts_and_destination_hooks(self):
        for (start, end, _), count in zip(CONTRACTS, INSTRUCTION_COUNTS):
            records = self.records(start >> 12)
            body = [r for r in records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(body[0]['address'], start)
            self.assertEqual(body[-1]['address'], end - 4)
            self.assertEqual(len({r['address'] for r in body}), count)
            self.assertEqual(body[-1]['assembly'], 'blr')
            require_string_core_contract(start >> 12, records)
            for local_temporaries in (False, True):
                code = generate_page(start >> 12, records, {r['address'] for r in records},
                                     local_temporaries=local_temporaries)
                self.assertIn('#include "string_core_adapter.hpp"', code)
                self.assertIn(f'case 0x{start:x}: goto i_{start:x};', code)
                fragment = code.split(f'i_{start:x}:', 1)[1].split(f'i_{start + 4:x}:', 1)[0]
                hook = f'if (try_modern_string_core(s, 0x{start:x})) return true;'
                self.assertEqual(fragment.count(hook), 1)
                self.assertLess(fragment.index(hook), fragment.index(f's.step(0x{start:x});'))
                # The declined hook retains the original first operation for
                # this module's independent private comparison mode.
                if not local_temporaries:
                    first_operation = operation(body[0]['ops'][0], start, 0,
                                                start >> 12, {start}, len(body[0]['ops']))
                    self.assertLess(fragment.index(hook), fragment.index(first_operation))
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

    def test_changed_or_incomplete_bodies_reject(self):
        for start, end, _ in CONTRACTS:
            page = start >> 12
            records = self.records(page)
            for victim in (start, start + 4, end - 4):
                incomplete = [r for r in records if r['address'] != victim]
                with self.assertRaisesRegex(ValueError, 'string core contract changed'):
                    generate_page(page, incomplete, {r['address'] for r in incomplete})
            fragment = [r for r in records if r['address'] == start]
            with self.assertRaisesRegex(ValueError, 'string core contract changed'):
                require_string_core_contract(page, fragment)
            for field, value in (('assembly', 'changed'), ('ops', []), ('flows', [-1]),
                                 ('fallthrough', -1), ('address', start - 4)):
                changed = deepcopy(records)
                next(r for r in changed if r['address'] == start)[field] = value
                with self.assertRaisesRegex(ValueError, 'string core contract changed'):
                    generate_page(page, changed, {r['address'] for r in changed})
            duplicated = deepcopy(records)
            duplicated.insert(0, deepcopy(next(r for r in records if r['address'] == start)))
            with self.assertRaisesRegex(ValueError, 'string core contract changed'):
                generate_page(page, duplicated, {r['address'] for r in duplicated})
            unrelated = [r for r in records if all(
                not begin <= r['address'] < limit for begin, limit, _ in CONTRACTS)]
            require_string_core_contract(page, unrelated)
            # A full different body on the same page does not require this
            # entry when a bounded export omits it completely.
            for other_start, other_end, _ in CONTRACTS:
                if other_start != start and other_start >> 12 == page:
                    other_body = [r for r in records if other_start <= r['address'] < other_end]
                    require_string_core_contract(page, other_body)

    def test_all_indexed_external_edges_target_entries_only(self):
        incoming = {start: [] for start, _, _ in CONTRACTS}
        # Every indexed flow and fallthrough participates; filtering the
        # destination first avoids publishing unrelated proprietary records.
        query = """SELECT i.address, edge.value
            FROM instructions AS i,
              json_each(json_insert(json_extract(i.body, '$.flows'),
                                    '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
            WHERE edge.value>=? AND edge.value<?"""
        with closing(sqlite3.connect(INDEX.resolve().as_uri() + '?mode=ro', uri=True)) as db:
            for source, target in db.execute(query, (min(c[0] for c in CONTRACTS), max(c[1] for c in CONTRACTS))):
                for start, end, _ in CONTRACTS:
                    if start <= target < end and not start <= source < end:
                        self.assertEqual(target, start, f'External interior edge from {source:x}')
                        incoming[start].append(source)

    def test_dependencies_and_null_guards_are_preserved(self):
        for page in (0x10713, 0x10739, 0x1074d):
            records = self.records(page)
            code = generate_page(page, records, {r['address'] for r in records})
            if page == 0x10713:
                for address in (0x10713aec, 0x10713b3c):
                    self.assertIn(f'i_{address:x}:', code)
                self.assertNotIn('try_modern_string_core', code)
            if page == 0x10739:
                for target in (0x1077dbd0, 0x10775130, 0x1077dbf0):
                    self.assertIn(f's.pc = 0x{target:x}; return true;', code)
            if page == 0x1074d:
                self.assertIn('s.pc = 0x107393f0; return true;', code)


if __name__ == '__main__':
    unittest.main()
