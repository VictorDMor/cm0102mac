"""Pin NATION_AWARDS boundaries, callbacks and complete production ownership.

Original listings stay private; this file records only compact inventory facts.
Missing private inputs fail certification instead of silently skipping it.
"""
from contextlib import closing
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import nation_awards_audit as audit
from nation_awards_audit import MODULE, ROUTINES, INCLUDES, rewritten, require_nation_awards_index
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/current/export/instructions.sqlite'
ENTRIES = (
    0x10444390, 0x104443b0, 0x10444490, 0x10444550, 0x10444580,
    0x104449c0, 0x10444c40, 0x10444e80, 0x104450c0, 0x104455b0,
    0x104459a0, 0x10446040, 0x104462b0, 0x10446510, 0x10446b00,
    0x10446c00, 0x10446c10, 0x10446d20, 0x10446d30, 0x10446d60,
)
COUNTS = (7, 52, 48, 11, 272, 159, 144, 140, 313, 251,
          423, 154, 150, 379, 64, 1, 68, 4, 9, 9)
VECTORS = {
    0x108b976c: 0x10446c00, 0x108b9774: 0x10446d20,
    0x108b977c: 0x10446d30, 0x108b9784: 0x10446d60,
    0x108b978c: 0x10444550, 0x108b9794: 0x10444490,
    0x108b979c: 0x104443b0,
}
GAPS = {0x104443ec, 0x10444fac, 0x104452ac, 0x10445750, 0x10445e0c, 0x1044649c}


def require_export(path):
    if not path.is_file():
        raise RuntimeError('Private NATION_AWARDS export unavailable; inventory cannot be certified')


class NationAwardsAuditTests(unittest.TestCase):
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
            self.assertEqual([owner for at, owner in owners if at == entry], ['nation_awards_adapter'])

    def test_complete_inventory_digests_counts_and_production_ownership(self):
        self.assertEqual(MODULE, (0x10444390, 0x10446d90))
        self.assertEqual(ROUTINES, tuple(zip(ENTRIES, ENTRIES[1:] + (MODULE[1],))))
        self.assertEqual(len(ROUTINES), 20)
        self.assertEqual(len(self.records), 2658)
        self.assertEqual(tuple(sum(a <= r['address'] < b for r in self.records)
                               for a, b in ROUTINES), COUNTS)
        from compile_recomp import OWNER_REPLACEMENTS
        self.assert_production_inventory(rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(INPUT)) as db:
            require_nation_awards_index(db)

    def test_source_numeric_pools_and_vtable_close_neighbor_boundaries(self):
        toc = 0x108bba90
        for slot, address, source in (
                (-0x4a30, 0x1082c04c, b'MONTH_RATINGS.CPP\0'),
                (-0x4a18, 0x1082c39c, b'NATION_AWARDS.CPP\0'),
                (-0x4a14, 0x1082c400, b'NORWAY_AWARDS.CPP\0')):
            self.assertEqual(self.word(toc + slot), address)
            self.assertEqual(self.original_bytes(address, len(source)), source)
        self.assertEqual(self.word(toc - 0x4a34), 0x1082c394)
        self.assertEqual(self.word(toc - 0x4a20), 0x1082c3c4)
        self.assertEqual(self.word(toc - 0x4a1c), 0x1082c3e0)
        self.assertEqual(self.original_bytes(0x1082c3c4, 28).hex(),
                         '3f28f5c33dcccccd404000003c23d70a40d000000000000041c80000')
        self.assertEqual(self.original_bytes(0x1082c3e0, 32).hex(),
                         '43300000800000003fb999999999999a00000000000000004330000000000000')
        self.assertEqual(self.word(toc - 0x61cc), 0x108cf4f8)
        self.assertEqual(self.word(toc - 0x61c8), 0x108cf514)
        # Source-independent leading methods share the base vtable; they
        # belong to NATION_AWARDS rather than the preceding MONTH_RATINGS.
        self.assertEqual([self.word(0x108cf4f8 + i * 4) for i in range(7)],
                         [0, 0, 0x108b976c, 0x108b9794, 0x108b978c, 0x108b95c4, 0x108b860c])
        with closing(sqlite3.connect(INPUT)) as db:
            def at(address):
                return json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                             (address,)).fetchone()[0])['assembly']
            self.assertEqual(at(0x10444324), 'lwz r7,-0x4a34(r2)')
            self.assertEqual(at(0x1044438c), 'blr')
            self.assertEqual(at(MODULE[0]), 'lwz r4,-0x61cc(r2)')
            self.assertEqual(at(0x104443d8), 'lwz r0,-0x61cc(r2)')
            self.assertEqual(at(MODULE[1]), 'mfspr r0,LR')
            self.assertEqual(at(0x10446d98), 'lwz r31,-0x4a14(r2)')
            for slot, count in ((-0x4a18, 16), (-0x4a20, 5), (-0x4a1c, 10)):
                users = [address for address, _ in db.execute(
                    'SELECT address,body FROM instructions WHERE body LIKE ?',
                    (f'%-0x{-slot:x}(r2)%',))]
                self.assertEqual(len(users), count)
                self.assertTrue(all(MODULE[0] <= address < MODULE[1] for address in users))

    def test_whole_export_incoming_flows_have_no_interior_entries(self):
        incoming = []
        with closing(sqlite3.connect(INPUT)) as db:
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    'WHERE f.value>=? AND f.value<?', MODULE):
                if MODULE[0] <= address < MODULE[1]:
                    continue
                record = json.loads(body)
                self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                for target in record['flows']:
                    if MODULE[0] <= target < MODULE[1]:
                        self.assertIn(target, ENTRIES)
                        incoming.append([address, target, record['flow']])
        self.assertEqual(len(incoming), 455)
        signature = hashlib.sha256(json.dumps(sorted(incoming), separators=(',', ':')).encode()).hexdigest()
        self.assertEqual(signature, '8fc1625cd8e4892203a292174ffd5de90e741e1e2ff3c5f91da79ec4e351000b')

    def test_no_cross_entry_calls_tail_jumps_or_computed_jumps(self):
        for entry, end in ROUTINES:
            for record in self.records:
                if not entry <= record['address'] < end:
                    continue
                self.assertNotIn('COMPUTED', record['flow'])
                for target in record['flows']:
                    if MODULE[0] <= target < MODULE[1]:
                        self.assertTrue(entry <= target < end,
                                        f'Unaccounted cross-entry flow at {record["address"]:x}')
        self.assertEqual([s for s in self.program['symbols'] if MODULE[0] <= s['address'] < MODULE[1]], [])

    def test_all_instructions_gaps_and_alignment_words_accounted(self):
        missing = set()
        incoming = {target for r in self.records for target in r['flows']}
        for entry, end in ROUTINES:
            records = [r for r in self.records if entry <= r['address'] < end]
            self.assertEqual(records[0]['address'], entry)
            self.assertEqual(records[-1]['assembly'], 'blr')
            last = records[-1]['address'] + 4
            self.assertLess(end - last, 16)
            self.assertEqual(self.original_bytes(last, end - last), bytes(end - last))
            for address in range(entry, last, 4):
                if address in self.by_address:
                    continue
                missing.add(address)
                self.assertEqual(self.word(address), 0x60000000)
                predecessor = self.by_address[address - 4]
                self.assertEqual(predecessor['flow'], 'UNCONDITIONAL_JUMP')
                self.assertIsNone(predecessor.get('fallthrough'))
                self.assertNotIn(address, incoming)
        self.assertEqual(missing, GAPS)

    def test_all_byte_pointer_scan_accounts_for_vectors_and_instruction_false_positives(self):
        pointers = {}
        for start, contents in self.segments:
            for match in re.finditer(b'\x10\x44', contents):
                offset = match.start()
                if offset + 4 <= len(contents):
                    target = int.from_bytes(contents[offset:offset + 4], 'big')
                    if MODULE[0] <= target < MODULE[1]:
                        pointers[start + offset] = target
        false = {0x1072d656: 0x10444805, 0x1072dad2: 0x10444805, 0x1072e15e: 0x10444804}
        self.assertEqual(pointers, VECTORS | false)
        for vector, target in VECTORS.items():
            self.assertEqual(self.word(vector + 4), 0x108bba90)
            self.assertIn(target, ENTRIES)
        with closing(sqlite3.connect(INPUT)) as db:
            for address in false:
                self.assertEqual(address % 4, 2)
                predecessor = json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                                     (address - 2,)).fetchone()[0])
                following = json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                                   (address + 2,)).fetchone()[0])
                self.assertEqual(predecessor['assembly'], 'addi r4,r30,0x1044')
                self.assertEqual(following['flow'], 'UNCONDITIONAL_CALL')

    def test_sort_and_virtual_callback_descriptors(self):
        for slot, vector in ((-0x4a2c, 0x108b9774), (-0x4a28, 0x108b977c), (-0x4a24, 0x108b9784)):
            self.assertEqual(self.word(0x108bba90 + slot), vector)
        sorts = [r['address'] for r in self.records if r['assembly'] == 'bl 0x10768300']
        self.assertEqual(sorts, [0x10445d68, 0x10445d80, 0x10446a14])
        for call, load, slot in ((0x10445d68, 0x10445d58, -0x4a24),
                                 (0x10445d80, 0x10445d70, -0x4a28),
                                 (0x10446a14, 0x10446a04, -0x4a2c)):
            self.assertEqual(self.by_address[load]['assembly'], f'lwz r6,-0x{-slot:x}(r2)')
            self.assertEqual(self.by_address[call - 4]['assembly'], 'li r5,0xc')
        virtuals = {0x1044440c: (0x10444408, 8), 0x10444518: (0x10444514, 0x14),
                    0x10444d40: (0x10444d3c, 0xa8), 0x10446b34: (0x10446b30, 8),
                    0x10446c64: (0x10446c60, 0xc)}
        self.assertEqual([r['address'] for r in self.records if r['assembly'] == 'bl 0x10781320'],
                         list(virtuals))
        for call, (load, method) in virtuals.items():
            self.assertEqual(self.by_address[load]['assembly'], f'lwz r12,0x{method:x}(r12)')
            self.assertEqual(self.by_address[call + 4]['assembly'], 'lwz r2,0x14(r1)')

    def test_audit_rejects_missing_export_and_stale_digest(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory) / 'missing.sqlite')
        stale = [(a, b, '0000000000000000') if index == 0 else (a, b, digest)
                 for index, (a, b, digest) in enumerate(rewritten(drafts=True))]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(audit, 'rewritten', return_value=stale):
            with self.assertRaisesRegex(ValueError, 'changed in the export'):
                require_nation_awards_index(db)

    def test_audit_rejects_duplicate_bad_end_disabled_and_missing_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = INCLUDES.read_text()
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            first = next(line for line in contents.splitlines() if 'NATION_AWARDS_ROUTINE(' in line)
            candidate.write_text(contents + '\n' + first + '\n')
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                rewritten(candidate)
            candidate.write_text(contents.replace('0x104443b0, R3', '0x104443b4, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                rewritten(candidate)
            candidate.write_text(contents.replace('NATION_AWARDS_ROUTINE(', 'NATION_AWARDS_DRAFT(', 1))
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
                    self.assertIn(f'if (try_modern_nation_awards(s, 0x{entry:x})) return true;', normal)
            self.assertIn('#include "nation_awards_adapter.hpp"', normal)
            reference = generate_page(page, part, selected, nation_awards_hooks=False)
            self.assertNotIn('try_modern_nation_awards', reference)


if __name__ == '__main__':
    unittest.main()
