"""Certify CUP.CPP's 47 entries, the unit's bounds and its production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest

import cup_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x10478f20, 0x10478f6c, 19, 'c9de01d6d1489f5a'), (0x10478f70, 0x10478fcc, 23, '1ad955f4dbb52c37'),
    (0x10478fd0, 0x1047905c, 35, '4e970eb5673e83f8'), (0x10479060, 0x104790ec, 35, '71a2a1c0bb6aeaba'),
    (0x104790f0, 0x104790fc, 3, '8040498c241f7ca8'), (0x10479100, 0x1047a9d4, 1581, 'b11039adefa29579'),
    (0x1047a9e0, 0x1047b354, 605, '21cb0f405ebbf5d7'), (0x1047b360, 0x1047b5a4, 145, '33b77b1651a945ae'),
    (0x1047b5b0, 0x1047b7d0, 136, '39fe8e4a7648cb25'), (0x1047b7d0, 0x1047ba48, 157, '45a26844176fc94a'),
    (0x1047ba50, 0x1047bba8, 84, 'b6bc7eb577cd6f92'), (0x1047bbb0, 0x1047bdd8, 138, 'a2c3e8df7f1b74c3'),
    (0x1047bde0, 0x1047bfa4, 111, '8fa1d8e64ba03d1a'), (0x1047bfb0, 0x1047e534, 2392, 'c350e3a30d31c8a2'),
    (0x1047e540, 0x1047e978, 269, 'ec2c1a3f4e54ee6a'), (0x1047e980, 0x1047ef48, 367, '3c58192e5aca444b'),
    (0x1047ef50, 0x1047f39c, 273, '1e6737fab40d9f04'), (0x1047f3a0, 0x1047fcf8, 597, '4290e87b00665fb9'),
    (0x1047fd00, 0x10480620, 584, 'dc5c12d3fc2455c6'), (0x10480620, 0x104810ac, 673, '3d46164fc59a54d3'),
    (0x104810b0, 0x10481ad0, 647, 'b7a444f47056cb5b'), (0x10481ad0, 0x10481e70, 231, '38f050121683b7ce'),
    (0x10481e70, 0x10481e88, 6, '00fd360c4c17f4a9'), (0x10481e90, 0x10482640, 491, 'f9ec63a8045484d7'),
    (0x10482640, 0x10482c98, 406, '9c585a2b903ab242'), (0x10482ca0, 0x10483194, 316, '9067961406f2d5e5'),
    (0x104831a0, 0x10484418, 1181, '693fe4c6cbf69830'), (0x10484420, 0x10484b8c, 473, '538ac11c688d7adc'),
    (0x10484b90, 0x10484e08, 158, 'f36aa01fcdf8d970'), (0x10484e10, 0x10484fcc, 111, '4f3aefeb565908a8'),
    (0x10484fd0, 0x10485334, 217, 'a738b2589a5db677'), (0x10485340, 0x10485404, 49, '94fa8bda601df0c8'),
    (0x10485410, 0x104854e4, 52, 'bc4386ea84044e6f'), (0x104854f0, 0x10485570, 32, '72485983f57e7423'),
    (0x10485570, 0x104855fc, 35, '847d2317fe4323c3'), (0x10485600, 0x10485830, 140, 'ccf3e76ae773ef60'),
    (0x10485830, 0x10485868, 14, 'b8aec5634b30f1aa'), (0x10485870, 0x104858a8, 14, '3d746d023267cc32'),
    (0x104858b0, 0x10485930, 32, '4338f07412f538ec'), (0x10485930, 0x10485ab0, 96, 'b4619df2967e1c1e'),
    (0x10485ab0, 0x10485b0c, 23, '68a8f013996c3dd7'), (0x10485b10, 0x10485d70, 151, '4d1a01bea49dc5b5'),
    (0x10485d70, 0x10486050, 183, '5b4e08465f8ba304'), (0x10486050, 0x10486094, 17, '721ec79a15072b2f'),
    (0x104860a0, 0x10486474, 245, '5ea2a43eeac98ee7'), (0x10486480, 0x104864c4, 17, 'a44de5a06b5e48d1'),
    (0x104864d0, 0x104866f0, 136, '08c5a44d5c24c1b8'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# The pilot block (2026-11-19), routed in production: the constructor, the destructor, the two "not
# overridden" assertion virtuals, the byte clear and the entrant query, and the three comparators.
PILOT = (0x10478f20, 0x10478f70, 0x10478fd0, 0x10479060, 0x104790f0, 0x10481e70, 0x10485830, 0x10485870,
         0x10486050)
# Calls from outside the unit, per entry: 1,248 sites, every one a `bl` of an entry.
INCOMING = {0x10478f20: 62, 0x10478f70: 60, 0x10479100: 1, 0x1047b5b0: 1, 0x1047bbb0: 1, 0x1047bfb0: 3,
            0x1047e980: 18, 0x1047ef50: 79, 0x1047f3a0: 2, 0x10482ca0: 13, 0x10484b90: 4, 0x104854f0: 454,
            0x10485570: 460, 0x10485600: 89, 0x104864d0: 1}
# Entries another routine of the unit calls.
CROSS = {0x1047a9e0, 0x1047ba50, 0x1047bde0, 0x1047e980, 0x1047ef50, 0x1047f3a0, 0x1047fd00, 0x10480620,
         0x104810b0, 0x10481ad0, 0x10481e90, 0x10484e10, 0x10484fd0, 0x10485340, 0x10485930, 0x10485d70,
         0x104860a0}
# The class's vtable (TOC -0x6184): its 40 words from +8, each a descriptor's code. Slots the cup does not override
# hold COMP.CPP's bodies (0x10450...-0x10453...), and 0x1006c1f0, 0x101b6ff0, 0x101b8af0, 0x101b75c0, 0x10070ba0
# are shared bodies of other units.
VTABLE = (0x10478f70, 0x1047bfb0, 0x104790f0, 0x10450900, 0x10485410, 0x10482ca0, 0x104831a0, 0x10484420,
          0x10450a40, 0x10450a30, 0x104509e0, 0x104509d0, 0x1047b7d0, 0x1006c1f0, 0x10450580, 0x10478fd0,
          0x1047b360, 0x1047b5b0, 0x10450030, 0x10451ac0, 0x10451c90, 0x104858b0, 0x10451d10, 0x10485b10,
          0x10451f50, 0x10453380, 0x10451f80, 0x10453390, 0x101b6ff0, 0x101b8af0, 0x101b75c0, 0x104530f0,
          0x10486050, 0x10486480, 0x10070ba0, 0x10479060, 0x10485ab0, 0x10479100, 0x1047e540, 0x10482640)
# The unit's descriptors, 0x108b9a4c..0x108b9b04: with the vtable and the TOC slots below, the only data words
# naming the unit.
DESCRIPTORS = (0x108b9a4c, 0x108b9b04)
# TOC slots with (loads, first load, last load), all inside the unit. -0x3764 holds a word itself.
PRIVATE_SLOTS = {
    '-0x47b8': (19, 0x10478fd8, 0x10485b38),  # the string pool, "CUP.CPP" at +0
    '-0x47bc': (5, 0x10479108, 0x104810c0),   # pool +0x4c0: 65535.0 and the int-to-double bias
    '-0x47c0': (2, 0x1047ff7c, 0x10480a24),   # 0x10481e70's descriptor
    '-0x47c4': (1, 0x1048481c, 0x1048481c),   # the word -1 before the pool
    '-0x47c8': (1, 0x104860e4, 0x104860e4), '-0x47cc': (1, 0x104860a8, 0x104860a8),
    '-0x6184': (2, 0x10478f38, 0x10478f90),   # the vtable
    '-0x3764': (5, 0x10484cec, 0x10484de8),
}
# The neighbouring units' slots on either side: (loads, first, last), all outside the unit.
NEIGHBOUR_SLOTS = {'-0x47dc': (2, 0x104782e8, 0x1047868c), '-0x47d0': (1, 0x10477d8c, 0x10477d8c),
                   '-0x47b4': (5, 0x10486860, 0x1048798c), '-0x47b0': (1, 0x10486848, 0x10486848),
                   '-0x6188': (56, 0x10463660, 0x10710bc8), '-0x6180': (1, 0x1048710c, 0x1048710c)}
# The entries the 2026-11-18 inventory did not attribute to the unit.
ADDED = {0x10478f20, 0x10478f70, 0x10486050, 0x10486480, 0x104864d0}
PREVIOUS_UNIT_LAST = (0x10478ea0, 0x10478f20)   # CONMEBOL_SEEDING.CPP's comparator
NEXT_UNIT = (0x104866f0, 0x10487ff0)            # FIFA_RANKINGS.CPP (tools/fifa_rankings_audit.py)
RETIREMENT_HOOK = 'if (inter_amer_cup_skip_daily(s)) return true;'


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private CUP.CPP export unavailable; inventory cannot be certified')


class CupAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        cls.program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes())
                        for s in cls.program['segments'] if 'file' in s]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            cls.records = routine_records(db, *audit.MODULE)
            cls.next_unit = routine_records(db, *NEXT_UNIT)
            cls.previous = routine_records(db, *PREVIOUS_UNIT_LAST)
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
        self.assertEqual(audit.MODULE, (0x10478f20, 0x104866f0))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.CUP_BASE_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 47)
        self.assertEqual(len(self.records), 13700)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_cup_base_index(db, complete=False)

    def test_listed_entries_are_pinned_and_routed_to_the_units_owner_alone(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        pinned = {a: (b, d) for a, b, _, d in CONTRACTS}
        listed = audit.rewritten(drafts=True)
        for entry, end, expected in listed:
            self.assertEqual(pinned[entry], (end, expected))
        production = {entry for entry, _, _ in audit.rewritten()}
        self.assertLessEqual(set(PILOT), production)
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ['cup_base_adapter'])
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_cup_base_index(db)
            if len(production) != len(ENTRIES):
                self.assertFalse(audit.COMPLETE)
                with self.assertRaisesRegex(ValueError, 'requires all 47'):
                    audit.require_cup_base_index(db, complete=True)

    def assert_hook(self, code, entry):
        hook = audit.cup_base_hook(entry)
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
            audit.require_cup_base_hooks(page, part, normal)
            self.assertIn('#include "cup_base_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.cup_base_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_cup_base_hooks(page, part, broken)

    def test_retirement_hook_stays_before_the_daily_routines_hook(self):
        from compile_recomp import generate_page
        entry = audit.RETIREMENT_ENTRY
        self.assertEqual(entry, 0x10482ca0)
        self.assertEqual(VTABLE[(0x1c - 8) // 4], entry)
        part = [r for r in self.records if r['address'] >> 12 == entry >> 12]
        normal = generate_page(entry >> 12, part, set(self.by_address), userops=self.program['userops'])
        fragment = normal.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertLess(fragment.index(RETIREMENT_HOOK), fragment.index(audit.cup_base_hook(entry)))
        swapped = normal.replace(RETIREMENT_HOOK, '@').replace(audit.cup_base_hook(entry), RETIREMENT_HOOK) \
                        .replace('@', audit.cup_base_hook(entry))
        with self.assertRaisesRegex(ValueError, 'must precede'):
            audit.require_cup_base_hooks(entry >> 12, part, swapped)

    def test_unit_bounds_are_its_pool_descriptors_vtable_slots_and_neighbours(self):
        pool = self.word(TOC - 0x47b8)
        self.assertEqual(pool, 0x10839e0c)
        self.assertEqual(self.original_bytes(pool, 8), b'CUP.CPP\0')
        self.assertEqual(self.original_bytes(pool + 0x2d, 22), b'CUP::map_fixture_tree ')
        self.assertEqual(self.word(TOC - 0x47bc), pool + 0x4c0)
        self.assertEqual(self.original_bytes(pool + 0x4c0, 16), bytes.fromhex('40efffe0000000004330000080000000'))
        self.assertEqual(self.word(TOC - 0x47c4), pool - 4)
        self.assertEqual(self.word(pool - 4), 0xffffffff)
        self.assertEqual(self.original_bytes(self.word(TOC - 0x47b4), 18), b'FIFA_RANKINGS.CPP\0')
        for slot, expected in PRIVATE_SLOTS.items():
            self.assertEqual(self.loads(slot), expected, slot)
            self.assertTrue(audit.MODULE[0] <= expected[1] <= expected[2] < audit.MODULE[1])
        for slot, expected in NEIGHBOUR_SLOTS.items():
            self.assertEqual(self.loads(slot), expected, slot)
            self.assertFalse(audit.MODULE[0] <= expected[1] < audit.MODULE[1])
            self.assertFalse(audit.MODULE[0] <= expected[2] < audit.MODULE[1])
        # Descriptors: one contiguous run of (code, TOC) pairs naming the unit, between the neighbours'.
        start, end = DESCRIPTORS
        named = [self.word(a) for a in range(start, end, 8)]
        self.assertEqual([self.word(a + 4) for a in range(start, end, 8)], [TOC] * len(named))
        self.assertEqual(len(named), 23)
        self.assertTrue(all(code in ENTRIES for code in named))
        self.assertEqual(self.word(start - 8), PREVIOUS_UNIT_LAST[0])  # CONMEBOL_SEEDING's comparator (slot -0x47dc)
        self.assertEqual(self.word(TOC - 0x47dc), start - 8)
        self.assertEqual(self.word(end), 0x10487fb0)                 # FIFA_RANKINGS's first descriptor
        self.assertFalse(audit.MODULE[0] <= self.word(end) < audit.MODULE[1])
        self.assertLessEqual({0x10478f70, 0x10486050, 0x10486480, 0x10481e70, 0x10485830, 0x10485870}, set(named))
        self.assertEqual(self.word(self.word(TOC - 0x47c0)), 0x10481e70)
        for slot, code in ((-0x60ec, 0x10485830), (-0x6128, 0x10485870)):
            self.assertEqual(self.word(self.word(TOC + slot)), code)
        # The vtable: the constructor and destructor store it, slot +8 is the destructor; its 40 slots end where
        # the next word is no descriptor.
        vtable = self.word(TOC - 0x6184)
        self.assertEqual(vtable, 0x108d7724)
        self.assertEqual((self.word(vtable), self.word(vtable + 4)), (0, 0))
        self.assertEqual(tuple(self.word(self.word(vtable + 8 + 4 * n)) for n in range(len(VTABLE))), VTABLE)
        self.assertFalse(0x108b0000 <= self.word(vtable + 8 + 4 * len(VTABLE)) < 0x108c0000)
        self.assertEqual(self.by_address[0x10478f38]['assembly'], 'lwz r5,-0x6184(r2)')
        self.assertEqual(self.by_address[0x10478f48]['assembly'], 'stw r5,0x0(r31)')
        self.assertEqual(self.by_address[0x10478f90]['assembly'], 'lwz r0,-0x6184(r2)')
        self.assertEqual(self.by_address[0x10478f34]['assembly'], 'bl 0x1044fed0')
        self.assertEqual(self.by_address[0x10478f4c]['assembly'], 'stb r4,0x43(r31)')
        self.assertEqual(self.by_address[0x10478f3c]['assembly'], 'li r4,0x1')
        # The previous unit ends with its comparator just before the constructor; the next starts with its own.
        self.assertEqual(self.previous[-1]['assembly'], 'blr')
        self.assertEqual(self.previous[-1]['address'], audit.MODULE[0] - 4)
        self.assertEqual(self.next_unit[0]['address'], NEXT_UNIT[0])
        self.assertEqual(self.next_unit[0]['assembly'], 'mfspr r0,LR')

    def test_trailing_routine_is_a_cup_method_and_the_next_unit_reads_no_cup_field(self):
        body = [r['assembly'] for r in self.records if 0x104864d0 <= r['address'] < 0x104866f0]
        self.assertIn('or r27,r3,r3', body)
        self.assertIn('lwz r4,0xa0(r3)', body)
        self.assertIn('lha r7,0x40(r27)', body)
        self.assertIn('mulli r3,r0,0x78', body)
        self.assertIn('bl 0x1048aea0', body)
        # Its one caller, FIX_MAN.CPP, tests the kind byte for 1 (what this unit's constructor stores) first.
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            caller = [json.loads(b)['assembly'] for _, b in db.execute(
                'SELECT address, body FROM instructions WHERE address>=? AND address<=? ORDER BY address',
                (0x104895ac, 0x104895c4))]
        self.assertEqual(caller[:3], ['lbz r0,0x43(r3)', 'cmpwi r0,0x1', 'bne 0x104895c8'])
        self.assertEqual(caller[-1], 'bl 0x104864d0')
        # The cup's word fields; FIFA_RANKINGS reads halfwords at +0xac of nations (0x10486d58), not these.
        field = re.compile(r'^(?:lwz|stw) r\d+,0x(a0|a4|ac)\(r(\d+)\)$')
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
        self.assertEqual(sum(incoming.values()), 1248)
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

    def test_every_entry_is_called_virtual_or_a_descriptor(self):
        start, end = DESCRIPTORS
        named = {self.word(a) for a in range(start, end, 8)}
        for entry in ENTRIES:
            self.assertTrue(entry in INCOMING or entry in CROSS or entry in VTABLE or entry in named, hex(entry))

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
        # The routines are contiguous up to the next unit's constructor.
        self.assertEqual(audit.ROUTINES[-1][1], NEXT_UNIT[0])

    def test_inventory_gap_is_the_added_entries(self):
        inventory = ROOT / '.local/inventory/2026-11-18/inventory.json'
        if not inventory.exists():
            self.skipTest('the 2026-11-18 inventory is not kept')
        attributed = {int(r['entry'], 16) for r in json.loads(inventory.read_text())['routines']
                      if r['module'] == 'CUP.CPP'}
        self.assertEqual(len(attributed), 42)
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
            line = f'CUP_BASE_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_cup_base_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_cup_base_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing CUP.CPP instructions'):
                    audit.require_cup_base_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)
            # A block's own .inc, included from the main list, is read too.
            block = Path(directory) / 'cup_base_routines_block.inc'
            block.write_text(line)
            candidate.write_text('#include "cup_base_routines_block.inc"\n')
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])


if __name__ == '__main__':
    unittest.main()
