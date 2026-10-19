"""Complete club shortlist storage contracts and productive ownership."""
from contextlib import closing
from copy import deepcopy
import hashlib
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, require_unique_semantic_replacements, generate_page
from club_shortlist_storage_audit import (CONTRACTS, FRAGMENTS, CLUB_SHORTLIST_STORAGE_ENTRIES,
    TABLES, PADDING, load_club_shortlist_storage_tables, require_club_shortlist_storage_tables,
    require_club_shortlist_storage_padding, require_club_shortlist_storage_bodies,
    require_club_shortlist_storage_contract, require_club_shortlist_storage_edges,
    require_club_shortlist_storage_index,require_club_shortlist_storage_hooks,club_shortlist_storage_hook)
from make_club_shortlist_storage_reference import render_reference
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'

class PreparedScopeTests(unittest.TestCase):
    def test_exact_accepted_prefix_and_seven_candidate_owners(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),946)
        prefix=[{'entry':hex(entry),'module':owner} for entry,owner in SEMANTIC_REPLACEMENTS[:832]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'6f8529d62a92287d805616952ddcf5af6b888c861f45fae25b0a8e8aa2ca580c')
        self.assertEqual(SEMANTIC_REPLACEMENTS[832:839],tuple((entry,'club_shortlist_storage_adapter') for entry in CLUB_SHORTLIST_STORAGE_ENTRIES))
        candidate=SEMANTIC_REPLACEMENTS
        self.assertEqual(len(candidate),946);require_unique_semantic_replacements(candidate)
        for entry in CLUB_SHORTLIST_STORAGE_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*candidate,(entry,'duplicate')))

    def test_existing_adjacent_owners_are_excluded(self):
        owners=dict(SEMANTIC_REPLACEMENTS)
        self.assertEqual(owners[0x100031b0],'awol_records_adapter')
        self.assertEqual(owners[0x10003dc0],'awol_records_adapter')
        self.assertEqual(owners[0x103563e0],'engine_initialization_adapter')
        self.assertNotIn(0x103563e0,CLUB_SHORTLIST_STORAGE_ENTRIES)

    def test_complete_boundaries_and_external_interior_edges(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS),1945)
        self.assertEqual(sum(row[2] for row in FRAGMENTS),1945)
        for start,end,*_ in CONTRACTS:
            require_club_shortlist_storage_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.assertRaises(ValueError):require_club_shortlist_storage_edges([(0,target)])

    def test_named_operations_do_not_replay_original_bodies(self):
        source='\n'.join(p.read_text() for p in (ROOT/'native').glob('club_shortlist_storage*.cpp') if 'checks' not in p.name and 'adapter' not in p.name)
        for forbidden in ('club_shortlist_storage_reference','dispatch_page','run_until','pcode','i_1035','page_1035'):
            self.assertNotIn(forbidden,source)
        for name in ('construct_shortlist_storage','destroy_shortlist_storage','reset_shortlist_storage','sample_shortlist_range','load_shortlist_storage','initialize_auxiliary_record','save_shortlist_storage'):
            self.assertIn(name,source)
        cmake=(ROOT/'native/CMakeLists.txt').read_text()
        self.assertIn('club_shortlist_storage_adapter.cpp',cmake)

    def test_shared_sampler_bias_is_selected_per_original_layout(self):
        engine=(ROOT/'native/range_sampling_detail.hpp').read_text()
        self.assertIn('bias=0xc0',engine);self.assertEqual(engine.count('a.bias_offset()'),2)
        self.assertIn('{sample_shortlist_range_entry,-0x4ddc,0,8}',(ROOT/'native/club_shortlist_storage_sampling.cpp').read_text())
        self.assertIn('{sample_contract_range_entry,-0x4dac,0}',(ROOT/'native/contract_terms_sampling.cpp').read_text())
        self.assertIn('{sample_offer_range_entry,-0x4b8c,0xa0}',(ROOT/'native/transfer_valuation_sampling.cpp').read_text())

@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(),'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_club_shortlist_storage_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']
        program=json.loads((INPUT/'program.json').read_text());cls.segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    @classmethod
    def read(cls,a,n):
        for base,data in cls.segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError('Missing original shortlist data')

    def test_original_tables_vectors_and_live_pointers_are_pinned(self):
        require_club_shortlist_storage_tables(self.read);load_club_shortlist_storage_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [*range(slot,slot+4),*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(self.read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_club_shortlist_storage_tables(changed)
        self.assertTrue(self.read(TABLES[0][1],22).startswith(b'SHORTLIST_MANAGER.CPP\0'))

    def test_nonindexed_padding_bytes_are_pinned(self):
        require_club_shortlist_storage_padding(self.read);self.assertEqual(len(PADDING),5)
        sites={r['address'] for r in self.records};self.assertFalse(sites&set(PADDING))
        for address in PADDING:
            def changed(a,n):
                b=bytearray(self.read(a,n))
                if a<=address<a+n:b[address-a]^=1
                return bytes(b)
            with self.assertRaises(ValueError):require_club_shortlist_storage_padding(changed)

    def test_complete_shared_sampler_has_exactly_five_layout_changes(self):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            old=[json.loads(row[0]) for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(0x10374570,0x10374ae8))]
        new=[r for r in self.records if 0x10356e40<=r['address']<0x103573b8];self.assertEqual(len(new),350)
        delta=0x10356e40-0x10374570
        def relocate(v):return v+delta if v is not None and 0x10374570<=v<0x10374ae8 else v
        differences=[]
        for a,b in zip(old,new):
            self.assertEqual(a['address']+delta,b['address']);self.assertEqual([relocate(v) for v in a['flows']],b['flows']);self.assertEqual(relocate(a.get('fallthrough')),b.get('fallthrough'))
            text=re.sub(r'0x[0-9a-f]+',lambda m:hex(relocate(int(m[0],16))),a['assembly'])
            if text!=b['assembly']:differences.append((a['address'],a['assembly'],b['assembly']))
        self.assertEqual(differences,[(0x1037457c,'lwz r25,-0x4dac(r2)','lwz r25,-0x4ddc(r2)'),*[(pc,'lfd f2,0xc0(r25)','lfd f2,0x8(r25)') for pc in (0x1037460c,0x103746f8,0x103747e4,0x103748d0)]])

    def test_twenty_five_complete_diagnostics_share_the_original_layout(self):
        blocks=[]
        for i,row in enumerate(self.records):
            if re.fullmatch(r'or r3,r(30|31),r(30|31)',row['assembly']) and i+17<len(self.records) and self.records[i+5]['assembly']=='bl 0x10739890' and self.records[i+17]['assembly']=='bl 0x10769b60':
                body=self.records[i:i+18];self.assertEqual(body[-1]['address'],row['address']+68)
                table=re.fullmatch(r'or r3,r(\d+),r\d+',body[0]['assembly'])[1]
                buffer=re.fullmatch(r'or r3,r(\d+),r\d+',body[8]['assembly'])[1]
                normalized=[]
                for n,x in enumerate(body):
                    text=re.sub(r'\br'+table+r'\b','TABLE',x['assembly']);text=re.sub(r'\br'+buffer+r'\b','BUFFER',text)
                    if n in (1,10):text='FIRST_SCRATCH'
                    if n in (2,11):text='SECOND_SCRATCH'
                    if n==12:text='SOURCE_LINE'
                    normalized.append(text)
                blocks.append(normalized)
        self.assertEqual(len(blocks),25)
        self.assertTrue(all(x==blocks[0] for x in blocks))

    def test_original_transfer_manager_owns_both_constructor_paths_and_save(self):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            def assembly(pc):return json.loads(db.execute('SELECT body FROM instructions WHERE address=?',(pc,)).fetchone()[0])['assembly']
            for pc,text in ((0x103df4ac,'li r3,0xe0'),(0x103df4b0,'bl 0x10773520'),(0x103df4bc,'li r4,0x0'),(0x103df4c0,'bl 0x10356410'),(0x103df4c8,'stw r23,0x988(r3)'),(0x103e0060,'li r3,0xe0'),(0x103e0070,'lwz r4,0x40(r1)'),(0x103e0074,'bl 0x10356410'),(0x103e0078,'stw r18,0x988(r31)'),(0x103e1918,'lwz r3,0x988(r27)'),(0x103e191c,'lwz r4,0x3c(r1)'),(0x103e1920,'bl 0x10358b30')):
                self.assertEqual(assembly(pc),text)

    def test_productive_hooks_precede_original_accounting(self):
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['club_shortlist_storage_hooks']=True
        for page in sorted({f[1] for f in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page]
            source=generate_page(page,rows,{r['address'] for r in self.records},userops=self.userops,**disabled)
            require_club_shortlist_storage_hooks(page,rows,source)
            for entry in CLUB_SHORTLIST_STORAGE_ENTRIES:
                if entry>>12!=page:continue
                hook=club_shortlist_storage_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_club_shortlist_storage_hooks(page,rows,changed)

    def test_reference_has_all_sites_with_all_productive_hooks_disabled(self):
        source=render_reference(self.records,self.userops)
        self.assertNotIn('try_modern_',source)
        for start,_,_,_ in CONTRACTS:self.assertIn(f'case 0x{start:x}:',source)
        for row in self.records:self.assertIn(f's.step(0x{row["address"]:x});',source)
        self.assertNotIn('_impl<true,',source)

    def test_changed_original_fields_are_rejected(self):
        for start, page, *_ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page and row['address'] >= start)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records); changed[index][key] = value
                with self.subTest(entry=hex(start), page=hex(page), key=key), self.assertRaises(ValueError):
                    require_club_shortlist_storage_bodies(changed, complete=True)

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
            require_club_shortlist_storage_contract(page, rows, selected)
            for start, end, *_ in CONTRACTS:
                if start >> 12 <= page <= (end-1) >> 12:
                    with self.assertRaises(ValueError):
                        require_club_shortlist_storage_contract(page, [row for row in rows if not start <= row['address'] < end], {start})


if __name__=="__main__":unittest.main()
