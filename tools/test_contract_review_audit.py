"""Complete original contracts and prepared ownership for contract review."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import unittest
from unittest.mock import patch
from compile_recomp import SEMANTIC_REPLACEMENTS, require_unique_semantic_replacements, generate_page
from contract_review_audit import (CONTRACTS, FRAGMENTS, CONTRACT_REVIEW_ENTRIES,
    TABLES, load_contract_review_tables, require_contract_review_tables, require_contract_review_bodies, require_contract_review_contract,
    require_contract_review_edges, require_contract_review_index, require_contract_review_hooks, contract_review_hook)
from make_contract_review_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_exact_accepted_prefix_and_complete_candidate_owner(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),815)
        self.assertEqual(len(CONTRACT_REVIEW_ENTRIES),13)
        prefix=[{'entry':hex(entry),'module':owner} for entry,owner in SEMANTIC_REPLACEMENTS[:783]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'74c489e7883513407ccee6e70f8a73e2b6351213e037ed6c7eba629fbcbb95ed')
        self.assertEqual(SEMANTIC_REPLACEMENTS[783:796],tuple((entry,'contract_review_adapter') for entry in CONTRACT_REVIEW_ENTRIES))
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry in CONTRACT_REVIEW_ENTRIES:
            with self.subTest(entry=hex(entry)),self.assertRaisesRegex(ValueError,'Duplicate semantic replacement'):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(entry,'other_owner')))

    def test_complete_boundaries_and_external_interior_edges(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS), 1447)
        self.assertEqual(sum(row[2] for row in FRAGMENTS), 1447)
        for start, end, *_ in CONTRACTS:
            require_contract_review_edges([(0, start), (start, end-4), (end-4, end)])
            for target in (start+4, end-4):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_contract_review_edges([(0, target)])

    def test_named_operations_do_not_replay_original_bodies(self):
        source='\n'.join((ROOT/'native'/name).read_text() for name in ['contract_review.cpp','contract_review_scoring.cpp','contract_review_consequences.cpp','contract_review_presentation.cpp'])
        for forbidden in ('contract_review_reference','dispatch_page','run_until','pcode','i_1037','i_1038'):
            self.assertNotIn(forbidden,source)
        for name in ('reset_review_state','has_review_trigger','refresh_review_conditions','apply_review_conditions','describe_squad_status','query_start_date_window'):
            self.assertIn(name,source)


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_contract_review_index(db)
            cls.records = [json.loads(row[0]) for start, end, *_ in CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        cls.userops = json.loads((INPUT / 'program.json').read_text())['userops']

    def test_original_tables_and_live_pointers_are_pinned(self):
        import struct
        program=json.loads((INPUT/'program.json').read_text())
        segments=[(s['address'],(INPUT/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
        def read(a,n):
            for base,data in segments:
                if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
            raise ValueError('Missing original table')
        require_contract_review_tables(read);load_contract_review_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [slot,*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_contract_review_tables(changed)

    def test_three_complete_sampler_normalization(self):
        starts=[0x1037fd1c,0x1037fe88,0x10380004]
        bodies=[]
        for number,start in enumerate(starts):
            rows=[r for r in self.records if start<=r['address']<start+0x148]
            self.assertEqual(len(rows),82)
            normalized=[]
            for row in rows:
                offset=row['address']-start
                asm=re.sub(r'0x[0-9a-f]+',lambda m:f'ROOT+{int(m.group(),16)-start}' if start<=int(m.group(),16)<=start+0x148 else m.group(),row['assembly'])
                if number==0 and offset in (0x50,0x60,0x70,0xdc,0xe0):
                    replacements={0x50:('0x4c(r1)','0x3c(r1)'),0x60:('0x48(r1)','0x38(r1)'),0x70:('0x48(r1)','0x38(r1)'),0xdc:('0x38(r1)','0x48(r1)'),0xe0:('0x3c(r1)','0x4c(r1)')}
                    old,new=replacements[offset];self.assertIn(old,asm);asm=asm.replace(old,new)
                if number==2 and offset in (0xe0,0x108,0x10c,0x114,0x13c,0x144):asm=re.sub(r'\br22\b','r20',asm)
                normalized.append(asm)
            bodies.append(normalized)
        self.assertEqual(bodies[0],bodies[1]);self.assertEqual(bodies[1],bodies[2])

    def test_complete_diagnostic_normalization(self):
        bodies=[]
        for start,table,buffer,line in [(0x1037f8d0,28,27,0x1e9b),(0x10380c34,31,30,0x20ec),(0x10380d7c,31,30,0x2125)]:
            rows=[r for r in self.records if start<=r['address']<start+0x48];self.assertEqual(len(rows),18)
            normalized=[]
            for row in rows:
                asm=row['assembly'];asm=re.sub(rf'\br{table}\b','TABLE',asm);asm=re.sub(rf'\br{buffer}\b','BUFFER',asm)
                asm=asm.replace(hex(line),'LINE');normalized.append(asm)
            bodies.append(normalized)
        self.assertEqual(bodies[0],bodies[1]);self.assertEqual(bodies[1],bodies[2])

    def test_changed_original_fields_are_rejected(self):
        for start, page, *_ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page and row['address'] >= start)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records); changed[index][key] = value
                with self.subTest(entry=hex(start), page=hex(page), key=key), self.assertRaises(ValueError):
                    require_contract_review_bodies(changed, complete=True)

    def test_missing_duplicate_reordered_and_absent_bodies_fail(self):
        for start, end, *_ in CONTRACTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] == start)
            for changed in (self.records[:index]+self.records[index+1:], self.records+[self.records[index]],
                            list(reversed(self.records)), [row for row in self.records if not start <= row['address'] < end]):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    render_reference(changed, self.userops)

    def test_partial_pages_fail(self):
        for page in sorted({row[1] for row in FRAGMENTS}):
            rows = [row for row in self.records if row['address'] >> 12 == page]
            selected = {row['address'] for row in self.records}
            require_contract_review_contract(page, rows, selected)
            for start, end, *_ in CONTRACTS:
                if start >> 12 <= page <= (end-1) >> 12:
                    with self.assertRaises(ValueError):
                        require_contract_review_contract(page, [row for row in rows if not start <= row['address'] < end], {start})

    def test_productive_hooks_precede_original_accounting(self):
        import inspect
        disabled = {name: False for name in inspect.signature(generate_page).parameters
                    if name.endswith('_hooks') or name == 'guest_context_transfers'}
        disabled['contract_review_hooks'] = True
        for page in sorted({r['address'] >> 12 for r in self.records}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            selected = {r['address'] for r in self.records}
            code = generate_page(page, part, selected, userops=self.userops, **disabled)
            require_contract_review_hooks(page, part, code)
            for entry in CONTRACT_REVIEW_ENTRIES:
                if entry >> 12 != page: continue
                hook = contract_review_hook(entry)
                step = f's.step(0x{entry:x});'
                for changed in (code.replace(hook, '', 1),
                                code.replace(hook, '', 1).replace(step, step+'\n'+hook, 1),
                                code.replace(hook, hook+'\n'+hook, 1)):
                    with self.subTest(entry=hex(entry)), self.assertRaises(ValueError):
                        require_contract_review_hooks(page, part, changed)

    def test_reference_retains_all_steps_without_semantic_hooks(self):
        source = render_reference(self.records, self.userops)
        expected = [f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', source, re.MULTILINE), expected)
        self.assertEqual(re.findall(r'contract_review_step_seen\(s, 0x([0-9a-f]+)\)', source), expected)
        self.assertNotIn('try_modern_', source)
        self.assertNotIn('complete_context_transfer', source)

    def test_reversed_contract_index_rejects_highest_interior_edge(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            db.executemany('INSERT INTO instructions VALUES (?,?)',
                           [(row['address'], json.dumps(row)) for row in self.records])
            edge = {'address': 0, 'flows': [CONTRACT_REVIEW_ENTRIES[-1]+4], 'fallthrough': None}
            db.execute('INSERT INTO instructions VALUES (?,?)', (0, json.dumps(edge)))
            with patch('contract_review_audit.CONTRACTS', tuple(reversed(CONTRACTS))):
                with self.assertRaisesRegex(ValueError, 'External contract-review interior edge'):
                    require_contract_review_index(db)


if __name__ == '__main__':
    unittest.main()
