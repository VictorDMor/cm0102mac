"""Complete actor-attribute component boundaries and independent references."""
from contextlib import closing
from copy import deepcopy
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS
from make_actor_attributes_reference import render_reference
from actor_attributes_audit import (CONTRACTS, FRAGMENTS, ACTOR_ATTRIBUTES_ENTRIES,
    require_actor_attributes_bodies, require_actor_attributes_contract,
    require_actor_attributes_edges, require_actor_attributes_index)

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'

class ScopeTests(unittest.TestCase):
    def test_complete_block_and_prior_manifest(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS),1349)
        self.assertEqual(sum(row[2] for row in FRAGMENTS),1349)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),682)
        self.assertEqual(len({entry for entry,_ in SEMANTIC_REPLACEMENTS}),682)
        self.assertFalse(set(ACTOR_ATTRIBUTES_ENTRIES) & {entry for entry,_ in SEMANTIC_REPLACEMENTS})
        self.assertEqual(ACTOR_ATTRIBUTES_ENTRIES,(0x10235ea0,0x102373a0))

    def test_external_interior_edges_fail(self):
        for entry,end,*_ in CONTRACTS:
            require_actor_attributes_edges([(0,entry),(entry,end-4),(end-4,end)])
            for target in (entry+4,end-4):
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    require_actor_attributes_edges([(0,target)])

    def test_named_component_bodies_do_not_replay(self):
        source=(ROOT/'native/actor_attributes.cpp').read_text()
        for forbidden in ('actor_attributes_reference','dispatch_page','run_until','pcode','i_101'):
            self.assertNotIn(forbidden,source)
        for name in ('advance_pool','wide_sample','condition_adjusted_pair','paired_trait','single_trait'):
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

if __name__=='__main__':unittest.main()
