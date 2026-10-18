"""Complete original contracts and prepared ownership for transfer record."""
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
from transfer_record_audit import (CONTRACTS, FRAGMENTS, TRANSFER_RECORD_ENTRIES,
    TABLES, load_transfer_record_tables, require_transfer_record_tables, require_transfer_record_bodies, require_transfer_record_contract,
    require_transfer_record_edges, require_transfer_record_index, require_transfer_record_hooks, transfer_record_hook)
from make_transfer_record_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_exact_accepted_prefix_and_complete_candidate_owner(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),859)
        prefix=[{'entry':hex(entry),'module':owner} for entry,owner in SEMANTIC_REPLACEMENTS[:815]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'2f665ed4572db48950a695d15f1a72f5f19ad23efd1931aab81c10dba229c96e')
        self.assertEqual(len(TRANSFER_RECORD_ENTRIES),8)
        self.assertEqual(SEMANTIC_REPLACEMENTS[815:823],tuple((entry,'transfer_record_adapter') for entry in TRANSFER_RECORD_ENTRIES))
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry in TRANSFER_RECORD_ENTRIES:
            with self.subTest(entry=hex(entry)),self.assertRaisesRegex(ValueError,'Duplicate semantic replacement'):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(entry,'other_owner')))

    def test_complete_boundaries_and_external_interior_edges(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS), 806)
        self.assertEqual(sum(row[2] for row in FRAGMENTS), 806)
        for start, end, *_ in CONTRACTS:
            require_transfer_record_edges([(0, start), (start, end-4), (end-4, end)])
            for target in (start+4, end-4):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_transfer_record_edges([(0, target)])

    def test_named_operations_do_not_replay_original_bodies(self):
        source='\n'.join((ROOT/'native'/name).read_text() for name in ['transfer_record_lifecycle.cpp','transfer_record_initialization.cpp','transfer_record_persistence.cpp','transfer_record_value.cpp'])
        for forbidden in ('transfer_record_reference','dispatch_page','run_until','pcode','i_1041','page_1041'):
            self.assertNotIn(forbidden,source)
        for name in ('construct_empty_offer','construct_generated_offer','destroy_offer','read_offer','write_offer','initialize_zero_fee_offer','initialize_priced_offer','offer_value'):
            self.assertIn(name,source)


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_transfer_record_index(db)
            cls.records = [json.loads(row[0]) for start, end, *_ in CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_original_tables_and_live_pointers_are_pinned(self):
        import struct
        program=json.loads((INPUT/'program.json').read_text())
        segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
        def read(a,n):
            for base,data in segments:
                if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
            raise ValueError('Missing original table')
        require_transfer_record_tables(read);load_transfer_record_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [slot,*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_transfer_record_tables(changed)

    def test_fourteen_complete_diagnostic_blocks_normalize(self):
        bodies=[]
        cases=[(0x10411fe4,31,30,0x140,0x78),(0x10412054,31,30,0x120,0x70),(0x104120bc,31,30,0x100,0x68),(0x10412154,31,30,0xe0,0x60),(0x104121cc,31,30,0xc0,0x58),(0x1041229c,31,30,0xa0,0x50),(0x10412338,31,30,0x80,0x40),(0x1041241c,30,29,0xe4,0x5c),(0x104124a8,30,29,0xc4,0x54),(0x10412530,30,29,0xa4,0x4c),(0x104125cc,30,29,0x84,0x44),(0x10412654,30,29,0x64,0x3c),(0x104163fc,31,30,0x40,0x38),(0x10416538,25,24,0x40,0x38)]
        for start,table,buffer,first,second in cases:
            rows=[r for r in self.records if start<=r['address']<start+0x48]
            self.assertEqual(len(rows),18)
            normalized=[]
            for row in rows:
                text=re.sub(r'\br'+str(table)+r'\b','TABLE',row['assembly'])
                text=re.sub(r'\br'+str(buffer)+r'\b','BUFFER',text)
                text=text.replace(f'addi r6,r1,{hex(first)}','FIRST_SCRATCH').replace(f'addi r7,r1,{hex(second)}','SECOND_SCRATCH')
                text=re.sub(r'li r8,0x[0-9a-f]+','LINE',text)
                normalized.append(text)
            bodies.append(normalized)
        for other in bodies[1:]:self.assertEqual(bodies[0],other)

    def test_changed_original_fields_are_rejected(self):
        for start, page, *_ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page and row['address'] >= start)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records); changed[index][key] = value
                with self.subTest(entry=hex(start), page=hex(page), key=key), self.assertRaises(ValueError):
                    require_transfer_record_bodies(changed, complete=True)

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
            require_transfer_record_contract(page, rows, selected)
            for start, end, *_ in CONTRACTS:
                if start >> 12 <= page <= (end-1) >> 12:
                    with self.assertRaises(ValueError):
                        require_transfer_record_contract(page, [row for row in rows if not start <= row['address'] < end], {start})

    def test_productive_hooks_precede_original_accounting(self):
        import inspect
        disabled = {name: False for name in inspect.signature(generate_page).parameters
                    if name.endswith('_hooks') or name == 'guest_context_transfers'}
        disabled['transfer_record_hooks'] = True
        for page in sorted({r['address'] >> 12 for r in self.records}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            selected = {r['address'] for r in self.records}
            code = generate_page(page, part, selected, userops=self.userops, **disabled)
            require_transfer_record_hooks(page, part, code)
            for entry in TRANSFER_RECORD_ENTRIES:
                if entry >> 12 != page: continue
                hook = transfer_record_hook(entry)
                step = f's.step(0x{entry:x});'
                for changed in (code.replace(hook, '', 1),
                                code.replace(hook, '', 1).replace(step, step+'\n'+hook, 1),
                                code.replace(hook, hook+'\n'+hook, 1)):
                    with self.subTest(entry=hex(entry)), self.assertRaises(ValueError):
                        require_transfer_record_hooks(page, part, changed)

    def test_reference_retains_all_steps_without_semantic_hooks(self):
        source = render_reference(self.records, self.userops)
        expected = [f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', source, re.MULTILINE), expected)
        self.assertEqual(re.findall(r'transfer_record_step_seen\(s, 0x([0-9a-f]+)\)', source), expected)
        self.assertNotIn('try_modern_', source)
        self.assertNotIn('complete_context_transfer', source)
        self.assertIn('transfer_record_expected_sites()',source)
        self.assertIn(','.join(hex(row['address']) for row in self.records),source)

    def test_reversed_contract_index_rejects_highest_interior_edge(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            db.executemany('INSERT INTO instructions VALUES (?,?)',
                           [(row['address'], json.dumps(row)) for row in self.records])
            edge = {'address': 0, 'flows': [TRANSFER_RECORD_ENTRIES[-1]+4], 'fallthrough': None}
            db.execute('INSERT INTO instructions VALUES (?,?)', (0, json.dumps(edge)))
            with patch('transfer_record_audit.CONTRACTS', tuple(reversed(CONTRACTS))):
                with self.assertRaisesRegex(ValueError, 'External transfer-record interior edge'):
                    require_transfer_record_index(db)


if __name__ == '__main__':
    unittest.main()
