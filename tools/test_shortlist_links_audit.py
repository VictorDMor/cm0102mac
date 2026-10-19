"""Prepared complete shortlist link-index, transition and contract-marker contracts."""
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
from shortlist_links_audit import (CONTRACTS,FRAGMENTS,SHORTLIST_LINKS_ENTRIES,TABLES,PADDING,fingerprint,
    load_shortlist_links_tables,require_shortlist_links_tables,require_shortlist_links_padding,
    require_shortlist_links_bodies,require_shortlist_links_contract,require_shortlist_links_edges,
    require_shortlist_links_index,require_shortlist_links_hooks,shortlist_links_hook)
from make_shortlist_links_reference import render_reference
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'

class PreparedScopeTests(unittest.TestCase):
    def test_exact_prefix_and_ten_productive_owners(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),963)
        prefix=[{'entry':hex(e),'module':m} for e,m in SEMANTIC_REPLACEMENTS[:844]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'11632c388a4b80aa7e2489aab349007d8a0c58fc97773237f14c5396b410b42c')
        self.assertEqual(SEMANTIC_REPLACEMENTS[844:854],tuple((e,'shortlist_links_adapter') for e in SHORTLIST_LINKS_ENTRIES))
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for e in SHORTLIST_LINKS_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(e,'duplicate')))

    def test_whole_boundaries_exclude_adjacent_loan_and_scanning_controllers(self):
        self.assertEqual(sum(x[2] for x in CONTRACTS),1122)
        self.assertEqual(sum(x[2] for x in FRAGMENTS),1122)
        for e in (0x103636b0,0x10363ca0,0x10363f60,0x10366470):self.assertNotIn(e,SHORTLIST_LINKS_ENTRIES)
        for start,end,*_ in CONTRACTS:
            require_shortlist_links_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.assertRaises(ValueError):require_shortlist_links_edges([(0,target)])

    def test_existing_storage_and_pair_comparators_keep_their_owners(self):
        owners=dict(SEMANTIC_REPLACEMENTS)
        self.assertEqual(owners[0x103568e0],'club_shortlist_storage_adapter')
        for entry in (0x103663b0,0x10366410,0x10366250):
            self.assertEqual(owners[entry],'shortlist_category_adapter');self.assertNotIn(entry,SHORTLIST_LINKS_ENTRIES)

    def test_named_operations_do_not_replay_original_code(self):
        names=('queries','storage','transition','markers')
        source='\n'.join((ROOT/f'native/shortlist_links_{n}.cpp').read_text() for n in names)
        for token in ('shortlist_links_reference','dispatch_page','run_until','pcode','i_1036','page_1036'):self.assertNotIn(token,source)
        self.assertIn('remove_pair_index',source);self.assertIn('append_cached_staff_links',source)
        self.assertIn('club_shortlist_storage::detail::fatal_allocation',source)
        self.assertIn('shortlist_links_adapter.cpp',(ROOT/'native/CMakeLists.txt').read_text())

@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(),'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_shortlist_links_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        program=json.loads((INPUT/'program.json').read_text());cls.userops=program['userops']
        cls.segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    @classmethod
    def read(cls,a,n):
        for base,data in cls.segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError('Missing original shortlist-link data')

    def test_diagnostic_and_owned_comparator_vectors_are_pinned(self):
        require_shortlist_links_tables(self.read);load_shortlist_links_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [*range(slot,slot+4),*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(self.read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_shortlist_links_tables(changed)
        self.assertEqual(self.read(0x108b920c,8),bytes.fromhex('10366410108bba90'))
        self.assertEqual(self.read(0x108b922c,8),bytes.fromhex('103663b0108bba90'))

    def test_nonindexed_padding_is_separate_from_the_counted_noop(self):
        require_shortlist_links_padding(self.read);self.assertEqual(len(PADDING),3)
        self.assertFalse(set(PADDING)&{r['address'] for r in self.records})
        self.assertEqual(next(r['assembly'] for r in self.records if r['address']==0x10362e90),'ori r0,r0,0x0')
        for address in PADDING:
            def changed(a,n):
                b=bytearray(self.read(a,n))
                if a<=address<a+n:b[address-a]^=1
                return bytes(b)
            with self.assertRaises(ValueError):require_shortlist_links_padding(changed)

    def test_original_career_club_table_setup_span(self):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            rows=[json.loads(x[0]) for x in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(0x10344068,0x1034412c))]
        self.assertEqual(len(rows),49)
        self.assertEqual(fingerprint(rows),'506b7c40fc252a19045254d57cb2e88af0346533b49a11cc78714f7fd94ac66d')
        source=(ROOT/'native/shortlist_links_production_checks.cpp').read_text()
        self.assertIn('setup(source,0x10344068,{},0x1034412c)',source)
        self.assertIn('state.pc==0x1034411c&&state.r<4>(28)==count',source)
        self.assertIn('state.budget=state.executed+3',source)

    def test_productive_hooks_precede_original_accounting(self):
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['shortlist_links_hooks']=True
        for page in sorted({f[1] for f in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page]
            source=generate_page(page,rows,{r['address'] for r in self.records},userops=self.userops,**disabled)
            require_shortlist_links_hooks(page,rows,source)
            for entry in SHORTLIST_LINKS_ENTRIES:
                if entry>>12!=page:continue
                hook=shortlist_links_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_shortlist_links_hooks(page,rows,changed)

    def test_reference_keeps_every_original_instruction_and_disables_all_hooks(self):
        source=render_reference(self.records,self.userops);self.assertNotIn('try_modern_',source);self.assertNotIn('_impl<true,',source)
        for e in SHORTLIST_LINKS_ENTRIES:self.assertIn(f'case 0x{e:x}:',source)
        for r in self.records:self.assertIn(f's.step(0x{r["address"]:x});',source)

    def test_changed_original_fields_fail(self):
        for start,page,*_ in FRAGMENTS:
            i=next(i for i,r in enumerate(self.records) if r['address']>=start and r['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[i][key]=value
                with self.subTest(start=hex(start),page=hex(page),key=key),self.assertRaises(ValueError):require_shortlist_links_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_bodies_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,r in enumerate(self.records) if r['address']==start)
            for changed in (self.records[:i]+self.records[i+1:],self.records+[self.records[i]],list(reversed(self.records)),[r for r in self.records if not start<=r['address']<end]):
                with self.subTest(start=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_partial_page_omissions_fail(self):
        for page in sorted({x[1] for x in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page];selected={r['address'] for r in self.records};require_shortlist_links_contract(page,rows,selected)
            for start,end,*_ in CONTRACTS:
                if start>>12<=page<=(end-1)>>12:
                    with self.assertRaises(ValueError):require_shortlist_links_contract(page,[r for r in rows if not start<=r['address']<end],{start})

if __name__=='__main__':unittest.main()
