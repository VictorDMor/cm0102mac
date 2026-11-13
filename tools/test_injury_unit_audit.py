"""Certify INJURY.CPP's 47 entries and the unit's bounds.

The private export is required evidence, rather than a reason to skip tests.
The C++ of this unit is not written yet: the production owners pinned here are
the nine older register-level ones, and that test turns over at delivery.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import injury_unit_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x101840e0, 0x10184308, 137, 'e8bdff1f1b8653bc'), (0x10184310, 0x101843c4, 45, 'a01a6d3bc2b42e3f'),
    (0x101843d0, 0x10184528, 85, '60cde53d6d2239e2'), (0x10184530, 0x10184e58, 584, '71b20cfe8566817e'),
    (0x10184e60, 0x10185404, 361, '70fabba08a1eb95f'), (0x10185410, 0x101864a0, 1060, 'e48f4463e5529c57'),
    (0x101864a0, 0x10186a18, 350, '73638cebd9e3bb54'), (0x10186a20, 0x10186cd0, 172, '923ca70372ca78fb'),
    (0x10186cd0, 0x10186e0c, 78, '1a0dce17cb7755e0'), (0x10186e10, 0x10186e94, 32, '44ba08c25989c501'),
    (0x10186ea0, 0x10186fb4, 69, '2232d1b4e4b86676'), (0x10186fc0, 0x10187450, 292, '2c7823120d9076d2'),
    (0x10187450, 0x10187644, 125, 'f30d14f1aa21da82'), (0x10187650, 0x10187680, 12, 'c8b2ab1123d00560'),
    (0x10187680, 0x101877dc, 87, 'b1d3024bb243a06a'), (0x101877e0, 0x101878d0, 60, 'df6a617a3b678d08'),
    (0x101878d0, 0x10187a1c, 83, '3897d989f147e6dc'), (0x10187a20, 0x10187a38, 6, '24c9977bff1debf3'),
    (0x10187a40, 0x10188488, 658, '59d8713db069d8f0'), (0x10188490, 0x101885f0, 88, '1ab99e8d48b699f9'),
    (0x101885f0, 0x10188924, 205, 'd6f35286444bfdfc'), (0x10188930, 0x10188cbc, 226, '4b18c38f9abae0bd'),
    (0x10188cc0, 0x1018aec4, 2175, '8152718bc92a4124'), (0x1018aed0, 0x1018b2b0, 248, 'cf5b7223158f6151'),
    (0x1018b2b0, 0x1018b6e0, 268, '724b8c29f7718e39'), (0x1018b6e0, 0x1018b9ec, 194, 'c4ea96e0ec4a8c48'),
    (0x1018b9f0, 0x1018bb4c, 87, '8798e4e5d82fb65c'), (0x1018bb50, 0x1018bdd0, 160, '02a20f5dbf66c542'),
    (0x1018bdd0, 0x1018c180, 236, '5758104134b0917e'), (0x1018c180, 0x1018c198, 6, '27d06b9528334466'),
    (0x1018c1a0, 0x1018c26c, 51, '826c2ab01663ff10'), (0x1018c270, 0x1018c2e4, 29, '671778d5265273c7'),
    (0x1018c2f0, 0x1018c33c, 19, 'ffab20f8680bb2a2'), (0x1018c340, 0x1018c448, 66, 'c8e67836260170c6'),
    (0x1018c450, 0x1018c560, 68, 'aee5b60ed4da5357'), (0x1018c560, 0x1018d140, 760, '5b1820231fbe07ae'),
    (0x1018d140, 0x1018d94c, 515, '9e4b952dfab21ce6'), (0x1018d950, 0x1018daa8, 86, '634292970266a21c'),
    (0x1018dab0, 0x1018dcb0, 128, '52c7cc854dc5a313'), (0x1018dcb0, 0x1018f33c, 1443, 'b0536a4493a4a701'),
    (0x1018f340, 0x1018f468, 74, '72a7db5221b5d250'), (0x1018f470, 0x1018fbd8, 474, '9b5edd4d6fa6ebe7'),
    (0x1018fbe0, 0x1018fdf0, 132, '534ce7673389917c'), (0x1018fdf0, 0x1018ff04, 69, 'd3af0f4a45e3401b'),
    (0x1018ff10, 0x1018ffe0, 52, '63390bac4d057aca'), (0x1018ffe0, 0x101900b0, 52, '46dc41c889bbdcf5'),
    (0x101900b0, 0x10190180, 52, '65c5d752b8dd41dd'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# Calls from outside the unit, per entry: 302 sites, every one a `bl` of an entry.
INCOMING = {0x101840e0: 2, 0x10184310: 1, 0x101843d0: 1, 0x10184e60: 1, 0x10185410: 2, 0x10186a20: 2,
            0x10186cd0: 1, 0x10186e10: 1, 0x10186ea0: 1, 0x10186fc0: 2, 0x10187450: 1, 0x10187650: 32,
            0x10187680: 75, 0x101877e0: 1, 0x101878d0: 33, 0x10187a20: 5, 0x10188cc0: 4, 0x1018bb50: 2,
            0x1018c180: 12, 0x1018c1a0: 1, 0x1018c270: 11, 0x1018c2f0: 5, 0x1018c340: 51, 0x1018c450: 3,
            0x1018c560: 9, 0x1018d140: 36, 0x1018d950: 1, 0x1018dab0: 1, 0x1018f340: 1, 0x1018f470: 1,
            0x1018fbe0: 1, 0x1018fdf0: 2}
# Entries another routine of the unit calls.
CROSS = {0x10184530, 0x101864a0, 0x10186a20, 0x10187680, 0x10187a40, 0x10188490, 0x101885f0, 0x10188930,
         0x10188cc0, 0x1018aed0, 0x1018b2b0, 0x1018b6e0, 0x1018b9f0, 0x1018bb50, 0x1018bdd0, 0x1018c180,
         0x1018c2f0, 0x1018c340, 0x1018c560, 0x1018d140, 0x1018dab0, 0x1018dcb0, 0x1018fdf0, 0x1018ff10,
         0x1018ffe0}
# The unit's private TOC slots with (loads, first load, last load). No other code loads them.
PRIVATE_SLOTS = {
    '-0x5430': (1, 0x101840fc, 0x101840fc),    # "CM3_TEMP"-style scrambled name
    '-0x5434': (1, 0x101840f8, 0x101840f8),
    '-0x5438': (18, 0x101840f0, 0x1018fe08),   # the string pool
    '-0x543c': (10, 0x10185420, 0x1018dcc8),   # floats
    '-0x5440': (1, 0x10185e88, 0x10185e88),    # jump table
    '-0x5444': (2, 0x10187864, 0x101879c8),    # floats
    '-0x5448': (1, 0x10187a58, 0x10187a58),    # three statics of 0x10187a40
    '-0x544c': (1, 0x10187a50, 0x10187a50),
    '-0x5450': (1, 0x10187a48, 0x10187a48),
    '-0x5454': (1, 0x1018b550, 0x1018b550),    # comparator vectors
    '-0x5458': (1, 0x1018b45c, 0x1018b45c),
    '-0x545c': (1, 0x1018b974, 0x1018b974),    # jump tables
    '-0x5460': (1, 0x1018bd98, 0x1018bd98),    # comparator vector
    '-0x5464': (1, 0x1018c608, 0x1018c608),    # jump tables
    '-0x5468': (1, 0x1018cba8, 0x1018cba8),
    '-0x546c': (1, 0x1018d170, 0x1018d170),
    '-0x5470': (1, 0x1018d410, 0x1018d410),
    '-0x5474': (1, 0x1018d6b0, 0x1018d6b0),
    '-0x5478': (1, 0x1018db40, 0x1018db40),
    '-0x64d0': (2, 0x1018541c, 0x10188948),    # injury tables
    '-0x64d4': (12, 0x10188608, 0x1018f488),
}
# The neighbouring units' slots on either side of each run: (loads, first, last), all outside the unit.
NEIGHBOUR_SLOTS = {'-0x547c': (11, 0x1014d59c, 0x10168a0c), '-0x542c': (1, 0x10192fdc, 0x10192fdc),
                   '-0x64d8': (6, 0x1014d74c, 0x1070a888), '-0x64cc': (1, 0x101935c0, 0x101935c0)}
# The eight jump tables: TOC slot -> (the `bctr`, the routine every case stays in).
JUMP_TABLES = {-0x5440: (0x10185e98, 0x10185410), -0x545c: (0x1018b984, 0x1018b6e0),
               -0x5464: (0x1018c618, 0x1018c560), -0x5468: (0x1018cbb8, 0x1018c560),
               -0x546c: (0x1018d180, 0x1018d140), -0x5470: (0x1018d420, 0x1018d140),
               -0x5474: (0x1018d6c0, 0x1018d140), -0x5478: (0x1018db50, 0x1018dab0)}
# The three pointers to the unit: transition vectors (code address, then the TOC) of the sort
# comparators, each loaded through its own TOC slot.
VECTORS = {0x108b8b44: (0x101900b0, -0x5460), 0x108b8b4c: (0x1018ffe0, -0x5458), 0x108b8b54: (0x1018ff10, -0x5454)}
# The register-level owners of the earlier process, which route every entry until the C++ lands.
OLD_OWNERS = {'injury_bootstrap_adapter': 6, 'injury_persistence_adapter': 2, 'injury_daily_adapter': 1,
              'actor_attributes_adapter': 16, 'injury_record_adapter': 3, 'injury_progression_adapter': 4,
              'injury_club_events_adapter': 3, 'injury_readouts_adapter': 8, 'injury_reporting_adapter': 4}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private INJURY export unavailable; inventory cannot be certified')


class InjuryUnitAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        cls.program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes())
                        for s in cls.program['segments'] if 'file' in s]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            cls.records = routine_records(db, *audit.MODULE)
        cls.by_address = {r['address']: r for r in cls.records}

    def original_bytes(self, address, count):
        for start, contents in self.segments:
            if start <= address and address + count <= start + len(contents):
                return contents[address - start:address - start + count]
        self.fail(f'Original bytes unavailable at {address:x}')

    def word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), 'big')

    def routine_of(self, address):
        return next((start for start, end in audit.ROUTINES if start <= address < end), None)

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.MODULE, (0x101840e0, 0x10190180))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.INJURY_UNIT_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 47)
        self.assertEqual(len(self.records), 12259)
        from tactics_audit import digest
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_injury_unit_index(db, complete=False)

    def test_production_owners_are_still_the_earlier_register_level_ones(self):
        # Turns over at delivery: every entry then has `injury_unit_adapter` alone, and
        # require_injury_unit_index runs with complete=True.
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        owners = {}
        for entry in ENTRIES:
            found = [owner for at, owner in OWNER_REPLACEMENTS if at == entry]
            self.assertEqual(len(found), 1)
            self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at == entry], found)
            owners[found[0]] = owners.get(found[0], 0) + 1
        self.assertEqual(owners, OLD_OWNERS)
        self.assertEqual(audit.rewritten(drafts=True), [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with self.assertRaisesRegex(ValueError, 'all 47 complete'):
                audit.require_injury_unit_index(db, complete=True)

    def test_unit_bounds_are_its_private_toc_slots_and_its_neighbours(self):
        pool = self.word(TOC - 0x5438)
        strings = self.original_bytes(pool, 0x80).split(b'\0')
        self.assertEqual(strings[:8], [b'Insert CD', b'Please insert the CM01/02 CD and click on OK', b'CM3_TEMP',
                                       b'injury_history.tmp', b'INJURY.CPP', b'v%s %s.%s %d', b'Error', b'injury.dat'])

        def loads(slot):
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                found = [a for a, in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                (f'%{slot}(r2)%',))]
            return len(found), found[0], found[-1]
        for slot, expected in PRIVATE_SLOTS.items():
            self.assertEqual(loads(slot), expected, slot)
            self.assertTrue(audit.MODULE[0] <= expected[1] <= expected[2] < audit.MODULE[1])
        # Each run is whole: every slot from -0x5478 to -0x5430, and the two injury tables.
        self.assertEqual({int(slot, 16) for slot in PRIVATE_SLOTS}, set(range(-0x5478, -0x542c, 4)) | {-0x64d4, -0x64d0})
        for slot, expected in NEIGHBOUR_SLOTS.items():
            self.assertEqual(loads(slot), expected, slot)
            self.assertFalse(audit.MODULE[0] <= expected[1] < audit.MODULE[1])
            self.assertFalse(audit.MODULE[0] <= expected[2] < audit.MODULE[1])
        # The slot before the run is last loaded in the routine that ends where the unit starts.
        self.assertEqual(self.word(audit.MODULE[0] - 16), 0x4e800020)   # its blr, then zero words
        self.assertEqual([self.word(audit.MODULE[0] - back) for back in (12, 8, 4)], [0, 0, 0])
        self.assertTrue(0x10168a0c < audit.MODULE[0] - 16)
        # The first routine loads the pool; the last pool user is the routine before the comparators.
        self.assertEqual(self.routine_of(0x101840f0), ENTRIES[0])
        self.assertEqual(self.routine_of(0x1018fe08), ENTRIES[-4])

    def test_all_incoming_edges_are_calls_of_entries_and_no_external_fallthrough(self):
        incoming = {}
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    'WHERE f.value>=? AND f.value<? ORDER BY i.address', audit.MODULE):
                if audit.MODULE[0] <= address < audit.MODULE[1]:
                    continue
                record = json.loads(body)
                for target in record['flows']:
                    if audit.MODULE[0] <= target < audit.MODULE[1]:
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        self.assertIn(target, ENTRIES)
                        incoming[target] = incoming.get(target, 0) + 1
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*audit.MODULE, *audit.MODULE)))
        self.assertEqual(incoming, INCOMING)
        self.assertEqual(sum(incoming.values()), 302)
        self.assertEqual(fallthroughs, [])
        # Every entry is reached: by an outside call, a call inside the unit, or a comparator vector.
        vectored = {target for target, _ in VECTORS.values()}
        self.assertEqual(set(INCOMING) | CROSS | vectored, set(ENTRIES))
        self.assertEqual(vectored - set(INCOMING) - CROSS, {0x101900b0})

    def test_cross_entry_edges_are_calls_of_entries_and_computed_flow_is_the_eight_tables(self):
        cross, computed = set(), {}
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                if 'COMPUTED' in record['flow']:
                    self.assertEqual((record['assembly'], record['flow']), ('bctr', 'COMPUTED_JUMP'))
                    computed[record['address']] = start
                targets = record['flows'] + ([record['fallthrough']] if record.get('fallthrough') is not None else [])
                for target in targets:
                    if not start <= target < end:
                        # No tail call and no branch out of a routine: only calls leave it.
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        if audit.MODULE[0] <= target < audit.MODULE[1]:
                            self.assertIn(target, ENTRIES)
                            cross.add(target)
        self.assertEqual(cross, CROSS)
        self.assertEqual(computed, dict(JUMP_TABLES.values()))
        self.assertEqual([s for s in self.program['symbols']
                          if audit.MODULE[0] <= s['address'] < audit.MODULE[1]], [])

    def test_every_instruction_and_alignment_word_accounted(self):
        addresses = set()
        for start, end in audit.ROUTINES:
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual([r['address'] for r in records],
                             [a for a in range(start, end, 4) if a not in audit.UNREACHABLE_NOPS])
            self.assertEqual(records[-1]['assembly'], 'blr')
            addresses.update(range(start, end, 4))
        for address in audit.UNREACHABLE_NOPS:
            self.assertEqual(self.word(address), 0x60000000)
            self.assertNotIn(address, self.by_address)
        for address in set(range(*audit.MODULE, 4)) - addresses:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)

    def test_pointers_to_the_unit_are_three_vectors_and_its_own_jump_tables(self):
        code_end = self.segments[0][0] + len(self.segments[0][1])
        tables = sorted((self.word(TOC + slot), routine) for slot, (_, routine) in JUMP_TABLES.items())
        cases = 0
        for start, contents in self.segments:
            for match in re.finditer(b'\x10[\x18\x19]', contents):
                at = start + match.start()
                if match.start() + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[match.start():match.start() + 4], 'big')
                if not audit.MODULE[0] <= target < audit.MODULE[1]:
                    continue
                if at in VECTORS:
                    entry, slot = VECTORS[at]
                    self.assertEqual((target, self.word(at + 4), self.word(TOC + slot)), (entry, TOC, at))
                elif at < code_end:
                    # Instruction bytes that read as an address: never a word-aligned pointer.
                    self.assertTrue(at % 4)
                else:
                    # A jump-table case: it belongs to the nearest table at or below it and stays in
                    # the routine that loads that table.
                    self.assertEqual(at % 4, 0)
                    base, routine = max(t for t in tables if t[0] <= at)
                    self.assertEqual(self.routine_of(target), routine)
                    self.assertNotEqual(self.word(at + 4), TOC)
                    cases += 1
        self.assertEqual(cases, 523)

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[1]
            line = f'INJURY_UNIT_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_injury_unit_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_injury_unit_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing INJURY.CPP instructions'):
                    audit.require_injury_unit_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)


if __name__ == '__main__':
    unittest.main()
