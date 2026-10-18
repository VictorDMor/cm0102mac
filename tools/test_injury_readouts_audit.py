"""Complete injury status readouts contracts."""
from contextlib import closing
from copy import deepcopy
import hashlib
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import SEMANTIC_REPLACEMENTS,generate_page
from make_injury_readouts_reference import render_reference
from injury_readouts_audit import (CONTRACTS,FRAGMENTS,INJURY_READOUTS_ENTRIES,TABLES,
    require_injury_readouts_bodies,require_injury_readouts_contract,require_injury_readouts_edges,
    require_injury_readouts_hooks,require_injury_readouts_index,require_injury_readouts_tables,load_injury_readouts_tables)
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/compiler-arithmetic-reconstruction/input'

class ScopeTests(unittest.TestCase):
    def test_complete_block_and_exact_prior_manifest(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS),1656)
        self.assertEqual(sum(row[2] for row in FRAGMENTS),1656)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),735)
        self.assertEqual(len({entry for entry,_ in SEMANTIC_REPLACEMENTS}),735)
        self.assertEqual(INJURY_READOUTS_ENTRIES,(0x1018c1a0,0x1018c270,0x1018c2f0,0x1018c450,0x1018c560,0x1018d140,0x1018d950,0x1018dab0))
        self.assertEqual(SEMANTIC_REPLACEMENTS[714:722],tuple((entry,'injury_readouts_adapter') for entry in INJURY_READOUTS_ENTRIES))
        prior=[{'entry':f'0x{entry:x}','module':module} for entry,module in SEMANTIC_REPLACEMENTS[:714]]
        self.assertEqual(hashlib.sha256(json.dumps(prior,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'03953d1e94f7bbb8ee7e9048191d6992e9953f4e00451c5ba5cfff32644cc521')

    def test_external_interior_edges_fail(self):
        for entry,end,*_ in CONTRACTS:
            require_injury_readouts_edges([(0,entry),(entry,end-4),(end-4,end)])
            for target in (entry+4,end-4):
                with self.subTest(entry=hex(entry)),self.assertRaises(ValueError):require_injury_readouts_edges([(0,target)])

    def test_named_components_do_not_replay(self):
        source=''.join((ROOT/'native'/name).read_text() for name in ('injury_readouts.cpp','injury_readouts_labels.cpp','injury_readouts_forms.cpp','injury_readouts_detail.hpp','injury_readouts_choices.hpp'))
        for forbidden in ('injury_readouts_reference','dispatch_page','run_until','pcode','i_101'):
            self.assertNotIn(forbidden,source)
        for name in ('find_history_record','risk_multiplier','risk_type','describe_current_injury','describe_injury_type',
                     'injury_type_form','describe_remaining_time','describe_injury_category','write_label','choose_form','report_invalid'):
            self.assertIn(name,source)


@unittest.skipUnless((INPUT/'instructions.sqlite').exists(),'Private original source unavailable')
class ContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_injury_readouts_index(db)
            cls.records=[json.loads(row[0]) for start,end,*_ in CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(start,end))]
        cls.userops=json.loads((INPUT/'program.json').read_text())['userops']

    def test_six_original_choice_tables_are_pinned(self):
        import struct
        program=json.loads((INPUT/'program.json').read_text())
        segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
        def word(address):
            for base,data in segments:
                if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
            raise ValueError('Missing original table word')
        addresses=set()
        for slot,base,count,_ in TABLES:addresses.update((slot,*range(base,base+4*count,4)))
        live={address:word(address) for address in addresses}
        require_injury_readouts_tables(live.__getitem__)
        load_injury_readouts_tables(INPUT)
        for address in live:
            changed=dict(live);changed[address]^=1
            with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_injury_readouts_tables(changed.__getitem__)

    def test_semantic_choices_match_complete_original_handlers(self):
        source=(ROOT/'native/injury_readouts_choices.hpp').read_text()
        by={r['address']:r['assembly'] for r in self.records}
        for name,count,output,strings,end in [('type_labels',164,27,30,0x1018d12c),('category_labels',13,5,31,0x1018dc74)]:
            part=source.split(name+'{{',1)[1].split('}};',1)[0]
            cases=[(int(a,16),int(v,16)) for a,v in re.findall(r'\{0x([0-9a-f]+),0x([0-9a-f]+)\}',part)]
            self.assertEqual(len(cases),count);self.assertEqual(len({a for a,_ in cases}),count)
            for address,value in cases:
                self.assertEqual([by[address+4*n] for n in range(4)],[f'or r3,r{output},r{output}',f'addi r4,r{strings},0x{value:x}','bl 0x10727890',f'b 0x{end:x}'])
        part=source.split('type_forms{{',1)[1].split('}};',1)[0]
        cases=[(int(a,16),int(v,16)) for a,v in re.findall(r'\{0x([0-9a-f]+),0x([0-9a-f]+)\}',part)]
        self.assertEqual(len(cases),234);self.assertEqual(len({a for a,_ in cases}),234)
        for address,value in cases:self.assertEqual([by[address],by[address+4]],[f'li r3,0x{value:x}','b 0x1018d938'])

    def test_changed_source_fields_fail(self):
        for entry,page,*_ in FRAGMENTS:
            end=next(row[1] for row in CONTRACTS if row[0]==entry)
            index=next(n for n,row in enumerate(self.records) if entry<=row['address']<end and row['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed=deepcopy(self.records);changed[index][key]=value
                with self.subTest(entry=hex(entry),page=hex(page),key=key),self.assertRaises(ValueError):
                    require_injury_readouts_bodies(changed,complete=True)

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
        self.assertEqual(re.findall(r'injury_readouts_step_seen\(s, 0x([0-9a-f]+)\)',source),expected)
        for forbidden in ('try_modern_','complete_context_transfer'):
            self.assertNotIn(forbidden,source)

    def test_partial_pages_fail_and_complete_pages_pass(self):
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows=[row for row in self.records if row['address']>>12==page]
            selected={row['address'] for row in self.records}
            require_injury_readouts_contract(page,rows,selected)
            for entry,end,*_ in CONTRACTS:
                if entry>>12<=page<=(end-1)>>12:
                    body=[row for row in rows if entry<=row['address']<end]
                    with self.assertRaises(ValueError):
                        require_injury_readouts_contract(page,[row for row in rows if row not in body],{entry})

    def test_productive_hooks_precede_accounting(self):
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows=[row for row in self.records if row['address']>>12==page]
            disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks')}
            disabled['injury_readouts_hooks']=True
            code=generate_page(page,rows,{row['address'] for row in self.records},userops=self.userops,**disabled)
            require_injury_readouts_hooks(page,rows,code)
            for entry in INJURY_READOUTS_ENTRIES:
                if entry>>12!=page:continue
                hook=f'if (try_modern_injury_readouts(s, 0x{entry:x})) return true;'
                with self.assertRaises(ValueError):require_injury_readouts_hooks(page,rows,code.replace(hook,''))

if __name__=='__main__':unittest.main()
