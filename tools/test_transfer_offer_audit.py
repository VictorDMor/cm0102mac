"""Complete original contracts and prepared ownership for transfer offer."""
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
from transfer_offer_audit import (CONTRACTS, FRAGMENTS, TRANSFER_OFFER_ENTRIES,
    TABLES, load_transfer_offer_tables, require_transfer_offer_tables, require_transfer_offer_bodies, require_transfer_offer_contract,
    require_transfer_offer_edges, require_transfer_offer_index, require_transfer_offer_hooks, transfer_offer_hook)
from make_transfer_offer_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_exact_accepted_prefix_and_complete_candidate_owner(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),949)
        self.assertEqual(len(TRANSFER_OFFER_ENTRIES),19)
        prefix=[{'entry':hex(entry),'module':owner} for entry,owner in SEMANTIC_REPLACEMENTS[:796]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'743dfb24d871008bdef6aef5e8ce29a1a6b97239982f52d0c2a05947968229c9')
        self.assertEqual(SEMANTIC_REPLACEMENTS[796:815],tuple((entry,'transfer_offer_adapter') for entry in TRANSFER_OFFER_ENTRIES))
        require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry in TRANSFER_OFFER_ENTRIES:
            with self.subTest(entry=hex(entry)),self.assertRaisesRegex(ValueError,'Duplicate semantic replacement'):
                require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(entry,'other_owner')))

    def test_complete_boundaries_and_external_interior_edges(self):
        self.assertEqual(sum(row[2] for row in CONTRACTS), 1111)
        self.assertEqual(sum(row[2] for row in FRAGMENTS), 1111)
        for start, end, *_ in CONTRACTS:
            require_transfer_offer_edges([(0, start), (start, end-4), (end-4, end)])
            for target in (start+4, end-4):
                with self.subTest(entry=hex(start)), self.assertRaises(ValueError):
                    require_transfer_offer_edges([(0, target)])

    def test_named_operations_do_not_replay_original_bodies(self):
        source='\n'.join((ROOT/'native'/name).read_text() for name in ['transfer_offer_membership.cpp','transfer_offer_policy.cpp','transfer_offer_collections.cpp','transfer_offer_selection.cpp'])
        for forbidden in ('transfer_offer_reference','dispatch_page','run_until','pcode','i_103e','i_103f','page_103e','page_103f'):
            self.assertNotIn(forbidden,source)
        for name in ('append_staff_entry','rejection_reason_allowed','describe_rejection_reason','collect_staff_offer_indices','best_qualifying_offer','query_special_staff_offer'):
            self.assertIn(name,source)


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT / 'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_transfer_offer_index(db)
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
        require_transfer_offer_tables(read);load_transfer_offer_tables(INPUT)
        for slot,base,size,_ in TABLES:
            for address in [slot,*range(base,base+size)]:
                def changed(a,n):
                    b=bytearray(read(a,n))
                    if a<=address<a+n:b[address-a]^=1
                    return bytes(b)
                with self.subTest(address=hex(address)),self.assertRaises(ValueError):require_transfer_offer_tables(changed)

    def test_three_complete_diagnostic_blocks_normalize(self):
        bodies=[]
        for start,first,line in [(0x103e62cc,0x4c,0xbe6),(0x103e638c,0x40,0xc04),(0x103ea624,0x40,0x163c)]:
            rows=[r for r in self.records if start<=r['address']<start+0x48]
            self.assertEqual(len(rows),18)
            bodies.append([r['assembly'].replace(f'addi r6,r1,{hex(first)}','FIRST_SCRATCH').replace(hex(line),'LINE') for r in rows])
        self.assertEqual(bodies[0],bodies[1]);self.assertEqual(bodies[1],bodies[2])

    def test_changed_original_fields_are_rejected(self):
        for start, page, *_ in FRAGMENTS:
            index = next(n for n, row in enumerate(self.records) if row['address'] >> 12 == page and row['address'] >= start)
            for key, value in (('address', -1), ('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1)):
                changed = deepcopy(self.records); changed[index][key] = value
                with self.subTest(entry=hex(start), page=hex(page), key=key), self.assertRaises(ValueError):
                    require_transfer_offer_bodies(changed, complete=True)

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
            require_transfer_offer_contract(page, rows, selected)
            for start, end, *_ in CONTRACTS:
                if start >> 12 <= page <= (end-1) >> 12:
                    with self.assertRaises(ValueError):
                        require_transfer_offer_contract(page, [row for row in rows if not start <= row['address'] < end], {start})

    def test_productive_hooks_precede_original_accounting(self):
        import inspect
        disabled = {name: False for name in inspect.signature(generate_page).parameters
                    if name.endswith('_hooks') or name == 'guest_context_transfers'}
        disabled['transfer_offer_hooks'] = True
        for page in sorted({r['address'] >> 12 for r in self.records}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            selected = {r['address'] for r in self.records}
            code = generate_page(page, part, selected, userops=self.userops, **disabled)
            require_transfer_offer_hooks(page, part, code)
            for entry in TRANSFER_OFFER_ENTRIES:
                if entry >> 12 != page: continue
                hook = transfer_offer_hook(entry)
                step = f's.step(0x{entry:x});'
                for changed in (code.replace(hook, '', 1),
                                code.replace(hook, '', 1).replace(step, step+'\n'+hook, 1),
                                code.replace(hook, hook+'\n'+hook, 1)):
                    with self.subTest(entry=hex(entry)), self.assertRaises(ValueError):
                        require_transfer_offer_hooks(page, part, changed)

    def test_reference_retains_all_steps_without_semantic_hooks(self):
        source = render_reference(self.records, self.userops)
        expected = [f'{row["address"]:x}' for row in self.records]
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', source, re.MULTILINE), expected)
        self.assertEqual(re.findall(r'transfer_offer_step_seen\(s, 0x([0-9a-f]+)\)', source), expected)
        self.assertNotIn('try_modern_', source)
        self.assertNotIn('complete_context_transfer', source)

    def test_reversed_contract_index_rejects_highest_interior_edge(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            db.executemany('INSERT INTO instructions VALUES (?,?)',
                           [(row['address'], json.dumps(row)) for row in self.records])
            edge = {'address': 0, 'flows': [TRANSFER_OFFER_ENTRIES[-1]+4], 'fallthrough': None}
            db.execute('INSERT INTO instructions VALUES (?,?)', (0, json.dumps(edge)))
            with patch('transfer_offer_audit.CONTRACTS', tuple(reversed(CONTRACTS))):
                with self.assertRaisesRegex(ValueError, 'External transfer-offer interior edge'):
                    require_transfer_offer_index(db)


if __name__ == '__main__':
    unittest.main()
