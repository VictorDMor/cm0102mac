"""Certify AWARD_SHORTLIST.CPP original inventory and normal production ownership.

Original listings stay private. Missing original inputs fail certification.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import award_shortlist_audit as audit
from award_shortlist_audit import (
    MODULE, ROUTINES, INCLUDES, rewritten, require_award_shortlist_index, volatile_reads,
)
from tactics_audit import routine_records, digest

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/current/export/instructions.sqlite'
ENTRIES = (
    0x10437cc0, 0x10437e40, 0x10437ea0, 0x104380c0, 0x104382c0,
    0x10438450, 0x104386b0, 0x104394c0, 0x10439a40, 0x10439c10,
    0x10439ea0, 0x10439ee0, 0x10439fa0, 0x10439fd0,
)
COUNTS = (95, 23, 134, 128, 100, 151, 893, 350, 114, 163, 13, 46, 10, 4)
DIGESTS = (
    '4bf871b0255db330', 'd18207febb6a649c', 'f3d50ac8e4181500',
    '7a9eeee56de2017d', 'f66d408bc2916062', 'bc2ae4a27fc96ac0',
    '31fadbf2c12ee6c7', 'b9f07d9e709b09ff', '0b231a8c33daee51',
    '85f96dbd67e6e775', '4ad66243b5fbd4bf', '25091ae7d30c89d9',
    '9ee5c9e4de9c24f8', 'b0d785e6af880f6a',
)
VECTORS = {0x108b961c: 0x10439fd0, 0x108b9624: 0x10439fa0,
           0x108b962c: 0x10439ee0}
SWITCH = dict(zip(range(0x108cf3ac, 0x108cf3c8, 4),
                  (0x10439d00, 0x10439d2c, 0x10439d64, 0x10439dac,
                   0x10439df4, 0x10439e08, 0x10439e1c)))
GAPS = {0x104386b0: {0x10438740, 0x10438938, 0x10438f30, 0x10439298},
        0x10439a40: {0x10439b4c}}
INCOMING_CALLS = [
    (0x1000264c, 0x10439c10), (0x1000290c, 0x10439c10),
    (0x10002998, 0x10439c10), (0x1038aa38, 0x10439c10),
    (0x103a73bc, 0x10439a40), (0x103a92fc, 0x10439a40),
    (0x1043748c, 0x10439c10), (0x104446f4, 0x10439ea0),
    (0x10444868, 0x104382c0), (0x10444890, 0x10438450),
    (0x10444f74, 0x104386b0), (0x10444f80, 0x10439ea0),
    (0x1044cba4, 0x10437cc0), (0x1044cc18, 0x10437e40),
    (0x1044cdc4, 0x10437cc0), (0x1044cf74, 0x104380c0),
]
INTERNAL_CALLS = [
    (0x10437e18, 0x10437ea0), (0x10438874, 0x104394c0),
    (0x10438a6c, 0x104394c0), (0x10438c68, 0x104394c0),
    (0x10438e68, 0x104394c0), (0x10439028, 0x104394c0),
    (0x104391d4, 0x104394c0), (0x10439390, 0x104394c0),
    (0x1043989c, 0x104394c0),
]


def require_export(path):
    if not path.is_file():
        raise RuntimeError('Private AWARD_SHORTLIST export unavailable; inventory cannot be certified')


class AwardShortlistAuditTests(unittest.TestCase):
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
                             ['award_shortlist_adapter'])
        self.assertFalse(any(MODULE[0] <= at < MODULE[1] and at not in ENTRIES
                             for at, _ in owners), 'Unowned interior production hook')

    def test_exact_inventory_counts_digests_and_production_ownership(self):
        self.assertEqual(MODULE, (0x10437cc0, 0x10439fe0))
        self.assertEqual(ROUTINES, tuple(zip(ENTRIES, ENTRIES[1:] + (MODULE[1],))))
        self.assertEqual(len(self.records), 2224)
        self.assertEqual(tuple(sum(a <= r['address'] < b for r in self.records)
                               for a, b in ROUTINES), COUNTS)
        with closing(sqlite3.connect(INPUT)) as db:
            self.assertEqual(tuple(digest(routine_records(db, a, b)) for a, b in ROUTINES), DIGESTS)
            require_award_shortlist_index(db)
        self.assertEqual(tuple(d for _, _, d in rewritten(drafts=True)), DIGESTS)
        from compile_recomp import OWNER_REPLACEMENTS
        self.assert_production_inventory(rewritten(), OWNER_REPLACEMENTS)

    def test_source_numeric_pools_and_neighbor_boundaries(self):
        toc = 0x108bba90
        for slot, address, source in (
                (-0x4a84, 0x1082b674, b'Insert CD\0'),
                (-0x4a64, 0x1082b850, b'AWARD_SHORTLIST.CPP\0v%s %s.%s %d\0Error\0\0'),
                (-0x4a60, 0x1082b888, b'BELGIUM_AWARDS.CPP\0')):
            self.assertEqual(self.word(toc + slot), address)
            self.assertEqual(self.original_bytes(address, len(source)), source)
        self.assertEqual(self.original_bytes(0x1082b6de, 18), b'AWARD_MANAGER.CPP\0')
        self.assertEqual(self.word(toc - 0x4a70), 0x1082b878)
        self.assertEqual(self.original_bytes(0x1082b878, 16).hex(),
                         '40efffe0000000004330000080000000')
        with closing(sqlite3.connect(INPUT)) as db:
            for slot, expected in (
                    (-0x4a64, [(0x10437cd8, 'lwz r30,-0x4a64(r2)'),
                               (0x10437ec0, 'lwz r31,-0x4a64(r2)'),
                               (0x104380e0, 'lwz r31,-0x4a64(r2)'),
                               (0x104382c8, 'lwz r31,-0x4a64(r2)'),
                               (0x10439a60, 'lwz r30,-0x4a64(r2)'),
                               (0x10439c20, 'lwz r30,-0x4a64(r2)')]),
                    (-0x4a70, [(0x104386c8, 'lwz r27,-0x4a70(r2)'),
                               (0x104394cc, 'lwz r25,-0x4a70(r2)')])):
                actual = [(a, json.loads(b)['assembly']) for a, b in db.execute(
                    'SELECT address,body FROM instructions WHERE body LIKE ? ORDER BY address',
                    (f'%-0x{-slot:x}(r2)%',))]
                self.assertEqual(actual, expected)
            for address, assembly in (
                    (0x10437cbc, 'blr'), (MODULE[0], 'mfspr r0,LR'),
                    (0x10439fe0, 'mfspr r0,LR'),
                    (0x1043a000, 'lwz r6,-0x61fc(r2)'),
                    (0x1043a0c4, 'lwz r24,-0x4a60(r2)')):
                self.assertEqual(json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                                       (address,)).fetchone()[0])['assembly'], assembly)
        # Belgium's default constructor precedes its source-pool-using initializer.
        # Its distinct vtable proves it must not inflate this module's inventory.
        self.assertEqual(self.word(toc - 0x61fc), 0x108cf3c8)
        self.assertEqual([self.word(0x108cf3c8 + i * 4) for i in range(7)],
                         [0, 0, 0x108b9644, 0x108b963c, 0x108b978c, 0x108b95c4, 0x108b9634])
        self.assertEqual(self.word(0x108b9644), 0x1043a0a0)
        self.assertEqual(self.word(0x108b963c), 0x1043a520)
        self.assertEqual(self.word(0x108b9634), 0x1043a7e0)

    def test_every_instruction_unindexed_nop_and_padding_accounted(self):
        for entry, end in ROUTINES:
            with self.subTest(entry=hex(entry)):
                records = [r for r in self.records if entry <= r['address'] < end]
                self.assertEqual(records[0]['address'], entry)
                self.assertEqual(records[-1]['assembly'], 'blr')
                prologues = sum(r['assembly'] == 'mfspr r0,LR' for r in records)
                self.assertEqual(prologues, 0 if entry in (0x10439fa0, 0x10439fd0) else 1)
                last = records[-1]['address'] + 4
                missing = set(range(entry, last, 4)) - {r['address'] for r in records}
                self.assertEqual(missing, GAPS.get(entry, set()))
                for address in missing:
                    self.assertEqual(self.word(address), 0x60000000)
                    predecessor = self.by_address[address - 4]
                    self.assertEqual(predecessor['flow'], 'UNCONDITIONAL_JUMP')
                    self.assertIsNone(predecessor.get('fallthrough'))
                    self.assertFalse(any(address in r['flows'] or r.get('fallthrough') == address
                                         for r in self.records))
                self.assertLess(end - last, 16)
                self.assertEqual(self.original_bytes(last, end - last), bytes(end - last))
                padding = set(range(last, end, 4))
                self.assertFalse(any(padding.intersection(r['flows']) or r.get('fallthrough') in padding
                                     for r in self.records))

    def test_whole_export_incoming_flows_and_fallthroughs(self):
        incoming = []
        with closing(sqlite3.connect(INPUT)) as db:
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    'WHERE f.value>=? AND f.value<?', MODULE):
                if MODULE[0] <= address < MODULE[1]:
                    continue
                record = json.loads(body)
                self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                incoming.extend((address, target) for target in record['flows']
                                if MODULE[0] <= target < MODULE[1])
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*MODULE, *MODULE)))
        self.assertEqual(sorted(incoming), INCOMING_CALLS)
        self.assertTrue(all(target in ENTRIES for _, target in incoming))
        self.assertEqual(fallthroughs, [])

    def test_owned_calls_recursion_and_only_known_switch_jump(self):
        calls, computed = [], []
        for entry, end in ROUTINES:
            for record in self.records:
                if not entry <= record['address'] < end:
                    continue
                if 'COMPUTED' in record['flow']:
                    computed.append((record['address'], record['assembly'], record['flow']))
                for target in record['flows']:
                    if not MODULE[0] <= target < MODULE[1]:
                        continue
                    if record['flow'] == 'UNCONDITIONAL_CALL':
                        calls.append((record['address'], target))
                        self.assertIn(target, ENTRIES)
                    else:
                        self.assertTrue(entry <= target < end,
                                        f'Unexpected cross-entry flow at {record["address"]:x}')
        self.assertEqual(calls, INTERNAL_CALLS)
        self.assertEqual(computed, [(0x10439cfc, 'bctr', 'COMPUTED_JUMP')])
        self.assertEqual([s for s in self.program['symbols']
                          if MODULE[0] <= s['address'] < MODULE[1]], [])

    def test_all_byte_pointer_census_callbacks_switch_and_false_positives(self):
        pointers = {}
        for start, contents in self.segments:
            for match in re.finditer(b'\x10\x43', contents):
                offset = match.start()
                if offset + 4 <= len(contents):
                    target = int.from_bytes(contents[offset:offset + 4], 'big')
                    if MODULE[0] <= target < MODULE[1]:
                        pointers[start + offset] = target
        false_positives = {0x105f9ab6: 0x10439061, 0x105f9b32: 0x10439001}
        self.assertEqual(pointers, VECTORS | SWITCH | false_positives)
        # These two unaligned byte sequences straddle immediate arithmetic and
        # store instructions, and are not relocated code addresses.
        with closing(sqlite3.connect(INPUT)) as db:
            for address, assembly in (
                    (0x105f9ab4, 'addi r3,r26,0x1043'), (0x105f9ab8, 'stw r3,0x50(r1)'),
                    (0x105f9b30, 'addi r0,r26,0x1043'), (0x105f9b34, 'stw r0,0x50(r1)')):
                self.assertEqual(json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                                       (address,)).fetchone()[0])['assembly'], assembly)
        for vector, target in VECTORS.items():
            self.assertEqual(self.word(vector + 4), 0x108bba90)
            self.assertIn(target, ENTRIES)
        for address, target in SWITCH.items():
            self.assertEqual(self.word(address), target)
            self.assertNotIn(target, ENTRIES)
            self.assertTrue(0x10439c10 < target < 0x10439ea0)
        self.assertEqual(self.word(0x108bba90 - 0x4a78), min(SWITCH))
        self.assertEqual(self.by_address[0x10439cec]['assembly'], 'lwz r5,-0x4a78(r2)')
        for slot, vector in ((-0x4a74, 0x108b961c), (-0x4a6c, 0x108b9624),
                             (-0x4a68, 0x108b962c)):
            self.assertEqual(self.word(0x108bba90 + slot), vector)

    def test_audit_rejects_missing_export_and_stale_digest(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory) / 'missing.sqlite')
        stale = [(a, b, '0000000000000000') if index == 0 else (a, b, value)
                 for index, (a, b, value) in enumerate(rewritten(drafts=True))]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(audit, 'rewritten', return_value=stale):
            with self.assertRaisesRegex(ValueError, 'changed in the export'):
                require_award_shortlist_index(db)

    def test_audit_rejects_duplicate_bad_end_disabled_and_missing_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = INCLUDES.read_text()
        # The structural corruption controls remain meaningful during drafts;
        # promotion is independently required by the production inventory test.
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            first = next(line for line in contents.splitlines()
                         if 'AWARD_SHORTLIST_ROUTINE(' in line or 'AWARD_SHORTLIST_DRAFT(' in line)
            candidate.write_text(contents + '\n' + first + '\n')
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                rewritten(candidate, drafts=True)
            candidate.write_text(contents.replace('0x10437e40, R3', '0x10437e44, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                rewritten(candidate, drafts=True)
            candidate.write_text(contents.replace('AWARD_SHORTLIST_ROUTINE(',
                                                   'AWARD_SHORTLIST_DRAFT(', 1))
            self.assertEqual(len(rewritten(candidate, drafts=True)), len(ENTRIES))
            with self.assertRaises(AssertionError):
                self.assert_production_inventory(rewritten(candidate), OWNER_REPLACEMENTS)
        complete = rewritten(drafts=True)
        owners = [(entry, 'award_shortlist_adapter') for entry in ENTRIES]
        self.assert_production_inventory(complete, owners)
        with self.assertRaises(AssertionError):
            self.assert_production_inventory(complete, owners[1:])
        with self.assertRaises(AssertionError):
            self.assert_production_inventory(complete, owners + owners[:1])
        with self.assertRaises(AssertionError):
            self.assert_production_inventory(complete, owners + [(0x10439d00, 'award_shortlist_adapter')])

    def test_default_generation_guards_every_entry_and_reference_bypasses(self):
        from compile_recomp import generate_page
        selected = {r['address'] for r in self.records}
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected)
            for entry in ENTRIES:
                if entry >> 12 == page:
                    self.assertTrue(f'if (try_modern_award_shortlist(s, 0x{entry:x})) return true;' in normal,
                                    f'Missing normal-production guard at {entry:x}')
            self.assertIn('#include "award_shortlist_adapter.hpp"', normal)
            reference = generate_page(page, part, selected, award_shortlist_hooks=False)
            self.assertNotIn('try_modern_award_shortlist', reference)

    def test_no_unmodelled_scratch_register_dependencies(self):
        with closing(sqlite3.connect(INPUT)) as db:
            self.assertEqual(volatile_reads(db), [])


if __name__ == '__main__':
    unittest.main()
