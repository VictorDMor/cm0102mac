"""Prepared complete shortlist recruitment distribution and registration contracts."""
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
from shortlist_recruitment_audit import (CONTRACTS,FRAGMENTS,SHORTLIST_RECRUITMENT_ENTRIES,TABLES,PADDING,
    load_shortlist_recruitment_tables,require_shortlist_recruitment_tables,require_shortlist_recruitment_padding,
    require_shortlist_recruitment_bodies,require_shortlist_recruitment_contract,require_shortlist_recruitment_edges,
    require_shortlist_recruitment_index,require_shortlist_recruitment_hooks,shortlist_recruitment_hook)
from make_shortlist_recruitment_reference import render_reference
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'

class PreparedScopeTests(unittest.TestCase):
    def test_exact_prefix_and_three_productive_owners(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),864)
        prefix=[{'entry':hex(e),'module':m} for e,m in SEMANTIC_REPLACEMENTS[:854]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'1b93e2d35b496b6ed2a83fd4086f58cba8a0d17147b2af01db9df1b6f6337982')
        self.assertEqual(SEMANTIC_REPLACEMENTS[854:857],tuple((e,'shortlist_recruitment_adapter') for e in SHORTLIST_RECRUITMENT_ENTRIES))
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for e in SHORTLIST_RECRUITMENT_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(e,'duplicate')))

    def test_whole_boundaries_exclude_adjacent_loan_and_scanning_controllers(self):
        self.assertEqual(sum(x[2] for x in CONTRACTS),1591)
        self.assertEqual(sum(x[2] for x in FRAGMENTS),1591)
        for e in (0x103636b0,0x10366470):self.assertNotIn(e,SHORTLIST_RECRUITMENT_ENTRIES)
        for start,end,*_ in CONTRACTS:
            require_shortlist_recruitment_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.assertRaises(ValueError):require_shortlist_recruitment_edges([(0,target)])

    def test_existing_storage_and_pair_comparators_keep_their_owners(self):
        owners=dict(SEMANTIC_REPLACEMENTS)
        self.assertEqual(owners[0x103568e0],'club_shortlist_storage_adapter')
        for entry in (0x103663b0,0x10366410,0x10366250):
            self.assertEqual(owners[entry],'shortlist_category_adapter');self.assertNotIn(entry,SHORTLIST_RECRUITMENT_ENTRIES)

    def test_named_operations_do_not_replay_original_code(self):
        names=('notify','register','requested')
        source='\n'.join((ROOT/f'native/shortlist_recruitment_{n}.cpp').read_text() for n in names)
        for token in ('shortlist_recruitment_reference','dispatch_page','run_until','pcode','i_1036','page_1036'):self.assertNotIn(token,source)
        self.assertIn('prepare_registration',source);self.assertIn('requested_previous_contribution',source)
        self.assertIn('expand_role_qualifications',(ROOT/'native/shortlist_recruitment_detail.hpp').read_text())
        self.assertIn('shortlist_recruitment_adapter.cpp',(ROOT/'native/CMakeLists.txt').read_text())

@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(),'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_shortlist_recruitment_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        program=json.loads((INPUT/'program.json').read_text());cls.userops=program['userops']
        cls.segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    @classmethod
    def read(cls,a,n):
        for base,data in cls.segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError('Missing original shortlist-recruitment data')

    def test_floating_constants_and_owned_comparator_vector_are_pinned(self):
        require_shortlist_recruitment_tables(self.read);load_shortlist_recruitment_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [*range(slot,slot+4),*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(self.read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_shortlist_recruitment_tables(changed)
        self.assertEqual(self.read(0x108b920c,8),bytes.fromhex('10366410108bba90'))

    def test_nonindexed_alignment_words_are_pinned(self):
        require_shortlist_recruitment_padding(self.read);self.assertEqual(len(PADDING),5)
        self.assertFalse({a for a,_ in PADDING}&{r['address'] for r in self.records})
        self.assertEqual(next(r['assembly'] for r in self.records if r['address']==0x1036426c),'ori r0,r0,0x0')
        for address,_ in PADDING:
            def changed(a,n):
                b=bytearray(self.read(a,n))
                if a<=address<a+n:b[address-a]^=1
                return bytes(b)
            with self.assertRaises(ValueError):require_shortlist_recruitment_padding(changed)

    def test_productive_hooks_precede_original_accounting(self):
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['shortlist_recruitment_hooks']=True
        for page in sorted({f[1] for f in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page]
            source=generate_page(page,rows,{r['address'] for r in self.records},userops=self.userops,**disabled)
            require_shortlist_recruitment_hooks(page,rows,source)
            for entry in SHORTLIST_RECRUITMENT_ENTRIES:
                if entry>>12!=page:continue
                hook=shortlist_recruitment_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_shortlist_recruitment_hooks(page,rows,changed)

    def test_reference_keeps_every_original_instruction_and_disables_all_hooks(self):
        source=render_reference(self.records,self.userops);self.assertNotIn('try_modern_',source);self.assertNotIn('_impl<true,',source)
        for e in SHORTLIST_RECRUITMENT_ENTRIES:self.assertIn(f'case 0x{e:x}:',source)
        for r in self.records:self.assertIn(f's.step(0x{r["address"]:x});',source)

    def test_changed_original_fields_fail(self):
        for start,page,*_ in FRAGMENTS:
            i=next(i for i,r in enumerate(self.records) if r['address']>=start and r['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[i][key]=value
                with self.subTest(start=hex(start),page=hex(page),key=key),self.assertRaises(ValueError):require_shortlist_recruitment_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_bodies_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,r in enumerate(self.records) if r['address']==start)
            for changed in (self.records[:i]+self.records[i+1:],self.records+[self.records[i]],list(reversed(self.records)),[r for r in self.records if not start<=r['address']<end]):
                with self.subTest(start=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_partial_page_omissions_fail(self):
        for page in sorted({x[1] for x in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page];selected={r['address'] for r in self.records};require_shortlist_recruitment_contract(page,rows,selected)
            for start,end,*_ in CONTRACTS:
                if start>>12<=page<=(end-1)>>12:
                    with self.assertRaises(ValueError):require_shortlist_recruitment_contract(page,[r for r in rows if not start<=r['address']<end],{start})

if __name__=='__main__':unittest.main()
