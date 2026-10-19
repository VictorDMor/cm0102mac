"""Prepared complete contract draft creation contracts."""
import inspect
from compile_recomp import generate_page
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS,require_unique_semantic_replacements
from contract_draft_audit import (CONTRACTS,FRAGMENTS,CONTRACT_DRAFT_ENTRIES,TABLES,PADDING,
    load_contract_draft_tables,require_contract_draft_tables,require_contract_draft_padding,
    require_contract_draft_bodies,require_contract_draft_contract,require_contract_draft_edges,
    require_contract_draft_index,require_contract_draft_hooks,contract_draft_hook)
from make_contract_draft_reference import render_reference
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'

class PreparedScopeTests(unittest.TestCase):
    def test_exact_prefix_and_three_productive_owners(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),911)
        prefix=[{'entry':hex(e),'module':m} for e,m in SEMANTIC_REPLACEMENTS[:868]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'7281368f2a8cc1c916ef97eab3543b05c534ff206a873d3ea8a92e89e255038b')
        self.assertEqual(SEMANTIC_REPLACEMENTS[868:871],tuple((e,'contract_draft_adapter') for e in CONTRACT_DRAFT_ENTRIES))
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for e in CONTRACT_DRAFT_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(e,'duplicate')))

    def test_whole_creation_scope_and_owned_dependency_boundary(self):
        self.assertEqual(sum(x[2] for x in CONTRACTS),228);self.assertEqual(sum(x[2] for x in FRAGMENTS),228)
        self.assertEqual(tuple((hex(a),hex(b),n) for a,b,n,_ in CONTRACTS),(
            ('0x103721f0','0x10372254',25),('0x10372260','0x103723b0',84),('0x103723b0','0x1037258c',119)))
        self.assertEqual(len(PADDING),4);self.assertEqual(tuple(x[2] for x in TABLES),(39,))
        owners=dict(SEMANTIC_REPLACEMENTS)
        for e in (0x10374b60,0x10374b80,0x10380700):self.assertEqual(owners[e],'contract_terms_adapter')
        self.assertEqual(owners[0x1037f050],'contract_review_adapter');self.assertEqual(owners[0x103727c0],'contract_storage_adapter')
        for start,end,*_ in CONTRACTS:
            require_contract_draft_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.assertRaises(ValueError):require_contract_draft_edges([(0,target)])

    def test_complete_native_stages_without_replay_or_blanket_clear(self):
        source=''.join((ROOT/('native/contract_draft_'+name+'.cpp')).read_text() for name in ('initialize','new','primary'))
        for token in ('dispatch_page','run_until','switch (s.pc)','goto ','_reference(s)','pcode','while(s.pc','memory.clear','memset'):
            self.assertNotIn(token,source)
        for token in ('initialize_draft','prepare_new_contract','prepare_from_primary_contract','report_missing_contract','clear_clause_tail','a.product(6,8,6);a.multiply_high(6,9,6)'):
            self.assertIn(token,source)
        self.assertIn('contract_draft_adapter.cpp',(ROOT/'native/CMakeLists.txt').read_text());self.assertNotIn('STAFF_CONTRACTS.CPP',source)

@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(),'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_contract_draft_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        program=json.loads((INPUT/'program.json').read_text());cls.userops=program['userops']
        cls.segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    @classmethod
    def read(cls,a,n):
        for base,data in cls.segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError('Missing original contract-draft data')

    def test_original_diagnostic_text_and_pointer_are_pinned(self):
        require_contract_draft_tables(self.read);load_contract_draft_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [*range(slot,slot+4),*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(self.read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_contract_draft_tables(changed)
        self.assertEqual(len(TABLES),1)

    def test_nonindexed_alignment_words_are_pinned(self):
        require_contract_draft_padding(self.read);self.assertEqual(len(PADDING),4)
        self.assertFalse({a for a,_ in PADDING}&{r['address'] for r in self.records})
        for address,_ in PADDING:
            def changed(a,n):
                b=bytearray(self.read(a,n))
                if a<=address<a+n:b[address-a]^=1
                return bytes(b)
            with self.assertRaises(ValueError):require_contract_draft_padding(changed)

    def test_productive_hooks_precede_original_accounting(self):
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['contract_draft_hooks']=True
        for page in sorted({f[1] for f in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page]
            source=generate_page(page,rows,{r['address'] for r in self.records},userops=self.userops,**disabled)
            require_contract_draft_hooks(page,rows,source)
            for entry in CONTRACT_DRAFT_ENTRIES:
                if entry>>12!=page:continue
                hook=contract_draft_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_contract_draft_hooks(page,rows,changed)

    def test_reference_keeps_every_original_instruction_and_disables_all_hooks(self):
        source=render_reference(self.records,self.userops);self.assertNotIn('try_modern_',source);self.assertNotIn('_impl<true,',source)
        for e in CONTRACT_DRAFT_ENTRIES:self.assertIn(f'case 0x{e:x}:',source)
        for r in self.records:self.assertIn(f's.step(0x{r["address"]:x});',source)

    def test_changed_original_fields_fail(self):
        for start,page,*_ in FRAGMENTS:
            i=next(i for i,r in enumerate(self.records) if r['address']>=start and r['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[i][key]=value
                with self.subTest(start=hex(start),page=hex(page),key=key),self.assertRaises(ValueError):require_contract_draft_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_bodies_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,r in enumerate(self.records) if r['address']==start)
            for changed in (self.records[:i]+self.records[i+1:],self.records+[self.records[i]],list(reversed(self.records)),[r for r in self.records if not start<=r['address']<end]):
                with self.subTest(start=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_partial_page_omissions_fail(self):
        for page in sorted({x[1] for x in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page];selected={r['address'] for r in self.records};require_contract_draft_contract(page,rows,selected)
            for start,end,*_ in CONTRACTS:
                if start>>12<=page<=(end-1)>>12:
                    with self.assertRaises(ValueError):require_contract_draft_contract(page,[r for r in rows if not start<=r['address']<end],{start})

if __name__=='__main__':unittest.main()
