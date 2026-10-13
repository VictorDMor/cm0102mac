"""Whole-body/page pins, destination hooks and generated parser dependencies."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import sqlite3
import unittest

from floating_input_audit import (CONTRACTS, FLOATING_INPUT_ENTRIES, INSTRUCTION_COUNTS,
    require_floating_input_contract, require_floating_input_bodies,
    require_floating_input_edges, require_floating_input_index, require_floating_input_hooks)
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page, operation

INDEX = Path(__file__).resolve().parent.parent / '.local/recomp-input/instructions.sqlite'


class FloatingInputManifestTests(unittest.TestCase):
    def test_one_entry_and_preserved_prior_manifest(self):
        self.assertEqual(FLOATING_INPUT_ENTRIES,(0x1077de10,))
        self.assertEqual(INSTRUCTION_COUNTS,(1025,))
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),668)
        self.assertEqual(len({a for a,_ in SEMANTIC_REPLACEMENTS}),668)
        self.assertEqual(SEMANTIC_REPLACEMENTS[99:100],((0x1077de10,'floating_input_adapter'),))
        prior=[{'entry':f'0x{entry:x}','module':module} for entry,module in SEMANTIC_REPLACEMENTS[:78]]
        self.assertEqual(hashlib.sha256(json.dumps(prior,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
            'fd8333c4a205239921b990433f4397bbab56c5acb0eb43ea0e584cf3de85219f')


@unittest.skipUnless(INDEX.exists(),'Private Mac export unavailable')
class FloatingInputAuditTests(unittest.TestCase):
    def records(self,page):
        with closing(sqlite3.connect(INDEX.resolve().as_uri()+'?mode=ro',uri=True)) as db:
            return [json.loads(r[0]) for r in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(page<<12,(page+1)<<12))]

    def test_full_bodies_page_crossing_and_destination_hooks(self):
        all_records=self.records(0x1077d)+self.records(0x1077e)
        require_floating_input_bodies(all_records)
        for (start,end,_),count in zip(CONTRACTS,INSTRUCTION_COUNTS):
            body=[r for r in all_records if start<=r['address']<end]
            self.assertEqual(len(body),count);self.assertEqual(len({r['address'] for r in body}),count)
            self.assertEqual(body[0]['address'],start);self.assertEqual(body[-1]['address'],end-4)
            self.assertEqual(body[-1]['assembly'],'blr')
            for page in range(start>>12,((end-1)>>12)+1):
                records=self.records(page);require_floating_input_contract(page,records)
                for local in (False,True):
                    code=generate_page(page,records,{r['address'] for r in records},local_temporaries=local)
                    if page==start>>12:
                        self.assertIn('#include "floating_input_adapter.hpp"',code)
                        self.assertIn(f'case 0x{start:x}: goto i_{start:x};',code)
                        fragment=code.split(f'i_{start:x}:',1)[1].split(f'i_{start+4:x}:',1)[0]
                        hook=f'if (try_modern_floating_input(s, 0x{start:x})) return true;'
                        self.assertEqual(fragment.count(hook),1)
                        with self.assertRaisesRegex(ValueError,'floating input hook'):
                            require_floating_input_hooks(page,records,code.replace(hook,''))
                        with self.assertRaisesRegex(ValueError,'floating input hook'):
                            require_floating_input_hooks(page,records,code+hook)
                        with self.assertRaisesRegex(ValueError,'floating input hook'):
                            require_floating_input_hooks(page,records,code.replace(hook,'').replace(f's.step(0x{start:x});',f's.step(0x{start:x});\n'+hook))
                        self.assertLess(fragment.index(hook),fragment.index(f's.step(0x{start:x});'))
                        if not local:self.assertLess(fragment.index(hook),fragment.index(operation(body[0]['ops'][0],start,0,page,{start},len(body[0]['ops']))))
            for kind in ('CALL','BRANCH'):
                self.assertEqual(operation([kind,None,['ram',str(start),4]],start-4,0,start>>12,{start},1),f'goto i_{start:x};')
                self.assertEqual(operation([kind,None,['ram',str(start),4]],start-4,0,(start>>12)+1,{start},1),f's.pc = 0x{start:x}; return true;')
            for kind in ('CALLIND','BRANCHIND'):
                self.assertEqual(operation([kind,None,['register','4200',4]],start-4,0,start>>12,{start},1),'s.pc = std::uint32_t(s.r<4>(4200)); return true;')

    def test_changed_partial_duplicate_and_missing_cross_page_reject(self):
        records=self.records(0x1077d)+self.records(0x1077e)
        for start,end,_ in CONTRACTS:
            body=[r for r in records if start<=r['address']<end]
            for victim in (start,start+4,end-4):
                partial=[r for r in records if r['address']!=victim]
                with self.assertRaisesRegex(ValueError,'floating input contract changed'):require_floating_input_bodies(partial)
                page=victim>>12;fragment=[r for r in partial if r['address']>>12==page]
                with self.assertRaisesRegex(ValueError,'floating input contract changed'):generate_page(page,fragment,{r['address'] for r in fragment})
            for field,value in (('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1),('address',start-4)):
                changed=deepcopy(body);changed[0][field]=value
                with self.assertRaisesRegex(ValueError,'floating input contract changed'):require_floating_input_bodies(changed)
            duplicate=body+[deepcopy(body[0])]
            with self.assertRaisesRegex(ValueError,'floating input contract changed'):require_floating_input_bodies(duplicate)
            for page in range(start>>12,((end-1)>>12)+1):
                fragment=[r for r in body if r['address']>>12==page]
                require_floating_input_contract(page,fragment)
                if start>>12!=(end-1)>>12:
                    with self.assertRaisesRegex(ValueError,'floating input contract changed'):require_floating_input_bodies(fragment)
        require_floating_input_bodies([]);require_floating_input_contract(0x1077e,[])

    def test_all_indexed_edges_and_new_interior_edge_rejection(self):
        with closing(sqlite3.connect(INDEX.resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_floating_input_index(db)
        for start,end,_ in CONTRACTS:
            for target in (start+4,end-4):
                with self.assertRaisesRegex(ValueError,'External floating input interior edge'):
                    require_floating_input_edges([(start-4,target)])
            require_floating_input_edges([(start-4,start),(start,start+4)])
        with self.assertRaisesRegex(ValueError,'External floating input interior edge'):
            require_floating_input_edges([(0x1077a000,0x1077de14)])

    def test_entire_missing_entry_cannot_emit_a_full_manifest(self):
        records=self.records(0x1077d)+self.records(0x1077e)
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            for start,end,_ in CONTRACTS:
                db.execute('DELETE FROM instructions')
                db.executemany('INSERT INTO instructions VALUES (?,?)',
                    ((r['address'],json.dumps(r)) for r in records if not start<=r['address']<end))
                with self.assertRaisesRegex(ValueError,'Missing complete Mac floating input body'):
                    require_floating_input_index(db)



if __name__=='__main__':unittest.main()
