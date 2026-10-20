"""Complete target-facing and corridor acceptance component boundaries."""
from contextlib import closing
from copy import deepcopy
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from target_acceptance_audit import (CONTRACTS, FRAGMENTS, TARGET_ACCEPTANCE_ENTRIES,
    require_target_acceptance_bodies, require_target_acceptance_edges, require_target_acceptance_index,
    require_target_acceptance_contract, require_target_acceptance_hooks, target_acceptance_hook)
from make_target_acceptance_reference import CONTRACTS as REFERENCES, render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/current/export'

class ScopeTests(unittest.TestCase):
    def test_complete_four_root_scope_preserves949_owners(self):
        self.assertEqual(TARGET_ACCEPTANCE_ENTRIES, (0x10244c60,0x101eeac0,0x101eeeb0,0x101f09d0))
        self.assertEqual(tuple(v[2] for v in CONTRACTS), (252,121,324,61))
        self.assertEqual(sum(v[2] for v in FRAGMENTS), 758)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 963)
        self.assertEqual(SEMANTIC_REPLACEMENTS[949:953],tuple((e,'target_acceptance_adapter')for e in TARGET_ACCEPTANCE_ENTRIES))
        import hashlib
        prefix=[{'entry':hex(e),'module':m} for e,m in SEMANTIC_REPLACEMENTS[:949]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'5e8f5b9162c20cdfcd2ac5dc2bbd36e71f5529f33d5d564aaa4c5b056d1ddfcc')
        for entry,owner in ((0x10245050,'actor_motion_adapter'),(0x10226520,'range_sampling_adapter'),(0x1077b770,'rng_core_adapter'),(0x101d5680,'spatial_role_adapter'),(0x101d5690,'spatial_role_adapter'),(0x101eea30,'movement_choice_adapter'),(0x101ef8d0,'pitch_distance_adapter'),(0x10283690,'marking_pressure_adapter'),(0x10773808,'register_context_adapter'),(0x10773858,'register_context_adapter'),(0x10775540,'stdlib_core_adapter')):
            self.assertIn((entry,owner),SEMANTIC_REPLACEMENTS)
        self.assertEqual(sum(v[2] for v in REFERENCES),758)

    def test_duplicate_owner_fails_closed(self):
        from compile_recomp import require_unique_semantic_replacements
        proposed=SEMANTIC_REPLACEMENTS[:949]+tuple((e,'target_acceptance_adapter') for e in TARGET_ACCEPTANCE_ENTRIES)
        require_unique_semantic_replacements(proposed)
        for entry in TARGET_ACCEPTANCE_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*proposed,(entry,'duplicate')))

    def test_external_interior_entrances_fail_closed(self):
        for start,end,*_ in CONTRACTS:
            require_target_acceptance_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_target_acceptance_edges([(0,target)])

    def test_named_complete_bodies_have_explicit_service_boundaries(self):
        source='\n'.join((ROOT/f'native/target_acceptance_{name}.cpp').read_text() for name in ('facing','eligibility','route','cell'))
        for forbidden in ('target_acceptance_reference','dispatch_page','run_until','pcode','goto ','switch (','state.pc'):
            self.assertNotIn(forbidden,source)
        self.assertIn('add_executable(cm-target-acceptance-checks target_acceptance_checks.cpp', (ROOT/'native/CMakeLists.txt').read_text())
        self.assertEqual(set(re.findall(r'a.call\((0x[0-9a-f]+)\)',source)),{'0x1077b770','0x10226520','0x101eeac0','0x101eeeb0','0x10775540','0x101d5680','0x101d5690','0x101eea30','0x101ef8d0','0x10283690','0x10773808','0x10773858','0x101f09d0','0x10239f10'})

@unittest.skipUnless((INPUT/'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_target_acceptance_index(db)
            cls.records=[json.loads(v[0]) for start,end,*_ in REFERENCES for v in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']

    def test_each_source_fragment_pins_all_semantic_fields(self):
        for entry,page,*_ in FRAGMENTS:
            i=next(i for i,v in enumerate(self.records) if v['address']>>12==page and entry<=v['address']<next(c[1] for c in CONTRACTS if c[0]==entry))
            for field,value in [('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)]:
                changed=deepcopy(self.records);changed[i][field]=value
                with self.subTest(entry=hex(entry),field=field),self.assertRaises(ValueError):
                    require_target_acceptance_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_roots_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,v in enumerate(self.records) if v['address']==start)
            for changed in (self.records[:i]+self.records[i+1:], self.records+[self.records[i]],list(reversed(self.records)),[v for v in self.records if not start<=v['address']<end]):
                with self.subTest(entry=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_all_own_instructions_remain_independent(self):
        source=render_reference(self.records,self.userops)
        expected=[f'{v["address"]:x}' for v in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):',source,re.M),expected)
        self.assertEqual(re.findall(r'target_acceptance_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        self.assertNotIn('try_modern_',source)
        self.assertNotIn('complete_context_transfer',source)

    def test_partial_page_fragment_is_rejected(self):
        selected={v['address']:v for v in self.records}
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows=[v for v in self.records if v['address']>>12==page]
            require_target_acceptance_contract(page,rows,selected)
            with self.assertRaises(ValueError):require_target_acceptance_contract(page,rows[1:],selected)

    def test_destination_guard_rejects_missing_duplicate_and_late_hook(self):
        disabled={n:False for n in inspect.signature(generate_page).parameters if n.endswith('_hooks') or n=='guest_context_transfers'}
        disabled['target_acceptance_hooks']=True
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows=[v for v in self.records if v['address']>>12==page]
            source=generate_page(page,rows,{v['address'] for v in self.records},userops=self.userops,**disabled)
            require_target_acceptance_hooks(page,rows,source)
            for entry in TARGET_ACCEPTANCE_ENTRIES:
                if entry>>12!=page:continue
                hook=target_acceptance_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,hook+'\n'+hook,1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_target_acceptance_hooks(page,rows,changed)

if __name__=='__main__':unittest.main()
