"""Complete automatic-offer boundary, independent reference and prepared scope."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, require_unique_semantic_replacements, generate_page
from transfer_synthesis_audit import (CONTRACTS,FRAGMENTS,TABLES,TRANSFER_SYNTHESIS_ENTRIES,
    require_transfer_synthesis_bodies,require_transfer_synthesis_edges,require_transfer_synthesis_contract,
    require_transfer_synthesis_index,require_transfer_synthesis_tables,load_transfer_synthesis_tables,
    require_transfer_synthesis_hooks,transfer_synthesis_hook)
from make_transfer_synthesis_reference import render_reference

ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'

class PreparedScopeTests(unittest.TestCase):
    def test_accepted_prefix_is_unchanged_and_candidate_has_one_new_owner(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),963)
        prefix=[{'entry':hex(entry),'module':owner} for entry,owner in SEMANTIC_REPLACEMENTS[:831]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'04f640a10822a7b413181c6611ca8fcb874072e85da005a69c9b597888321c8d')
        self.assertEqual(TRANSFER_SYNTHESIS_ENTRIES,(0x104126d0,))
        self.assertEqual(SEMANTIC_REPLACEMENTS[831:832],((0x104126d0,'transfer_synthesis_adapter'),))
        candidate=SEMANTIC_REPLACEMENTS
        self.assertEqual(len(candidate),963);require_unique_semantic_replacements(candidate)
        with self.assertRaisesRegex(ValueError,'Duplicate semantic replacement'):
            require_unique_semantic_replacements((*candidate,(0x104126d0,'other_owner')))

    def test_full_large_boundary_and_external_interior_edges(self):
        self.assertEqual(CONTRACTS[0][:3],(0x104126d0,0x10415e4c,3551))
        self.assertEqual(sum(r[2] for r in FRAGMENTS),3551)
        start,end,*_=CONTRACTS[0]
        require_transfer_synthesis_edges([(0,start),(start,end-4),(end-4,end)])
        for target in (start+4,end-4):
            with self.assertRaises(ValueError):require_transfer_synthesis_edges([(0,target)])

    def test_named_logic_has_no_original_body_replay(self):
        names=['transfer_synthesis.cpp','transfer_synthesis_preparation.cpp','transfer_synthesis_base.cpp','transfer_synthesis_terms.cpp','transfer_synthesis_distribution.cpp','transfer_synthesis_refinement.cpp','transfer_synthesis_finalization.cpp','transfer_synthesis_detail.hpp','transfer_synthesis_terms.hpp']
        source='\n'.join((ROOT/'native'/n).read_text() for n in names)
        for forbidden in ('transfer_synthesis_reference','dispatch_page','run_until','pcode','i_1041','page_1041'):
            self.assertNotIn(forbidden,source)
        for name in ('synthesize_offer','prepare_offer','select_base_and_period','constrain_payment_distribution','refine_offer_value','finalize_offer'):
            self.assertIn(name,source)

@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(),'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_transfer_synthesis_index(db)
            cls.records=[json.loads(r[0]) for start,end,*_ in CONTRACTS for r in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']

    def body(self,start,end):return [r for r in self.records if start<=r['address']<end]

    def test_every_changed_original_field_is_rejected(self):
        for _,page,*_ in FRAGMENTS:
            index=next(n for n,r in enumerate(self.records) if r['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[index][key]=value
                with self.subTest(page=hex(page),key=key),self.assertRaises(ValueError):require_transfer_synthesis_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_original_fails(self):
        for changed in (self.records[1:],self.records+[self.records[0]],list(reversed(self.records)),[]):
            with self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_partial_pages_cannot_claim_complete_fragment(self):
        for _,page,*_ in FRAGMENTS:
            rows=[r for r in self.records if r['address']>>12==page];selected={r['address'] for r in self.records}
            require_transfer_synthesis_contract(page,rows,selected)
            for changed in (rows[1:],rows[:-1],[]):
                with self.assertRaises(ValueError):require_transfer_synthesis_contract(page,changed,selected)

    def test_all_table_bytes_and_live_pointers_are_pinned(self):
        program=json.loads((INPUT/'program.json').read_text());segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
        def read(a,n):
            for base,data in segments:
                if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
            raise ValueError('Missing original table')
        require_transfer_synthesis_tables(read);load_transfer_synthesis_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [slot,*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_transfer_synthesis_tables(changed)

    def test_reference_retains_every_instruction_and_disables_all_hooks(self):
        source=render_reference(self.records,self.userops);expected=[f'{r["address"]:x}' for r in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):',source,re.MULTILINE),expected)
        self.assertEqual(re.findall(r'transfer_synthesis_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        self.assertNotIn('try_modern_',source);self.assertNotIn('complete_context_transfer',source)
        self.assertIn(','.join(hex(r['address']) for r in self.records),source)

    def test_productive_hook_precedes_original_accounting(self):
        import inspect
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['transfer_synthesis_hooks']=True
        page=0x104126d0>>12;part=[r for r in self.records if r['address']>>12==page]
        source=generate_page(page,part,{r['address'] for r in self.records},userops=self.userops,**disabled)
        require_transfer_synthesis_hooks(page,part,source);hook=transfer_synthesis_hook(0x104126d0);step='s.step(0x104126d0);'
        for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
            with self.assertRaises(ValueError):require_transfer_synthesis_hooks(page,part,changed)

    def test_actual_call_forms_and_original_policy_row_layout(self):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            def assembly(pc):
                row=db.execute('SELECT body FROM instructions WHERE address=?',(pc,)).fetchone()
                self.assertIsNotNone(row);return json.loads(row[0])['assembly']
            for pc in (0x1036e198,0x103e358c,0x10411eec):self.assertEqual(assembly(pc),'bl 0x104126d0')
            self.assertEqual(assembly(0x1036e190),'add r6,r0,r23')
            self.assertEqual(assembly(0x1036e194),'lwz r7,0x8(r6)')
            self.assertEqual(assembly(0x103e3584),'li r6,0x0')
            for pc,zero in ((0x1009e51c,0x1009e50c),(0x10417f3c,0x10417f30),(0x10423344,0x10423338)):
                self.assertEqual(assembly(pc),'bl 0x10411ea0');self.assertEqual(assembly(zero),'li r6,0x0')
            self.assertEqual(assembly(0x103df4c0),'bl 0x10356410')
            self.assertEqual(assembly(0x103df4c8),'stw r23,0x988(r3)')
            self.assertEqual(assembly(0x10356630),'li r4,0xb')
            self.assertEqual(assembly(0x1035664c),'stbx r4,r3,r26')
            self.assertEqual(assembly(0x1035672c),'cmpwi r22,0x28')
            self.assertEqual(assembly(0x10356730),'addi r26,r26,0x2c')

    def test_original_staff_status_service_preparation(self):
        expected={0x1034530c:'li r3,0x10',0x1034531c:'li r4,0x0',0x10345320:'bl 0x100031b0',0x10345324:'lwz r3,-0x7b24(r2)',0x1034532c:'stw r15,0x0(r3)',0x100031d4:'stw r0,0xc(r3)',0x1000328c:'bl 0x10775200'}
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            for pc,assembly in expected.items():
                row=db.execute('SELECT body FROM instructions WHERE address=?',(pc,)).fetchone();self.assertIsNotNone(row);self.assertEqual(json.loads(row[0])['assembly'],assembly)

    def test_ordinary_amount_blocks_have_only_declared_increment_changes(self):
        def normalized(start,increment):
            rows=self.body(start,start+0xac);self.assertEqual(len(rows),43)
            return [r['assembly'].replace(f'addi r0,r3,{hex(increment)}','INCREMENT') for r in rows]
        self.assertEqual(normalized(0x10413a58,5),normalized(0x10413cfc,10))
        self.assertEqual(normalized(0x10413a58,5),normalized(0x10413f9c,5))
        def alternate(start,increment):
            rows=self.body(start,start+0xac);self.assertEqual(len(rows),43)
            return [r['assembly'].replace(f'addi r0,r3,{hex(increment)}','INCREMENT') for r in rows]
        old=alternate(0x104146d4,5)
        for pc,increment in ((0x10414e1c,5),(0x104151c8,5),(0x104153c8,10),(0x104155c4,5)):
            self.assertEqual(old,alternate(pc,increment))

    def test_three_payment_samplers_share_the_complete_original_body(self):
        def normalized(start):
            rows=self.body(start,start+0x120);self.assertEqual(len(rows),72)
            def reloc(m):
                value=int(m[0],16)
                return hex(value-start) if start<=value<=start+0x120 else m[0]
            return [re.sub('0x[0-9a-f]+',reloc,r['assembly']) for r in rows]
        for start in (0x104147c0,0x10414c5c):self.assertEqual(normalized(0x104144b4),normalized(start))

if __name__=='__main__':unittest.main()
