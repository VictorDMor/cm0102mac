"""Certify INTERNATIONAL_AWARDS boundaries and default C++ production ownership.

Original listings remain private. Missing original input fails certification.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import international_awards_audit as audit
from international_awards_audit import (
    MODULE, ROUTINES, INCLUDES, rewritten, require_international_awards_index,
)
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/current/export/instructions.sqlite'
ENTRIES = (0x10440620, 0x104406d0, 0x104409c0)
COUNTS = (41, 186, 170)
DIGESTS = ('c59fc850c2cfde96', '1084fe55ad2739a3', '4024b029b4feed35')
VECTORS = {0x108b9704: 0x104409c0, 0x108b970c: 0x104406d0}
PADDING = {0x104406c4, 0x104406c8, 0x104406cc, 0x104409b8,
           0x104409bc, 0x10440c68, 0x10440c6c}


def require_export(path):
    if not path.is_file():
        raise RuntimeError('Private INTERNATIONAL_AWARDS export unavailable; inventory cannot be certified')


class InternationalAwardsAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(INPUT)
        cls.program = json.loads((INPUT.parent / 'program.json').read_text())
        cls.segments = [(s['address'], (INPUT.parent / s['file']).read_bytes())
                        for s in cls.program['segments'] if 'file' in s]
        with closing(sqlite3.connect(INPUT)) as db:
            cls.records = routine_records(db, *MODULE)
        cls.by_address = {r['address']: r for r in cls.records}

    def original_bytes(self, address, size):
        for start, contents in self.segments:
            if start <= address and address + size <= start + len(contents):
                return contents[address - start:address - start + size]
        self.fail(f'Original bytes unavailable at {address:x}')

    def word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), 'big')

    def assert_production_inventory(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(ROUTINES))
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in owners if at == entry],
                             ['international_awards_adapter'])

    def test_complete_inventory_counts_digests_and_production_ownership(self):
        self.assertEqual(MODULE, (0x10440620, 0x10440c70))
        self.assertEqual(ROUTINES, tuple(zip(ENTRIES, ENTRIES[1:] + (MODULE[1],))))
        self.assertEqual(len(self.records), 397)
        self.assertEqual(tuple(sum(a <= r['address'] < b for r in self.records)
                               for a, b in ROUTINES), COUNTS)
        self.assertEqual(tuple(d for _, _, d in rewritten()), DIGESTS)
        from compile_recomp import OWNER_REPLACEMENTS
        self.assert_production_inventory(rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(INPUT)) as db:
            require_international_awards_index(db)

    def test_source_pools_and_distinct_vtables_close_neighbor_boundaries(self):
        toc = 0x108bba90
        for slot, address, source in (
                (-0x4a48, 0x1082b978, b'HOLLAND_AWARDS.CPP\0'),
                (-0x4a44, 0x1082b9a0, b'INTERNATIONAL_AWARDS.CPP\0'),
                (-0x4a40, 0x1082b9cc, b'ITALY_AWARDS.CPP\0')):
            self.assertEqual(self.word(toc + slot), address)
            self.assertEqual(self.original_bytes(address, len(source)), source)
        for slot, table, vectors in (
                (-0x61e4, 0x108cf470, (0x108b96fc, 0x108b96f4)),
                (-0x61e0, 0x108cf48c, (0x108b970c, 0x108b9704)),
                (-0x61dc, 0x108cf4a8, (0x108b9724, 0x108b971c))):
            self.assertEqual(self.word(toc + slot), table)
            self.assertEqual([self.word(table + i * 4) for i in range(4)], [0, 0, *vectors])
        self.assertEqual([self.word(0x108cf48c + i * 4) for i in range(4, 7)],
                         [0x108b978c, 0x108b95c4, 0x108b860c])
        expected = {
            0x1043ff20: 'lwz r25,-0x4a48(r2)', 0x10440614: 'blr',
            0x10440620: 'mfspr r0,LR', 0x10440640: 'lwz r6,-0x61e0(r2)',
            0x104406e0: 'lwz r27,-0x4a44(r2)', 0x10440c64: 'blr',
            0x10440c70: 'mfspr r0,LR', 0x10440c90: 'lwz r6,-0x61dc(r2)',
            0x10440d68: 'lwz r19,-0x4a40(r2)',
        }
        with closing(sqlite3.connect(INPUT)) as db:
            for address, assembly in expected.items():
                record = json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                                (address,)).fetchone()[0])
                self.assertEqual(record['assembly'], assembly)
            for slot, address in ((-0x4a44, 0x104406e0), (-0x61e0, 0x10440640)):
                users = list(db.execute('SELECT address FROM instructions WHERE body LIKE ?',
                                        (f'%-0x{-slot:x}(r2)%',)))
                self.assertEqual(users, [(address,)])

    def test_whole_export_incoming_flows_and_fallthroughs(self):
        incoming = []
        with closing(sqlite3.connect(INPUT)) as db:
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    'WHERE f.value>=? AND f.value<?', MODULE):
                if MODULE[0] <= address < MODULE[1]:
                    continue
                record = json.loads(body)
                for target in record['flows']:
                    if MODULE[0] <= target < MODULE[1]:
                        incoming.append((address, target, record['flow']))
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*MODULE, *MODULE)))
        self.assertEqual(incoming, [(0x10436620, 0x10440620, 'UNCONDITIONAL_CALL')])
        self.assertEqual(fallthroughs, [])

    def test_no_cross_entry_tail_or_computed_flows_or_symbols(self):
        for entry, end in ROUTINES:
            for record in self.records:
                if not entry <= record['address'] < end:
                    continue
                self.assertNotIn('COMPUTED', record['flow'])
                targets = record['flows'] + ([record['fallthrough']]
                                              if record.get('fallthrough') is not None else [])
                for target in targets:
                    if MODULE[0] <= target < MODULE[1]:
                        self.assertTrue(entry <= target < end,
                                        f'Unaccounted cross-entry flow at {record["address"]:x}')
        self.assertEqual([s for s in self.program['symbols']
                          if MODULE[0] <= s['address'] < MODULE[1]], [])

    def test_every_instruction_and_zero_padding_word_is_accounted(self):
        missing = set()
        for entry, end in ROUTINES:
            records = [r for r in self.records if entry <= r['address'] < end]
            self.assertEqual(records[0]['address'], entry)
            self.assertEqual(records[-1]['assembly'], 'blr')
            last = records[-1]['address'] + 4
            self.assertLess(end - last, 16)
            self.assertEqual([r['address'] for r in records], list(range(entry, last, 4)))
            for address in range(last, end, 4):
                missing.add(address)
                self.assertEqual(self.word(address), 0)
        self.assertEqual(missing, PADDING)
        for record in self.records:
            self.assertTrue(PADDING.isdisjoint(record['flows']))
            self.assertNotIn(record.get('fallthrough'), PADDING)

    def test_aligned_original_pointer_census_matches_exact_transition_vectors(self):
        pointers = {}
        for start, contents in self.segments:
            for match in re.finditer(b'\x10\x44', contents):
                offset = match.start()
                if (start + offset) % 4 or offset + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if MODULE[0] <= target < MODULE[1]:
                    pointers[start + offset] = target
        self.assertEqual(pointers, VECTORS)
        for vector, target in VECTORS.items():
            self.assertEqual(self.word(vector + 4), 0x108bba90)
            self.assertIn(target, ENTRIES)

    def test_constructor_virtual_call_routes_to_owned_initialization(self):
        virtuals = [r['address'] for r in self.records if r['assembly'] == 'bl 0x10781320']
        self.assertEqual(virtuals, [0x104406a4])
        self.assertEqual(self.by_address[0x1044069c]['assembly'], 'lwz r12,0x0(r3)')
        self.assertEqual(self.by_address[0x104406a0]['assembly'], 'lwz r12,0x8(r12)')
        self.assertEqual(self.by_address[0x104406a8]['assembly'], 'lwz r2,0x14(r1)')
        self.assertEqual(self.word(self.word(0x108cf48c + 8)), ENTRIES[1])

    def test_audit_rejects_missing_export_and_stale_digest(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory) / 'missing.sqlite')
        stale = [(a, b, '0000000000000000') if index == 0 else (a, b, digest)
                 for index, (a, b, digest) in enumerate(rewritten(drafts=True))]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(audit, 'rewritten', return_value=stale):
            with self.assertRaisesRegex(ValueError, 'changed in the export'):
                require_international_awards_index(db)

    def test_audit_rejects_duplicate_bad_end_draft_and_missing_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = INCLUDES.read_text()
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            first = next(line for line in contents.splitlines() if 'INTERNATIONAL_AWARDS_ROUTINE(' in line)
            candidate.write_text(contents + '\n' + first + '\n')
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                rewritten(candidate)
            candidate.write_text(contents.replace('0x104406d0, R3', '0x104406d4, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                rewritten(candidate)
            candidate.write_text(contents.replace('INTERNATIONAL_AWARDS_ROUTINE(',
                                                   'INTERNATIONAL_AWARDS_DRAFT(', 1))
            self.assertEqual(len(rewritten(candidate, drafts=True)), len(ENTRIES))
            with self.assertRaises(AssertionError):
                self.assert_production_inventory(rewritten(candidate), OWNER_REPLACEMENTS)
        missing = [(entry, owner) for entry, owner in OWNER_REPLACEMENTS if entry != ENTRIES[0]]
        with self.assertRaises(AssertionError):
            self.assert_production_inventory(rewritten(), missing)

    def test_default_generation_guards_every_entry_and_reference_bypasses(self):
        from compile_recomp import generate_page
        selected = {r['address'] for r in self.records}
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected)
            for entry in ENTRIES:
                if entry >> 12 == page:
                    self.assertIn(f'if (try_modern_international_awards(s, 0x{entry:x})) return true;', normal)
            self.assertIn('#include "international_awards_adapter.hpp"', normal)
            reference = generate_page(page, part, selected, international_awards_hooks=False)
            self.assertNotIn('try_modern_international_awards', reference)


if __name__ == '__main__':
    unittest.main()
