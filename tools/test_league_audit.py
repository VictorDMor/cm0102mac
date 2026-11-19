"""Certify LEAGUE.CPP's 67 entries, the unit's bounds and its production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest

import league_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x1049e4f0, 0x1049e600, 68, '09021602fb692776'), (0x1049e600, 0x1049e678, 30, '1432c08b7ecacce9'),
    (0x1049e680, 0x1049e81c, 103, '28ba9cd3d2d6a31e'), (0x1049e820, 0x1049e958, 78, '6703b3c793d24beb'),
    (0x1049e960, 0x1049e964, 1, 'cf739f6894197cb4'), (0x1049e970, 0x1049e9fc, 35, '524b76a3058220b3'),
    (0x1049ea00, 0x1049ea10, 4, 'f062783dbf49e05a'), (0x1049ea10, 0x1049ea9c, 35, '1c5ae190a4a435ec'),
    (0x1049eaa0, 0x1049eb28, 34, 'a47a2cc88280c2ab'), (0x1049eb30, 0x1049f030, 317, 'a51811b4130d4028'),
    (0x1049f030, 0x1049f318, 182, 'c5d487064e9387a4'), (0x1049f320, 0x1049f53c, 135, '4ad67bc0f29180a5'),
    (0x1049f540, 0x1049f6d0, 100, 'fe44d8e3251760a3'), (0x1049f6d0, 0x1049f77c, 43, '849da473ab4f8281'),
    (0x1049f780, 0x1049f8d8, 86, 'e33505ebe5f587fe'), (0x1049f8e0, 0x1049f998, 46, '007b8ce62fdbba1a'),
    (0x1049f9a0, 0x1049fc74, 178, 'c6794985f8027236'), (0x1049fc80, 0x1049fdb4, 77, 'c9a688a53f424f32'),
    (0x1049fdc0, 0x1049ff88, 113, 'e2d2740b622b11ca'), (0x1049ff90, 0x104a00a0, 67, '732479d41fae0fbb'),
    (0x104a00a0, 0x104a0aa8, 639, '668c3b7d6c8bbc4b'), (0x104a0ab0, 0x104a0e34, 223, '8f2e000e491dc475'),
    (0x104a0e40, 0x104a10d0, 163, '6fe39c29dc533cd5'), (0x104a10d0, 0x104a1348, 158, 'f144db8965cce601'),
    (0x104a1350, 0x104a14e8, 102, 'a6266495c1b7b6aa'), (0x104a14f0, 0x104a1518, 10, '787b2d36d30a0be1'),
    (0x104a1520, 0x104a153c, 7, '3bc0b6f2b77734f9'), (0x104a1540, 0x104a1c30, 444, '19102a0314f2265f'),
    (0x104a1c30, 0x104a26a8, 668, '565f9b8e241261c3'), (0x104a26b0, 0x104a28c4, 132, '2d5c5506a1a5c847'),
    (0x104a28d0, 0x104a3344, 669, '4afd3f7ff08fa52f'), (0x104a3350, 0x104a38c8, 350, '77201ba98a841c4f'),
    (0x104a38d0, 0x104a3924, 21, '701ca413bc57ef9c'), (0x104a3930, 0x104a3b88, 150, '0ffff4a1f2c39230'),
    (0x104a3b90, 0x104a44ec, 598, '985fdabf82945024'), (0x104a44f0, 0x104a47f0, 192, 'dc56b9c455ebe2e9'),
    (0x104a47f0, 0x104a4864, 29, '287cc728ab79f2a9'), (0x104a4870, 0x104a4968, 62, 'e1f98585a45d59fd'),
    (0x104a4970, 0x104a60b4, 1485, '430a12119265ff7c'), (0x104a60c0, 0x104a60c4, 1, '9bd679489624eb8d'),
    (0x104a60d0, 0x104a60d8, 2, '8341e11f256d2d22'), (0x104a60e0, 0x104a6440, 214, '0381a1fa8ae55c70'),
    (0x104a6440, 0x104a6654, 132, '361ec03623575c1b'), (0x104a6660, 0x104a6754, 60, '3e1dcefc8f713aaf'),
    (0x104a6760, 0x104a6850, 60, '20f1931de00dd20f'), (0x104a6850, 0x104a8530, 1844, '87af26d1561f124e'),
    (0x104a8530, 0x104a8f34, 639, 'fb8cb1317e050d8a'), (0x104a8f40, 0x104a9160, 135, 'f9e8d8f7d900ed12'),
    (0x104a9160, 0x104a9208, 42, '1de81e820b666e0e'), (0x104a9210, 0x104a95f8, 247, 'e8e17e2b77bf383e'),
    (0x104a9600, 0x104a9808, 130, '4b6948cf13bcee1f'), (0x104a9810, 0x104a988c, 31, '16a83ac5504bda6b'),
    (0x104a9890, 0x104a9a94, 129, '43969f816f120a87'), (0x104a9aa0, 0x104a9bc0, 71, 'f0b2e57e62debea7'),
    (0x104a9bc0, 0x104aa0bc, 314, 'de59487f65e2133a'), (0x104aa0c0, 0x104aa23c, 94, 'dd35f038135e8482'),
    (0x104aa240, 0x104aa33c, 62, 'f3d732a31474dc55'), (0x104aa340, 0x104aa398, 21, '4562a7f94683bb69'),
    (0x104aa3a0, 0x104aa42c, 34, '9d58a7e9363111d9'), (0x104aa430, 0x104aa55c, 75, 'e202ba85bcc8f0ce'),
    (0x104aa560, 0x104aa5b8, 21, 'edbea3f83b3ae0e4'), (0x104aa5c0, 0x104aa620, 23, 'ef03bdc8682b4b5e'),
    (0x104aa620, 0x104aaae4, 301, '2e0a265409b0d3bb'), (0x104aaaf0, 0x104aab00, 4, '03a9f7462ec2cb6c'),
    (0x104aab00, 0x104aab18, 6, 'a3d51a83f610cbe0'), (0x104aab20, 0x104aab30, 4, '7815ceadf3b3489f'),
    (0x104aab30, 0x104aac14, 57, '8716f25d0dacfc3a'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# The pilot block (2026-11-18), routed in production: the constructor, the destructor, the three one-line
# virtuals, the three "not overridden" assertion virtuals, the row and flag leaves and the three comparators.
PILOT = (0x1049e4f0, 0x1049e600, 0x1049e960, 0x1049e970, 0x1049ea00, 0x1049ea10, 0x1049eaa0, 0x104a14f0,
         0x104a1520, 0x104a60c0, 0x104a60d0, 0x104aaaf0, 0x104aab00, 0x104aab20)
# Calls from outside the unit, per entry: 13,164 sites, every one a `bl` of an entry.
INCOMING = {0x1049e4f0: 118, 0x1049e600: 120, 0x1049e680: 1, 0x1049ea00: 220, 0x1049eb30: 227, 0x1049f030: 2,
            0x1049f540: 40, 0x1049f6d0: 38, 0x1049f780: 31, 0x1049f8e0: 31, 0x104a00a0: 202, 0x104a0ab0: 1,
            0x104a0e40: 75, 0x104a1350: 4, 0x104a1540: 32, 0x104a1c30: 5, 0x104a26b0: 1, 0x104a3930: 12,
            0x104a3b90: 3, 0x104a47f0: 7, 0x104a60e0: 135, 0x104a6660: 228, 0x104a9210: 55, 0x104a9810: 3562,
            0x104a9890: 5647, 0x104aa240: 28, 0x104aa3a0: 63, 0x104aa430: 14, 0x104aa560: 46, 0x104aab30: 2216}
# Entries another routine of the unit calls.
CROSS = {0x1049e820, 0x1049eb30, 0x1049f030, 0x1049f320, 0x1049f780, 0x1049f9a0, 0x1049fdc0, 0x1049ff90,
         0x104a0ab0, 0x104a10d0, 0x104a28d0, 0x104a3350, 0x104a3b90, 0x104a4870, 0x104a4970, 0x104a6440,
         0x104a6660, 0x104a9210, 0x104a9600, 0x104a9bc0, 0x104aa560}
# The class's vtable (TOC -0x6158): its words from +8, each a descriptor's code. Slots the league does not
# override hold COMP.CPP's bodies (0x10450...-0x10453...), and 0x1006c1f0, 0x101b6ff0, 0x101b8af0,
# 0x101b75c0, 0x10070ba0 are shared bodies of other units.
VTABLE = (0x1049e600, 0x104a3930, 0x1049eaa0, 0x10450900, 0x104a8f40, 0x104a1540, 0x104a6850, 0x104a8530,
          0x10450a40, 0x10450a30, 0x1049e960, 0x104509d0, 0x104a1350, 0x1006c1f0, 0x10450580, 0x1049e970,
          0x104a44f0, 0x104a47f0, 0x10450030, 0x10451ac0, 0x10451c90, 0x104a9aa0, 0x10451d10, 0x104aa0c0,
          0x10451f50, 0x10453380, 0x10451f80, 0x10453390, 0x101b6ff0, 0x101b8af0, 0x101b75c0, 0x104530f0,
          0x104aa5c0, 0x104aa620, 0x10070ba0, 0x1049ea10, 0x104aa430, 0x104a1c30, 0x104a14f0, 0x104a1520,
          0x104a6760, 0x104aa340, 0x104a60d0, 0x104a60c0, 0x104a9160)
# The unit's descriptors, 0x108b9b94..0x108b9c9c: with the vtable and the TOC slots below, the only data
# words naming the unit.
DESCRIPTORS = (0x108b9b94, 0x108b9c9c)
# TOC slots with (loads, first load, last load), all inside the unit. -0x3750 and -0x3754 hold words
# themselves (a sort's league and the comparison it calls back).
PRIVATE_SLOTS = {
    '-0x470c': (21, 0x1049e978, 0x104aa640),  # the string pool, "LEAGUE.CPP" at +0
    '-0x4710': (1, 0x1049f238, 0x1049f238), '-0x4714': (1, 0x104a2974, 0x104a2974),
    '-0x4718': (1, 0x104a2ab8, 0x104a2ab8), '-0x471c': (3, 0x104a28d8, 0x104aa638),
    '-0x4720': (1, 0x104a8900, 0x104a8900), '-0x4724': (1, 0x104aa630, 0x104aa630),
    '-0x4728': (1, 0x104aa7a4, 0x104aa7a4), '-0x472c': (1, 0x104aa62c, 0x104aa62c),
    '-0x6158': (3, 0x1049e508, 0x1049e70c),   # the vtable
    '-0x6160': (1, 0x1049eb38, 0x1049eb38),
    '-0x3750': (9, 0x1049ebac, 0x1049ffcc), '-0x3754': (7, 0x1049ec64, 0x1049fe2c),
}
# Slots this unit loads first and another later.
SHARED_SLOTS = {'-0x615c': (3, 0x1049ec7c, 0x1054aaf0)}
# COMP.CPP's vtable slot, loaded last by the inline copy of its copy constructor in 0x1049e820.
COMP_VTABLE_SLOT = ('-0x619c', (3, 0x1044fed0, 0x1049e838))
# The neighbouring units' slots on either side: (loads, first, last), all outside the unit.
NEIGHBOUR_SLOTS = {'-0x4730': (1, 0x1049ba48, 0x1049ba48), '-0x4738': (6, 0x1049ba40, 0x1049e3d8),
                   '-0x6164': (2, 0x1049d43c, 0x1049d7d0), '-0x4708': (2, 0x104aac28, 0x104ac310),
                   '-0x6154': (2, 0x104aaed4, 0x104ac850)}
# The entries the 2026-11-18 inventory did not attribute to the unit.
ADDED = {0x1049e4f0, 0x1049e600, 0x1049e960, 0x104aaaf0, 0x104aab00, 0x104aab20, 0x104aab30}
NEXT_UNIT = (0x104aac20, 0x104aca48)


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private LEAGUE.CPP export unavailable; inventory cannot be certified')


class LeagueAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        cls.program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes())
                        for s in cls.program['segments'] if 'file' in s]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            cls.records = routine_records(db, *audit.MODULE)
            cls.next_unit = routine_records(db, *NEXT_UNIT)
        cls.by_address = {r['address']: r for r in cls.records}

    def original_bytes(self, address, count):
        for start, contents in self.segments:
            if start <= address and address + count <= start + len(contents):
                return contents[address - start:address - start + count]
        self.fail(f'Original bytes unavailable at {address:x}')

    def word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), 'big')

    def loads(self, slot):
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            found = [a for a, in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                            (f'%{slot}(r2)%',))]
        return len(found), found[0], found[-1]

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.MODULE, (0x1049e4f0, 0x104aac14))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.LEAGUE_BASE_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 67)
        self.assertEqual(len(self.records), 12587)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_league_base_index(db, complete=False)

    def test_listed_entries_are_pinned_and_routed_to_the_units_owner_alone(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        pinned = {a: (b, d) for a, b, _, d in CONTRACTS}
        listed = audit.rewritten(drafts=True)
        for entry, end, expected in listed:
            self.assertEqual(pinned[entry], (end, expected))
        production = {entry for entry, _, _ in audit.rewritten()}
        self.assertLessEqual(set(PILOT), production)
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ['league_base_adapter'])
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_league_base_index(db)
            if len(production) != len(ENTRIES):
                self.assertFalse(audit.COMPLETE)
                with self.assertRaisesRegex(ValueError, 'requires all 67'):
                    audit.require_league_base_index(db, complete=True)

    def assert_hook(self, code, entry):
        hook = audit.league_base_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_normal_hooks_precede_original_steps_and_detect_missing_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_league_base_hooks(page, part, normal)
            self.assertIn('#include "league_base_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.league_base_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_league_base_hooks(page, part, broken)

    def test_unit_bounds_are_its_pool_descriptors_vtable_slots_and_neighbours(self):
        pool = self.word(TOC - 0x470c)
        self.assertEqual(pool, 0x1083a804)
        self.assertEqual(self.original_bytes(pool, 11), b'LEAGUE.CPP\0')
        self.assertEqual(self.original_bytes(pool + 0x1e, 25), b'home != -1 && away != -1\0')
        self.assertTrue(self.original_bytes(self.word(TOC - 0x4738), 0x100).find(b'HOST_COUNTRY.CPP') >= 0)
        for slot, expected in PRIVATE_SLOTS.items():
            self.assertEqual(self.loads(slot), expected, slot)
            self.assertTrue(audit.MODULE[0] <= expected[1] <= expected[2] < audit.MODULE[1])
        for slot, expected in SHARED_SLOTS.items():
            self.assertEqual(self.loads(slot), expected, slot)
            self.assertTrue(audit.MODULE[0] <= expected[1] < audit.MODULE[1])
        slot, expected = COMP_VTABLE_SLOT
        self.assertEqual(self.loads(slot), expected)
        self.assertEqual(self.by_address[0x1049e838]['assembly'], 'lwz r0,-0x619c(r2)')
        for slot, expected in NEIGHBOUR_SLOTS.items():
            self.assertEqual(self.loads(slot), expected, slot)
            self.assertFalse(audit.MODULE[0] <= expected[1] < audit.MODULE[1])
            self.assertFalse(audit.MODULE[0] <= expected[2] < audit.MODULE[1])
        # The run of data slots -0x472c..-0x470c and of vtable slots -0x6160..-0x6158 sit between the neighbours'.
        self.assertTrue(self.by_address[0x1049e978]['assembly'].endswith('-0x470c(r2)'))
        self.assertTrue(self.by_address[0x104aa640]['assembly'].endswith('-0x470c(r2)'))
        # Descriptors: one contiguous run of (code, TOC) pairs naming the unit, between the neighbours'.
        start, end = DESCRIPTORS
        named = [self.word(a) for a in range(start, end, 8)]
        self.assertEqual([self.word(a + 4) for a in range(start, end, 8)], [TOC] * len(named))
        self.assertEqual(len(named), 33)
        self.assertTrue(all(code in ENTRIES for code in named))
        self.assertEqual(self.word(start - 8), 0x1049e4c0)  # HOST_COUNTRY's comparator (its slot -0x6164)
        self.assertEqual(self.word(TOC - 0x6164), start - 8)
        self.assertEqual(self.word(end), 0x104ac9f0)        # the next unit's first descriptor
        self.assertFalse(audit.MODULE[0] <= self.word(end) < audit.MODULE[1])
        self.assertLessEqual({0x1049e600, 0x1049e960, 0x104aaaf0, 0x104aab00, 0x104aab20}, set(named))
        for slot, code in (('-0x472c', 0x104aab20), ('-0x4728', 0x104aab00), ('-0x4718', 0x104aaaf0)):
            self.assertEqual(self.word(self.word(TOC + int(slot, 16))), code)
        # The vtable: the constructor and destructor store it, slot +8 is the destructor.
        vtable = self.word(TOC - 0x6158)
        self.assertEqual(vtable, 0x108d796c)
        self.assertEqual((self.word(vtable), self.word(vtable + 4)), (0, 0))
        self.assertEqual(tuple(self.word(self.word(vtable + 8 + 4 * n)) for n in range(len(VTABLE))), VTABLE)
        self.assertEqual(self.word(vtable + 8 + 4 * len(VTABLE)), 0)
        self.assertEqual(self.by_address[0x1049e508]['assembly'], 'lwz r0,-0x6158(r2)')
        self.assertEqual(self.by_address[0x1049e620]['assembly'], 'lwz r0,-0x6158(r2)')
        self.assertEqual(self.by_address[0x1049e504]['assembly'], 'bl 0x1044fed0')
        # The next unit starts with its constructor: COMP's constructor and its own first slots.
        self.assertEqual(self.next_unit[0]['address'], 0x104aac20)
        first = {r['address']: r['assembly'] for r in self.next_unit[:12]}
        self.assertEqual(first[0x104aac28], 'lwz r29,-0x4708(r2)')
        self.assertEqual(first[0x104aac44], 'bl 0x1044fed0')
        self.assertEqual(first[0x104aac48], 'lwz r4,-0x614c(r2)')

    def test_trailing_fixture_builder_uses_the_leagues_fields_and_the_next_unit_does_not(self):
        body = [r['assembly'] for r in self.records if 0x104aab30 <= r['address'] < 0x104aac14]
        self.assertIn('or r29,r4,r4', body)
        self.assertIn('lwz r3,0xb0(r29)', body)
        self.assertIn('lha r0,0x3e(r29)', body)
        self.assertIn('bl 0x107158a0', body)
        self.assertEqual(self.by_address[0x1049ea00]['assembly'], 'mulli r0,r4,0x44')
        self.assertEqual(self.by_address[0x1049ea04]['assembly'], 'lwz r3,0xb0(r3)')
        self.assertEqual(self.by_address[0x1049e628]['assembly'], 'lwz r3,0xe8(r30)')
        field = re.compile(r',0x(b0|e8)\(r(\d+)\)$')
        for record in self.next_unit:
            found = field.search(record['assembly'])
            if found:
                self.assertEqual(found.group(2), '1', hex(record['address']))

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
        self.assertEqual(sum(incoming.values()), 13164)
        self.assertEqual(fallthroughs, [])

    def test_cross_entry_edges_are_calls_of_entries_and_no_computed_flow(self):
        cross = set()
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                self.assertNotIn('COMPUTED', record['flow'])
                targets = record['flows'] + ([record['fallthrough']] if record.get('fallthrough') is not None else [])
                for target in targets:
                    if not start <= target < end:
                        # No tail call and no branch out of a routine: only calls leave it.
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        if audit.MODULE[0] <= target < audit.MODULE[1]:
                            self.assertIn(target, ENTRIES)
                            cross.add(target)
        self.assertEqual(cross, CROSS)
        self.assertEqual([s for s in self.program['symbols']
                          if audit.MODULE[0] <= s['address'] < audit.MODULE[1]], [])

    def test_every_instruction_and_unreachable_word_accounted(self):
        addresses = set()
        for start, end in audit.ROUTINES:
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual([r['address'] for r in records],
                             [a for a in range(start, end, 4) if a not in audit.UNREACHABLE])
            self.assertEqual(records[-1]['assembly'], 'blr')
            addresses.update(range(start, end, 4))
        targets = {t for r in self.records for t in r['flows']}
        for address in audit.UNREACHABLE:
            self.assertEqual(self.word(address), 0x60000000)
            self.assertNotIn(address, self.by_address)
            self.assertNotIn(address, targets)
            self.assertIn(self.by_address[address - 4]['flow'], ('UNCONDITIONAL_JUMP', 'TERMINATOR'))
        for address in set(range(*audit.MODULE, 4)) - addresses:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)
        # The neighbours' padding: after HOST_COUNTRY's last routine and before the next unit's constructor.
        self.assertEqual(self.word(audit.MODULE[0] - 4), 0)
        self.assertNotIn(audit.MODULE[0] - 4, self.by_address)
        self.assertEqual([self.word(a) for a in range(audit.MODULE[1], NEXT_UNIT[0], 4)], [0, 0, 0])

    def test_inventory_gap_is_the_added_entries(self):
        inventory = ROOT / '.local/inventory/2026-11-18/inventory.json'
        if not inventory.exists():
            self.skipTest('the 2026-11-18 inventory is not kept')
        attributed = {int(r['entry'], 16) for r in json.loads(inventory.read_text())['routines']
                      if r['module'] == 'LEAGUE.CPP'}
        self.assertEqual(len(attributed), 60)
        self.assertEqual(set(ENTRIES) - attributed, ADDED)
        self.assertEqual(attributed - set(ENTRIES), set())

    def test_no_other_pointer_names_the_unit(self):
        code_end = self.segments[0][0] + len(self.segments[0][1])
        start, end = DESCRIPTORS
        for base, contents in self.segments:
            for offset in range(0, len(contents) - 3, 4):
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if audit.MODULE[0] <= target < audit.MODULE[1] and base + offset >= code_end:
                    # Only the unit's descriptors name it: no jump table or other vector.
                    self.assertTrue(start <= base + offset < end and (base + offset - start) % 8 == 0,
                                    hex(base + offset))

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[1]
            line = f'LEAGUE_BASE_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_league_base_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_league_base_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing LEAGUE.CPP instructions'):
                    audit.require_league_base_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)
            # A block's own .inc, included from the main list, is read too.
            block = Path(directory) / 'league_base_routines_block.inc'
            block.write_text(line)
            candidate.write_text('#include "league_base_routines_block.inc"\n')
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])


if __name__ == '__main__':
    unittest.main()
