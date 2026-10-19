"""Complete shortlist category, query and pair-comparison contracts with productive ownership."""
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
from shortlist_category_audit import (CONTRACTS, FRAGMENTS, SHORTLIST_CATEGORY_ENTRIES,
    TABLES, PADDING, load_shortlist_category_tables, require_shortlist_category_tables,
    require_shortlist_category_padding, require_shortlist_category_bodies,
    require_shortlist_category_contract, require_shortlist_category_edges,
    require_shortlist_category_index, require_shortlist_category_hooks, shortlist_category_hook)
from make_shortlist_category_reference import render_reference
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'

class PreparedScopeTests(unittest.TestCase):
    def test_exact_prefix_and_five_new_owners(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),939)
        prefix=[{'entry':hex(entry),'module':owner} for entry,owner in SEMANTIC_REPLACEMENTS[:839]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'2614f691b5971a5c67f3e38f92f71466fb3f4aba9252fd5802fd29433fef1b71')
        self.assertEqual(SEMANTIC_REPLACEMENTS[839:844],tuple((entry,'shortlist_category_adapter') for entry in SHORTLIST_CATEGORY_ENTRIES))
        candidate=SEMANTIC_REPLACEMENTS
        self.assertEqual(len(candidate),939);require_unique_semantic_replacements(candidate)
        for entry in SHORTLIST_CATEGORY_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*candidate,(entry,'duplicate')))

    def test_existing_storage_and_neighbors_are_excluded(self):
        owners=dict(SEMANTIC_REPLACEMENTS)
        self.assertEqual(owners[0x10356410],'club_shortlist_storage_adapter')
        self.assertEqual(owners[0x10358b30],'club_shortlist_storage_adapter')
        self.assertEqual(owners[0x103563e0],'engine_initialization_adapter')
        self.assertNotIn(0x103563e0,SHORTLIST_CATEGORY_ENTRIES)

    def test_complete_boundaries_and_external_interior_edges(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS),467)
        self.assertEqual(sum(row[2] for row in FRAGMENTS),467)
        for start,end,*_ in CONTRACTS:
            require_shortlist_category_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.assertRaises(ValueError):require_shortlist_category_edges([(0,target)])

    def test_named_operations_do_not_replay_original_bodies(self):
        source='\n'.join((ROOT/'native'/name).read_text() for name in ('shortlist_category_membership.cpp','shortlist_category_queries.cpp'))
        for forbidden in ('shortlist_category_reference','dispatch_page','run_until','pcode','i_1036','page_1036'):
            self.assertNotIn(forbidden,source)
        for name in ('eligible_for_category','count_and_prune_club_rows','refresh_unassigned_rows','compare_pairs_by_second','compare_pairs_by_first'):
            self.assertIn(name,source)
        self.assertIn('shortlist_category_adapter.cpp',(ROOT/'native/CMakeLists.txt').read_text())

@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(),'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_shortlist_category_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']
        program=json.loads((INPUT/'program.json').read_text());cls.segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    @classmethod
    def read(cls,a,n):
        for base,data in cls.segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError('Missing original shortlist data')

    def test_original_tables_vectors_and_live_pointers_are_pinned(self):
        require_shortlist_category_tables(self.read);load_shortlist_category_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [*range(slot,slot+4),*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(self.read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_shortlist_category_tables(changed)
        self.assertTrue(self.read(TABLES[0][1],22).startswith(b'SHORTLIST_MANAGER.CPP\0'))

    def test_nonindexed_padding_bytes_are_pinned(self):
        require_shortlist_category_padding(self.read);self.assertEqual(len(PADDING),2)
        sites={r['address'] for r in self.records};self.assertFalse(sites&set(PADDING))
        for address in PADDING:
            def changed(a,n):
                b=bytearray(self.read(a,n))
                if a<=address<a+n:b[address-a]^=1
                return bytes(b)
            with self.assertRaises(ValueError):require_shortlist_category_padding(changed)

    def test_original_category_destinations_and_qualification_fields(self):
        import struct
        table=struct.unpack('>16I',self.read(0x108ceecc,64))
        self.assertEqual(table,(0x10365d74,0x10365d8c,0x10365db4,0x10365e04,0x10365ddc,0x10365e2c,0x10365e54,0x10365e7c,0x10365ea4,0x10365ecc,0x10365ef4,0x10365f1c,0x10365f44,0x10365f6c,0x10365f94,0x10365fac))
        body={r['address']:r['assembly'] for r in self.records}
        for target in table[:15]:
            self.assertRegex(body[target],r'lbz r0,0x[0-9a-f]+\(r3\)')
            self.assertEqual(body[target+4],'extsb r0,r0');self.assertEqual(body[target+8],'cmpwi r0,0xf')
        self.assertEqual(body[0x1036616c],'bl 0x10717b20')
        self.assertEqual(body[0x10366384],'bl 0x10365590')

    def test_diagnostic_matches_the_storage_block(self):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            old=[json.loads(x[0])['assembly'] for x in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(0x10358070,0x103580b8))]
        new=[x['assembly'] for x in self.records if 0x10366030<=x['address']<0x10366078]
        def normalize(rows):return [re.sub(r'li r8,0x[0-9a-f]+','LINE',re.sub(r'addi r([67]),r1,0x[0-9a-f]+',r'SCRATCH_\1',text)) for text in rows]
        self.assertEqual(len(new),18);self.assertEqual(normalize(new),normalize(old))

    def test_original_reset_callers_and_pair_transition_vectors(self):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            for pc in (0x103569a0,0x10356ad0):
                self.assertEqual(json.loads(db.execute('SELECT body FROM instructions WHERE address=?',(pc,)).fetchone()[0])['assembly'],'bl 0x10365d10')
        for slot,pointer,entry in ((0x108b57e8,0x108b920c,0x10366410),(0x108b57f8,0x108b922c,0x103663b0)):
            self.assertEqual(int.from_bytes(self.read(slot,4),'big'),pointer)
            self.assertEqual(self.read(pointer,8),entry.to_bytes(4,'big')+(0x108bba90).to_bytes(4,'big'))

    def test_productive_hooks_precede_original_accounting(self):
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['shortlist_category_hooks']=True
        for page in sorted({f[1] for f in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page]
            source=generate_page(page,rows,{r['address'] for r in self.records},userops=self.userops,**disabled)
            require_shortlist_category_hooks(page,rows,source)
            for entry in SHORTLIST_CATEGORY_ENTRIES:
                if entry>>12!=page:continue
                hook=shortlist_category_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_shortlist_category_hooks(page,rows,changed)

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
                    require_shortlist_category_bodies(changed, complete=True)

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
            require_shortlist_category_contract(page, rows, selected)
            for start, end, *_ in CONTRACTS:
                if start >> 12 <= page <= (end-1) >> 12:
                    with self.assertRaises(ValueError):
                        require_shortlist_category_contract(page, [row for row in rows if not start <= row['address'] < end], {start})


if __name__=="__main__":unittest.main()
