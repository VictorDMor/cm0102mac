"""Daily injury source contracts, productive hooks and unique ownership."""
from contextlib import closing
from copy import deepcopy
import hashlib
import inspect
import json
from pathlib import Path
import re
import sqlite3
import struct
import unittest

import injury_daily_audit as audit
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page, require_unique_semantic_replacements
from make_injury_daily_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class ManifestTests(unittest.TestCase):
    def test_exact_accepted_prefix_and_one_new_owner(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 946)
        self.assertEqual(len({entry for entry, _ in SEMANTIC_REPLACEMENTS}), 946)
        prefix = [{'entry': f'0x{entry:x}', 'module': owner} for entry, owner in SEMANTIC_REPLACEMENTS[:734]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         'c8e02f08c40cfd609332fbf48036d1be9a1d6461da13e47452f9da786a0ea93e')
        self.assertEqual(SEMANTIC_REPLACEMENTS[734:735], ((0x10185410, 'injury_daily_adapter'),))

    def test_duplicate_ownership_is_rejected_before_generation(self):
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for owner in ('actor_attributes_adapter', 'injury_daily_adapter'):
            with self.subTest(owner=owner), self.assertRaisesRegex(ValueError, 'Duplicate semantic replacement at 10187a40'):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS, (0x10187a40, owner)))


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class DailySourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
            audit.require_injury_daily_index(db)
            cls.records = [json.loads(row[0]) for start, end, *_ in audit.CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.program = json.loads((INPUT / 'program.json').read_text())

    def test_complete_body_and_discriminating_source_guards(self):
        self.assertEqual(len(self.records), 1060)
        self.assertEqual(audit.INJURY_DAILY_ENTRIES, (0x10185410,))
        audit.require_injury_daily_bodies(self.records, complete=True)
        for field, value in (('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
            changed = deepcopy(self.records)
            changed[0][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                audit.require_injury_daily_bodies(changed, complete=True)
        with self.assertRaises(ValueError):
            audit.require_injury_daily_bodies(self.records[1:], complete=True)
        with self.assertRaises(ValueError):
            audit.require_injury_daily_edges([(0, 0x10185414)])

    def test_reference_retains_every_original_instruction(self):
        source = render_reference(self.records, self.program['userops'])
        expected = [f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', source, re.MULTILINE), expected)
        self.assertEqual(re.findall(r'injury_daily_step_seen\(s, 0x([0-9a-f]+)\)', source), expected)
        self.assertNotIn('try_modern_', source)

    def test_table_and_pointer_mutations_are_rejected(self):
        segments = [(s['address'], (INPUT / s['file']).read_bytes()) for s in self.program['segments'] if s.get('file')]

        def word(address):
            for base, data in segments:
                if base <= address and address + 4 <= base + len(data):
                    return struct.unpack_from('>I', data, address - base)[0]
            raise ValueError('Missing daily injury table word')

        values = {address: word(address) for slot, base, size, _ in audit.TABLES
                  for address in (slot, *range(base, base + size, 4))}
        audit.require_injury_daily_tables(values.__getitem__)
        for slot, base, *_ in audit.TABLES:
            for address in (slot, base):
                changed = dict(values)
                changed[address] ^= 1
                with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                    audit.require_injury_daily_tables(changed.__getitem__)

    def test_productive_hook_precedes_original_accounting(self):
        selected = {row['address'] for row in self.records}
        disabled = {name: False for name in inspect.signature(generate_page).parameters
                    if name.endswith('_hooks') or name == 'guest_context_transfers'}
        disabled['injury_daily_hooks'] = True
        for page in sorted({row['address'] >> 12 for row in self.records}):
            records = [row for row in self.records if row['address'] >> 12 == page]
            code = generate_page(page, records, selected, userops=self.program['userops'], **disabled)
            audit.require_injury_daily_hooks(page, records, code)
            if page == 0x10185:
                hook = audit.injury_daily_hook(0x10185410)
                with self.assertRaises(ValueError):
                    audit.require_injury_daily_hooks(page, records, code.replace(hook, ''))

    def test_named_semantics_do_not_replay_original_instructions(self):
        for name in ('injury_daily.cpp', 'injury_daily_detail.hpp', 'injury_daily_selection.cpp', 'injury_daily_effects.cpp'):
            source = (ROOT / 'native' / name).read_text()
            for token in ('_reference(', 'dispatch_page(', 'run_until(', 'pcode', 'i_101'):
                self.assertNotIn(token, source)


if __name__ == '__main__':
    unittest.main()
