"""Complete original contracts and prepared ownership for contract terms."""
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
from contract_terms_audit import (CONTRACTS, FRAGMENTS, CONTRACT_TERMS_ENTRIES,
    TABLES, load_contract_terms_tables, require_contract_terms_tables, require_contract_terms_bodies, require_contract_terms_contract,
    require_contract_terms_edges, require_contract_terms_index, require_contract_terms_hooks, contract_terms_hook)
from make_contract_terms_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_exact_accepted_prefix_and_complete_candidate_owner(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 857)
        self.assertEqual(len(CONTRACT_TERMS_ENTRIES),21)
        prefix = [{'entry':hex(entry),'module':owner} for entry,owner in SEMANTIC_REPLACEMENTS[:762]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'1fe78aed88e787d03784f0fdc001fab69bd8c7b431be974224cbb24acc4ea828')
        self.assertEqual(SEMANTIC_REPLACEMENTS[762:783],tuple((entry,'contract_terms_adapter') for entry in CONTRACT_TERMS_ENTRIES))
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry in CONTRACT_TERMS_ENTRIES:
            with self.subTest(entry=hex(entry)), self.assertRaisesRegex(ValueError,'Duplicate semantic replacement'):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(entry,'other_owner')))

    def test_complete_boundaries_and_external_interior_edges(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS), 1203)
        self.assertEqual(sum(row[2] for row in FRAGMENTS), 1203)
        for start, end, *_ in CONTRACTS:
            require_contract_terms_edges([(0, start), (start, end-4), (end-4, end)])
            for target in (start+4, end-4):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_contract_terms_edges([(0, target)])

    def test_named_operations_do_not_replay_original_bodies(self):
        source='\n'.join((ROOT/'native'/name).read_text() for name in ['contract_terms.cpp','range_sampling_detail.hpp','contract_terms_sampling.cpp','contract_terms_presentation.cpp','contract_terms_expiry.cpp'])
        for forbidden in ('contract_terms_reference','dispatch_page','run_until','pcode','i_1037','i_1038'):
            self.assertNotIn(forbidden,source)
        for name in ('sample_contract_range','enable_option','reset_terms','supports_option','describe_bonus','expiry_band'):
            self.assertIn(name,source)


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_contract_terms_index(db)
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
        require_contract_terms_tables(read);load_contract_terms_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [slot,*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_contract_terms_tables(changed)

    def test_original_sampler_normalization(self):
        from awol_records_audit import require_awol_records_bodies
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:
            prior=[json.loads(r[0]) for r in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',(0x10006080,0x100065f8))]
        require_awol_records_bodies(prior)
        from awol_records_audit import load_awol_records_tables
        import struct
        old=load_awol_records_tables(INPUT)[1]
        old_bytes=b''.join(struct.pack('>I',w) for w in old)
        new_bytes=load_contract_terms_tables(INPUT)[1]
        self.assertEqual(old_bytes[:8],new_bytes[:8])
        self.assertEqual(old_bytes[8:16],new_bytes[0xc0:0xc8])
        def normalized(body,start):
            result=[]
            for row in body:
                asm=re.sub(r'0x[0-9a-f]+',lambda m:f'ROOT+{int(m.group(),16)-start}' if start<=int(m.group(),16)<start+0x578 else m.group(),row['assembly'])
                if start==0x10374570:asm=asm.replace('-0x4dac(r2)','-0x5a0c(r2)')
                if asm.startswith('lfd ') and '(r25)' in asm:asm=asm.replace('0xc0(r25)','0x8(r25)')
                result.append(asm)
            return result
        self.assertEqual(len(prior),350)
        self.assertEqual(normalized(prior,0x10006080),normalized(self.records[:350],0x10374570))

    def test_periodic_adjustment_normalization(self):
        bodies=[]
        for start,service in [(0x10380560,0x1000a060),(0x10380630,0x1000a170)]:
            rows=[r for r in self.records if start<=r['address']<start+0xd0]
            self.assertEqual(len(rows),52)
            bodies.append([re.sub(r'0x[0-9a-f]+',lambda m:'SERVICE' if int(m.group(),16)==service else f'ROOT+{int(m.group(),16)-start}' if start<=int(m.group(),16)<start+0xd0 else m.group(),r['assembly']) for r in rows])
        self.assertEqual(*bodies)

    def test_changed_original_fields_are_rejected(self):
        for start, page, *_ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page and row['address'] >= start)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records); changed[index][key] = value
                with self.subTest(entry=hex(start), page=hex(page), key=key), self.assertRaises(ValueError):
                    require_contract_terms_bodies(changed, complete=True)

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
            require_contract_terms_contract(page, rows, selected)
            for start, end, *_ in CONTRACTS:
                if start >> 12 <= page <= (end-1) >> 12:
                    with self.assertRaises(ValueError):
                        require_contract_terms_contract(page, [row for row in rows if not start <= row['address'] < end], {start})

    def test_productive_hooks_precede_original_accounting(self):
        import inspect
        disabled = {name: False for name in inspect.signature(generate_page).parameters
                    if name.endswith('_hooks') or name == 'guest_context_transfers'}
        disabled['contract_terms_hooks'] = True
        for page in sorted({r['address'] >> 12 for r in self.records}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            selected = {r['address'] for r in self.records}
            code = generate_page(page, part, selected, userops=self.userops, **disabled)
            require_contract_terms_hooks(page, part, code)
            for entry in CONTRACT_TERMS_ENTRIES:
                if entry >> 12 != page: continue
                hook = contract_terms_hook(entry)
                step = f's.step(0x{entry:x});'
                for changed in (code.replace(hook, '', 1),
                                code.replace(hook, '', 1).replace(step, step+'\n'+hook, 1),
                                code.replace(hook, hook+'\n'+hook, 1)):
                    with self.subTest(entry=hex(entry)), self.assertRaises(ValueError):
                        require_contract_terms_hooks(page, part, changed)

    def test_reference_retains_all_steps_without_semantic_hooks(self):
        source = render_reference(self.records, self.userops)
        expected = [f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', source, re.MULTILINE), expected)
        self.assertEqual(re.findall(r'contract_terms_step_seen\(s, 0x([0-9a-f]+)\)', source), expected)
        self.assertNotIn('try_modern_', source)
        self.assertNotIn('complete_context_transfer', source)

    def test_reversed_contract_index_rejects_highest_interior_edge(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            db.executemany('INSERT INTO instructions VALUES (?,?)',
                           [(row['address'], json.dumps(row)) for row in self.records])
            edge = {'address': 0, 'flows': [CONTRACT_TERMS_ENTRIES[-1]+4], 'fallthrough': None}
            db.execute('INSERT INTO instructions VALUES (?,?)', (0, json.dumps(edge)))
            with patch('contract_terms_audit.CONTRACTS', tuple(reversed(CONTRACTS))):
                with self.assertRaisesRegex(ValueError, 'External contract-terms interior edge'):
                    require_contract_terms_index(db)


if __name__ == '__main__':
    unittest.main()
