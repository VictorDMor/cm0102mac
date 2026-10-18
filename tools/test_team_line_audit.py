"""Complete team-line body, pinned references and productive hooks."""
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
from make_team_line_reference import render_reference
from team_line_audit import (CONTRACTS, FRAGMENTS, TEAM_LINE_ENTRIES,
    require_team_line_bodies, require_team_line_contract,
    require_team_line_edges, require_team_line_hooks, require_team_line_index)

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'

class ScopeTests(unittest.TestCase):
    def test_complete_block_and_prior_manifest(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS),172)
        self.assertEqual(sum(row[2] for row in FRAGMENTS),172)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),832)
        self.assertEqual(len({entry for entry,_ in SEMANTIC_REPLACEMENTS}),832)
        self.assertEqual(SEMANTIC_REPLACEMENTS[672:677],tuple((entry,'team_line_adapter') for entry in TEAM_LINE_ENTRIES))
        prior=[{'entry':f'0x{entry:x}','module':module} for entry,module in SEMANTIC_REPLACEMENTS[:672]]
        self.assertEqual(hashlib.sha256(json.dumps(prior,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
            'cfd7c91dac9af6ee945faa871e8a923b67ac1245a9d81e1b1db8e44195d71e84')

    def test_external_interior_edges_fail(self):
        for entry,end,*_ in CONTRACTS:
            require_team_line_edges([(0,entry),(entry,end-4),(end-4,end)])
            for target in (entry+4,end-4):
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    require_team_line_edges([(0,target)])

    def test_named_production_bodies_do_not_replay(self):
        source=(ROOT/'native/team_line.cpp').read_text()
        for forbidden in ('team_line_reference','dispatch_page','pcode','i_101'):
            self.assertNotIn(forbidden,source)
        for name in ('tactical_flags','line_adjustment_enabled','actor_column_excess','column_excess','row_lane_clear','blocks_lane'):
            self.assertIn(name,source)
        adapter=(ROOT/'native/team_line_adapter.cpp').read_text()
        self.assertIn('methods[index](s, nullptr, d.observer)',adapter)
        self.assertIn('d.original_body == entry',adapter)

@unittest.skipUnless((INPUT/'instructions.sqlite').exists(),'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_team_line_index(db)
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
                    require_team_line_bodies(changed,complete=True)

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
        self.assertEqual(re.findall(r'team_line_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        for forbidden in ('try_modern_','complete_context_transfer'):
            self.assertNotIn(forbidden,source)

    def test_hooks_precede_original_accounting_and_missing_hooks_fail(self):
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows=[row for row in self.records if row['address']>>12==page]
            disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks')}
            disabled['team_line_hooks']=True
            code=generate_page(page,rows,{row['address'] for row in self.records},userops=self.userops,**disabled)
            require_team_line_hooks(page,rows,code)
            for entry in (entry for entry in TEAM_LINE_ENTRIES if entry>>12==page):
                hook=f'if (try_modern_team_line(s, 0x{entry:x})) return true;'
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    require_team_line_hooks(page,rows,code.replace(hook,''))
            for entry,end,*_ in CONTRACTS:
                if entry>>12<=page<=(end-1)>>12:
                    body=[row for row in rows if entry<=row['address']<end]
                    with self.assertRaises(ValueError):
                        require_team_line_contract(page,[row for row in rows if row not in body],{entry})

if __name__=='__main__':unittest.main()
