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
    COST_TABLE, load_actor_attributes_tables, require_actor_attributes_tables)

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'

class ScopeTests(unittest.TestCase):
    def test_complete_block_and_prior_manifest(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS),1650)
        self.assertEqual(sum(row[2] for row in FRAGMENTS),1650)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),687)
        self.assertEqual(len({entry for entry,_ in SEMANTIC_REPLACEMENTS}),687)
        self.assertEqual(SEMANTIC_REPLACEMENTS[682:],tuple((entry,'actor_attributes_adapter') for entry in ACTOR_ATTRIBUTES_ENTRIES))
        prior=[{'entry':f'0x{entry:x}','module':module} for entry,module in SEMANTIC_REPLACEMENTS[:682]]
        self.assertEqual(hashlib.sha256(json.dumps(prior,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
            '0da57a2d0a0463cf13a501936c55b7795da6218e670ef21d37375d7ea75c61d3')
        self.assertEqual(ACTOR_ATTRIBUTES_ENTRIES,(0x10235ea0,0x102373a0,0x10235db0,0x102393c0,0x10239590))

    def test_external_interior_edges_fail(self):
        for entry,end,*_ in CONTRACTS:
            require_actor_attributes_edges([(0,entry),(entry,end-4),(end-4,end)])
            for target in (entry+4,end-4):
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    require_actor_attributes_edges([(0,target)])

    def test_named_component_bodies_do_not_replay(self):
        source=(ROOT/'native/actor_attributes.cpp').read_text()+(ROOT/'native/actor_condition.cpp').read_text()
        for forbidden in ('actor_attributes_reference','dispatch_page','run_until','pcode','i_101'):
            self.assertNotIn(forbidden,source)
        for name in ('advance_pool','wide_sample','condition_adjusted_pair','paired_trait','single_trait','recover_condition',
                     'refresh_condition','consume_condition','periodic_pool_draw','movement_condition_cost','action_condition_cost'):
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

if __name__=='__main__':unittest.main()
