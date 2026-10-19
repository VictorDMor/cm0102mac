"""Complete actor strategy flag query component boundaries."""
from contextlib import closing
from copy import deepcopy
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page
from actor_flags_audit import (CONTRACTS, FRAGMENTS, ACTOR_FLAGS_ENTRIES,
    require_actor_flags_bodies, require_actor_flags_edges, require_actor_flags_index,
    require_actor_flags_contract, require_actor_flags_hooks, actor_flags_hook)
from make_actor_flags_reference import CONTRACTS as REFERENCES, render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'

class ScopeTests(unittest.TestCase):
    def test_complete_seven_root_scope_excludes_existing_shared_owners(self):
        self.assertEqual(ACTOR_FLAGS_ENTRIES, (0x102862b0,0x10286320,0x10286390,0x10286400,0x10286470,0x102864b0,0x102864f0))
        self.assertEqual(tuple(v[2] for v in CONTRACTS), (28,28,28,28,15,15,43))
        self.assertEqual(sum(v[2] for v in FRAGMENTS), 185)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 946)
        self.assertEqual(SEMANTIC_REPLACEMENTS[939:], tuple((e,'actor_flags_adapter') for e in ACTOR_FLAGS_ENTRIES))
        import hashlib
        prefix=[{'entry':hex(e),'module':m} for e,m in SEMANTIC_REPLACEMENTS[:939]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(), '9e0716b918c906bb3ca689cbd5f8184da6cfc719ab0cdff3bdc7cfe4ecd9ebe2')
        for entry in (0x10113e10,0x102860c0,0x102861b0):
            self.assertIn((entry,'match_target_adapter'), SEMANTIC_REPLACEMENTS)
        self.assertEqual(sum(v[2] for v in REFERENCES), 191)

    def test_external_interior_entrances_fail_closed(self):
        for start,end,*_ in CONTRACTS:
            require_actor_flags_edges([(0,start),(start,end-4),(end-4,end)])
            for target in (start+4,end-4):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_actor_flags_edges([(0,target)])

    def test_named_bodies_use_existing_shared_reader_without_original_fallback(self):
        source=(ROOT/'native/actor_flags.cpp').read_text()
        for forbidden in ('actor_flags_reference','dispatch_page','run_until','pcode','goto ','switch (','state.pc'):
            self.assertNotIn(forbidden,source)
        self.assertEqual(source.count('a.call(0x10113e10)'),7)
        self.assertIn('add_executable(cm-actor-flags-checks actor_flags_checks.cpp', (ROOT/'native/CMakeLists.txt').read_text())

@unittest.skipUnless((INPUT/'instructions.sqlite').exists(), 'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_actor_flags_index(db)
            cls.records=[json.loads(v[0]) for start,end,*_ in REFERENCES for v in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']

    def test_each_source_fragment_pins_all_semantic_fields(self):
        for entry,page,*_ in FRAGMENTS:
            i=next(i for i,v in enumerate(self.records) if v['address']==entry)
            for field,value in [('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)]:
                changed=deepcopy(self.records);changed[i][field]=value
                with self.subTest(entry=hex(entry),field=field),self.assertRaises(ValueError):
                    require_actor_flags_bodies(changed,complete=True)

    def test_missing_duplicate_reordered_or_absent_roots_fail(self):
        for start,end,*_ in CONTRACTS:
            i=next(i for i,v in enumerate(self.records) if v['address']==start)
            for changed in (self.records[:i]+self.records[i+1:], self.records+[self.records[i]],list(reversed(self.records)),[v for v in self.records if not start<=v['address']<end]):
                with self.subTest(entry=hex(start)),self.assertRaises(ValueError):render_reference(changed,self.userops)

    def test_shared_reader_is_pinned_independently(self):
        i=next(i for i,v in enumerate(self.records) if v['address']==0x10113e10)
        changed=deepcopy(self.records);changed[i]['assembly']='changed'
        with self.assertRaises(ValueError):render_reference(changed,self.userops)
        with self.assertRaises(ValueError):render_reference(self.records[:i],self.userops)

    def test_all_own_and_shared_instructions_remain_independent(self):
        source=render_reference(self.records,self.userops)
        expected=[f'{v["address"]:x}' for v in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):',source,re.M),expected)
        self.assertEqual(re.findall(r'actor_flags_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        self.assertNotIn('try_modern_',source)
        self.assertNotIn('complete_context_transfer',source)

    def test_partial_page_fragment_is_rejected(self):
        selected={v['address']:v for v in self.records}
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows=[v for v in self.records if v['address']>>12==page]
            require_actor_flags_contract(page,rows,selected)
            with self.assertRaises(ValueError):require_actor_flags_contract(page,rows[1:],selected)

    def test_destination_guard_rejects_missing_duplicate_and_late_hook(self):
        disabled={n:False for n in inspect.signature(generate_page).parameters if n.endswith('_hooks') or n=='guest_context_transfers'}
        disabled['actor_flags_hooks']=True
        for page in sorted({v[1] for v in FRAGMENTS}):
            rows=[v for v in self.records if v['address']>>12==page]
            source=generate_page(page,rows,{v['address'] for v in self.records},userops=self.userops,**disabled)
            require_actor_flags_hooks(page,rows,source)
            for entry in ACTOR_FLAGS_ENTRIES:
                if entry>>12!=page:continue
                hook=actor_flags_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,hook+'\n'+hook,1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_actor_flags_hooks(page,rows,changed)

if __name__=='__main__':unittest.main()
