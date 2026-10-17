"""Whole-body/page pins, destination hooks and generated parser dependencies."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import sqlite3
import unittest

from formatted_output_audit import (CONTRACTS, FORMATTED_OUTPUT_ENTRIES, INSTRUCTION_COUNTS, SHARED_EDGE,
    require_formatted_output_contract, require_formatted_output_bodies,
    require_formatted_output_edges, require_formatted_output_index, require_formatted_output_hooks)
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page, operation

INDEX = Path(__file__).resolve().parent.parent / '.local/recomp-input/instructions.sqlite'


class FormattedOutputManifestTests(unittest.TestCase):
    def test_six_entries_and_preserved_prior_manifest(self):
        self.assertEqual(len(FORMATTED_OUTPUT_ENTRIES),15)
        self.assertEqual(sum(INSTRUCTION_COUNTS),2649)
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),700)
        self.assertEqual(len({a for a,_ in SEMANTIC_REPLACEMENTS}),700)
        self.assertEqual(SEMANTIC_REPLACEMENTS[78:93],tuple((e,'formatted_output_adapter') for e in FORMATTED_OUTPUT_ENTRIES))
        prior=[{'entry':f'0x{entry:x}','module':module} for entry,module in SEMANTIC_REPLACEMENTS[:78]]
        self.assertEqual(hashlib.sha256(json.dumps(prior,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
            'fd8333c4a205239921b990433f4397bbab56c5acb0eb43ea0e584cf3de85219f')


@unittest.skipUnless(INDEX.exists(),'Private Mac export unavailable')
class FormattedOutputAuditTests(unittest.TestCase):
    def records(self,page):
        with closing(sqlite3.connect(INDEX.resolve().as_uri()+'?mode=ro',uri=True)) as db:
            return [json.loads(r[0]) for r in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(page<<12,(page+1)<<12))]

    def test_full_bodies_page_crossing_and_destination_hooks(self):
        all_records=sum((self.records(p) for p in (0x10778,0x10779,0x1077a,0x1077b)),[])
        require_formatted_output_bodies(all_records)
        for (start,end,_),count in zip(CONTRACTS,INSTRUCTION_COUNTS):
            body=[r for r in all_records if start<=r['address']<end]
            self.assertEqual(len(body),count);self.assertEqual(len({r['address'] for r in body}),count)
            self.assertEqual(body[0]['address'],start);self.assertEqual(body[-1]['address'],end-4)
            self.assertEqual(body[-1]['assembly'],'blr')
            for page in range(start>>12,((end-1)>>12)+1):
                records=self.records(page);require_formatted_output_contract(page,records)
                for local in (False,True):
                    code=generate_page(page,records,{r['address'] for r in records},local_temporaries=local)
                    if page==start>>12:
                        self.assertIn('#include "formatted_output_adapter.hpp"',code)
                        self.assertIn(f'case 0x{start:x}: goto i_{start:x};',code)
                        fragment=code.split(f'i_{start:x}:',1)[1].split(f'i_{start+4:x}:',1)[0]
                        hook=f'if (try_modern_formatted_output(s, 0x{start:x})) return true;'
                        self.assertEqual(fragment.count(hook),1)
                        with self.assertRaisesRegex(ValueError,'formatted output hook'):
                            require_formatted_output_hooks(page,records,code.replace(hook,''))
                        with self.assertRaisesRegex(ValueError,'formatted output hook'):
                            require_formatted_output_hooks(page,records,code+hook)
                        with self.assertRaisesRegex(ValueError,'formatted output hook'):
                            require_formatted_output_hooks(page,records,code.replace(hook,'').replace(f's.step(0x{start:x});',f's.step(0x{start:x});\n'+hook))
                        self.assertLess(fragment.index(hook),fragment.index(f's.step(0x{start:x});'))
                        if not local:self.assertLess(fragment.index(hook),fragment.index(operation(body[0]['ops'][0],start,0,page,{start},len(body[0]['ops']))))
            for kind in ('CALL','BRANCH'):
                self.assertEqual(operation([kind,None,['ram',str(start),4]],start-4,0,start>>12,{start},1),f'goto i_{start:x};')
                self.assertEqual(operation([kind,None,['ram',str(start),4]],start-4,0,(start>>12)+1,{start},1),f's.pc = 0x{start:x}; return true;')
            for kind in ('CALLIND','BRANCHIND'):
                self.assertEqual(operation([kind,None,['register','4200',4]],start-4,0,start>>12,{start},1),'s.pc = std::uint32_t(s.r<4>(4200)); return true;')

    def test_changed_partial_duplicate_and_missing_cross_page_reject(self):
        records=sum((self.records(p) for p in (0x10778,0x10779,0x1077a,0x1077b)),[])
        for start,end,_ in CONTRACTS:
            body=[r for r in records if start<=r['address']<end]
            for victim in (start,start+4,end-4):
                partial=[r for r in records if r['address']!=victim]
                with self.assertRaisesRegex(ValueError,'formatted output contract changed'):require_formatted_output_bodies(partial)
                page=victim>>12;fragment=[r for r in partial if r['address']>>12==page]
                with self.assertRaisesRegex(ValueError,'formatted output contract changed'):generate_page(page,fragment,{r['address'] for r in fragment})
            for field,value in (('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1),('address',start-4)):
                changed=deepcopy(body);changed[0][field]=value
                with self.assertRaisesRegex(ValueError,'formatted output contract changed'):require_formatted_output_bodies(changed)
            duplicate=body+[deepcopy(body[0])]
            with self.assertRaisesRegex(ValueError,'formatted output contract changed'):require_formatted_output_bodies(duplicate)
            for page in range(start>>12,((end-1)>>12)+1):
                fragment=[r for r in body if r['address']>>12==page]
                require_formatted_output_contract(page,fragment)
                if start>>12!=(end-1)>>12:
                    with self.assertRaisesRegex(ValueError,'formatted output contract changed'):require_formatted_output_bodies(fragment)
        require_formatted_output_bodies([]);require_formatted_output_contract(0x10779,[])

    def test_all_indexed_edges_and_new_interior_edge_rejection(self):
        with closing(sqlite3.connect(INDEX.resolve().as_uri()+'?mode=ro',uri=True)) as db:
            require_formatted_output_index(db)
        for start,end,_ in CONTRACTS:
            for target in (start+4,end-4):
                with self.assertRaisesRegex(ValueError,'External formatted output interior edge'):
                    require_formatted_output_edges([(start-4,target)])
            require_formatted_output_edges([(start-4,start),(start,start+4)])
        with self.assertRaisesRegex(ValueError,'External formatted output interior edge'):
            require_formatted_output_edges([(0x1077de10,0x1077a004)])

    def test_shared_reset_edge_and_hook(self):
        require_formatted_output_edges([SHARED_EDGE],require_shared=True)
        with self.assertRaisesRegex(ValueError,'Missing shared'):
            require_formatted_output_edges([],require_shared=True)
        with self.assertRaisesRegex(ValueError,'External formatted output interior edge'):
            require_formatted_output_edges([(SHARED_EDGE[0],SHARED_EDGE[1]+4)],require_shared=True)
        records=self.records(0x10779)
        code=generate_page(0x10779,records,{r['address'] for r in records})
        hook='if (try_modern_formatted_output_shared_tail(s)) return true;'
        self.assertEqual(code.count(hook),1)
        fragment=code.split('i_10779ed8:',1)[1].split('i_10779edc:',1)[0]
        self.assertLess(fragment.index(hook),fragment.index('s.step(0x10779ed8);'))
        with self.assertRaisesRegex(ValueError,'formatted output hook'):
            require_formatted_output_hooks(0x10779,records,code.replace(hook,''))

    def test_entire_missing_entry_cannot_emit_a_full_manifest(self):
        records=sum((self.records(p) for p in (0x10778,0x10779,0x1077a,0x1077b)),[])
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            for start,end,_ in CONTRACTS:
                db.execute('DELETE FROM instructions')
                db.executemany('INSERT INTO instructions VALUES (?,?)',
                    ((r['address'],json.dumps(r)) for r in records if not start<=r['address']<end))
                with self.assertRaisesRegex(ValueError,'Missing complete Mac formatted output body'):
                    require_formatted_output_index(db)



if __name__=='__main__':unittest.main()
