"""Complete pitch-distance body, pinned references and productive hooks."""
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
from make_pitch_distance_reference import render_reference
from pitch_distance_audit import (CONTRACTS, FRAGMENTS, PITCH_DISTANCE_ENTRIES,
    require_pitch_distance_bodies, require_pitch_distance_contract,
    require_pitch_distance_edges, require_pitch_distance_hooks, require_pitch_distance_index)

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'

class ScopeTests(unittest.TestCase):
    def test_complete_block_and_prior_manifest(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS),129)
        self.assertEqual(sum(row[2] for row in FRAGMENTS),129)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),905)
        self.assertEqual(len({entry for entry,_ in SEMANTIC_REPLACEMENTS}),905)
        self.assertEqual(SEMANTIC_REPLACEMENTS[660:663],tuple((entry,'pitch_distance_adapter') for entry in PITCH_DISTANCE_ENTRIES))
        prior=[{'entry':f'0x{entry:x}','module':module} for entry,module in SEMANTIC_REPLACEMENTS[:660]]
        self.assertEqual(hashlib.sha256(json.dumps(prior,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
            '797dc66686e0b700d23ec291d36cb087f6d31d2fadaf0ab5c08be2c0afcbf8ed')

    def test_external_interior_edges_fail(self):
        for entry,end,*_ in CONTRACTS:
            require_pitch_distance_edges([(0,entry),(entry,end-4),(end-4,end)])
            for target in (entry+4,end-4):
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    require_pitch_distance_edges([(0,target)])

    def test_named_production_bodies_do_not_replay(self):
        source=(ROOT/'native/pitch_distance.cpp').read_text()
        for forbidden in ('pitch_distance_reference','dispatch_page','pcode','i_101'):
            self.assertNotIn(forbidden,source)
        for name in ('point_to_point','actor_to_cell','actor_to_attacking_goal','bounded_delta'):
            self.assertIn(name,source)
        adapter=(ROOT/'native/pitch_distance_adapter.cpp').read_text()
        self.assertIn('methods[index](s, nullptr, d.observer)',adapter)
        self.assertIn('d.original_body == entry',adapter)

@unittest.skipUnless((INPUT/'instructions.sqlite').exists(),'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_pitch_distance_index(db)
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
                    require_pitch_distance_bodies(changed,complete=True)

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
        self.assertEqual(re.findall(r'pitch_distance_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        for forbidden in ('try_modern_','complete_context_transfer'):
            self.assertNotIn(forbidden,source)

    def test_hooks_precede_original_accounting_and_missing_hooks_fail(self):
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows=[row for row in self.records if row['address']>>12==page]
            disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks')}
            disabled['pitch_distance_hooks']=True
            code=generate_page(page,rows,{row['address'] for row in self.records},userops=self.userops,**disabled)
            require_pitch_distance_hooks(page,rows,code)
            for entry in (entry for entry in PITCH_DISTANCE_ENTRIES if entry>>12==page):
                hook=f'if (try_modern_pitch_distance(s, 0x{entry:x})) return true;'
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    require_pitch_distance_hooks(page,rows,code.replace(hook,''))
            for entry,end,*_ in CONTRACTS:
                if entry>>12<=page<=(end-1)>>12:
                    body=[row for row in rows if entry<=row['address']<end]
                    with self.assertRaises(ValueError):
                        require_pitch_distance_contract(page,[row for row in rows if row not in body],{entry})

if __name__=='__main__':unittest.main()
