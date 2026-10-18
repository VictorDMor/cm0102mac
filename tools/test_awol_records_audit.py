"""Original-source and candidate-integration guards for the AWOL records block."""
from contextlib import closing
from copy import deepcopy
import importlib
import hashlib
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'
OWNERS=('awol_records',)


class ManifestTests(unittest.TestCase):
    def test_exact_accepted_prefix_and_complete_candidate_owner(self):
        from compile_recomp import SEMANTIC_REPLACEMENTS, require_unique_semantic_replacements
        from awol_records_audit import AWOL_RECORDS_ENTRIES
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),756)
        self.assertEqual(len({entry for entry,_ in SEMANTIC_REPLACEMENTS}),756)
        self.assertEqual(len(AWOL_RECORDS_ENTRIES),21)
        prefix=[{'entry':f'0x{entry:x}','module':owner} for entry,owner in SEMANTIC_REPLACEMENTS[:735]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
                         'aa3d2cc801e513cc3fca8d7c83d1f0f250baee4d9d2dcb78a0c09c19580e3dc5')
        self.assertEqual(SEMANTIC_REPLACEMENTS[735:],tuple((entry,'awol_records_adapter') for entry in AWOL_RECORDS_ENTRIES))
        self.assertNotIn(0x10009ef0,{entry for entry,_ in SEMANTIC_REPLACEMENTS})
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry in AWOL_RECORDS_ENTRIES:
            with self.subTest(entry=hex(entry)),self.assertRaisesRegex(ValueError,'Duplicate semantic replacement'):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(entry,'other_owner')))


@unittest.skipUnless((INPUT/'instructions.sqlite').is_file(),'Private original source unavailable')
class PreparedBatchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.groups=[]
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            for owner in OWNERS:
                audit=importlib.import_module(owner+'_audit')
                getattr(audit,'require_'+owner+'_index')(db)
                records=[json.loads(row[0]) for start,end,*_ in audit.CONTRACTS for row in db.execute(
                    'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
                cls.groups.append((owner,audit,records))
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']

    def test_complete_boundaries_and_discriminating_source_guards(self):
        self.assertEqual(sum(len(records) for _,_,records in self.groups),6958)
        self.assertEqual(sum(len(audit.CONTRACTS) for _,audit,_ in self.groups),21)
        for owner,audit,records in self.groups:
            check=getattr(audit,'require_'+owner+'_bodies')
            check(records,complete=True)
            for start,end,*_ in audit.CONTRACTS:
                index=next(n for n,row in enumerate(records) if row['address']==start)
                for field,value in (('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                    changed=deepcopy(records);changed[index][field]=value
                    with self.subTest(owner=owner,start=hex(start),field=field),self.assertRaises(ValueError):check(changed,complete=True)
                with self.assertRaises(ValueError):check([r for r in records if r['address']!=start],complete=True)
                with self.assertRaises(ValueError):getattr(audit,'require_'+owner+'_edges')([(0,start+4)])
                self.assertEqual(sum(start<=r['address']<end for r in records),(end-start)//4-sum(start<=pc<end for pc,_ in getattr(audit,'PADDING',())))

    def test_independent_references_retain_every_original_site(self):
        for owner,_,records in self.groups:
            source=importlib.import_module('make_'+owner+'_reference').render_reference(records,self.userops)
            expected=[f'{r["address"]:x}' for r in records]
            self.assertEqual(re.findall(r'^i_([0-9a-f]+):',source,re.MULTILINE),expected)
            self.assertEqual(re.findall(owner+r'_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
            self.assertNotIn('try_modern_',source)

    def test_productive_hooks_precede_original_accounting(self):
        from compile_recomp import generate_page
        disabled={name:False for name in inspect.signature(generate_page).parameters
                  if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['awol_records_hooks']=True
        covered=set();pages=set()
        for owner,audit,records in self.groups:
            selected={row['address'] for row in records}
            for page in sorted({row['address']>>12 for row in records}):
                pages.add(page)
                part=[row for row in records if row['address']>>12==page]
                code=generate_page(page,part,selected,userops=self.userops,**disabled)
                audit.require_awol_records_hooks(page,part,code)
                for entry in audit.AWOL_RECORDS_ENTRIES:
                    if entry>>12!=page:continue
                    covered.add(entry)
                    hook=audit.awol_records_hook(entry)
                    step=next(token for token in (f's.step(0x{entry:x});',f'counter.step(0x{entry:x});') if token in code)
                    for changed in (code.replace(hook,'',1),code.replace(hook,'',1).replace(step,step+'\n'+hook,1),
                                    code.replace(hook,hook+'\n'+hook,1)):
                        with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):
                            audit.require_awol_records_hooks(page,part,changed)
        self.assertEqual(len(pages),7)
        self.assertEqual(len(covered),21)

    def test_index_edge_scan_covers_non_sorted_contracts(self):
        # Deliberately reverse contract order so first/last endpoints cannot
        # stand in for the complete interval containing all prepared roots.
        for owner,audit,records in self.groups:
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY,body TEXT)')
                db.executemany('INSERT INTO instructions VALUES(?,?)',[(r['address'],json.dumps(r)) for r in records])
                with patch.object(audit,'CONTRACTS',tuple(reversed(audit.CONTRACTS))):
                    getattr(audit,'require_'+owner+'_index')(db)
                    high_root=max(start for start,*_ in audit.CONTRACTS)
                    db.execute('INSERT INTO instructions VALUES(?,?)',(0,json.dumps({'address':0,'flows':[high_root+4]})))
                    with self.assertRaises(ValueError):getattr(audit,'require_'+owner+'_index')(db)

    def test_original_constant_and_pointer_mutations_fail(self):
        import struct
        program=json.loads((INPUT/'program.json').read_text())
        segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
        def word(address):
            for base,data in segments:
                if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
            raise ValueError('Missing original batch table word')
        for owner,audit,_ in self.groups:
            check=getattr(audit,'require_'+owner+'_tables')
            values={address:word(address) for slot,base,size,_ in audit.TABLES for address in (slot,*range(base,base+size,4))}
            values.update({address:word(address) for address,_ in getattr(audit,'PADDING',())})
            check(values.__getitem__)
            for slot,base,*_ in (*audit.TABLES,*((pc,pc) for pc,_ in getattr(audit,'PADDING',()))):
                for address in {slot,base}:
                    changed=dict(values);changed[address]^=1
                    with self.subTest(owner=owner,address=hex(address)),self.assertRaises(ValueError):check(changed.__getitem__)

    def test_named_semantics_do_not_replay_original_instructions(self):
        for name in ('awol_records_lifecycle.cpp','awol_records_load.cpp','awol_records_save.cpp','awol_records_queries.cpp','awol_records_notice.cpp','awol_sampling.cpp','awol_daily.cpp','awol_daily_return.cpp','awol_daily_detail.hpp','awol_registration.cpp','awol_training_notice.cpp','awol_event_random.hpp','awol_primary_message.cpp','awol_training_message.cpp','awol_fitness_message.cpp','awol_message_detail.hpp','awol_absence_events.cpp','awol_fitness_events.cpp'):
            source=(ROOT/'native'/name).read_text()
            for token in ('_reference(', 'dispatch_page(', 'run_until(', 'pcode', 'i_101'):
                self.assertNotIn(token,source)

    def test_emitted_fixture_payloads_retain_original_hashes_without_overlap(self):
        import hashlib
        import struct
        for owner,audit,_ in self.groups:
            generator=importlib.import_module('make_'+owner+'_reference')
            source=generator.render_fixture_tables(getattr(audit,'load_'+owner+'_tables')(INPUT))
            cursor=0;slots={};memory={}
            for line in source.splitlines():
                if match:=re.fullmatch(r's.memory.write<4>\(toc \+ \((-?\d+)\), base\);',line):slots[int(match[1])]=cursor
                elif match:=re.fullmatch(r's.memory.write<4>\(base \+ (\d+), 0x([0-9a-f]+)u\);',line):
                    address=cursor+int(match[1]);self.assertNotIn(address,memory);memory[address]=int(match[2],16)
                elif match:=re.fullmatch(r'base \+= 0x([0-9a-f]+);',line):cursor+=int(match[1],16)
            self.assertEqual(slots,{-0x5a08:0,-0x5a0c:0x2000})
            for offset,(_,_,size,expected) in zip((-0x5a08,-0x5a0c),audit.TABLES):
                data=b''.join(struct.pack('>I',memory[slots[offset]+n]) for n in range(0,size,4))
                self.assertEqual(hashlib.sha256(data).hexdigest(),expected)


if __name__=='__main__':unittest.main()
