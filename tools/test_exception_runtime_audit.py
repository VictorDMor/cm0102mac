"""Complete exception-scope pins and missing-context recovery negatives."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
import os
import re
from pathlib import Path
import sqlite3
import subprocess
import tempfile
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from exception_runtime_audit import (CONTRACTS, EXCEPTION_RUNTIME_ENTRIES, INSTRUCTION_COUNTS,
    PAGE_FRAGMENTS, require_exception_runtime_bodies, require_exception_runtime_contract,
    require_exception_runtime_edges, require_exception_runtime_index, require_exception_runtime_hooks)

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_EXCEPTION_RUNTIME_INPUT', ROOT / '.local/recomp-input-exceptions'))
INDEX = INPUT / 'instructions.sqlite'


class ScopeTests(unittest.TestCase):
    def test_complete_21_entry_scope_and_prior_delivery(self):
        self.assertEqual(len(CONTRACTS), 21)
        self.assertEqual(sum(INSTRUCTION_COUNTS), 1657)
        self.assertEqual(EXCEPTION_RUNTIME_ENTRIES, tuple(sorted(set(EXCEPTION_RUNTIME_ENTRIES))))
        prior = [{'entry': f'0x{entry:x}', 'module': module} for entry, module in SEMANTIC_REPLACEMENTS[:205]]
        self.assertEqual(hashlib.sha256(json.dumps(prior, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '9045711f1a6d4d64eb731a58039a9a69cd9e1e113a9e20628031df85a54b7cfc')
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 629)
        self.assertEqual(len({entry for entry, _ in SEMANTIC_REPLACEMENTS}), 629)
        self.assertEqual(SEMANTIC_REPLACEMENTS[205:226], tuple((entry, 'exception_runtime_adapter') for entry in EXCEPTION_RUNTIME_ENTRIES))
        self.assertNotIn(0x107724f4, {entry for entry, _ in SEMANTIC_REPLACEMENTS})
        for previous in (0x10771150, 0x107713e0, 0x107715f0, 0x10771620, 0x10772df0, 0x10772e10, 0x10773150):
            self.assertNotIn(previous, EXCEPTION_RUNTIME_ENTRIES)


@unittest.skipUnless(INDEX.exists(), 'Private recovered exception export unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect(INDEX.resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = [json.loads(row[0]) for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                (CONTRACTS[0][0], CONTRACTS[-1][1]))]

    def test_every_production_hook_and_unexpected_landing_precede_original_effects(self):
        metadata = json.loads((INPUT / 'program.json').read_text())
        for page in (0x10771, 0x10772, 0x10773):
            records = [r for r in self.records if r['address'] >> 12 == page]
            code = generate_page(page, records, {r['address'] for r in records}, userops=metadata['userops'])
            require_exception_runtime_hooks(page, records, code)
            entries = [entry for entry in (*EXCEPTION_RUNTIME_ENTRIES, 0x107724f4) if entry >> 12 == page]
            for entry in entries:
                hook = ('if (try_modern_exception_unexpected_landing(s)) return true;' if entry == 0x107724f4 else
                        f'if (try_modern_exception_runtime(s, 0x{entry:x})) return true;')
                self.assertEqual(code.count(hook), 1)
                step = f'counter.step(0x{entry:x});' if f'counter.step(0x{entry:x});' in code else f's.step(0x{entry:x});'
                for bad in (code.replace(hook, '', 1), code.replace(hook, hook + hook, 1),
                            code.replace(hook, '', 1).replace(step, step + hook, 1)):
                    with self.subTest(entry=hex(entry)), self.assertRaises(ValueError):
                        require_exception_runtime_hooks(page, records, bad)
            original = generate_page(page, records, {r['address'] for r in records}, userops=metadata['userops'], exception_runtime_hooks=False)
            self.assertNotIn('try_modern_exception_', original)

    def test_actual_complete_index_and_cross_page_fragments(self):
        with closing(sqlite3.connect(INDEX.resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_exception_runtime_index(db)
        for page in (0x10771, 0x10772, 0x10773):
            require_exception_runtime_contract(page, [r for r in self.records if r['address'] >> 12 == page])
        for (start, page), (count, _) in PAGE_FRAGMENTS.items():
            records = [r for r in self.records if r['address'] >> 12 == page and
                       any(a == start and a <= r['address'] < b for a, b, _ in CONTRACTS)]
            self.assertEqual(len(records), count)

    def test_every_body_rejects_changed_missing_and_duplicate_records(self):
        for start, end, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for key, value in (('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                mutation = deepcopy(body); mutation[0][key] = value
                with self.subTest(entry=hex(start), key=key), self.assertRaises(ValueError):
                    require_exception_runtime_bodies(mutation)
            for mutation in (body[1:], body[:-1], body + [body[0]]):
                with self.assertRaises(ValueError): require_exception_runtime_bodies(mutation)
            with self.assertRaisesRegex(ValueError, 'Missing complete'):
                require_exception_runtime_bodies([r for r in self.records if not start <= r['address'] < end], complete=True)

    def test_pre_recovery_missing_vector_and_landing_pad_bodies_reject(self):
        for a, b in ((0x107724c0, 0x1077259c), (0x10772698, 0x107726f8), (0x10772b80, 0x10772c9c),
                     (0x10772d80, 0x10772dc4), (0x10772ee0, 0x10772f08)):
            with self.subTest(missing=hex(a)), self.assertRaises(ValueError):
                require_exception_runtime_bodies([r for r in self.records if not a <= r['address'] < b], complete=True)

    def test_unknown_external_interior_flow_rejects_for_every_body(self):
        for start, end, _ in CONTRACTS:
            require_exception_runtime_edges([(start - 4, start), (start, end - 4)])
            with self.assertRaisesRegex(ValueError, 'External exception_runtime interior edge'):
                require_exception_runtime_edges([(start - 4, end - 4)])

    def test_bounded_reference_retains_all_pinned_instructions_and_search_boundary(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / 'reference.cpp'
            result = subprocess.run(['python3', str(ROOT / 'tools/make_exception_context_reference.py'),
                                     str(INPUT), str(output)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            source = output.read_text()
            self.assertIn('bool exception_context_reference(State& s)', source)
            self.assertIn('s.pc = 0x10772810; return true;', source)
            self.assertNotIn('i_10772810:', source)
            self.assertNotIn('try_modern_exception_runtime', source)
            for page in (0x10771, 0x10772, 0x10773):
                self.assertTrue(f'page_{page:x}_impl' not in source, 'Reference templates must not replace production-page symbols')
            # Only page 10773 uses the production generator's Turbo template.
            self.assertTrue('context_reference_10773_impl' in source, 'Missing isolated Turbo implementation')
            bodies = [r for r in self.records if 0x10771640 <= r['address'] < 0x10771b28 or
                      0x10771b30 <= r['address'] < 0x10772430 or
                      0x10772430 <= r['address'] < 0x107724bc or
                      0x10772650 <= r['address'] < 0x10772778 or
                      0x10772b10 <= r['address'] < 0x10772c9c or
                      0x10772f10 <= r['address'] < 0x10773148]
            self.assertEqual(len(bodies), 1233)
            for record in bodies:
                self.assertEqual(source.count(f"i_{record['address']:x}: // {record['assembly']}"), 1)
            dispatcher = source.split('bool exception_context_reference(State& s)', 1)[1]
            selected = [int(address, 16) for address in re.findall(r'case 0x([0-9a-f]+):', dispatcher)]
            self.assertCountEqual(selected, [r['address'] for r in bodies])
            self.assertNotIn(0x10773700, selected, 'Unselected same-page callback needs the full dispatcher')

    def test_complete_search_reference_and_template_isolation(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / 'search.cpp'
            result = subprocess.run(['python3', str(ROOT / 'tools/make_exception_context_reference.py'),
                                     str(INPUT), str(output), '--with-search'], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            source = output.read_text()
            self.assertTrue('bool exception_search_reference(State& s)' in source)
            self.assertTrue('i_10772810:' in source)
            self.assertTrue('context_reference_' not in source and 'try_modern_exception_runtime' not in source)
            for page in (0x10771, 0x10772, 0x10773):
                self.assertTrue(f'page_{page:x}_impl' not in source, 'Search templates must be isolated')
            entries = {0x10771640, 0x107717f0, 0x10771b30, 0x10772430, 0x10772650,
                       0x10772780, 0x10772810, 0x10772b10, 0x10772f10}
            bodies = [r for r in self.records if any(a in entries and a <= r['address'] < b for a, b, _ in CONTRACTS)]
            self.assertEqual(len(bodies), 1439)
            for record in bodies:
                self.assertEqual(source.count(f"i_{record['address']:x}: // {record['assembly']}"), 1)
            dispatcher = source.split('bool exception_search_reference(State& s)', 1)[1]
            self.assertCountEqual([int(a, 16) for a in re.findall(r'case 0x([0-9a-f]+):', dispatcher)],
                                  [r['address'] for r in bodies])

    def test_original_transfer_markers_after_pinned_loads_and_return(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / 'transfer.cpp'
            result = subprocess.run(['python3', str(ROOT / 'tools/make_exception_context_reference.py'),
                                     str(INPUT), str(output), '--with-search', '--with-transfers'], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            source = output.read_text()
            self.assertTrue('bool exception_transfer_reference(State& s)' in source)
            self.assertEqual(source.count('record_context_function(s,'), 2)
            self.assertEqual(source.count('record_context_frame(s,'), 1)
            self.assertEqual(source.count('complete_context_transfer(s);'), 1)
            self.assertTrue('s.pc = std::uint32_t(s.r<4>(4128)); complete_context_transfer(s); return true;' in source)
            for address, marker in ((0x107727d4, 'record_context_function'), (0x10772a94, 'record_context_function'),
                                    (0x10772768, 'record_context_frame')):
                section = source.split(f'i_{address:x}: //', 1)[1].split(f'i_{address+4:x}: //', 1)[0]
                self.assertLess(section.index('s.memory.read<4>'), section.index(marker))
            self.assertTrue('page_10773_impl' not in source and 'search_reference_' not in source)

    def test_complete_capture_and_scalar_prefix_references(self):
        with tempfile.TemporaryDirectory() as temporary:
            for name, arguments, count, entries in (
                    ('capture', ['--with-search', '--with-capture', '--with-transfers'], 1507,
                     {0x10771640, 0x107717f0, 0x10771b30, 0x10772430, 0x10772650,
                      0x10772780, 0x10772810, 0x10772ad0, 0x10772b10, 0x10772ca0, 0x10772f10}),
                    ('scalar', ['--scalar-prefix'], 55, {0x10772ca0})):
                output = Path(temporary) / f'{name}.cpp'
                result = subprocess.run(['python3', str(ROOT / 'tools/make_exception_context_reference.py'),
                                         str(INPUT), str(output), *arguments], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                source = output.read_text()
                self.assertIn(f'bool exception_{name}_reference(State& s)', source)
                bodies = [r for r in self.records if any(a in entries and a <= r['address'] < b for a, b, _ in CONTRACTS)]
                self.assertEqual(len(bodies), count)
                for record in bodies:
                    self.assertEqual(source.count(f"i_{record['address']:x}: // {record['assembly']}"), 1)
                dispatcher = source.split(f'bool exception_{name}_reference(State& s)', 1)[1]
                self.assertCountEqual([int(a, 16) for a in re.findall(r'case 0x([0-9a-f]+):', dispatcher)],
                                      [r['address'] for r in bodies])
                self.assertNotIn('page_10773_impl', source)
                self.assertNotIn('try_modern_exception_runtime', source)
                if name == 'capture':
                    self.assertEqual(source.count('complete_context_transfer(s);'), 1)
                    self.assertIn('i_10772b00:', source)
                    self.assertIn('i_10772c98:', source)
                    self.assertIn('i_10772d78:', source)
                else:
                    self.assertIn('s.pc = 0x10772810; return true;', source)
                    self.assertNotIn('i_10772810:', source)
                    self.assertNotIn('complete_context_transfer', source)

    def test_complete_reference_retains_unexpected_landing_and_all_entries(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / 'full.cpp'
            result = subprocess.run(['python3', str(ROOT / 'tools/make_exception_context_reference.py'),
                                     str(INPUT), str(output), '--complete', '--with-transfers'], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            source = output.read_text(); self.assertIn('bool exception_full_reference(State& s)', source)
            bodies = [r for r in self.records if any(a <= r['address'] < b for a, b, _ in CONTRACTS)]
            self.assertEqual(len(bodies), 1657)
            for record in bodies:
                self.assertEqual(source.count(f"i_{record['address']:x}: // {record['assembly']}"), 1)
            dispatcher = source.split('bool exception_full_reference(State& s)', 1)[1]
            self.assertCountEqual([int(a, 16) for a in re.findall(r'case 0x([0-9a-f]+):', dispatcher)],
                                  [r['address'] for r in bodies])
            self.assertIn('i_107724f4:', source)
            self.assertEqual(source.count('complete_context_transfer(s);'), 1)
            self.assertNotIn('page_10773_impl', source)
            self.assertNotIn('try_modern_exception_runtime', source)


if __name__ == '__main__':
    unittest.main()
