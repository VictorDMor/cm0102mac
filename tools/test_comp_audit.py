"""Certify COMP.CPP's 53 entries, the unit's bounds and its production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import json
from pathlib import Path
import sqlite3
import tempfile
import unittest

import comp_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x1044fed0, 0x1044ff58, 34, 'fa83dcbcc185cd2b'), (0x1044ff60, 0x1044ffdc, 31, '70fe51b6c85acf52'),
    (0x1044ffe0, 0x1044ffe8, 2, '9ede408bdfe5a611'), (0x1044fff0, 0x1044fff8, 2, '20fdf3fea3768977'),
    (0x10450000, 0x10450004, 1, '689ecda2bae7e35e'), (0x10450010, 0x10450018, 2, 'e287e26e5e67809e'),
    (0x10450020, 0x10450028, 2, '3cbd448fcdea48df'), (0x10450030, 0x10450578, 338, 'acff56618904b41b'),
    (0x10450580, 0x104506fc, 95, '5193e361dafdb36e'), (0x10450700, 0x10450704, 1, '88dbdcab3538ce51'),
    (0x10450710, 0x104508f4, 118, 'fe5eee0c3c6b2503'), (0x10450900, 0x104509c0, 48, 'a9358a4db6e4eb3b'),
    (0x104509c0, 0x104509c8, 2, 'b3433c5754b1225a'), (0x104509d0, 0x104509d8, 2, 'b7568200b1e2c1a7'),
    (0x104509e0, 0x104509e4, 1, 'f1deb3d12dd90992'), (0x104509f0, 0x104509f8, 2, '3bc34d57aea8e9d2'),
    (0x10450a00, 0x10450a08, 2, '145cf4adec45baa9'), (0x10450a10, 0x10450a14, 1, '4119de576746e960'),
    (0x10450a20, 0x10450a28, 2, 'b671f92e8dcb8c2e'), (0x10450a30, 0x10450a34, 1, '27b7268d05043624'),
    (0x10450a40, 0x10450a48, 2, 'fff598b06ffd4b9c'), (0x10450a50, 0x10450b60, 66, '6734f52690a06b73'),
    (0x10450b60, 0x10450c1c, 47, 'cd7fdda32069f203'), (0x10450c20, 0x10450da4, 96, '5545c4c179c392d5'),
    (0x10450db0, 0x10450ed8, 74, 'dc27c06a57d935ba'), (0x10450ee0, 0x10451470, 356, '2eb7fd6b33b09aa5'),
    (0x10451470, 0x10451570, 64, 'c4ad7f0959154a78'), (0x10451570, 0x1045171c, 107, '82a57e785caa09c4'),
    (0x10451720, 0x104517f4, 52, '5b94bed0fc98b9fb'), (0x10451800, 0x10451990, 98, '2376315e4051e176'),
    (0x10451990, 0x10451a2c, 38, '7a62f42fa70ca923'), (0x10451a30, 0x10451ab4, 32, 'b72713e66e30f2a1'),
    (0x10451ac0, 0x10451c90, 115, '8091e43a03d7a015'), (0x10451c90, 0x10451c98, 2, '39273133149bb5b8'),
    (0x10451ca0, 0x10451cfc, 23, '47abc7e19edb948d'), (0x10451d00, 0x10451d08, 2, '9b9400dc37943eee'),
    (0x10451d10, 0x10451f48, 140, '9d2d34e2414ba46d'), (0x10451f50, 0x10451f80, 12, 'dae5ce56a564144b'),
    (0x10451f80, 0x10452010, 36, '3785bbef246de496'), (0x10452010, 0x10452284, 157, '06de1f9511b62974'),
    (0x10452290, 0x104527c0, 332, '872ee469e8dca215'), (0x104527c0, 0x10452880, 48, 'c598e620a5de57b8'),
    (0x10452880, 0x10452a58, 118, '917e94e4648ba0dc'), (0x10452a60, 0x10452bac, 83, '18b4f0e4e9540af5'),
    (0x10452bb0, 0x10452ec4, 197, '3ecb513472a5ce90'), (0x10452ed0, 0x104530e4, 132, '03d4d236f5338b72'),
    (0x104530f0, 0x10453184, 37, 'aa1f1804b9755bb5'), (0x10453190, 0x104532b8, 74, 'da1706ee5f7cd1ba'),
    (0x104532c0, 0x104532f4, 13, 'e776dd81df45e611'), (0x10453300, 0x10453364, 25, '50d2cbcc624518cf'),
    (0x10453370, 0x10453374, 1, 'b47420bb1ff391d9'), (0x10453380, 0x10453388, 2, '732e6e4d3da0654e'),
    (0x10453390, 0x10453394, 1, '72e2fb2115ecb3de'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# The pilot block (2026-11-18), routed in production: the constructor, the destructor, the twenty
# one-instruction virtuals, the stage accessor, the nation-region leaf and the two Argentine comparators.
PILOT = (0x1044fed0, 0x1044ff60, 0x1044ffe0, 0x1044fff0, 0x10450000, 0x10450010, 0x10450020, 0x10450700,
         0x104509c0, 0x104509d0, 0x104509e0, 0x104509f0, 0x10450a00, 0x10450a10, 0x10450a20, 0x10450a30,
         0x10450a40, 0x10450b60, 0x10451a30, 0x10451c90, 0x10451d00, 0x104532c0, 0x10453300, 0x10453370,
         0x10453380, 0x10453390)
# Calls from outside the unit, per entry: 222 sites, every one a `bl` of an entry.
INCOMING = {0x1044fed0: 8, 0x1044ff60: 8, 0x10450030: 103, 0x10450710: 1, 0x10450a50: 1, 0x10450b60: 15,
            0x10450c20: 1, 0x10450db0: 3, 0x10450ee0: 3, 0x10451570: 7, 0x10451720: 1, 0x10451800: 16,
            0x10451990: 5, 0x10451a30: 2, 0x10451ca0: 2, 0x10451d10: 1, 0x10451f80: 1, 0x10452010: 13,
            0x10452290: 1, 0x104527c0: 1, 0x10452880: 1, 0x10452a60: 9, 0x10452bb0: 1, 0x10452ed0: 14,
            0x10453190: 4}
# Entries another routine of the unit calls.
CROSS = {0x10450b60, 0x10450c20, 0x10451990}
# The class's vtable (TOC -0x619c): its words from +8, each a descriptor's code; outside the unit
# 0x1006c1f0, 0x101b6ff0, 0x101b8af0 and 0x101b75c0 (inherited or shared bodies).
VTABLE = (0x1044ff60, 0x1044ffe0, 0x10450000, 0x10450900, 0x104509c0, 0x104509f0, 0x10450a20, 0x10450a10,
          0x10450a40, 0x10450a30, 0x104509e0, 0x104509d0, 0x10450700, 0x1006c1f0, 0x10450580, 0x1044fff0,
          0x10450010, 0x10450020, 0x10450030, 0x10451ac0, 0x10451c90, 0x10451d00, 0x10451d10, 0x10453370,
          0x10451f50, 0x10453380, 0x10451f80, 0x10453390, 0x101b6ff0, 0x101b8af0, 0x101b75c0, 0x104530f0)
# The unit's descriptors, 0x108b98ec..0x108b99e4: the only data words naming the unit.
DESCRIPTORS = (0x108b98ec, 0x108b99ec)
# TOC slots with (loads, first load, last load).
PRIVATE_SLOTS = {
    '-0x49c0': (13, 0x1045003c, 0x10452bc8),  # the string pool, "COMP.CPP" at +0x380
    '-0x61a0': (1, 0x10451450, 0x10451450),   # 0x10451470's descriptor, passed to the sort by 0x10450ee0
}
# Slots this unit loads first and others later.
SHARED_SLOTS = {
    '-0x619c': (3, 0x1044fed0, 0x1049e838),   # the vtable; 0x1049e838 is LEAGUE.CPP's inline copy constructor
    '-0x61a4': (3, 0x10453300, 0x1050ddf4),   # the sort column byte; the Argentine units set it
}
# The neighbouring units' slots on either side: (loads, first, last), all outside the unit.
NEIGHBOUR_SLOTS = {'-0x49e0': (9, 0x1044f2bc, 0x1044fc8c), '-0x61a8': (2, 0x1044cb14, 0x1044cc00),
                   '-0x6198': (3, 0x10459cbc, 0x1046164c)}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private COMP.CPP export unavailable; inventory cannot be certified')


class CompAuditTests(unittest.TestCase):
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

    def loads(self, slot):
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            found = [a for a, in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                            (f'%{slot}(r2)%',))]
        return len(found), found[0], found[-1]

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.MODULE, (0x1044fed0, 0x10453394))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.COMP_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 53)
        self.assertEqual(len(self.records), 3269)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_comp_index(db, complete=False)

    def test_listed_entries_are_pinned_and_routed_to_the_units_owner_alone(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        pinned = {a: (b, d) for a, b, _, d in CONTRACTS}
        listed = audit.rewritten(drafts=True)
        for entry, end, expected in listed:
            self.assertEqual(pinned[entry], (end, expected))
        production = {entry for entry, _, _ in audit.rewritten()}
        self.assertLessEqual(set(PILOT), production)
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ['comp_adapter'])
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_comp_index(db)
            if len(production) != len(ENTRIES):
                with self.assertRaisesRegex(ValueError, 'requires all 53'):
                    audit.require_comp_index(db, complete=True)

    def assert_hook(self, code, entry):
        hook = audit.comp_hook(entry)
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
            audit.require_comp_hooks(page, part, normal)
            self.assertIn('#include "comp_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.comp_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_comp_hooks(page, part, broken)

    def test_unit_bounds_are_its_pool_descriptors_vtable_and_neighbours(self):
        pool = self.word(TOC - 0x49c0)
        self.assertEqual(pool, 0x1082de80)
        self.assertEqual(self.original_bytes(pool + 0x380, 9), b'COMP.CPP\0')
        self.assertTrue(self.original_bytes(self.word(TOC - 0x4984), 0x100).find(b'COMP_STATS.CPP') >= 0)
        for slot, expected in PRIVATE_SLOTS.items():
            self.assertEqual(self.loads(slot), expected, slot)
            self.assertTrue(audit.MODULE[0] <= expected[1] <= expected[2] < audit.MODULE[1])
        for slot, expected in SHARED_SLOTS.items():
            self.assertEqual(self.loads(slot), expected, slot)
            self.assertTrue(audit.MODULE[0] <= expected[1] < audit.MODULE[1])
        for slot, expected in NEIGHBOUR_SLOTS.items():
            self.assertEqual(self.loads(slot), expected, slot)
            self.assertFalse(audit.MODULE[0] <= expected[1] < audit.MODULE[1])
            self.assertFalse(audit.MODULE[0] <= expected[2] < audit.MODULE[1])
        # The pool span: the first and last routines that load the pool.
        self.assertTrue(self.by_address[0x1045003c]['assembly'].endswith('-0x49c0(r2)'))
        self.assertTrue(self.by_address[0x10452bc8]['assembly'].endswith('-0x49c0(r2)'))
        # Descriptors: one contiguous run of (code, TOC) pairs naming the unit, between the neighbours'.
        start, end = DESCRIPTORS
        named = [self.word(a) for a in range(start, end, 8)]
        self.assertEqual([self.word(a + 4) for a in range(start, end, 8)], [TOC] * len(named))
        self.assertEqual(len(named), 32)
        self.assertTrue(all(code in ENTRIES for code in named))
        self.assertFalse(audit.MODULE[0] <= self.word(start - 8) < audit.MODULE[1])
        self.assertFalse(audit.MODULE[0] <= self.word(end) < audit.MODULE[1])
        boundary = {0x1044ff60, 0x1044ffe0, 0x1044fff0, 0x10450000, 0x10450010, 0x10450020, 0x104530f0,
                    0x104532c0, 0x10453300, 0x10453370, 0x10453380, 0x10453390}
        self.assertLessEqual(boundary, set(named))
        # The vtable: the constructor stores it, slot 0 is the destructor.
        vtable = self.word(TOC - 0x619c)
        self.assertEqual(vtable, 0x108cf608)
        self.assertEqual((self.word(vtable), self.word(vtable + 4)), (0, 0))
        self.assertEqual(tuple(self.word(self.word(vtable + 8 + 4 * n)) for n in range(len(VTABLE))), VTABLE)
        self.assertEqual(self.by_address[0x1044fed0]['assembly'], 'lwz r0,-0x619c(r2)')
        self.assertEqual(self.by_address[0x1044fee0]['assembly'], 'stw r0,0x0(r3)')
        self.assertEqual(self.by_address[0x1044ff80]['assembly'], 'lwz r0,-0x619c(r2)')
        # The comparators' vectors are taken by the Argentine units.
        self.assertEqual(self.word(self.word(TOC - 0x6060)), 0x104532c0)
        self.assertEqual(self.word(self.word(TOC - 0x6058)), 0x10453300)
        self.assertEqual(self.word(self.word(TOC - 0x61a0)), 0x10451470)

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
        self.assertEqual(sum(incoming.values()), 222)
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
        # The neighbours' padding: the previous unit's last word and COMP_STATS.CPP's first routine.
        self.assertEqual(self.word(audit.MODULE[0] - 4), 0)
        self.assertNotIn(audit.MODULE[0] - 4, self.by_address)
        self.assertEqual(self.word(0x10453394), 0)
        self.assertEqual(self.word(0x10453398), 0)

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
            line = f'COMP_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_comp_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_comp_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing COMP.CPP instructions'):
                    audit.require_comp_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)
            # A block's own .inc, included from the main list, is read too.
            block = Path(directory) / 'comp_routines_block.inc'
            block.write_text(line)
            candidate.write_text('#include "comp_routines_block.inc"\n')
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])


if __name__ == '__main__':
    unittest.main()
