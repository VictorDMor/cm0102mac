"""Complete body/fragment pins, destination hooks and isolated reference."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from import_descriptor_audit import (CONTRACTS, FRAGMENT_CONTRACTS, IMPORT_DESCRIPTOR_ENTRIES,
    require_import_descriptor_bodies, require_import_descriptor_edges, require_import_descriptor_index,
    require_import_descriptor_contract, require_import_descriptor_hooks, import_descriptor_hook)
from make_import_descriptor_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get('CM_IMPORT_DESCRIPTOR_INPUT', ROOT / '.local/compiler-arithmetic-reconstruction/input'))


class ScopeTests(unittest.TestCase):
    def test_complete_260_entry_scope_and_prior_production_preserved(self):
        self.assertEqual(len(CONTRACTS), 260)
        self.assertEqual(sum(n for _, _, n, _ in CONTRACTS), 1559)
        self.assertEqual(IMPORT_DESCRIPTOR_ENTRIES, tuple(sorted(set(IMPORT_DESCRIPTOR_ENTRIES))))
        self.assertEqual(IMPORT_DESCRIPTOR_ENTRIES[:-1], tuple(range(0x1077fad8, 0x10781320, 24)))
        self.assertEqual(IMPORT_DESCRIPTOR_ENTRIES[-1], 0x10781320)
        baseline = [{'entry': f'0x{a:x}', 'module': m} for a, m in SEMANTIC_REPLACEMENTS]
        self.assertEqual(len(baseline), 919)
        self.assertEqual(len({a for a, _ in SEMANTIC_REPLACEMENTS}), 919)
        self.assertEqual(hashlib.sha256(json.dumps(baseline[:276], sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         'ba418ebeea27f3e546fe4f02aed4ddaa192bbcafa009d0c8552411a739b4ebb2')
        self.assertEqual(SEMANTIC_REPLACEMENTS[276:536], tuple((a, 'import_descriptor_adapter') for a in IMPORT_DESCRIPTOR_ENTRIES))
        self.assertEqual([(a, p, n) for a, p, n, _ in FRAGMENT_CONTRACTS], [(0x10780ff0, 0x10780, 4), (0x10780ff0, 0x10781, 2)])

    def test_only_complete_roots_are_exterior_entrances(self):
        for start, end, _, _ in CONTRACTS:
            require_import_descriptor_edges([(0, start), (start, end - 4)])
            for target in range(start + 4, end, 4):
                with self.subTest(target=hex(target)), self.assertRaises(ValueError):
                    require_import_descriptor_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private import-descriptor source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = [json.loads(r[0]) for r in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                (CONTRACTS[0][0], CONTRACTS[-1][1]))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_complete_index_and_all_interior_edges(self):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            require_import_descriptor_index(db)

    def test_each_contract_rejects_changed_fields(self):
        for start, end, _, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for key, value in (('address', start + 1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                bad = deepcopy(body); bad[0][key] = value
                with self.subTest(entry=hex(start), key=key), self.assertRaises(ValueError):
                    require_import_descriptor_bodies(bad)

    def test_missing_duplicate_reordered_and_missing_whole_bodies_fail(self):
        for start, end, _, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for bad in (body[1:], body[:-1], body + [body[0]], list(reversed(body))):
                with self.assertRaises(ValueError): require_import_descriptor_bodies(bad)
            with self.assertRaises(ValueError):
                require_import_descriptor_bodies([r for r in self.records if not start <= r['address'] < end], complete=True)

    def test_all_1559_reference_labels_are_original_only(self):
        code = render_reference(self.records, self.userops)
        labels = re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE)
        self.assertEqual(labels, [f'{r["address"]:x}' for r in self.records])
        self.assertEqual(len(set(labels)), 1559)
        for token in ('try_modern_', 'import_descriptor::', 'complete_context_transfer', 'bool page_'):
            self.assertNotIn(token, code)

    def test_page_fragments_and_selected_missing_roots(self):
        selected = {r['address']: r for r in self.records}
        for page in (0x1077f, 0x10780, 0x10781):
            body = [r for r in self.records if r['address'] >> 12 == page]
            require_import_descriptor_contract(page, body, selected)
            roots = [a for a in IMPORT_DESCRIPTOR_ENTRIES if a >> 12 == page]
            for start in roots:
                end = next(e for a, e, _, _ in CONTRACTS if a == start)
                with self.assertRaises(ValueError):
                    require_import_descriptor_contract(page, [r for r in body if not start <= r['address'] < end], selected)
            crossing = [r for r in body if 0x10780ff0 <= r['address'] < 0x10781008]
            if crossing:
                with self.assertRaises(ValueError): require_import_descriptor_contract(page, [r for r in body if r != crossing[-1]], selected)

    def test_all_destination_hooks_precede_first_original_operation(self):
        selected = {r['address']: r for r in self.records}
        for page in (0x1077f, 0x10780, 0x10781):
            body = [r for r in self.records if r['address'] >> 12 == page]
            code = generate_page(page, body, selected, userops=self.userops)
            require_import_descriptor_hooks(page, body, code)
            for entry in IMPORT_DESCRIPTOR_ENTRIES:
                if entry >> 12 != page: continue
                hook = import_descriptor_hook(entry); step = f's.step(0x{entry:x});'
                for broken in (code.replace(hook, ''), code.replace(hook, hook + '\n' + hook),
                               code.replace(hook + '\n' + step, step + '\n' + hook)):
                    with self.assertRaises(ValueError): require_import_descriptor_hooks(page, body, broken)

    def test_live_toc_displacements_match_pinned_contracts(self):
        header = (ROOT / 'native/import_descriptor.hpp').read_text()
        table = header.split('toc_displacements{', 1)[1].split('};', 1)[0]
        actual = [-int(s, 16) for s in re.findall(r'-0x([0-9a-f]+)', table)]
        expected = [-int(re.fullmatch(r'lwz r12,-0x([0-9a-f]+)\(r2\)', r['assembly']).group(1), 16)
                    for r in self.records if r['address'] in IMPORT_DESCRIPTOR_ENTRIES[:-1]]
        self.assertEqual(actual, expected)


if __name__ == '__main__': unittest.main()
