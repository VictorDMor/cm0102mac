"""Prepared complete monetary conversion, rounding and editing contracts."""
from contextlib import closing
from copy import deepcopy
import hashlib,json,sqlite3,unittest
from pathlib import Path
from compile_recomp import SEMANTIC_REPLACEMENTS,require_unique_semantic_replacements
from money_value_audit import (CONTRACTS,FRAGMENTS,MONEY_VALUE_ENTRIES,TABLES,WORDS,PADDING,
    load_money_value_tables,require_money_value_tables,require_money_value_words,require_money_value_padding,
    require_money_value_bodies,require_money_value_contract,require_money_value_edges,require_money_value_index)
from contract_display_audit import CONTRACT_DISPLAY_ENTRIES
from make_money_value_reference import render_reference
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'
class PreparedScopeTests(unittest.TestCase):
    def test_exact_installed_prefix_and_seven_unowned_roots(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),871)
        prefix=[{'entry':hex(e),'module':m}for e,m in SEMANTIC_REPLACEMENTS]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'78870ba89f43481d334e7ab2b0da6a9cfa478cc6f42a343fdc4ff2ed5616e2ec')
        self.assertFalse(set(MONEY_VALUE_ENTRIES)&dict(SEMANTIC_REPLACEMENTS).keys())
        candidate=(*SEMANTIC_REPLACEMENTS,*((e,'money_value_adapter')for e in MONEY_VALUE_ENTRIES))
        self.assertEqual(len(candidate),878);require_unique_semantic_replacements(candidate)
        combined=(*candidate,*((e,'contract_display_adapter')for e in CONTRACT_DISPLAY_ENTRIES));self.assertEqual(len(combined),880);require_unique_semantic_replacements(combined)
        for e in MONEY_VALUE_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*candidate,(e,'duplicate')))
    def test_whole_value_editing_scope_and_owned_dependency_boundary(self):
        self.assertEqual(sum(x[2]for x in CONTRACTS),593);self.assertEqual(sum(x[2]for x in FRAGMENTS),593)
        self.assertEqual(tuple(x[2]for x in CONTRACTS),(58,19,65,63,47,68,273));self.assertEqual(len(PADDING),11)
        self.assertEqual(tuple(x[2]for x in TABLES),(448,28,28));self.assertEqual(tuple(x[1]for x in WORDS),(24,))
        owners=dict(SEMANTIC_REPLACEMENTS)
        for e in (0x10773960,0x10773c80,0x10773de0):self.assertEqual(owners[e],'compiler_arithmetic_adapter')
        for e in (0x10773808,0x10773858):self.assertEqual(owners[e],'register_context_adapter')
        for start,end,*_ in CONTRACTS:
            require_money_value_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.assertRaises(ValueError):require_money_value_edges([(0,target)])
    def test_complete_named_stages_without_replay_or_embedded_banks(self):
        source=''.join((ROOT/('native/'+name)).read_text()for name in ('money_value.cpp','money_value_detail.hpp','money_value_quantum.cpp','money_value_presets.cpp'))
        for token in ('dispatch_page','run_until','switch (s.pc)','goto ','_reference(s)','pcode','while(s.pc','memory.clear','memset','CASH.CPP'):
            self.assertNotIn(token,source)
        for token in ('to_display_amount','from_display_amount','increase_amount','decrease_amount','select_standard_amount','select_table_amount','amount_quantum','align_scaled_pair','large_steps','fine_steps'):
            self.assertIn(token,source)
        self.assertNotIn('money_value_adapter',(ROOT/'native/CMakeLists.txt').read_text())

@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(),'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_money_value_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        program=json.loads((INPUT/'program.json').read_text());cls.userops=program['userops']
        cls.segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    @classmethod
    def read(cls,a,n):
        for base,data in cls.segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError('Missing original money-value data')

    def test_original_presentation_text_and_pointer_are_pinned(self):
        require_money_value_tables(self.read);require_money_value_words(self.read);self.assertEqual(len(load_money_value_tables(INPUT)),4)
        for slot,base,size,_ in TABLES:
            for address in [*range(slot,slot+4),*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(self.read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_money_value_tables(changed)
        self.assertEqual(len(TABLES),3)

    def test_initial_mutable_scale_bytes_are_pinned(self):
        require_money_value_words(self.read)
        for base,size,_ in WORDS:
            for address in range(base,base+size):
                def changed(a,n):
                    b=bytearray(self.read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_money_value_words(changed)

    def test_nonindexed_alignment_words_are_pinned(self):
        require_money_value_padding(self.read);self.assertEqual(len(PADDING),11)
        self.assertFalse({a for a,_ in PADDING}&{r['address'] for r in self.records})
        for address,_ in PADDING:
            def changed(a,n):
                b=bytearray(self.read(a,n))
                if a<=address<a+n:b[address-a]^=1
                return bytes(b)
            with self.assertRaises(ValueError):require_money_value_padding(changed)

    def test_reference_keeps_every_original_instruction_and_disables_all_hooks(self):
        source=render_reference(self.records,self.userops);self.assertNotIn('try_modern_',source);self.assertNotIn('_impl<true,',source)
        for e in MONEY_VALUE_ENTRIES:self.assertIn(f'case 0x{e:x}:',source)
        for r in self.records:self.assertIn(f's.step(0x{r["address"]:x});',source)

    def test_changed_original_fields_fail(self):
        for start,page,*_ in FRAGMENTS:
            i=next(i for i,r in enumerate(self.records) if r['address']>=start and r['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[i][key]=value
                with self.subTest(start=hex(start),page=hex(page),key=key),self.assertRaises(ValueError):require_money_value_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_bodies_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,r in enumerate(self.records) if r['address']==start)
            for changed in (self.records[:i]+self.records[i+1:],self.records+[self.records[i]],list(reversed(self.records)),[r for r in self.records if not start<=r['address']<end]):
                with self.subTest(start=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_partial_page_omissions_fail(self):
        for page in sorted({x[1] for x in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page];selected={r['address'] for r in self.records};require_money_value_contract(page,rows,selected)
            for start,end,*_ in CONTRACTS:
                if start>>12<=page<=(end-1)>>12:
                    with self.assertRaises(ValueError):require_money_value_contract(page,[r for r in rows if not start<=r['address']<end],{start})

if __name__=='__main__':unittest.main()
