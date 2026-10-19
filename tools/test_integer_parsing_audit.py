"""Whole-body/page pins, destination hooks and generated parser dependencies."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import sqlite3
import unittest

from integer_parsing_audit import (CONTRACTS, INTEGER_PARSING_ENTRIES, INSTRUCTION_COUNTS,
    require_integer_parsing_contract, require_integer_parsing_bodies,
    require_integer_parsing_edges, require_integer_parsing_index, require_integer_parsing_hooks)
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page, operation

INDEX = Path(__file__).resolve().parent.parent / '.local/recomp-input/instructions.sqlite'


class IntegerParsingManifestTests(unittest.TestCase):
    def test_five_entries_extend_exact_prior_66(self):
        self.assertEqual(INTEGER_PARSING_ENTRIES,(0x1077ee20,0x1077f180,0x1077f570,0x1077f670,0x1077f6a0))
        self.assertEqual(INSTRUCTION_COUNTS,(215,249,61,10,10))
        self.assertEqual(sum(INSTRUCTION_COUNTS),545)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),946)
        self.assertEqual(len({a for a,_ in SEMANTIC_REPLACEMENTS}),946)
        self.assertEqual(SEMANTIC_REPLACEMENTS[66:71],tuple((e,'integer_parsing_adapter') for e in INTEGER_PARSING_ENTRIES))
        prior=[{'entry':f'0x{entry:x}','module':module} for entry,module in SEMANTIC_REPLACEMENTS[:66]]
        self.assertEqual(hashlib.sha256(json.dumps(prior,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
            '186bf921ba5058f50ce04bd2fb5222f57d2c2045e72373b4dc46db08293705ab')


@unittest.skipUnless(INDEX.exists(),'Private Mac export unavailable')
class IntegerParsingAuditTests(unittest.TestCase):
    def records(self,page):
        with closing(sqlite3.connect(INDEX.resolve().as_uri()+'?mode=ro',uri=True)) as db:
            return [json.loads(r[0]) for r in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(page<<12,(page+1)<<12))]

    def test_full_bodies_page_crossing_and_destination_hooks(self):
        all_records=self.records(0x1077e)+self.records(0x1077f)
        require_integer_parsing_bodies(all_records)
        for (start,end,_),count in zip(CONTRACTS,INSTRUCTION_COUNTS):
            body=[r for r in all_records if start<=r['address']<end]
            self.assertEqual(len(body),count);self.assertEqual(len({r['address'] for r in body}),count)
            self.assertEqual(body[0]['address'],start);self.assertEqual(body[-1]['address'],end-4)
            self.assertEqual(body[-1]['assembly'],'blr')
            for page in range(start>>12,((end-1)>>12)+1):
                records=self.records(page);require_integer_parsing_contract(page,records)
                for local in (False,True):
                    code=generate_page(page,records,{r['address'] for r in records},local_temporaries=local)
                    if page==start>>12:
                        self.assertIn('#include "integer_parsing_adapter.hpp"',code)
                        self.assertIn(f'case 0x{start:x}: goto i_{start:x};',code)
                        fragment=code.split(f'i_{start:x}:',1)[1].split(f'i_{start+4:x}:',1)[0]
                        hook=f'if (try_modern_integer_parsing(s, 0x{start:x})) return true;'
                        self.assertEqual(fragment.count(hook),1)
                        with self.assertRaisesRegex(ValueError,'integer parsing hook'):
                            require_integer_parsing_hooks(page,records,code.replace(hook,''))
                        with self.assertRaisesRegex(ValueError,'integer parsing hook'):
                            require_integer_parsing_hooks(page,records,code+hook)
                        with self.assertRaisesRegex(ValueError,'integer parsing hook'):
                            require_integer_parsing_hooks(page,records,code.replace(hook,'').replace(f's.step(0x{start:x});',f's.step(0x{start:x});\n'+hook))
                        self.assertLess(fragment.index(hook),fragment.index(f's.step(0x{start:x});'))
                        if not local:self.assertLess(fragment.index(hook),fragment.index(operation(body[0]['ops'][0],start,0,page,{start},len(body[0]['ops']))))
            for kind in ('CALL','BRANCH'):
                self.assertEqual(operation([kind,None,['ram',str(start),4]],start-4,0,start>>12,{start},1),f'goto i_{start:x};')
                self.assertEqual(operation([kind,None,['ram',str(start),4]],start-4,0,(start>>12)+1,{start},1),f's.pc = 0x{start:x}; return true;')
            for kind in ('CALLIND','BRANCHIND'):
                self.assertEqual(operation([kind,None,['register','4200',4]],start-4,0,start>>12,{start},1),'s.pc = std::uint32_t(s.r<4>(4200)); return true;')

    def test_changed_partial_duplicate_and_missing_cross_page_reject(self):
        records=self.records(0x1077e)+self.records(0x1077f)
        for start,end,_ in CONTRACTS:
            body=[r for r in records if start<=r['address']<end]
            for victim in (start,start+4,end-4):
                partial=[r for r in records if r['address']!=victim]
                with self.assertRaisesRegex(ValueError,'integer parsing contract changed'):require_integer_parsing_bodies(partial)
                page=victim>>12;fragment=[r for r in partial if r['address']>>12==page]
                with self.assertRaisesRegex(ValueError,'integer parsing contract changed'):generate_page(page,fragment,{r['address'] for r in fragment})
            for field,value in (('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1),('address',start-4)):
                changed=deepcopy(body);changed[0][field]=value
                with self.assertRaisesRegex(ValueError,'integer parsing contract changed'):require_integer_parsing_bodies(changed)
            duplicate=body+[deepcopy(body[0])]
            with self.assertRaisesRegex(ValueError,'integer parsing contract changed'):require_integer_parsing_bodies(duplicate)
            for page in range(start>>12,((end-1)>>12)+1):
                fragment=[r for r in body if r['address']>>12==page]
                require_integer_parsing_contract(page,fragment)
                if start>>12!=(end-1)>>12:
                    with self.assertRaisesRegex(ValueError,'integer parsing contract changed'):require_integer_parsing_bodies(fragment)
        require_integer_parsing_bodies([]);require_integer_parsing_contract(0x1077f,[])

    def test_all_indexed_edges_and_new_interior_edge_rejection(self):
        with closing(sqlite3.connect(INDEX.resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_integer_parsing_index(db)
        for start,end,_ in CONTRACTS:
            for target in (start+4,end-4):
                with self.assertRaisesRegex(ValueError,'External integer parsing interior edge'):
                    require_integer_parsing_edges([(start-4,target)])
            require_integer_parsing_edges([(start-4,start),(start,start+4)])
        with self.assertRaisesRegex(ValueError,'External integer parsing interior edge'):
            require_integer_parsing_edges([(0x1077c254,0x1077f000)])

    def test_generated_scanner_callback_stub_division_and_guards_remain(self):
        pages={page:generate_page(page,self.records(page),{r['address'] for r in self.records(page)})
               for page in (0x10713,0x10773,0x1077c,0x1077d,0x1077e,0x1077f,0x10781,0x10205)}
        for source,target in ((0x1077c224,0x1077f180),(0x1077c254,0x1077ee20),
                              (0x1077c700,0x1077f180),(0x1077c730,0x1077ee20),(0x10205240,0x1077f6a0)):
            fragment=pages[source>>12].split(f'i_{source:x}:',1)[1].split(f'i_{source+4:x}:',1)[0]
            self.assertIn(f's.pc = 0x{target:x}; return true;',fragment)
        self.assertIn('s.pc = 0x1077f000; return true;',pages[0x1077e])
        self.assertIn('try_modern_compiler_arithmetic(s, 0x10773870)',pages[0x10773])
        for entry in (0x1077d960,0x1077d9e0,0x1077da70,0x10781320,0x1077de10):
            self.assertIn(f'i_{entry:x}:',pages[entry>>12]);self.assertNotIn(f'try_modern_integer_parsing(s, 0x{entry:x})',pages[entry>>12])
        for entry in (0x10713aec,0x10713b3c):
            fragment=pages[0x10713].split(f'i_{entry:x}:',1)[1].split(f'i_{entry+4:x}:',1)[0]
            self.assertIn('if (s.r<4>(12) == 0)',fragment)
        self.assertIn('s.pc = 0x10773870; return true;',pages[0x1077f])


if __name__=='__main__':unittest.main()
