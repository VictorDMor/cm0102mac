"""Complete actor-attribute component boundaries and independent references."""
from contextlib import closing
from copy import deepcopy
import hashlib
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from make_actor_attributes_reference import render_reference
from actor_attributes_audit import (CONTRACTS, FRAGMENTS, ACTOR_ATTRIBUTES_ENTRIES,
    require_actor_attributes_bodies, require_actor_attributes_contract,
    require_actor_attributes_edges, require_actor_attributes_hooks, require_actor_attributes_index,
    COST_TABLE, INJURY_TABLES, load_actor_attributes_tables, require_actor_attributes_tables, require_injury_tables)

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/current/export'

class ScopeTests(unittest.TestCase):
    def test_complete_block_and_prior_manifest(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS),4405)
        self.assertEqual(sum(row[2] for row in FRAGMENTS),4405)
        # INJURY.CPP's injury_unit_adapter owns 16 of the 22 entries; the audit keeps the bodies and digests of
        # all 22 and routes the other six, which stay one block of SEMANTIC_REPLACEMENTS.
        from injury_unit_audit import INJURY_UNIT_ENTRIES
        self.assertEqual(len({entry for entry,_ in SEMANTIC_REPLACEMENTS}),len(SEMANTIC_REPLACEMENTS))
        kept=tuple(entry for entry in ACTOR_ATTRIBUTES_ENTRIES if entry not in INJURY_UNIT_ENTRIES)
        self.assertEqual(kept,(0x10235ea0,0x102373a0,0x10235db0,0x102393c0,0x10239590,0x10239790))
        self.assertEqual(sum(entry in INJURY_UNIT_ENTRIES for entry in ACTOR_ATTRIBUTES_ENTRIES),16)
        owned=[(entry,module) for entry,module in SEMANTIC_REPLACEMENTS if module=='actor_attributes_adapter']
        self.assertEqual(owned,[(entry,'actor_attributes_adapter') for entry in kept])
        start=SEMANTIC_REPLACEMENTS.index(owned[0])
        self.assertEqual(SEMANTIC_REPLACEMENTS[start:start+6],tuple(owned))
        self.assertEqual(ACTOR_ATTRIBUTES_ENTRIES,(0x10235ea0,0x102373a0,0x10235db0,0x102393c0,0x10239590,0x10239790,0x10187650,0x10187680,0x101878d0,0x10187a40,0x101877e0,0x10186a20,0x1018b9f0,0x10186ea0,0x101864a0,0x10186cd0,0x10186e10,0x10186fc0,0x10187450,0x10188930,0x1018c180,0x1018c340))

    def test_external_interior_edges_fail(self):
        for entry,end,*_ in CONTRACTS:
            require_actor_attributes_edges([(0,entry),(entry,end-4),(end-4,end)])
            for target in (entry+4,end-4):
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    require_actor_attributes_edges([(0,target)])

    def test_named_component_bodies_do_not_replay(self):
        source=''.join((ROOT/'native'/name).read_text() for name in ('actor_attributes.cpp','actor_condition.cpp','actor_condition_restore.cpp','actor_condition_records.cpp','actor_condition_refresh.cpp','actor_condition_lifecycle.cpp','actor_condition_sampling.cpp','actor_condition_queue.cpp','actor_condition_match.cpp','actor_injury_selection.cpp'))
        for forbidden in ('actor_attributes_reference','dispatch_page','run_until','pcode','i_101'):
            self.assertNotIn(forbidden,source)
        for name in ('advance_pool','wide_sample','condition_adjusted_pair','paired_trait','single_trait','recover_condition',
                     'refresh_condition','consume_condition','periodic_pool_draw','movement_condition_cost','action_condition_cost','restore_condition','record_condition_scale','record_availability','effective_condition','refresh_condition_record','partition_injury_days','partition_training_days','record_capacity','assign_capacity','retire_injury','reset_condition_record','sample_condition_range','scaled_condition_draw','recursive_correction_draw','merge_condition_update','find_condition_update','merge_first_update','apply_match_condition','match_minutes','match_scalar_effect','apply_queued_injuries','maintain_condition_scalar','decay_condition_scalar','advance_scalar_pool','select_injury','injury_category','current_injury','advance_injury_pool','repeated_injury_score','scaled_injury_score'):
            self.assertIn(name,source)

@unittest.skipUnless((INPUT/'instructions.sqlite').exists(),'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_actor_attributes_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']

    def test_original_action_table_pointer_and_every_word_are_pinned(self):
        words=load_actor_attributes_tables(INPUT);slot,table,count,_=COST_TABLE
        live={slot:table,**{table+n*4:word for n,word in enumerate(words)}}
        self.assertEqual(require_actor_attributes_tables(live.__getitem__),words)
        self.assertEqual(len(words),count)
        for address in live:
            changed=dict(live);changed[address]^=4
            with self.subTest(address=hex(address)),self.assertRaises(ValueError):
                require_actor_attributes_tables(changed.__getitem__)

    def test_injury_table_pointers_and_every_word_are_pinned(self):
        program=json.loads((INPUT/'program.json').read_text())
        segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
        import struct
        def word(address):
            for base,data in segments:
                if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
            raise ValueError('Missing table word')
        live={address:word(address) for slot,table,count,_ in INJURY_TABLES for address in (slot,*range(table,table+4*count,4))}
        self.assertEqual([len(w) for w in require_injury_tables(live.__getitem__)],[246,9])
        for address in live:
            changed=dict(live);changed[address]^=1
            with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_injury_tables(changed.__getitem__)

    def test_changed_source_fields_fail(self):
        for entry,page,*_ in FRAGMENTS:
            end=next(row[1] for row in CONTRACTS if row[0]==entry)
            index=next(n for n,row in enumerate(self.records) if entry<=row['address']<end and row['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[index][key]=value
                with self.subTest(entry=hex(entry),page=hex(page),key=key),self.assertRaises(ValueError):
                    require_actor_attributes_bodies(changed,complete=True)

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
        self.assertEqual(re.findall(r'actor_attributes_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        for forbidden in ('try_modern_','complete_context_transfer'):
            self.assertNotIn(forbidden,source)

    def test_partial_pages_fail_and_complete_pages_pass(self):
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows=[row for row in self.records if row['address']>>12==page]
            selected={row['address'] for row in self.records}
            require_actor_attributes_contract(page,rows,selected)
            for entry,end,*_ in CONTRACTS:
                if entry>>12<=page<=(end-1)>>12:
                    body=[row for row in rows if entry<=row['address']<end]
                    with self.assertRaises(ValueError):
                        require_actor_attributes_contract(page,[row for row in rows if row not in body],{entry})

    def test_productive_hooks_precede_accounting(self):
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows=[row for row in self.records if row['address']>>12==page]
            disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks')}
            disabled['actor_attributes_hooks']=True
            code=generate_page(page,rows,{row['address'] for row in self.records},userops=self.userops,**disabled)
            require_actor_attributes_hooks(page,rows,code)
            for entry in ACTOR_ATTRIBUTES_ENTRIES:
                if entry>>12!=page:continue
                hook=f'if (try_modern_actor_attributes(s, 0x{entry:x})) return true;'
                with self.assertRaises(ValueError):require_actor_attributes_hooks(page,rows,code.replace(hook,''))

    def test_production_hooks_none_inside_injury_unit(self):
        # The six entries outside INJURY.CPP are MATCH_PL.CPP's takeovers in production, so no page carries an
        # actor_attributes hook; the 16 inside INJURY.CPP carry injury_unit's alone.
        from compile_recomp import MATCH_PL_TAKEOVERS
        from injury_unit_audit import INJURY_UNIT_ENTRIES
        taken=frozenset(INJURY_UNIT_ENTRIES)|MATCH_PL_TAKEOVERS
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows=[row for row in self.records if row['address']>>12==page]
            code=generate_page(page,rows,{row['address'] for row in self.records},userops=self.userops)
            require_actor_attributes_hooks(page,rows,code,taken)
            for entry in ACTOR_ATTRIBUTES_ENTRIES:
                if entry>>12!=page:continue
                hook=f'if (try_modern_actor_attributes(s, 0x{entry:x})) return true;'
                self.assertEqual(code.count(hook),0 if entry in taken else 1,hex(entry))

if __name__=='__main__':unittest.main()
