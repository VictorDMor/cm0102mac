"""Complete club injury event queue contracts."""
from contextlib import closing
from copy import deepcopy
import hashlib
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS,generate_page
from make_injury_club_events_reference import render_reference
from injury_club_events_audit import (CONTRACTS,FRAGMENTS,INJURY_CLUB_EVENTS_ENTRIES,CALLBACK_DESCRIPTORS,
    require_injury_club_events_bodies,require_injury_club_events_contract,require_injury_club_events_edges,
    require_injury_club_events_hooks,require_injury_club_events_index,require_injury_club_events_tables,load_injury_club_events_tables)
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'

class ScopeTests(unittest.TestCase):
    def test_complete_block_and_exact_prior_manifest(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS),448)
        self.assertEqual(sum(row[2] for row in FRAGMENTS),448)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),930)
        self.assertEqual(len({entry for entry,_ in SEMANTIC_REPLACEMENTS}),930)
        self.assertEqual(INJURY_CLUB_EVENTS_ENTRIES,(0x1018bb50,0x1018bdd0,0x101900b0))
        self.assertEqual(SEMANTIC_REPLACEMENTS[711:714],tuple((entry,'injury_club_events_adapter') for entry in INJURY_CLUB_EVENTS_ENTRIES))
        prior=[{'entry':f'0x{entry:x}','module':module} for entry,module in SEMANTIC_REPLACEMENTS[:711]]
        self.assertEqual(hashlib.sha256(json.dumps(prior,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'908a19f74bd3b3d5493df0c62ac725c4893494355005fef6713b965752509c9b')

    def test_external_interior_edges_fail(self):
        for entry,end,*_ in CONTRACTS:
            require_injury_club_events_edges([(0,entry),(entry,end-4),(end-4,end)])
            for target in (entry+4,end-4):
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):require_injury_club_events_edges([(0,target)])

    def test_named_components_do_not_replay(self):
        source=''.join((ROOT/'native'/name).read_text() for name in ('injury_club_events.cpp','injury_club_events_dates.cpp'))
        for forbidden in ('injury_club_events_reference','dispatch_page','run_until','pcode','i_101'):
            self.assertNotIn(forbidden,source)
        for name in ('enqueue_club_event','process_club_events','compare_event_date','eligible_event','grow_event_queue',
                     'store_event','current_date_precedes_event','event_precedes_cutoff','select_event_count',
                     'apply_event_to_roster','compact_expired_events','advance_pool'):
            self.assertIn(name,source)


@unittest.skipUnless((INPUT/'instructions.sqlite').exists(),'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_injury_club_events_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']

    def test_original_sort_descriptor_is_pinned(self):
        import struct
        program=json.loads((INPUT/'program.json').read_text())
        segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
        def word(address):
            for base,data in segments:
                if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
            raise ValueError('Missing original table word')
        addresses=set()
        for slot,descriptor,*_ in CALLBACK_DESCRIPTORS:addresses.update((slot,descriptor,descriptor+4))
        live={address:word(address) for address in addresses}
        require_injury_club_events_tables(live.__getitem__)
        load_injury_club_events_tables(INPUT)
        for address in live:
            changed=dict(live);changed[address]^=1
            with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_injury_club_events_tables(changed.__getitem__)

    def test_changed_source_fields_fail(self):
        for entry,page,*_ in FRAGMENTS:
            end=next(row[1] for row in CONTRACTS if row[0]==entry)
            index=next(n for n,row in enumerate(self.records) if entry<=row['address']<end and row['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[index][key]=value
                with self.subTest(entry=hex(entry),page=hex(page),key=key),self.assertRaises(ValueError):
                    require_injury_club_events_bodies(changed,complete=True)

    def test_missing_reordered_duplicate_absent_bodies_fail(self):
        for entry,end,*_ in CONTRACTS:
            index=next(n for n,row in enumerate(self.records) if row['address']==entry)
            for changed in (self.records[:index]+self.records[index+1:],self.records+[self.records[index]],
                            list(reversed(self.records)),[row for row in self.records if not entry<=row['address']<end]):
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    render_reference(changed,self.userops)

    def test_independent_reference_keeps_all_steps_without_hooks(self):
        source=render_reference(self.records,self.userops)
        expected=[f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):',source,re.MULTILINE),expected)
        self.assertEqual(re.findall(r'injury_club_events_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        for forbidden in ('try_modern_','complete_context_transfer'):
            self.assertNotIn(forbidden,source)

    def test_partial_pages_fail_and_complete_pages_pass(self):
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows=[row for row in self.records if row['address']>>12==page]
            selected={row['address'] for row in self.records}
            require_injury_club_events_contract(page,rows,selected)
            for entry,end,*_ in CONTRACTS:
                if entry>>12<=page<=(end-1)>>12:
                    body=[row for row in rows if entry<=row['address']<end]
                    with self.assertRaises(ValueError):
                        require_injury_club_events_contract(page,[row for row in rows if row not in body],{entry})

    def test_productive_hooks_precede_accounting(self):
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows=[row for row in self.records if row['address']>>12==page]
            disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks')}
            disabled['injury_club_events_hooks']=True
            code=generate_page(page,rows,{row['address'] for row in self.records},userops=self.userops,**disabled)
            require_injury_club_events_hooks(page,rows,code)
            for entry in INJURY_CLUB_EVENTS_ENTRIES:
                if entry>>12!=page:continue
                hook=f'if (try_modern_injury_club_events(s, 0x{entry:x})) return true;'
                with self.assertRaises(ValueError):require_injury_club_events_hooks(page,rows,code.replace(hook,''))

if __name__=='__main__':unittest.main()
