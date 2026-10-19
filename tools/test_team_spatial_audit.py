"""Complete team-spatial body, pinned references and productive hooks."""
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
from make_team_spatial_reference import render_reference
from team_spatial_audit import (CONTRACTS, FRAGMENTS, TEAM_SPATIAL_ENTRIES,
    require_team_spatial_bodies, require_team_spatial_contract,
    require_team_spatial_edges, require_team_spatial_hooks, require_team_spatial_index)

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'

class ScopeTests(unittest.TestCase):
    def test_complete_block_and_prior_manifest(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS),1624)
        self.assertEqual(sum(row[2] for row in FRAGMENTS),1624)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),946)
        self.assertEqual(len({entry for entry,_ in SEMANTIC_REPLACEMENTS}),946)
        self.assertEqual(SEMANTIC_REPLACEMENTS[656:660],tuple((entry,'team_spatial_adapter') for entry in TEAM_SPATIAL_ENTRIES))
        prior=[{'entry':f'0x{entry:x}','module':module} for entry,module in SEMANTIC_REPLACEMENTS[:656]]
        self.assertEqual(hashlib.sha256(json.dumps(prior,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
            '6619436c27e93ba3be0a0931c24b9c3c664948c22f6f132938570aa1b04c58ab')

    def test_external_interior_edges_fail(self):
        for entry,end,*_ in CONTRACTS:
            require_team_spatial_edges([(0,entry),(entry,end-4),(end-4,end)])
            for target in (entry+4,end-4):
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    require_team_spatial_edges([(0,target)])

    def test_named_production_bodies_do_not_replay(self):
        source=(ROOT/'native/team_spatial.cpp').read_text()
        for forbidden in ('team_spatial_reference','dispatch_page','run_until','pcode','i_101'):
            self.assertNotIn(forbidden,source)
        for name in ('team_actor_base','eligible_nearest_actor','consider_nearest_actor','scan_column_limits'):
            self.assertIn(name,source)
        adapter=(ROOT/'native/team_spatial_adapter.cpp').read_text()
        self.assertIn('methods[index](s, nullptr, d.observer)',adapter)
        self.assertIn('d.original_body == entry',adapter)

@unittest.skipUnless((INPUT/'instructions.sqlite').exists(),'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_team_spatial_index(db)
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
                    require_team_spatial_bodies(changed,complete=True)

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
        self.assertEqual(re.findall(r'team_spatial_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        for forbidden in ('try_modern_','complete_context_transfer'):
            self.assertNotIn(forbidden,source)

    def test_hooks_precede_original_accounting_and_missing_hooks_fail(self):
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows=[row for row in self.records if row['address']>>12==page]
            disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks')}
            disabled['team_spatial_hooks']=True
            code=generate_page(page,rows,{row['address'] for row in self.records},userops=self.userops,**disabled)
            require_team_spatial_hooks(page,rows,code)
            for entry in (entry for entry in TEAM_SPATIAL_ENTRIES if entry>>12==page):
                hook=f'if (try_modern_team_spatial(s, 0x{entry:x})) return true;'
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    require_team_spatial_hooks(page,rows,code.replace(hook,''))
            for entry,end,*_ in CONTRACTS:
                if entry>>12<=page<=(end-1)>>12:
                    body=[row for row in rows if entry<=row['address']<end]
                    with self.assertRaises(ValueError):
                        require_team_spatial_contract(page,[row for row in rows if row not in body],{entry})

if __name__=='__main__':unittest.main()
