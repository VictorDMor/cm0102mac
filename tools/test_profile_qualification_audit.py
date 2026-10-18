"""Prepared complete profile qualification and category-reference contracts."""
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
from profile_qualification_audit import (CONTRACTS,FRAGMENTS,PROFILE_QUALIFICATION_ENTRIES,TABLES,PADDING,
    load_profile_qualification_tables,require_profile_qualification_tables,require_profile_qualification_padding,
    require_profile_qualification_bodies,require_profile_qualification_contract,require_profile_qualification_edges,
    require_profile_qualification_index,require_profile_qualification_hooks,profile_qualification_hook)
from make_profile_qualification_reference import render_reference
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'

class PreparedScopeTests(unittest.TestCase):
    def test_exact_prefix_and_four_productive_owners(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),871)
        prefix=[{'entry':hex(e),'module':m} for e,m in SEMANTIC_REPLACEMENTS[:864]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'9ecc35d26bb22037787db2192fafcb04402e5704d7e9d12205f17925e3091359')
        self.assertEqual(SEMANTIC_REPLACEMENTS[864:868],tuple((e,'profile_qualification_adapter') for e in PROFILE_QUALIFICATION_ENTRIES))
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for e in PROFILE_QUALIFICATION_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(e,'duplicate')))

    def test_whole_boundaries_and_holes(self):
        self.assertEqual(sum(x[2] for x in CONTRACTS),287)
        self.assertEqual(tuple((hex(a),hex(b),n) for a,b,n,_ in CONTRACTS),(
            ('0x10117360','0x10117488',73),('0x10117490','0x10117688',126),
            ('0x1036f670','0x1036f6e8',30),('0x1036f6f0','0x1036f7d8',58)))
        self.assertEqual(len(PADDING),9)
        self.assertEqual(sum(x[2] for x in FRAGMENTS),287)
        self.assertIn((0x10117404,'60000000'),PADDING)
        self.assertEqual(tuple(x[2] for x in TABLES),(16,12,47))
        for start,end,*_ in CONTRACTS:
            require_profile_qualification_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.assertRaises(ValueError):require_profile_qualification_edges([(0,target)])

    def test_named_complete_native_operations_without_replay(self):
        source=''.join((ROOT/('native/profile_qualification_'+name+'.cpp')).read_text() for name in ('masks','classify','category'))
        for token in ('dispatch_page','run_until','switch (s.pc)','goto ','_reference(s)','pcode','while(s.pc'):
            self.assertNotIn(token,source)
        for token in ('combine_qualification_masks','classify_profile_qualifications','candidate_category_reference','club_category_reference','scan_primary_qualifications','scan_secondary_qualifications','report_missing_profile','category_four_reference'):
            self.assertIn(token,source)
        self.assertIn('profile_qualification_adapter.cpp',(ROOT/'native/CMakeLists.txt').read_text())
        self.assertNotIn('0x107a6ba8',source);self.assertNotIn('0x107a6bb8',source)

@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(),'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_profile_qualification_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        program=json.loads((INPUT/'program.json').read_text());cls.userops=program['userops']
        cls.segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    @classmethod
    def read(cls,a,n):
        for base,data in cls.segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError('Missing original profile-qualification data')

    def test_original_mask_banks_diagnostic_text_and_slots_are_pinned(self):
        require_profile_qualification_tables(self.read);load_profile_qualification_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [*range(slot,slot+4),*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(self.read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_profile_qualification_tables(changed)
        self.assertEqual(len(TABLES),3)

    def test_nonindexed_words_including_skipped_nop_are_pinned(self):
        require_profile_qualification_padding(self.read);self.assertEqual(len(PADDING),9)
        self.assertFalse({a for a,_ in PADDING}&{r['address'] for r in self.records})
        for address,_ in PADDING:
            def changed(a,n):
                b=bytearray(self.read(a,n))
                if a<=address<a+n:b[address-a]^=1
                return bytes(b)
            with self.assertRaises(ValueError):require_profile_qualification_padding(changed)

    def test_productive_hooks_precede_original_accounting(self):
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['profile_qualification_hooks']=True
        for page in sorted({f[1] for f in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page]
            source=generate_page(page,rows,{r['address'] for r in self.records},userops=self.userops,**disabled)
            require_profile_qualification_hooks(page,rows,source)
            for entry in PROFILE_QUALIFICATION_ENTRIES:
                if entry>>12!=page:continue
                hook=profile_qualification_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_profile_qualification_hooks(page,rows,changed)

    def test_reference_keeps_every_original_instruction_and_disables_all_hooks(self):
        source=render_reference(self.records,self.userops);self.assertNotIn('try_modern_',source);self.assertNotIn('_impl<true,',source)
        for e in PROFILE_QUALIFICATION_ENTRIES:self.assertIn(f'case 0x{e:x}:',source)
        for r in self.records:self.assertIn(f's.step(0x{r["address"]:x});',source)

    def test_changed_original_fields_fail(self):
        for start,page,*_ in FRAGMENTS:
            i=next(i for i,r in enumerate(self.records) if r['address']>=start and r['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[i][key]=value
                with self.subTest(start=hex(start),page=hex(page),key=key),self.assertRaises(ValueError):require_profile_qualification_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_bodies_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,r in enumerate(self.records) if r['address']==start)
            for changed in (self.records[:i]+self.records[i+1:],self.records+[self.records[i]],list(reversed(self.records)),[r for r in self.records if not start<=r['address']<end]):
                with self.subTest(start=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_partial_page_omissions_fail(self):
        for page in sorted({x[1] for x in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page];selected={r['address'] for r in self.records};require_profile_qualification_contract(page,rows,selected)
            for start,end,*_ in CONTRACTS:
                if start>>12<=page<=(end-1)>>12:
                    with self.assertRaises(ValueError):require_profile_qualification_contract(page,[r for r in rows if not start<=r['address']<end],{start})

if __name__=='__main__':unittest.main()
