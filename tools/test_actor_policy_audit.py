"""Complete actor decision-policy contracts, owners and destination hooks."""
from contextlib import closing
from copy import deepcopy
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from actor_policy_audit import (CONTRACTS, FRAGMENTS, ACTOR_POLICY_ENTRIES,
    require_actor_policy_bodies, require_actor_policy_edges, require_actor_policy_index,
    require_actor_policy_contract, require_actor_policy_hooks, actor_policy_hook)
from make_actor_policy_reference import CONTRACTS as REFERENCES, render_reference
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/current/export'
class ScopeTests(unittest.TestCase):
    def test_complete_five_root_scope_preserves958_owners(self):
        self.assertEqual(ACTOR_POLICY_ENTRIES,(0x101f5280,0x10285750,0x10285790,0x102857c0,0x10285ef0))
        self.assertEqual(tuple(v[2]for v in CONTRACTS),(50,16,9,449,116))
        self.assertEqual(sum(v[2]for v in FRAGMENTS),640)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),963)
        self.assertEqual(SEMANTIC_REPLACEMENTS[958:],tuple((e,'actor_policy_adapter')for e in ACTOR_POLICY_ENTRIES))
        import hashlib
        prefix=[{'entry':hex(e),'module':m}for e,m in SEMANTIC_REPLACEMENTS[:958]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'6d9dee2bb0edee8d4963510206e28e87e06bc74dbff1152255ca0e53635cf264')
        for entry,owner in ((0x102775e0,'actor_action_adapter'),(0x10285ed0,'actor_effects_adapter'),(0x102860c0,'match_target_adapter'),(0x102862b0,'actor_flags_adapter'),(0x10289560,'spatial_role_adapter'),(0x10283690,'marking_pressure_adapter'),(0x101e4e10,'team_line_adapter'),(0x10226520,'range_sampling_adapter')):
            self.assertIn((entry,owner),SEMANTIC_REPLACEMENTS)
        self.assertEqual(sum(v[2]for v in REFERENCES),640)
    def test_duplicate_owners_fail_closed(self):
        from compile_recomp import require_unique_semantic_replacements
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry in ACTOR_POLICY_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(entry,'duplicate')))
    def test_external_interior_entrances_fail_closed(self):
        for start,end,*_ in CONTRACTS:
            require_actor_policy_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.subTest(entry=hex(start)),self.assertRaises(ValueError):require_actor_policy_edges([(0,target)])
    def test_named_bodies_have_explicit_services_and_a_structured_scan(self):
        source='\n'.join((ROOT/f'native/actor_policy_{part}.cpp').read_text()for part in ('context','column','phase','bias','opponent'))
        for forbidden in ('actor_policy_reference','dispatch_page','run_until','pcode','goto ','state.pc','switch ('):
            self.assertNotIn(forbidden,source)
        self.assertEqual(source.count('for (;;)'),1)
        self.assertIn('scan_linked_opponents',source)
        self.assertEqual(set(re.findall(r'a.call\((0x[0-9a-f]+)\)',source)),{'0x10289560','0x10283690','0x101e4e10','0x1077b770','0x10226520','0x101149f0','0x10114b10','0x102895e0'})

@unittest.skipUnless((INPUT/'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_actor_policy_index(db)
            cls.records=[json.loads(v[0]) for start,end,*_ in REFERENCES for v in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']

    def test_each_source_fragment_pins_all_semantic_fields(self):
        for entry,page,*_ in FRAGMENTS:
            i=next(i for i,v in enumerate(self.records) if v['address']>>12==page and entry<=v['address']<next(c[1] for c in CONTRACTS if c[0]==entry))
            for field,value in [('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)]:
                changed=deepcopy(self.records);changed[i][field]=value
                with self.subTest(entry=hex(entry),field=field),self.assertRaises(ValueError):
                    require_actor_policy_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_roots_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,v in enumerate(self.records) if v['address']==start)
            for changed in (self.records[:i]+self.records[i+1:], self.records+[self.records[i]],list(reversed(self.records)),[v for v in self.records if not start<=v['address']<end]):
                with self.subTest(entry=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_all_own_instructions_remain_independent(self):
        source=render_reference(self.records,self.userops)
        expected=[f'{v["address"]:x}' for v in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):',source,re.M),expected)
        self.assertEqual(re.findall(r'actor_policy_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        self.assertNotIn('try_modern_',source)
        self.assertNotIn('complete_context_transfer',source)

    def test_partial_page_fragment_is_rejected(self):
        selected={v['address']:v for v in self.records}
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows=[v for v in self.records if v['address']>>12==page]
            require_actor_policy_contract(page,rows,selected)
            with self.assertRaises(ValueError):require_actor_policy_contract(page,rows[1:],selected)

    def test_hook_validator_rejects_missing_duplicate_and_late_hook(self):
        # Render actual destination hooks with every unrelated adapter disabled.
        disabled={n:False for n in inspect.signature(generate_page).parameters if n.endswith('_hooks') or n=='guest_context_transfers'}
        disabled['actor_policy_hooks']=True
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows=[v for v in self.records if v['address']>>12==page]
            source=generate_page(page,rows,{v['address'] for v in self.records},userops=self.userops,**disabled)
            require_actor_policy_hooks(page,rows,source)
            for entry in ACTOR_POLICY_ENTRIES:
                if entry>>12!=page:continue
                hook=actor_policy_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,hook+'\n'+hook,1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_actor_policy_hooks(page,rows,changed)

if __name__=='__main__':unittest.main()
