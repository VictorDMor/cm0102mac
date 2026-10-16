"""Complete contact boundaries and independent component-reference safeguards."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import unittest
from ball_contact_audit import (CONTRACTS, ENTRIES, FACADE_ENTRIES, TABLES, load_ball_contact,
    load_ball_contact_tables, require_ball_contact_bodies, require_ball_contact_edges, require_ball_contact_tables)
from compile_recomp import SEMANTIC_REPLACEMENTS
from make_ball_contact_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class ScopeTests(unittest.TestCase):
    def test_complete_block_is_pinned_and_partial_facades_are_not_productive(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS), 2043)
        self.assertEqual(sum(row[2] for row in CONTRACTS[:4]), 98)
        self.assertEqual(ENTRIES, (0x101f5050, 0x101f50c0, 0x101f5110, 0x101f5180, 0x101f5350, 0x10270320))
        self.assertEqual(FACADE_ENTRIES, ENTRIES[:4])
        manifest = [{'entry': f'0x{entry:x}', 'module': module} for entry, module in SEMANTIC_REPLACEMENTS]
        self.assertEqual(len(manifest), 678)
        self.assertEqual(hashlib.sha256(json.dumps(manifest, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         'a7ed184a519e48bfbe1612d909630b90c35527dc12ec2c88972b77c4831f419f')
        for filename in ('ball_contact_facades.cpp', 'ball_contact_failure.cpp', 'ball_contact_description_preparation.cpp', 'ball_contact_description_flow.cpp', 'ball_contact_description_weight.cpp', 'ball_contact_description_assessment.cpp', 'ball_contact_description_reactions.cpp', 'ball_contact_description.cpp'):
            source = (ROOT / 'native' / filename).read_text()
            for forbidden in ('_reference(', 'dispatch_page', 'pcode', 'i_101'):
                self.assertNotIn(forbidden, source)
        cmake = (ROOT / 'native/CMakeLists.txt').read_text().split('target_sources(cm_aot PRIVATE', 1)[1].split('add_library(cm_match_debugger_database', 1)[0]
        for source in (ROOT / 'native').glob('ball_contact*.cpp'):
            self.assertNotIn(source.name, cmake)

    def test_external_interiors_fail_for_each_complete_root(self):
        for start, end, *_ in CONTRACTS:
            require_ball_contact_edges([(0, start), (start, end - 4), (end - 4, end)])
            for target in (start + 4, end - 4):
                with self.subTest(entry=hex(start), target=hex(target)), self.assertRaises(ValueError):
                    require_ball_contact_edges([(0, target)])


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            cls.records = load_ball_contact(db)
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']
        cls.tables = load_ball_contact_tables(INPUT)

    def test_changed_missing_duplicate_and_reordered_bodies_fail(self):
        for start, end, *_ in CONTRACTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] == start)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = list(self.records); changed[index] = deepcopy(changed[index]); changed[index][key] = value
                with self.subTest(entry=hex(start), key=key), self.assertRaises(ValueError):
                    require_ball_contact_bodies(changed)
            for changed in (self.records[:index] + self.records[index + 1:], self.records + [self.records[index]],
                            list(reversed(self.records)), [row for row in self.records if not start <= row['address'] < end]):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    render_reference(changed, self.userops, 'facades')

    def test_all_independent_reference_labels_and_steps_remain_without_hooks(self):
        for component, contracts in (('facades', CONTRACTS[:4]), ('description', CONTRACTS[4:5]), ('description_preparation', ((0x101f5350, 0x101f5800),)), ('description_exit', ((0x101f5800, 0x101f5a0c),)), ('description_weight', ((0x101f5a2c, 0x101f5ea4),)), ('description_assessment', ((0x101f5ea4, 0x101f5f58),)), ('description_reactions', ((0x101f5f58, 0x101f6914),)), ('description_players', ((0x101f5a0c, 0x101f5a2c),)), ('description_next', ((0x101f6914, 0x101f6924),)), ('description_return', ((0x101f6924, 0x101f6940),)), ('failure', CONTRACTS[5:])):
            code = render_reference(self.records, self.userops, component)
            expected = [f'{address:x}' for start, end, *_ in contracts for address in range(start, end, 4)]
            self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE), expected)
            self.assertEqual(re.findall(r'ball_contact_step_seen\(s, 0x([0-9a-f]+)\)', code), expected)
            for forbidden in ('try_modern_', 'complete_context_transfer'):
                self.assertNotIn(forbidden, code)

    def test_three_live_description_tables_and_all_words_are_pinned(self):
        body = [row for row in self.records if CONTRACTS[4][0] <= row['address'] < CONTRACTS[4][1]]
        self.assertEqual([row['address'] for row in body if row['assembly'] == 'bctr'], [row[0] for row in TABLES])
        self.assertEqual([len(self.tables[row[0]]) for row in TABLES], [48, 10, 9])
        self.assertEqual([len(set(self.tables[row[0]])) for row in TABLES], [13, 10, 9])
        words = {}
        for site, slot, table, *_ in TABLES:
            words[slot] = table; words.update({table + 4 * n: word for n, word in enumerate(self.tables[site])})
        require_ball_contact_tables(words.__getitem__)
        for address in words:
            changed = dict(words); changed[address] ^= 1
            with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                require_ball_contact_tables(changed.__getitem__)


if __name__ == '__main__':
    unittest.main()
