"""Prepared complete money text, label and settings contracts."""
import inspect
from compile_recomp import generate_page
from contextlib import closing
from copy import deepcopy
import hashlib,json,sqlite3,unittest
from pathlib import Path
from compile_recomp import SEMANTIC_REPLACEMENTS,require_unique_semantic_replacements
from money_text_audit import (CONTRACTS,FRAGMENTS,MONEY_TEXT_ENTRIES,TABLES,WORDS,PADDING,
    load_money_text_tables,require_money_text_tables,require_money_text_words,require_money_text_padding,
    require_money_text_bodies,require_money_text_contract,require_money_text_edges,require_money_text_index,require_money_text_hooks,money_text_hook)
from contract_display_audit import CONTRACT_DISPLAY_ENTRIES
from money_value_audit import MONEY_VALUE_ENTRIES
from make_money_text_reference import render_reference
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'
class PreparedScopeTests(unittest.TestCase):
    def test_exact_prefix_and_thirteen_productive_owners(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),953)
        prefix=[{'entry':hex(e),'module':m}for e,m in SEMANTIC_REPLACEMENTS[:871]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'78870ba89f43481d334e7ab2b0da6a9cfa478cc6f42a343fdc4ff2ed5616e2ec')
        from contract_display_audit import CONTRACT_DISPLAY_ENTRIES
        from money_value_audit import MONEY_VALUE_ENTRIES
        from money_text_audit import MONEY_TEXT_ENTRIES
        expected=tuple((e,m+'_adapter')for m,es in [('contract_display',CONTRACT_DISPLAY_ENTRIES),('money_value',MONEY_VALUE_ENTRIES),('money_text',MONEY_TEXT_ENTRIES)]for e in es)
        self.assertEqual(SEMANTIC_REPLACEMENTS[871:884],expected);require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry,_ in expected:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(entry,'duplicate')))

    def test_whole_text_scope_and_owned_dependency_boundary(self):
        self.assertEqual(sum(x[2]for x in CONTRACTS),1420);self.assertEqual(sum(x[2]for x in FRAGMENTS),1420)
        self.assertEqual(tuple(x[2]for x in CONTRACTS),(10,2,172,1236));self.assertEqual(len(PADDING),8)
        self.assertEqual(tuple(x[2]for x in TABLES),(128,100,64,64,856,52,52,52,52,52,52));self.assertEqual(tuple(x[1]for x in WORDS),(4,))
        owners=dict(SEMANTIC_REPLACEMENTS)
        for e in (0x10773960,0x10773b70):self.assertEqual(owners[e],'compiler_arithmetic_adapter')
        for e in (0x1077dbd0,0x1077dbf0):self.assertEqual(owners[e],'string_core_adapter')
        for start,end,*_ in CONTRACTS:
            require_money_text_edges([(0,start),(start,end-4),(end-4,end)])
            for target in {start+4,end-4}-{start}:
                with self.assertRaises(ValueError):require_money_text_edges([(0,target)])
    def test_complete_named_stages_without_replay_or_embedded_banks(self):
        source=''.join((ROOT/('native/'+name)).read_text()for name in ('money_text.cpp','money_text_detail.hpp','money_text_digits.cpp','money_text_grouped.cpp','money_text_compact.cpp'))
        for token in ('dispatch_page','run_until','switch (s.pc)','goto ','_reference(s)','pcode','while(s.pc','memory.clear','memset','CASH.CPP'):
            self.assertNotIn(token,source)
        for token in ('selected_symbol_length','display_setting','currency_label','format_amount','build_decimal_digits','replace_spaces','render_grouped','render_compact'):
            self.assertIn(token,source)
        self.assertIn('money_text_adapter.cpp',(ROOT/'native/CMakeLists.txt').read_text())

@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(),'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_money_text_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        program=json.loads((INPUT/'program.json').read_text());cls.userops=program['userops']
        cls.segments=[(s['address'],(INPUT/s['file']).read_bytes() if s.get('file') else bytes(s['size'])) for s in program['segments']]
    @classmethod
    def read(cls,a,n):
        for base,data in cls.segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError('Missing original money-text data')

    def test_original_presentation_text_and_pointer_are_pinned(self):
        require_money_text_tables(self.read);require_money_text_words(self.read);self.assertEqual(len(load_money_text_tables(INPUT)),12)
        for slot,base,size,_ in TABLES:
            for address in [*range(slot,slot+4),*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(self.read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_money_text_tables(changed)
        self.assertEqual(len(TABLES),11)

    def test_initial_mutable_scale_bytes_are_pinned(self):
        require_money_text_words(self.read)
        for base,size,_ in WORDS:
            for address in range(base,base+size):
                def changed(a,n):
                    b=bytearray(self.read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_money_text_words(changed)

    def test_nonindexed_alignment_words_are_pinned(self):
        require_money_text_padding(self.read);self.assertEqual(len(PADDING),8)
        self.assertFalse({a for a,_ in PADDING}&{r['address'] for r in self.records})
        for address,_ in PADDING:
            def changed(a,n):
                b=bytearray(self.read(a,n))
                if a<=address<a+n:b[address-a]^=1
                return bytes(b)
            with self.assertRaises(ValueError):require_money_text_padding(changed)

    def test_productive_hooks_precede_original_accounting(self):
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['money_text_hooks']=True
        for page in sorted({f[1] for f in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page]
            source=generate_page(page,rows,{r['address'] for r in self.records},userops=self.userops,**disabled)
            require_money_text_hooks(page,rows,source)
            for entry in MONEY_TEXT_ENTRIES:
                if entry>>12!=page:continue
                hook=money_text_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_money_text_hooks(page,rows,changed)

    def test_reference_keeps_every_original_instruction_and_disables_all_hooks(self):
        source=render_reference(self.records,self.userops);self.assertNotIn('try_modern_',source);self.assertNotIn('_impl<true,',source)
        for e in MONEY_TEXT_ENTRIES:self.assertIn(f'case 0x{e:x}:',source)
        for r in self.records:self.assertIn(f's.step(0x{r["address"]:x});',source)

    def test_changed_original_fields_fail(self):
        for start,page,*_ in FRAGMENTS:
            i=next(i for i,r in enumerate(self.records) if r['address']>=start and r['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[i][key]=value
                with self.subTest(start=hex(start),page=hex(page),key=key),self.assertRaises(ValueError):require_money_text_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_bodies_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,r in enumerate(self.records) if r['address']==start)
            for changed in (self.records[:i]+self.records[i+1:],self.records+[self.records[i]],list(reversed(self.records)),[r for r in self.records if not start<=r['address']<end]):
                with self.subTest(start=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_partial_page_omissions_fail(self):
        for page in sorted({x[1] for x in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page];selected={r['address'] for r in self.records};require_money_text_contract(page,rows,selected)
            for start,end,*_ in CONTRACTS:
                if start>>12<=page<=(end-1)>>12:
                    with self.assertRaises(ValueError):require_money_text_contract(page,[r for r in rows if not start<=r['address']<end],{start})

if __name__=='__main__':unittest.main()
