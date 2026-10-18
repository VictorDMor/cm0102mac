from contextlib import closing
"""Complete actor-motion boundaries, manifest, independent reference and hooks."""
from copy import deepcopy
import hashlib
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest
from actor_motion_audit import BODIES, ACTOR_MOTION_ENTRIES, load_actor_motion, require_actor_motion_contract, require_actor_motion_edges, require_actor_motion_hooks
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from make_actor_motion_reference import render_reference

ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'

class ScopeTests(unittest.TestCase):
    def test_five_whole_bodies_and_preserved_prefix(self):
        self.assertEqual(sum(row[2] for row in BODIES),462)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),783)
        self.assertEqual(len({a for a,_ in SEMANTIC_REPLACEMENTS}),783)
        self.assertEqual(SEMANTIC_REPLACEMENTS[643:648],tuple((a,'actor_motion_adapter') for a in ACTOR_MOTION_ENTRIES))
        prior=[{'entry':f'0x{a:x}','module':m} for a,m in SEMANTIC_REPLACEMENTS[:643]]
        self.assertEqual(hashlib.sha256(json.dumps(prior,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
                         '3f290aa60e8a00b40e6bceba6d3abfbe86e6a32413fa267d7ec7c2e07a7c1398')
        self.assertNotIn(0x1022e7e0,ACTOR_MOTION_ENTRIES)

    def test_exterior_interior_entries_fail(self):
        for entry,end,*_ in BODIES:
            require_actor_motion_edges([(0,entry),(entry,end-4),(end-4,end)])
            for target in (entry+4,end-4):
                with self.subTest(entry=hex(entry),target=hex(target)),self.assertRaises(ValueError):
                    require_actor_motion_edges([(0,target)])

    def test_named_production_bodies_do_not_replay_themselves(self):
        source=(ROOT/'native/actor_motion.cpp').read_text()
        for forbidden in ('actor_motion_reference','dispatch_page','run_until','pcode','i_102'):
            self.assertNotIn(forbidden,source)
        targets={int(value,16) for value in re.findall(r'\.call\((0x[0-9a-f]+)\)',source)}
        self.assertEqual(targets&set(ACTOR_MOTION_ENTRIES),{0x102775c0})
        for name in ('migrate_registered_cell','register_new_cell','apply_heading_jitter','apply_player_heading_bias','can_face_destination'):
            self.assertIn(name,source)

@unittest.skipUnless((INPUT/'instructions.sqlite').exists(),'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            cls.records=load_actor_motion(db)
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']

    def test_missing_reordered_duplicate_and_changed_fields_fail(self):
        for entry,end,*_ in BODIES:
            index=next(n for n,row in enumerate(self.records) if row['address']==entry)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[index][key]=value
                with self.subTest(entry=hex(entry),key=key),self.assertRaises(ValueError):
                    require_actor_motion_contract(entry>>12,changed)
            for changed in (self.records[:index]+self.records[index+1:],self.records+[self.records[index]],
                            [row for row in self.records if not entry<=row['address']<end],list(reversed(self.records))):
                # A completely absent body is checked when its entry is selected.
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    require_actor_motion_contract(entry>>12,changed,{entry})

    def test_reference_has_every_label_step_and_no_hooks(self):
        code=render_reference(self.records,self.userops)
        expected=[f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):',code,re.MULTILINE),expected)
        self.assertEqual(re.findall(r'actor_motion_reference_step_seen\(s, 0x([0-9a-f]+)\)',code),expected)
        self.assertNotIn('try_modern_',code)

    def test_production_hooks_precede_original_accounting_and_negatives_fail(self):
        for page in {entry>>12 for entry in ACTOR_MOTION_ENTRIES}:
            records=[row for row in self.records if row['address']>>12==page]
            disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks')}
            disabled['actor_motion_hooks']=True
            code=generate_page(page,records,{row['address'] for row in self.records},userops=self.userops,**disabled)
            require_actor_motion_hooks(page,records,code)
            for entry in (a for a in ACTOR_MOTION_ENTRIES if a>>12==page):
                hook=f'if (try_modern_actor_motion(s, 0x{entry:x})) return true;'
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                    require_actor_motion_hooks(page,records,code.replace(hook,''))

if __name__=='__main__':unittest.main()
