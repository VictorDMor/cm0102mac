"""Complete actor balance and temporary counter component boundaries."""
from contextlib import closing
from copy import deepcopy
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from actor_effects_audit import (CONTRACTS, FRAGMENTS, ACTOR_EFFECTS_ENTRIES,
    require_actor_effects_bodies, require_actor_effects_edges, require_actor_effects_index,
    require_actor_effects_contract, require_actor_effects_hooks, actor_effects_hook)
from make_actor_effects_reference import CONTRACTS as REFERENCES, render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/current/export'

class ScopeTests(unittest.TestCase):
    def test_complete_three_root_scope_preserves946_owners(self):
        self.assertEqual(ACTOR_EFFECTS_ENTRIES, (0x10237b50,0x10285ed0,0x10285ee0))
        self.assertEqual(tuple(v[2] for v in CONTRACTS), (1563,4,2))
        self.assertEqual(sum(v[2] for v in FRAGMENTS), 1569)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 963)
        self.assertEqual(SEMANTIC_REPLACEMENTS[946:949],tuple((e,'actor_effects_adapter') for e in ACTOR_EFFECTS_ENTRIES))
        import hashlib
        prefix=[{'entry':hex(e),'module':m} for e,m in SEMANTIC_REPLACEMENTS[:946]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'5fcd731f409c4d9c373ffca63b23ce14688c074e40267a6506bb574576690082')
        for entry,owner in ((0x10226520,'range_sampling_adapter'),(0x102865a0,'actor_encounter_adapter'),(0x10289560,'spatial_role_adapter'),(0x1077b770,'rng_core_adapter')):
            self.assertIn((entry,owner),SEMANTIC_REPLACEMENTS)
        self.assertEqual(sum(v[2] for v in REFERENCES),1569)

    def test_duplicate_owner_fails_closed(self):
        from compile_recomp import require_unique_semantic_replacements
        proposed=SEMANTIC_REPLACEMENTS[:946]+tuple((e,'actor_effects_adapter') for e in ACTOR_EFFECTS_ENTRIES)
        require_unique_semantic_replacements(proposed)
        for entry in ACTOR_EFFECTS_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*proposed,(entry,'duplicate')))

    def test_external_interior_entrances_fail_closed(self):
        for start,end,*_ in CONTRACTS:
            require_actor_effects_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_actor_effects_edges([(0,target)])

    def test_named_complete_bodies_have_explicit_service_boundaries(self):
        source=(ROOT/'native/actor_effects.cpp').read_text()
        for forbidden in ('actor_effects_reference','dispatch_page','run_until','pcode','goto ','switch (','state.pc'):
            self.assertNotIn(forbidden,source)
        self.assertEqual(set(re.findall(r'a.call\((0x[0-9a-f]+)\)',source)),{'0x1077b770','0x10226520','0x10289560','0x102865a0'})
        self.assertIn('add_executable(cm-actor-effects-checks actor_effects_checks.cpp', (ROOT/'native/CMakeLists.txt').read_text())

@unittest.skipUnless((INPUT/'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_actor_effects_index(db)
            cls.records=[json.loads(v[0]) for start,end,*_ in REFERENCES for v in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']

    def test_each_source_fragment_pins_all_semantic_fields(self):
        for entry,page,*_ in FRAGMENTS:
            i=next(i for i,v in enumerate(self.records) if v['address']==entry)
            for field,value in [('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)]:
                changed=deepcopy(self.records);changed[i][field]=value
                with self.subTest(entry=hex(entry),field=field),self.assertRaises(ValueError):
                    require_actor_effects_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_roots_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,v in enumerate(self.records) if v['address']==start)
            for changed in (self.records[:i]+self.records[i+1:], self.records+[self.records[i]],list(reversed(self.records)),[v for v in self.records if not start<=v['address']<end]):
                with self.subTest(entry=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_all_own_instructions_remain_independent(self):
        source=render_reference(self.records,self.userops)
        expected=[f'{v["address"]:x}' for v in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):',source,re.M),expected)
        self.assertEqual(re.findall(r'actor_effects_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        self.assertNotIn('try_modern_',source)
        self.assertNotIn('complete_context_transfer',source)

    def test_partial_page_fragment_is_rejected(self):
        selected={v['address']:v for v in self.records}
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows=[v for v in self.records if v['address']>>12==page]
            require_actor_effects_contract(page,rows,selected)
            with self.assertRaises(ValueError):require_actor_effects_contract(page,rows[1:],selected)

    def test_destination_guard_rejects_missing_duplicate_and_late_hook(self):
        disabled={n:False for n in inspect.signature(generate_page).parameters if n.endswith('_hooks') or n=='guest_context_transfers'}
        disabled['actor_effects_hooks']=True
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows=[v for v in self.records if v['address']>>12==page]
            source=generate_page(page,rows,{v['address'] for v in self.records},userops=self.userops,**disabled)
            require_actor_effects_hooks(page,rows,source)
            for entry in ACTOR_EFFECTS_ENTRIES:
                if entry>>12!=page:continue
                hook=actor_effects_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,hook+'\n'+hook,1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_actor_effects_hooks(page,rows,changed)

if __name__=='__main__':unittest.main()
