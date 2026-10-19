"""Complete actor-action routing and selection component boundaries."""
from contextlib import closing
from copy import deepcopy
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from actor_action_audit import (CONTRACTS, FRAGMENTS, ACTOR_ACTION_ENTRIES,
    require_actor_action_bodies, require_actor_action_edges, require_actor_action_index,
    require_actor_action_contract, require_actor_action_hooks, actor_action_hook)
from make_actor_action_reference import CONTRACTS as REFERENCES, render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'

class ScopeTests(unittest.TestCase):
    def test_complete_five_root_scope_preserves953_owners(self):
        self.assertEqual(ACTOR_ACTION_ENTRIES, (0x102775e0,0x10278220,0x10281780,0x102818f0,0x10281af0))
        self.assertEqual(tuple(v[2] for v in CONTRACTS), (783,9558,92,127,1683))
        self.assertEqual(sum(v[2] for v in FRAGMENTS), 12243)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 958)
        self.assertEqual(SEMANTIC_REPLACEMENTS[953:],tuple((e,'actor_action_adapter')for e in ACTOR_ACTION_ENTRIES))
        import hashlib
        prefix=[{'entry':hex(e),'module':m} for e,m in SEMANTIC_REPLACEMENTS[:953]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'e6d8eb80de5d02ed31b144d7b350c5fec009df7f22edec85b9e2bf1f7271bf03')
        for entry,owner in ((0x102775c0,'actor_motion_adapter'),(0x102836a0,'player_decision_adapter'),(0x101d8120,'marking_pressure_adapter'),(0x10235db0,'actor_attributes_adapter'),(0x101eea30,'movement_choice_adapter'),(0x10226520,'range_sampling_adapter'),(0x1077b770,'rng_core_adapter')):
            self.assertIn((entry,owner),SEMANTIC_REPLACEMENTS)
        self.assertEqual(sum(v[2] for v in REFERENCES),12243)

    def test_duplicate_owner_fails_closed(self):
        from compile_recomp import require_unique_semantic_replacements
        proposed=SEMANTIC_REPLACEMENTS[:953]+tuple((e,'actor_action_adapter') for e in ACTOR_ACTION_ENTRIES)
        require_unique_semantic_replacements(proposed)
        for entry in ACTOR_ACTION_ENTRIES:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*proposed,(entry,'duplicate')))

    def test_external_interior_entrances_fail_closed(self):
        for start,end,*_ in CONTRACTS:
            require_actor_action_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_actor_action_edges([(0,target)])

    def test_named_complete_bodies_have_explicit_service_boundaries(self):
        source='\n'.join((ROOT/f'native/actor_action_{name}.cpp').read_text() for name in ('routing','primary','destination','turn','secondary'))
        for forbidden in ('actor_action_reference','dispatch_page','run_until','pcode','goto ','state.pc'):
            self.assertNotIn(forbidden,source)
        self.assertEqual(source.count('switch ('),1)
        self.assertIn('switch (a.r(0) >> 2)',source)
        self.assertEqual(set(re.findall(r'case (\d+):',source)),set(map(str,range(9))))
        self.assertEqual(set(re.findall(r'a.call\((0x[0-9a-f]+)\)',source)),{'0x101eea30', '0x101ee770', '0x101e4020', '0x101ee610', '0x102374f0', '0x10235db0', '0x101e4e10', '0x101f1e10', '0x10773858', '0x10245220', '0x101e4ea0', '0x101d7dc0', '0x101e28b0', '0x102818f0', '0x10289560', '0x1022e7e0', '0x10239e50', '0x102836a0', '0x10286400', '0x10286390', '0x101149f0', '0x10252080', '0x10278220', '0x10281780', '0x1028abc0', '0x10768810', '0x101eeac0', '0x101ef3c0', '0x102862b0', '0x10738c00', '0x101d8120', '0x101ee680', '0x10773808', '0x10201180', '0x101d8260', '0x102576e0', '0x10286320', '0x1077b720', '0x101f5280', '0x10285790', '0x102864f0', '0x10739890', '0x101fd000', '0x101ee9c0', '0x10281af0', '0x102775e0', '0x101e25a0', '0x102857c0', '0x10274170', '0x101ee9f0', '0x10226520', '0x102864b0', '0x101d8d70', '0x10780fc0', '0x101e3d50', '0x1076f7d0', '0x101f0dd0', '0x10237830', '0x10769b60', '0x10245870', '0x10775540', '0x101e8950', '0x101d69c0', '0x10244c60', '0x101ef8d0', '0x10738c20', '0x101f21d0', '0x101e3f10', '0x1077b770'})

@unittest.skipUnless((INPUT/'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_actor_action_index(db)
            cls.records=[json.loads(v[0]) for start,end,*_ in REFERENCES for v in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']

    def test_each_source_fragment_pins_all_semantic_fields(self):
        for entry,page,*_ in FRAGMENTS:
            i=next(i for i,v in enumerate(self.records) if v['address']>>12==page and entry<=v['address']<next(c[1] for c in CONTRACTS if c[0]==entry))
            for field,value in [('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)]:
                changed=deepcopy(self.records);changed[i][field]=value
                with self.subTest(entry=hex(entry),field=field),self.assertRaises(ValueError):
                    require_actor_action_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_roots_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,v in enumerate(self.records) if v['address']==start)
            for changed in (self.records[:i]+self.records[i+1:], self.records+[self.records[i]],list(reversed(self.records)),[v for v in self.records if not start<=v['address']<end]):
                with self.subTest(entry=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_all_own_instructions_remain_independent(self):
        source=render_reference(self.records,self.userops)
        expected=[f'{v["address"]:x}' for v in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):',source,re.M),expected)
        self.assertEqual(re.findall(r'actor_action_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        self.assertNotIn('try_modern_',source)
        self.assertNotIn('complete_context_transfer',source)

    def test_partial_page_fragment_is_rejected(self):
        selected={v['address']:v for v in self.records}
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows=[v for v in self.records if v['address']>>12==page]
            require_actor_action_contract(page,rows,selected)
            with self.assertRaises(ValueError):require_actor_action_contract(page,rows[1:],selected)

    def test_hook_validator_rejects_missing_duplicate_and_late_hook(self):
        # Render actual destination hooks with every unrelated adapter disabled.
        disabled={n:False for n in inspect.signature(generate_page).parameters if n.endswith('_hooks') or n=='guest_context_transfers'}
        disabled['actor_action_hooks']=True
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows=[v for v in self.records if v['address']>>12==page]
            source=generate_page(page,rows,{v['address'] for v in self.records},userops=self.userops,**disabled)
            require_actor_action_hooks(page,rows,source)
            for entry in ACTOR_ACTION_ENTRIES:
                if entry>>12!=page:continue
                hook=actor_action_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,hook+'\n'+hook,1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_actor_action_hooks(page,rows,changed)

    def test_nine_way_original_classification_table_is_pinned(self):
        import struct
        from actor_action_audit import PRIMARY_ACTION_TABLE, require_actor_action_tables, load_actor_action_table
        slot,base,size,_=PRIMARY_ACTION_TABLE
        data=load_actor_action_table(INPUT)
        self.assertEqual(size,36)
        self.assertEqual(struct.unpack('>9I',data),(0x10278cfc,0x10278cfc,0x1027846c,0x10278360,0x102785f4,0x102787b0,0x1027895c,0x10278890,0x10278350))
        def read(address,n):
            if address==slot and n==4:return struct.pack('>I',base)
            if address==base and n==size:return data
            raise ValueError('Unexpected actor-action source read')
        require_actor_action_tables(read)
        with self.assertRaises(ValueError):require_actor_action_tables(lambda a,n:bytes(n))
        for i in range(len(data)):
            changed=data[:i]+bytes([data[i]^1])+data[i+1:]
            with self.assertRaises(ValueError):require_actor_action_tables(lambda a,n:struct.pack('>I',base)if a==slot else changed)

if __name__=='__main__':unittest.main()
