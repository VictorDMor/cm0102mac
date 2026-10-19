"""Complete original contracts and prepared ownership for transfer valuation."""
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
from transfer_valuation_audit import (CONTRACTS, FRAGMENTS, TRANSFER_VALUATION_ENTRIES,
    TABLES, load_transfer_valuation_tables, require_transfer_valuation_tables, require_transfer_valuation_bodies, require_transfer_valuation_contract,
    require_transfer_valuation_edges, require_transfer_valuation_index, require_transfer_valuation_hooks, transfer_valuation_hook)
from make_transfer_valuation_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_exact_accepted_prefix_and_complete_candidate_owner(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),939)
        prefix=[{'entry':hex(entry),'module':owner} for entry,owner in SEMANTIC_REPLACEMENTS[:815]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'2f665ed4572db48950a695d15f1a72f5f19ad23efd1931aab81c10dba229c96e')
        self.assertEqual(len(TRANSFER_VALUATION_ENTRIES),8)
        self.assertEqual(SEMANTIC_REPLACEMENTS[823:831],tuple((entry,'transfer_valuation_adapter') for entry in TRANSFER_VALUATION_ENTRIES))
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry in TRANSFER_VALUATION_ENTRIES:
            with self.subTest(entry=hex(entry)),self.assertRaisesRegex(ValueError,'Duplicate semantic replacement'):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(entry,'other_owner')))

    def test_complete_boundaries_and_external_interior_edges(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS), 1188)
        self.assertEqual(sum(row[2] for row in FRAGMENTS), 1188)
        for start, end, *_ in CONTRACTS:
            require_transfer_valuation_edges([(0, start), (start, end-4), (end-4, end)])
            for target in (start+4, end-4):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_transfer_valuation_edges([(0, target)])

    def test_named_operations_do_not_replay_original_bodies(self):
        source='\n'.join((ROOT/'native'/name).read_text() for name in ['transfer_valuation_sampling.cpp','transfer_valuation_amounts.cpp','transfer_valuation_comparison.cpp','transfer_valuation_exchange.cpp','transfer_valuation_rebalance.cpp','range_sampling_detail.hpp','transfer_valuation_detail.hpp'])
        for forbidden in ('transfer_valuation_reference','dispatch_page','run_until','pcode','i_1041','page_1041'):
            self.assertNotIn(forbidden,source)
        for name in ('sample_offer_range','adjusted_offer_value','positive_installment_total','staff_offer_status','equivalent_offer_terms','remove_exchange_staff','find_exchange_staff','rebalance_offer_terms'):
            self.assertIn(name,source)

    def test_shared_sampler_wrappers_select_each_original_layout(self):
        self.assertIn('{sample_contract_range_entry,-0x4dac,0}',(ROOT/'native/contract_terms_sampling.cpp').read_text())
        self.assertIn('{sample_offer_range_entry,-0x4b8c,0xa0}',(ROOT/'native/transfer_valuation_sampling.cpp').read_text())
        engine=(ROOT/'native/range_sampling_detail.hpp').read_text()
        self.assertIn('SemanticGuestCall::call(layout.entry)',engine)
        self.assertIn('layout.entry+offset',engine)
        self.assertNotIn('0x10374',engine)


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_transfer_valuation_index(db)
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
        require_transfer_valuation_tables(read);load_transfer_valuation_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [slot,*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_transfer_valuation_tables(changed)

    def test_complete_diagnostic_reuses_the_record_contract(self):
        rows=[r for r in self.records if 0x10417124<=r['address']<0x1041716c]
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            old=[json.loads(row[0]) for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(0x10411fe4,0x1041202c))]
        self.assertEqual(len(rows),18);self.assertEqual(len(old),18)
        def normalize(records,table,buffer,first,second):
            result=[]
            for row in records:
                text=re.sub(r'\br'+str(table)+r'\b','TABLE',row['assembly']);text=re.sub(r'\br'+str(buffer)+r'\b','BUFFER',text)
                text=text.replace(f'addi r6,r1,{hex(first)}','FIRST_SCRATCH').replace(f'addi r7,r1,{hex(second)}','SECOND_SCRATCH');text=re.sub(r'li r8,0x[0-9a-f]+','LINE',text);result.append(text)
            return result
        self.assertEqual(normalize(old,31,30,0x140,0x78),normalize(rows,28,27,0x40,0x38))

    def test_complete_shared_sampler_has_exactly_five_layout_changes(self):
        from contract_terms_audit import require_contract_terms_bodies
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            old=[json.loads(row[0]) for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(0x10374570,0x10374ae8))]
        require_contract_terms_bodies(old);new=[row for row in self.records if 0x10415e50<=row['address']<0x104163c8];self.assertEqual(len(old),350);self.assertEqual(len(new),350)
        delta=0x10415e50-0x10374570
        def relocate(v):return v+delta if v is not None and 0x10374570<=v<0x10374ae8 else v
        differences=[]
        for a,b in zip(old,new):
            self.assertEqual(a['address']+delta,b['address']);self.assertEqual([relocate(v) for v in a['flows']],b['flows']);self.assertEqual(relocate(a.get('fallthrough')),b.get('fallthrough'))
            text=re.sub(r'0x[0-9a-f]+',lambda m:hex(relocate(int(m[0],16))),a['assembly'])
            if text!=b['assembly']:differences.append((a['address'],a['assembly'],b['assembly']))
        self.assertEqual(differences,[(0x1037457c,'lwz r25,-0x4dac(r2)','lwz r25,-0x4b8c(r2)'),*[(pc,'lfd f0,0x0(r25)','lfd f0,0xa0(r25)') for pc in (0x1037461c,0x10374708,0x103747f4,0x103748e0)]])

    def test_changed_original_fields_are_rejected(self):
        for start, page, *_ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page and row['address'] >= start)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records); changed[index][key] = value
                with self.subTest(entry=hex(start), page=hex(page), key=key), self.assertRaises(ValueError):
                    require_transfer_valuation_bodies(changed, complete=True)

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
            require_transfer_valuation_contract(page, rows, selected)
            for start, end, *_ in CONTRACTS:
                if start >> 12 <= page <= (end-1) >> 12:
                    with self.assertRaises(ValueError):
                        require_transfer_valuation_contract(page, [row for row in rows if not start <= row['address'] < end], {start})

    def test_productive_hooks_precede_original_accounting(self):
        import inspect
        disabled = {name: False for name in inspect.signature(generate_page).parameters
                    if name.endswith('_hooks') or name == 'guest_context_transfers'}
        disabled['transfer_valuation_hooks'] = True
        for page in sorted({r['address'] >> 12 for r in self.records}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            selected = {r['address'] for r in self.records}
            code = generate_page(page, part, selected, userops=self.userops, **disabled)
            require_transfer_valuation_hooks(page, part, code)
            for entry in TRANSFER_VALUATION_ENTRIES:
                if entry >> 12 != page: continue
                hook = transfer_valuation_hook(entry)
                step = f's.step(0x{entry:x});'
                for changed in (code.replace(hook, '', 1),
                                code.replace(hook, '', 1).replace(step, step+'\n'+hook, 1),
                                code.replace(hook, hook+'\n'+hook, 1)):
                    with self.subTest(entry=hex(entry)), self.assertRaises(ValueError):
                        require_transfer_valuation_hooks(page, part, changed)

    def test_reference_retains_all_steps_without_semantic_hooks(self):
        source = render_reference(self.records, self.userops)
        expected = [f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', source, re.MULTILINE), expected)
        self.assertEqual(re.findall(r'transfer_valuation_step_seen\(s, 0x([0-9a-f]+)\)', source), expected)
        self.assertNotIn('try_modern_', source)
        self.assertNotIn('complete_context_transfer', source)
        self.assertIn('transfer_valuation_expected_sites()',source)
        self.assertIn(','.join(hex(row['address']) for row in self.records),source)

    def test_reversed_contract_index_rejects_highest_interior_edge(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            db.executemany('INSERT INTO instructions VALUES (?,?)',
                           [(row['address'], json.dumps(row)) for row in self.records])
            edge = {'address': 0, 'flows': [TRANSFER_VALUATION_ENTRIES[-1]+4], 'fallthrough': None}
            db.execute('INSERT INTO instructions VALUES (?,?)', (0, json.dumps(edge)))
            with patch('transfer_valuation_audit.CONTRACTS', tuple(reversed(CONTRACTS))):
                with self.assertRaisesRegex(ValueError, 'External transfer-valuation interior edge'):
                    require_transfer_valuation_index(db)


if __name__ == '__main__':
    unittest.main()
