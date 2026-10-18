"""Complete contract storage creation scope; three operations prepared."""
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
from contract_storage_audit import (CONTRACTS, FRAGMENTS, CONTRACT_STORAGE_ENTRIES, TABLES, PADDING, PREPARED_CONTRACT_STORAGE_ENTRIES,
    require_contract_storage_tables, load_contract_storage_tables,
    require_contract_storage_bodies, require_contract_storage_contract,
    require_contract_storage_edges, require_contract_storage_index, require_contract_storage_hooks, contract_storage_hook)
from make_contract_storage_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_exact_accepted_prefix_and_complete_candidate_owner(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 783)
        self.assertEqual(CONTRACT_STORAGE_ENTRIES, (0x10078e70, 0x10078f00, 0x103727c0))
        prefix = [{'entry': f'0x{entry:x}', 'module': owner} for entry, owner in SEMANTIC_REPLACEMENTS[:756]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '227fca872a4b07455474488cab1d1d46728cd5bed1cbc2aa82e71ca0b3cb98f9')
        self.assertEqual(SEMANTIC_REPLACEMENTS[759:762], tuple((entry, 'contract_storage_adapter') for entry in CONTRACT_STORAGE_ENTRIES))
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry in CONTRACT_STORAGE_ENTRIES:
            with self.subTest(entry=hex(entry)), self.assertRaisesRegex(ValueError, 'Duplicate semantic replacement'):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS, (entry, 'other_owner')))

    def test_complete_boundaries_and_external_interior_edges(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS), 2016)
        self.assertEqual(PREPARED_CONTRACT_STORAGE_ENTRIES, CONTRACT_STORAGE_ENTRIES)
        self.assertEqual(sum(row[2] for row in FRAGMENTS), 2016)
        for start, end, *_ in CONTRACTS:
            require_contract_storage_edges([(0, start), (start, end-4), (end-4, end)])
            for target in (start+4, end-4):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_contract_storage_edges([(0, target)])

    def test_named_operations_do_not_replay_original_bodies(self):
        paths = ['contract_storage.cpp', 'contract_initializer.cpp', 'contract_initializer_detail.hpp',
                 'contract_initializer_options.cpp', 'contract_initializer_nations.cpp']
        source = '\n'.join((ROOT / 'native' / name).read_text() for name in paths)
        for forbidden in ('contract_storage_reference', 'dispatch_page', 'run_until', 'pcode', 'i_1007', 'i_1037'):
            self.assertNotIn(forbidden, source)
        for name in ('bind_primary', 'reserve_record', 'reuse_empty_record', 'append_record', 'report_allocation_failure',
                     'initialize_contract', 'prepare_identity_and_rating', 'select_dates_and_base_value',
                     'select_contract_options', 'apply_nation_value_rules', 'finish_contract'):
            self.assertIn(name, source)


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_contract_storage_index(db)
            cls.records = [json.loads(row[0]) for start, end, *_ in CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_original_tables_and_padding_are_pinned(self):
        import struct
        program = json.loads((INPUT / 'program.json').read_text())
        segments = [(s['address'], (INPUT / s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
        def word(address):
            for base, data in segments:
                if base <= address and address+4 <= base+len(data):
                    return struct.unpack_from('>I', data, address-base)[0]
            raise ValueError('Missing original contract storage table word')
        addresses = {address for address, _ in PADDING}
        for slot, base, size, _ in TABLES:
            addresses.update((slot, *range(base, base+size, 4)))
        live = {address: word(address) for address in addresses}
        require_contract_storage_tables(live.__getitem__)
        load_contract_storage_tables(INPUT)
        for address in live:
            changed = dict(live); changed[address] ^= 1
            with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                require_contract_storage_tables(changed.__getitem__)

    def test_changed_original_fields_are_rejected(self):
        for start, page, *_ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page and row['address'] >= start)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records); changed[index][key] = value
                with self.subTest(entry=hex(start), page=hex(page), key=key), self.assertRaises(ValueError):
                    require_contract_storage_bodies(changed, complete=True)

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
            require_contract_storage_contract(page, rows, selected)
            for start, end, *_ in CONTRACTS:
                if start >> 12 <= page <= (end-1) >> 12:
                    with self.assertRaises(ValueError):
                        require_contract_storage_contract(page, [row for row in rows if not start <= row['address'] < end], {start})

    def test_productive_hooks_precede_original_accounting(self):
        import inspect
        disabled = {name: False for name in inspect.signature(generate_page).parameters
                    if name.endswith('_hooks') or name == 'guest_context_transfers'}
        disabled['contract_storage_hooks'] = True
        for page in sorted({r['address'] >> 12 for r in self.records}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            selected = {r['address'] for r in self.records}
            code = generate_page(page, part, selected, userops=self.userops, **disabled)
            require_contract_storage_hooks(page, part, code)
            for entry in CONTRACT_STORAGE_ENTRIES:
                if entry >> 12 != page: continue
                hook = contract_storage_hook(entry)
                step = f's.step(0x{entry:x});'
                for changed in (code.replace(hook, '', 1),
                                code.replace(hook, '', 1).replace(step, step+'\n'+hook, 1),
                                code.replace(hook, hook+'\n'+hook, 1)):
                    with self.subTest(entry=hex(entry)), self.assertRaises(ValueError):
                        require_contract_storage_hooks(page, part, changed)

    def test_reference_retains_all_steps_without_semantic_hooks(self):
        source = render_reference(self.records, self.userops)
        expected = [f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', source, re.MULTILINE), expected)
        self.assertEqual(re.findall(r'contract_storage_step_seen\(s, 0x([0-9a-f]+)\)', source), expected)
        self.assertNotIn('try_modern_', source)
        self.assertNotIn('complete_context_transfer', source)

    def test_shared_sampler_normalization_and_distinct_final_variant(self):
        blocks = [(0x1037304c, 14, 15, 0x84), (0x10373294, 14, 15, 0x84),
                  (0x103735f4, 13, 14, 0x6c), (0x10373884, 13, 14, 0x6c),
                  (0x10373a5c, 13, 14, 0x6c), (0x10373c10, 13, 14, 0x6c)]
        normalized = []
        for start, bound, result, scratch in blocks:
            body = [r for r in self.records if start <= r['address'] < start+0x120]
            self.assertEqual(len(body), 72)
            canonical = []
            for record in body:
                asm = re.sub(r'0x[0-9a-f]+(?=\(r1\))', lambda m: f'SP+{int(m.group(), 16)-scratch}', record['assembly'])
                asm = re.sub(r'0x[0-9a-f]+', lambda m: f'BLOCK+{int(m.group(), 16)-start}'
                             if start <= int(m.group(), 16) <= start+0x120 else m.group(), asm)
                asm = re.sub(r'\br(\d+)\b', lambda m: 'BOUND' if int(m.group(1)) == bound
                             else 'RESULT' if int(m.group(1)) == result else m.group(), asm)
                canonical.append(asm)
            normalized.append(canonical)
        for body in normalized[1:5]: self.assertEqual(body, normalized[0])
        self.assertNotEqual(normalized[5], normalized[0])

    def test_complete_initializer_record_flag_sites(self):
        record_sites = [(r['address'], r['assembly']) for r in self.records
                        if r['assembly'].startswith(('addze.', 'srawi.'))]
        self.assertEqual(record_sites, [(0x10373014, 'addze. r14,r0'),
                                      (0x10373260, 'addze. r14,r0'),
                                      (0x103735bc, 'srawi. r13,r0,0x1')])

    def test_reversed_contract_index_rejects_highest_interior_edge(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            db.executemany('INSERT INTO instructions VALUES (?,?)',
                           [(row['address'], json.dumps(row)) for row in self.records])
            edge = {'address': 0, 'flows': [CONTRACT_STORAGE_ENTRIES[-1]+4], 'fallthrough': None}
            db.execute('INSERT INTO instructions VALUES (?,?)', (0, json.dumps(edge)))
            with patch('contract_storage_audit.CONTRACTS', tuple(reversed(CONTRACTS))):
                with self.assertRaisesRegex(ValueError, 'External contract-storage interior edge'):
                    require_contract_storage_index(db)


if __name__ == '__main__':
    unittest.main()
