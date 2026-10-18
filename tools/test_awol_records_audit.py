"""Original-source guards for the prepared AWOL records components."""
from contextlib import closing
from copy import deepcopy
import importlib
import json
from pathlib import Path
import re
import sqlite3
import unittest

ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'
OWNERS=('awol_records',)


class PreparedOwnershipTests(unittest.TestCase):
    def test_all_thirteen_prepared_entries_have_no_accepted_owner(self):
        from compile_recomp import SEMANTIC_REPLACEMENTS, require_unique_semantic_replacements
        from awol_records_audit import AWOL_RECORDS_ENTRIES
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),735)
        self.assertEqual(len({entry for entry,_ in SEMANTIC_REPLACEMENTS}),735)
        self.assertEqual(len(AWOL_RECORDS_ENTRIES),13)
        self.assertFalse(set(AWOL_RECORDS_ENTRIES)&{entry for entry,_ in SEMANTIC_REPLACEMENTS})
        require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,*((entry,'awol_adapter') for entry in AWOL_RECORDS_ENTRIES)))


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
        self.assertEqual(sum(len(records) for _,_,records in self.groups),1591)
        self.assertEqual(sum(len(audit.CONTRACTS) for _,audit,_ in self.groups),13)
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
        for name in ('awol_records_lifecycle.cpp','awol_records_load.cpp','awol_records_save.cpp','awol_records_queries.cpp','awol_records_notice.cpp','awol_sampling.cpp'):
            source=(ROOT/'native'/name).read_text()
            for token in ('_reference(', 'dispatch_page(', 'run_until(', 'pcode', 'i_101'):
                self.assertNotIn(token,source)


if __name__=='__main__':unittest.main()
