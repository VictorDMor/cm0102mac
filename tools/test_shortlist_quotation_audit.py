"""Complete shortlist context and reference-quotation contracts under characterization."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import sqlite3
import unittest
import tempfile
from compile_recomp import SEMANTIC_REPLACEMENTS,require_unique_semantic_replacements
from shortlist_quotation_audit import (CONTRACTS,FRAGMENTS,SHORTLIST_QUOTATION_ENTRIES,TABLES,PADDING,
    load_shortlist_quotation_tables,require_shortlist_quotation_tables,require_shortlist_quotation_padding,
    require_shortlist_quotation_bodies,require_shortlist_quotation_contract,require_shortlist_quotation_edges,
    require_shortlist_quotation_index)
from make_shortlist_quotation_reference import render_reference
from shortlist_quotation_proofs import (STRUCTURALLY_UNREACHABLE,
    require_shortlist_quotation_structural_proofs)
from shortlist_quotation_cases import CASE_PATH, load_shortlist_quotation_cases, render_shortlist_quotation_cases
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'

class PreparedScopeTests(unittest.TestCase):
    def test_exact_installed_prefix_and_two_unowned_operations(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),859)
        prefix=[{'entry':hex(e),'module':m} for e,m in SEMANTIC_REPLACEMENTS]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'80d0e741d08631bb20bafa1b5e563d20fb02f0b8ce2759b5f07a07f408c81a53')
        self.assertFalse(set(SHORTLIST_QUOTATION_ENTRIES)&dict(SEMANTIC_REPLACEMENTS).keys())
        candidate=(*SEMANTIC_REPLACEMENTS,*((e,'shortlist_quotation_adapter') for e in SHORTLIST_QUOTATION_ENTRIES))
        self.assertEqual(len(candidate),861);require_unique_semantic_replacements(candidate)
        for e in SHORTLIST_QUOTATION_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*candidate,(e,'duplicate')))

    def test_whole_boundaries_exclude_adjacent_loan_and_scanning_controllers(self):
        self.assertEqual(sum(x[2] for x in CONTRACTS),5689)
        self.assertEqual(sum(x[2] for x in FRAGMENTS),5689)
        for e in (0x103636b0,0x10363b80,0x10369080):self.assertNotIn(e,SHORTLIST_QUOTATION_ENTRIES)
        for start,end,*_ in CONTRACTS:
            require_shortlist_quotation_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.assertRaises(ValueError):require_shortlist_quotation_edges([(0,target)])

    def test_existing_registration_and_storage_owners_are_retained(self):
        owners=dict(SEMANTIC_REPLACEMENTS)
        self.assertEqual(owners[0x103568e0],'club_shortlist_storage_adapter')
        for entry in (0x10363ca0,0x10363f60,0x10364d00):
            self.assertEqual(owners[entry],'shortlist_recruitment_adapter');self.assertNotIn(entry,SHORTLIST_QUOTATION_ENTRIES)

    def test_complete_named_operations_use_native_control_flow(self):
        names=('context','quote_profile','quote_country')
        source=(ROOT/'native/shortlist_quotation.cpp').read_text()+'\n'+'\n'.join((ROOT/f'native/shortlist_quotation_{n}.cpp').read_text() for n in names)
        for token in ('shortlist_quotation_reference','dispatch_page','run_until','pcode','i_1036','page_1036','quotation_stage_103','switch(s.pc)','while(s.pc'):
            self.assertNotIn(token,source)
        self.assertIn('prepare_candidate_context',source);self.assertIn('finish_quote_contract_limits',source)
        self.assertNotIn('shortlist_quotation_adapter',(ROOT/'native/CMakeLists.txt').read_text())
        support=(ROOT/'native/shortlist_quotation_detail.hpp').read_text()
        for token in ('divide_record_at','product_difference_single_at','negative_product_difference_single_at'):
            self.assertIn(token,support)

class RetainedInputTests(unittest.TestCase):
    def test_synthetic_complete_entry_inputs_cover_both_roots(self):
        cases=load_shortlist_quotation_cases()
        self.assertEqual(len(cases),294);self.assertEqual({c['root'] for c in cases},{0,1})
        emitted=render_shortlist_quotation_cases(cases)
        self.assertIn('retained_quotation_cases=294',emitted)
        for token in ('shortlist_quotation_reference','pcode','dispatch_page','run_until'):
            self.assertNotIn(token,emitted)

    def test_input_generator_rejects_non_synthetic_data_and_unknown_services(self):
        original=json.loads(CASE_PATH.read_text())
        for kind in ('memory','maps','dependency','option','duplicate'):
            changed=deepcopy(original)
            if kind=='memory':changed['cases'][0]['memory']=[['0x1080c538','00']]
            if kind=='maps':changed['cases'][0]['maps']=[[0x1080c538,8]]
            if kind=='dependency':changed['cases'][0]['words']=[[0x1037b6f0,0]]
            if kind=='option':changed['cases'][0]['options']['unknown']=True
            if kind=='duplicate':changed['cases'].append(changed['cases'][0])
            with tempfile.TemporaryDirectory() as directory:
                path=Path(directory)/'changed.json';path.write_text(json.dumps(changed))
                with self.subTest(kind=kind),self.assertRaises(ValueError):load_shortlist_quotation_cases(path)

@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(),'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_shortlist_quotation_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        program=json.loads((INPUT/'program.json').read_text());cls.userops=program['userops']
        cls.segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    @classmethod
    def read(cls,a,n):
        for base,data in cls.segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError('Missing original shortlist-quotation data')

    def test_floating_bank_fallback_and_five_classification_tables_are_pinned(self):
        require_shortlist_quotation_tables(self.read);load_shortlist_quotation_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [*range(slot,slot+4),*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(self.read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_shortlist_quotation_tables(changed)
        self.assertEqual(len(TABLES),11)

    def test_structural_proofs_retain_seventy_five_original_sites(self):
        sites=require_shortlist_quotation_structural_proofs(self.records,self.read)
        self.assertEqual(sites,STRUCTURALLY_UNREACHABLE)
        self.assertEqual(len(sites),75);self.assertEqual(len(set(sites)),75)
        self.assertTrue(set(sites)<={r['address'] for r in self.records})
        header=(ROOT/'native/shortlist_quotation.hpp').read_text()
        for pc in sites:self.assertIn(hex(pc),header)

    def test_structural_proofs_reject_changed_premises(self):
        for pc in (0x1036659c,0x10367074,0x10367434,0x10367fd8,
                   0x10368aa8,0x10368efc,0x1037c8e4,0x1037cf48,0x1037dbc0):
            changed=deepcopy(self.records)
            next(row for row in changed if row['address']==pc)['assembly']='changed'
            with self.subTest(pc=hex(pc)),self.assertRaises(ValueError):
                require_shortlist_quotation_structural_proofs(changed,self.read)

    def test_nonindexed_alignment_words_are_pinned(self):
        require_shortlist_quotation_padding(self.read);self.assertEqual(len(PADDING),7)
        self.assertFalse({a for a,_ in PADDING}&{r['address'] for r in self.records})
        for address,_ in PADDING:
            def changed(a,n):
                b=bytearray(self.read(a,n))
                if a<=address<a+n:b[address-a]^=1
                return bytes(b)
            with self.assertRaises(ValueError):require_shortlist_quotation_padding(changed)

    def test_reference_keeps_every_original_instruction_and_disables_all_hooks(self):
        source=render_reference(self.records,self.userops);self.assertNotIn('try_modern_',source);self.assertNotIn('_impl<true,',source)
        for e in SHORTLIST_QUOTATION_ENTRIES:self.assertIn(f'case 0x{e:x}:',source)
        for r in self.records:self.assertIn(f's.step(0x{r["address"]:x});',source)

    def test_changed_original_fields_fail(self):
        for start,page,*_ in FRAGMENTS:
            i=next(i for i,r in enumerate(self.records) if r['address']>=start and r['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[i][key]=value
                with self.subTest(start=hex(start),page=hex(page),key=key),self.assertRaises(ValueError):require_shortlist_quotation_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_bodies_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,r in enumerate(self.records) if r['address']==start)
            for changed in (self.records[:i]+self.records[i+1:],self.records+[self.records[i]],list(reversed(self.records)),[r for r in self.records if not start<=r['address']<end]):
                with self.subTest(start=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_partial_page_omissions_fail(self):
        for page in sorted({x[1] for x in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page];selected={r['address'] for r in self.records};require_shortlist_quotation_contract(page,rows,selected)
            for start,end,*_ in CONTRACTS:
                if start>>12<=page<=(end-1)>>12:
                    with self.assertRaises(ValueError):require_shortlist_quotation_contract(page,[r for r in rows if not start<=r['address']<end],{start})

if __name__=='__main__':unittest.main()
