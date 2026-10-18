"""Mac memory contracts, destination hooks and preserved generated callers."""
from contextlib import closing
from copy import deepcopy
import json
from pathlib import Path
import sqlite3
import unittest

from memory_primitives_audit import (
    MEMORY_PRIMITIVES_ENTRIES, CONTRACTS, INSTRUCTION_COUNTS,
    require_memory_primitives_contract)
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page, operation

INDEX = Path(__file__).resolve().parent.parent / '.local/recomp-input/instructions.sqlite'


class MemoryPrimitivesManifestTests(unittest.TestCase):
    def test_six_entries_extend_the_exact_prior_60(self):
        import hashlib
        self.assertEqual(MEMORY_PRIMITIVES_ENTRIES,
                         (0x10739ef0, 0x10778300, 0x10778330, 0x10778360, 0x10778390, 0x107783e0))
        self.assertEqual(INSTRUCTION_COUNTS, (15, 12, 12, 12, 20, 48))
        self.assertEqual(sum(INSTRUCTION_COUNTS), 119)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 796)
        self.assertEqual(len({a for a, _ in SEMANTIC_REPLACEMENTS}), 796)
        self.assertEqual(SEMANTIC_REPLACEMENTS[60:66], tuple(
            (entry, 'memory_primitives_adapter') for entry in MEMORY_PRIMITIVES_ENTRIES))
        prior = [{'entry': f'0x{entry:x}', 'module': module}
                 for entry, module in SEMANTIC_REPLACEMENTS[:60]]
        self.assertEqual(hashlib.sha256(json.dumps(prior, sort_keys=True,
                         separators=(',', ':')).encode()).hexdigest(),
                         'fee7650b6c813a7bc23405667fa582478492a4b4be23c3a2f0974ead569bb4e8')


@unittest.skipUnless(INDEX.exists(), 'Private Mac export unavailable')
class MemoryPrimitivesAuditTests(unittest.TestCase):
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
            require_memory_primitives_contract(start >> 12, records)
            for local_temporaries in (False, True):
                code = generate_page(start >> 12, records, {r['address'] for r in records},
                                     local_temporaries=local_temporaries)
                self.assertIn('#include "memory_primitives_adapter.hpp"', code)
                self.assertIn(f'case 0x{start:x}: goto i_{start:x};', code)
                fragment = code.split(f'i_{start:x}:', 1)[1].split(f'i_{start + 4:x}:', 1)[0]
                hook = f'if (try_modern_memory_primitives(s, 0x{start:x})) return true;'
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
                with self.assertRaisesRegex(ValueError, 'memory primitives contract changed'):
                    generate_page(page, incomplete, {r['address'] for r in incomplete})
            fragment = [r for r in records if r['address'] == start]
            with self.assertRaisesRegex(ValueError, 'memory primitives contract changed'):
                require_memory_primitives_contract(page, fragment)
            for field, value in (('assembly', 'changed'), ('ops', []), ('flows', [-1]),
                                 ('fallthrough', -1), ('address', start - 4)):
                changed = deepcopy(records)
                next(r for r in changed if r['address'] == start)[field] = value
                with self.assertRaisesRegex(ValueError, 'memory primitives contract changed'):
                    generate_page(page, changed, {r['address'] for r in changed})
            duplicated = deepcopy(records)
            duplicated.insert(0, deepcopy(next(r for r in records if r['address'] == start)))
            with self.assertRaisesRegex(ValueError, 'memory primitives contract changed'):
                generate_page(page, duplicated, {r['address'] for r in duplicated})
            unrelated = [r for r in records if all(
                not begin <= r['address'] < limit for begin, limit, _ in CONTRACTS)]
            require_memory_primitives_contract(page, unrelated)
            # A full different body on the same page does not require this
            # entry when a bounded export omits it completely.
            for other_start, other_end, _ in CONTRACTS:
                if other_start != start and other_start >> 12 == page:
                    other_body = [r for r in records if other_start <= r['address'] < other_end]
                    require_memory_primitives_contract(page, other_body)

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

    def test_generated_dependencies_consumers_and_guards_remain(self):
        pages = {}
        for page in (0x10713, 0x10739, 0x10774, 0x10775, 0x10776,
                     0x10778, 0x1077b, 0x1077f):
            records = self.records(page)
            pages[page] = generate_page(page, records, {r['address'] for r in records})
        self.assertIn('s.pc = 0x1077fc28; return true;', pages[0x10739])
        self.assertIn('goto i_107783e0;', pages[0x10778])
        self.assertIn('s.pc = 0x1077dbd0; return true;', pages[0x10778])
        self.assertIn('s.pc = 0x10739ef0; return true;', pages[0x10778])
        for caller, target in ((0x10774af4, 0x10778300), (0x1077510c, 0x10778300),
                               (0x10775298, 0x10778300), (0x1077b0a8, 0x10778330)):
            fragment = pages[caller >> 12].split(f'i_{caller:x}:', 1)[1].split(
                f'i_{caller + 4:x}:', 1)[0]
            self.assertIn(f's.pc = 0x{target:x}; return true;', fragment)
        for entry in (0x107784c0, 0x10776530, 0x1077b680, 0x1077fc28):
            self.assertIn(f'i_{entry:x}:', pages[entry >> 12])
            self.assertNotIn(f'try_modern_memory_primitives(s, 0x{entry:x})', pages[entry >> 12])
        for entry in (0x10713aec, 0x10713b3c):
            fragment = pages[0x10713].split(f'i_{entry:x}:', 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
            self.assertIn('if (s.r<4>(12) == 0)', fragment)
        self.assertNotIn('try_modern_memory_primitives', pages[0x1077f])


if __name__ == '__main__':
    unittest.main()
