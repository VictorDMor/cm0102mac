"""Certify PLAYER_REGEN.CPP's 47 entries, the unit's bounds and which of them are routed to C++ (the 27 of the blocks
regen_queries, candidate_score, regen_lists, club_placement and detach).

The private export is required evidence, rather than a reason to skip tests. Production ownership is asserted,
not skipped: exactly the routed entries are listed, owned and hooked, and the hook checker rejects a page that carries a
hook for an entry that is not hooked or lacks one for an entry that is.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest import mock

import player_regen_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
SLOT = re.compile(r'(-?0x[0-9a-f]+)\(r2\)')
CONTRACTS = (
    (0x102d8230, 0x102d85c0, 227, '68785b5e452dd485'), (0x102d85c0, 0x102d86b4, 61, '0ab10baefe69f0e7'),
    (0x102d86c0, 0x102d8830, 92, 'eb82736b3324d019'), (0x102d8830, 0x102d8918, 58, 'c4ee97119de1d7d0'),
    (0x102d8920, 0x102da630, 1860, 'f9140ee6e710b6cc'), (0x102da630, 0x102daba8, 350, 'b2cd027f36d6559a'),
    (0x102dabb0, 0x102dbe84, 1203, 'a1363a969717bb5c'), (0x102dbe90, 0x102dbea8, 6, 'ae1d60b15a642783'),
    (0x102dbeb0, 0x102dbef8, 18, '7f98dc43fcf99a5e'), (0x102dbf00, 0x102dbff8, 61, '9a4cb7a4297923cb'),
    (0x102dc000, 0x102dc404, 257, 'bb495027046db1b6'), (0x102dc410, 0x102dcb20, 452, '14e349132e6479a5'),
    (0x102dcb20, 0x102dd904, 889, '1b1c7171357985f0'), (0x102dd910, 0x102de3c0, 684, '46a515834be043f8'),
    (0x102de3c0, 0x102de3e4, 9, 'b99f774168f53592'), (0x102de3f0, 0x102de464, 28, '708369b8707a38e1'),
    (0x102de470, 0x102de6d4, 153, 'ac1beed6768e065b'), (0x102de6e0, 0x102dea48, 218, '603d1e9266164d03'),
    (0x102dea50, 0x102deb70, 72, '4eb907ddd90eb5b7'), (0x102deb70, 0x102dec48, 53, '15eb7bad40be6752'),
    (0x102dec50, 0x102ded84, 77, 'ec4d3ee3b7bcfb54'), (0x102ded90, 0x102dee68, 53, 'a1dfd6bc3b62bb8e'),
    (0x102dee70, 0x102df020, 108, '3279253b9def3e3b'), (0x102df020, 0x102df198, 94, 'bea88a303106bc09'),
    (0x102df1a0, 0x102df6cc, 331, 'ab9966a89350fdff'), (0x102df6d0, 0x102df778, 42, '94791078758a5507'),
    (0x102df780, 0x102dfbb4, 269, '358fc24a0e51ffd5'), (0x102dfbc0, 0x102e0084, 305, 'dbb33758b52e7ce9'),
    (0x102e0090, 0x102e0508, 286, '7cc1544c2c27d0c0'), (0x102e0510, 0x102e13c0, 940, '09a622103999baa2'),
    (0x102e13c0, 0x102e15a8, 122, 'e6e6492fa64eddf0'), (0x102e15b0, 0x102e1f18, 602, '0783374977edfab3'),
    (0x102e1f20, 0x102e21b4, 163, '4c884e4710c6abf7'), (0x102e21c0, 0x102e27d4, 388, '8697331f246f7112'),
    (0x102e27e0, 0x102e4524, 1872, '235a4c3adba11962'), (0x102e4530, 0x102e9894, 5336, '0f785dd74ccb94b6'),
    (0x102e98a0, 0x102e99e4, 80, 'be28e2a4632fb7d8'), (0x102e99f0, 0x102e9b88, 101, '64733c1c071fc30c'),
    (0x102e9b90, 0x102ea3b8, 522, 'e692be053bd483dc'), (0x102ea3c0, 0x102ea664, 167, '63ea8bfc47d5f062'),
    (0x102ea670, 0x102ea870, 128, '74d2983b2f864eac'), (0x102ea870, 0x102ead68, 318, 'b4fc2d5301f96b6c'),
    (0x102ead70, 0x102eb4d4, 473, '76a66e9fe4a91d49'), (0x102eb4e0, 0x102eb54c, 27, '17467a60446f9a8f'),
    (0x102eb550, 0x102eb610, 48, '986c2dbfe793f876'), (0x102eb610, 0x102eb670, 23, 'fbc7a2c6eca95e81'),
    (0x102eb670, 0x102eb6f8, 34, 'a76fe8bacf1ecd52'),
)
INCOMING = {0x102d8230: 2, 0x102d85c0: 1, 0x102d86c0: 1, 0x102d8920: 1, 0x102dabb0: 1, 0x102dbe90: 1, 0x102dbeb0: 2,
            0x102de3c0: 31, 0x102de3f0: 3, 0x102de470: 5, 0x102dea50: 2, 0x102dee70: 1, 0x102dfbc0: 1, 0x102e0510: 1,
            0x102e15b0: 1, 0x102ea870: 1, 0x102eb610: 1}
CROSS = {0x102d8830, 0x102da630, 0x102dbf00, 0x102dc000, 0x102dc410, 0x102dcb20, 0x102dd910, 0x102de3c0, 0x102de6e0,
         0x102dea50, 0x102deb70, 0x102dec50, 0x102ded90, 0x102dee70, 0x102df020, 0x102df1a0, 0x102df6d0, 0x102df780,
         0x102e0090, 0x102e13c0, 0x102e1f20, 0x102e21c0, 0x102e27e0, 0x102e4530, 0x102e98a0, 0x102e99f0, 0x102e9b90,
         0x102ea3c0, 0x102ea670, 0x102ead70, 0x102eb610, 0x102eb670}
ASSERT_LINES = (
    (0x102d8458, 127), (0x102d84bc, 131), (0x102d8520, 135), (0x102d8584, 139),
    (0x102dacc4, 878), (0x102deaf0, 2500), (0x102decec, 2570), (0x102def10, 2643),
    (0x102df21c, 2717), (0x102df2c0, 2732), (0x102df3b4, 2757), (0x102df63c, 2793),
    (0x102ea534, 6633), (0x102ea8d8, 6726), (0x102ea950, 6734), (0x102ea9c0, 6741),
    (0x102eaa40, 6748), (0x102eaab4, 6755), (0x102eab34, 6762), (0x102eaba8, 6769),
    (0x102eac28, 6776), (0x102eac9c, 6783), (0x102ead1c, 6790), (0x102eadd8, 6813),
    (0x102eae54, 6821), (0x102eaec8, 6828), (0x102eaf54, 6838), (0x102eafd8, 6846),
    (0x102eb054, 6855), (0x102eb0e0, 6865), (0x102eb164, 6873), (0x102eb1e0, 6882),
    (0x102eb26c, 6892), (0x102eb2f0, 6900), (0x102eb36c, 6909), (0x102eb3f8, 6919),
    (0x102eb47c, 6927),
)

ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# Calls from outside the unit, per entry: 56 sites, every one a `bl` of an entry.
# Entries another routine of the unit calls (0x102da630 also calls itself).
# The pool (TOC -0x50a0): the assertion preamble, then the name and the version format, then the unit's texts.
POOL = 0x107f63a4
NAME = 0x107f63db
POOL_USERS = (0x102d824c, 0x102ead7c)
# The unit's TOC slot block -0x50bc..-0x5098: 10 slots, every load inside the unit.
SLOT_BLOCK = (-0x50bc, -0x5098)
JUMP_SLOTS = {-0x50bc: 0x108ceb18, -0x50b8: 0x108cea98, -0x50b4: 0x108cead8, -0x50b0: 0x108cea10, -0x50ac: 0x108cea54}
JUMP_TABLES = (0x108cea10, 0x108ceb44)
JUMP_WORDS = {0x102e1f20: 34, 0x102e21c0: 32, 0x102e27e0: 11}
BCTR = {0x102e1f20: 2, 0x102e21c0: 2, 0x102e27e0: 1}
# The loader cluster's private slot (4 loads, all in the cluster) and PLAYER_SEARCH.CPP's first (one load).
LOADER_SLOT = ('-0x50c0', 0x107f57c4, (0x102d5b38, 0x102d5bb8, 0x102d5dc8, 0x102d67a4))
NEXT_UNIT_SLOT = ('-0x5094', 0x108ceb44, 0x10300114)
# The unit's two descriptors (the comparators) and their neighbours in the run: OFFICIALS_MANAGER.CPP's last, PLAYER_SEARCH.CPP's first.
DESCRIBED = (0x108b8db4, 0x102eb4e0), (0x108b8dbc, 0x102eb550)
DESCRIPTOR_SLOTS = {'-0x632c': (1, 0x102da614, 0x108b8db4, 0x102eb4e0), '-0x6328': (4, 0x102d8814, 0x108b8dbc, 0x102eb550)}
RUN_NEIGHBOURS = (0x108b8dac, 0x102d59e0), (0x108b8dc4, 0x102fe010)
# The previous unit's last assertion (OFFICIALS_MANAGER.CPP line 2657) and the next unit's first (PLAYER_SEARCH.CPP line 176).
PREVIOUS_LAST_LINE = (0x102d5aa8, 2657)
NEXT_FIRST_LINE = (0x102eb9b0, 176)
# The entries the 2026-11-21 inventory did not attribute to the unit, and those it did and the audit does not.
ADDED = {0x102eb4e0, 0x102eb550}
REMOVED = {0x102d5b30, 0x102d5bb0, 0x102d5dc0, 0x102d6770, 0x102d7dc0, 0x102d7e80, 0x102d81a0}
LOADER = tuple(a for a, *_ in audit.AMBIGUOUS_NEIGHBOURS[:7])
SEARCH_SIDE = tuple(a for a, *_ in audit.AMBIGUOUS_NEIGHBOURS[7:])
# The routed entries: block regen_queries (0x102de3c0, 0x102de3f0, 0x102dbe90, 0x102eb4e0, 0x102eb550, 0x102eb610), block
# candidate_score (0x102da630, 0x102dc410, 0x102dbf00), block regen_lists (0x102deb70, 0x102ded90, 0x102dbeb0, 0x102dea50,
# 0x102dec50, 0x102dee70, 0x102df020, 0x102df1a0, 0x102df6d0, 0x102eb670), block club_placement (0x102ea670, 0x102e9b90,
# 0x102e99f0, 0x102ea3c0, 0x102e98a0, 0x102dc000), block detach (0x102e1f20, 0x102e21c0), block retire_decision (0x102dcb20, 0x102dd910,
# 0x102de6e0, 0x102de470), block persistence (0x102ea870, 0x102ead70), block news_posting (0x102df780, 0x102e0090, 0x102e13c0) and
# block news_texts (0x102dfbc0, 0x102e0510, 0x102e15b0).
ROUTED = frozenset({0x102de3c0, 0x102de3f0, 0x102dbe90, 0x102eb4e0, 0x102eb550, 0x102eb610,
                    0x102da630, 0x102dc410, 0x102dbf00,
                    0x102deb70, 0x102ded90, 0x102dbeb0, 0x102dea50, 0x102dec50, 0x102dee70, 0x102df020, 0x102df1a0, 0x102df6d0, 0x102eb670,
                    0x102ea670, 0x102e9b90, 0x102e99f0, 0x102ea3c0, 0x102e98a0, 0x102dc000,
                    0x102e1f20, 0x102e21c0,
                    0x102dcb20, 0x102dd910, 0x102de6e0, 0x102de470,
                    0x102ea870, 0x102ead70,
                    0x102df780, 0x102e0090, 0x102e13c0, 0x102dfbc0, 0x102e0510, 0x102e15b0})
# The three audited units whose code loads a literal before their pool load only binary data and preamble pieces.
BINARY_BEFORE_POOL = {'CONTRACT_SCREENS.CPP', 'FORMATION.CPP', 'MANAGER_MANAGER.CPP'}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private PLAYER_REGEN.CPP export unavailable; inventory cannot be certified')


class PlayerRegenAuditTests(unittest.TestCase):
    slot_loads = None

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
        """Addresses of the instructions that use `slot(r2)`, in order (one scan of the export)."""
        if PlayerRegenAuditTests.slot_loads is None:
            found = {}
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                for address, body in db.execute("SELECT address, body FROM instructions WHERE body LIKE '%(r2)%' "
                                                'ORDER BY address'):
                    match = SLOT.search(json.loads(body)['assembly'])
                    if match:
                        found.setdefault(match.group(1), []).append(address)
            PlayerRegenAuditTests.slot_loads = found
        return PlayerRegenAuditTests.slot_loads.get(slot, [])

    def entry_of(self, address):
        return next((s for s, e in audit.ROUTINES if s <= address < e), None)

    def assertion_sites(self, start, end):
        """(call address, line) of each `bl 0x1077b720` (the assertion's sprintf) whose `li r8,line` precedes it."""
        found = []
        addresses = sorted(a for a in self.by_address if start <= a < end)
        for address in addresses:
            if self.by_address[address]['assembly'] != 'bl 0x1077b720':
                continue
            for back in range(address - 4, address - 80, -4):
                text = self.by_address.get(back, {}).get('assembly', '')
                if text.startswith('bl '):
                    break
                match = re.fullmatch(r'li r8,(-?0x[0-9a-f]+|-?\d+)', text)
                if match:
                    found.append((address, int(match.group(1), 0)))
                    break
        return found

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.MODULE, (0x102d8230, 0x102eb6f8))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.PLAYER_REGEN_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 47)
        self.assertEqual(len(self.records), 19660)
        self.assertEqual(sum(count for _, _, count, _ in CONTRACTS), 19660)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_player_regen_index(db, complete=False)

    def test_exactly_the_routed_blocks_are_routed_and_hooked_and_the_hook_checks_fail_when_they_should(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS, generate_page
        pinned = {a: (b, d) for a, b, _, d in CONTRACTS}
        self.assertFalse(audit.COMPLETE)
        listed = audit.rewritten(drafts=True)
        for entry, end, expected in listed:
            self.assertEqual(pinned[entry], (end, expected))
        self.assertEqual({entry for entry, _, _ in listed}, ROUTED)
        self.assertEqual({entry for entry, _, _ in audit.rewritten()}, ROUTED)
        self.assertEqual(set(audit.PLAYER_REGEN_HOOKED), ROUTED)
        for entry in ENTRIES:
            owners = [owner for at, owner in OWNER_REPLACEMENTS if at == entry]
            self.assertEqual(owners, [audit.OWNER] if entry in ROUTED else [], hex(entry))
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_player_regen_index(db)
            # The completeness check fails while an entry is not a production routine.
            with self.assertRaisesRegex(ValueError, 'requires all 47'):
                audit.require_player_regen_index(db, complete=True)
        selected = set(self.by_address)
        # Every page of the unit: its hooked entries carry the hook once, before the first step; the others none.
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_player_regen_hooks(page, part, normal)
            on_page = [entry for entry in ENTRIES if entry >> 12 == page]
            hooked = [entry for entry in on_page if entry in ROUTED]
            self.assertEqual('#include "player_regen_adapter.hpp"' in normal, bool(hooked), hex(page))
            for entry in on_page:
                hook = audit.player_regen_hook(entry)
                fragment = normal.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
                step = f's.step(0x{entry:x});'
                if entry not in ROUTED:
                    self.assertNotIn(hook, normal)
                    # An unrouted entry given a hook is rejected.
                    with self.assertRaisesRegex(ValueError, 'Unexpected'):
                        audit.require_player_regen_hooks(page, part, normal.replace(step, hook + '\n' + step, 1))
                    # A hooked entry (simulated) without its hook, or with it misplaced, is rejected.
                    with mock.patch.object(audit, 'PLAYER_REGEN_HOOKED', audit.PLAYER_REGEN_HOOKED + (entry,)):
                        with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                            audit.require_player_regen_hooks(page, part, normal)
                    continue
                self.assertEqual(normal.count(hook), 1)
                self.assertIn(hook, fragment)
                self.assertLess(fragment.index(hook), fragment.index(step))
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_player_regen_hooks(page, part, broken)
                # An entry that is not hooked is simulated: its hook is then unexpected.
                with mock.patch.object(audit, 'PLAYER_REGEN_HOOKED', tuple(e for e in audit.PLAYER_REGEN_HOOKED if e != entry)):
                    with self.assertRaisesRegex(ValueError, 'Unexpected'):
                        audit.require_player_regen_hooks(page, part, normal)

    def test_unit_bounds_are_its_pool_assertion_lines_slots_descriptors_and_neighbours(self):
        pool = self.word(TOC - 0x50a0)
        self.assertEqual(pool, POOL)
        self.assertEqual(self.original_bytes(NAME, 17), b'PLAYER_REGEN.CPP\0')
        self.assertEqual(NAME - pool, 0x37)
        self.assertEqual(self.original_bytes(NAME + 0x11, 13), b'v%s %s.%s %d\0')
        self.assertEqual(self.original_bytes(NAME + 0x1e, 6), b'Error\0')
        self.assertIn(b'Invalid staff_id for regen: %ld', self.original_bytes(pool, 0x200))
        self.assertIn(b'promoted from youth academy', self.original_bytes(pool, 0x200))
        # The preamble slots point at the preamble that precedes the name, the neighbours' pools at theirs.
        self.assertEqual(self.original_bytes(self.word(TOC - 0x5098), 17), b'\x10C 10:xrsqrfrtecp')
        self.assertEqual(self.original_bytes(self.word(TOC - 0x50cc) + 0x3f, 22), b'OFFICIALS_MANAGER.CPP\0')
        # The pool's thirteen users all lie inside the span, in thirteen routines, first and last as in the docstring.
        users = self.loads('-0x50a0')
        self.assertEqual((len(users), users[0], users[-1]), (13, *POOL_USERS))
        self.assertEqual(len({self.entry_of(a) for a in users}), 13)
        self.assertTrue(all(self.entry_of(a) for a in users))
        # The assertion lines (the sprintf of the version format, line in r8) rise with the address, from the first
        # entry to the last pool user; the neighbours' lines lie outside and restart.
        found = self.assertion_sites(*audit.MODULE)
        self.assertEqual([(a, v) for a, v in found if v is not None], list(ASSERT_LINES))
        lines = [v for _, v in ASSERT_LINES]
        self.assertEqual(lines, sorted(set(lines)))
        self.assertEqual((lines[0], lines[-1]), (127, 6927))
        self.assertEqual(self.entry_of(ASSERT_LINES[0][0]), 0x102d8230)
        self.assertEqual(self.entry_of(ASSERT_LINES[-1][0]), 0x102ead70)
        self.assertEqual(len(ASSERT_LINES), 37)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            previous = {r['address']: r['assembly'] for r in routine_records(db, 0x102d5a50, 0x102d5ab0)}
            following = {r['address']: r['assembly'] for r in routine_records(db, 0x102eb9b0, 0x102eba90)}
        self.assertEqual(previous[PREVIOUS_LAST_LINE[0]], f'li r8,0x{PREVIOUS_LAST_LINE[1]:x}')
        self.assertEqual(previous[0x102d5aac], 'bl 0x1077b720')
        self.assertTrue(any(t == f'li r8,0x{NEXT_FIRST_LINE[1]:x}' for t in following.values()))
        # The slot block: 10 slots, every load inside the unit, each slot loaded.
        slots = range(SLOT_BLOCK[0], SLOT_BLOCK[1] + 4, 4)
        self.assertEqual(len(slots), 10)
        for slot in slots:
            found_loads = self.loads(f'-0x{-slot:x}')
            self.assertTrue(found_loads, hex(slot))
            self.assertTrue(all(audit.MODULE[0] <= a < audit.MODULE[1] for a in found_loads), hex(slot))
        # Five of them name jump tables, which are the words 0x108cea10..0x108ceb43 in order.
        for slot, table in JUMP_SLOTS.items():
            self.assertEqual(self.word(TOC + slot), table)
            self.assertTrue(JUMP_TABLES[0] <= table < JUMP_TABLES[1])
        # The neighbours' slots: the loader group's private one (its string comes before this unit's preamble) and the next unit's.
        slot, target, expected = LOADER_SLOT
        found_loads = self.loads(slot)
        self.assertEqual(tuple(found_loads), expected)
        self.assertEqual(self.word(TOC + int(slot, 16)), target)
        self.assertEqual(self.original_bytes(target, 9), b'CM3_DATA\0')
        self.assertLess(target, self.word(TOC - 0x5098))
        self.assertTrue(all(self.entry_of(a) is None and any(s <= a < e for s, e, *_ in audit.AMBIGUOUS_NEIGHBOURS[:7])
                            for a in found_loads))
        slot, target, first = NEXT_UNIT_SLOT
        self.assertEqual(self.loads(slot), [first])
        self.assertEqual(self.word(TOC + int(slot, 16)), target)
        self.assertEqual(target, JUMP_TABLES[1])
        self.assertGreater(first, audit.MODULE[1])
        # The descriptors: the two comparators, between OFFICIALS_MANAGER.CPP's run and PLAYER_SEARCH.CPP's.
        for address, code in DESCRIBED:
            self.assertEqual(self.word(address), code)
            self.assertEqual(self.word(address + 4), TOC)
            self.assertIn(code, ENTRIES)
        # Their TOC slots are loaded only by this unit's routines.
        for slot, (count, first, address, code) in DESCRIPTOR_SLOTS.items():
            found_loads = self.loads(slot)
            self.assertEqual((len(found_loads), found_loads[0]), (count, first), slot)
            self.assertTrue(all(self.entry_of(a) for a in found_loads), slot)
            self.assertEqual(self.word(TOC + int(slot, 16)), address)
            self.assertEqual(self.word(address), code)
        for address, code in RUN_NEIGHBOURS:
            self.assertEqual((self.word(address), self.word(address + 4)), (code, TOC))
            self.assertNotIn(code, ENTRIES)
        self.assertEqual(RUN_NEIGHBOURS[0][0] + 8, DESCRIBED[0][0])
        self.assertEqual(DESCRIBED[1][0] + 8, RUN_NEIGHBOURS[1][0])
        # The first entry and the last are routine starts and ends; the helpers after the comparators are only called by this unit.
        self.assertEqual(self.by_address[0x102d8230]['assembly'], 'mfspr r0,LR')
        self.assertEqual(self.by_address[0x102eb6f4]['assembly'], 'blr')

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
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL', hex(address))
                        self.assertIn(target, ENTRIES)
                        incoming[target] = incoming.get(target, 0) + 1
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*audit.MODULE, *audit.MODULE)))
        self.assertEqual(incoming, INCOMING)
        self.assertEqual(sum(incoming.values()), 56)
        self.assertEqual(fallthroughs, [])

    def test_cross_edges_are_calls_of_entries_jumps_stay_inside_and_tables_are_labels(self):
        cross = set()
        bctr = {}
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                targets = record['flows'] + ([record['fallthrough']] if record.get('fallthrough') is not None else [])
                if record['assembly'] == 'bctr':
                    bctr[start] = bctr.get(start, 0) + 1
                    self.assertTrue(all(start <= t < end for t in record['flows']))
                else:
                    self.assertNotIn('COMPUTED', record['flow'], hex(record['address']))
                for target in targets:
                    if not start <= target < end:
                        # No tail call and no branch out of a routine: only calls leave it.
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        if audit.MODULE[0] <= target < audit.MODULE[1]:
                            self.assertIn(target, ENTRIES)
                            cross.add(target)
        self.assertEqual(cross, CROSS)
        self.assertEqual(bctr, BCTR)
        self.assertEqual([s for s in self.program['symbols']
                          if audit.MODULE[0] <= s['address'] < audit.MODULE[1]], [])
        # The 77 words of the five tables, one table per `bctr` slot, name the inside of the three routines.
        words = {}
        for address in range(*JUMP_TABLES, 4):
            entry = self.entry_of(self.word(address))
            self.assertIsNotNone(entry, hex(address))
            self.assertNotIn(self.word(address), ENTRIES)
            words[entry] = words.get(entry, 0) + 1
        self.assertEqual(words, JUMP_WORDS)
        self.assertEqual(sum(JUMP_WORDS.values()), 77)
        self.assertEqual(sum(BCTR.values()), len(JUMP_SLOTS))

    def test_every_entry_is_called_by_a_descriptor_or_another_entry_or_a_caller_outside(self):
        named = {code for _, code in DESCRIBED}
        for entry in ENTRIES:
            self.assertTrue(entry in INCOMING or entry in CROSS or entry in named, hex(entry))

    def test_every_instruction_and_unreachable_word_accounted(self):
        addresses = set()
        for start, end in audit.ROUTINES:
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual([r['address'] for r in records],
                             [a for a in range(start, end, 4) if a not in audit.UNREACHABLE])
            self.assertEqual(records[-1]['assembly'], 'blr')
            addresses.update(range(start, end, 4))
        targets = {t for r in self.records for t in r['flows']}
        self.assertEqual(len(audit.UNREACHABLE), 17)
        for address in audit.UNREACHABLE:
            self.assertEqual(self.word(address), 0x60000000)
            self.assertNotIn(address, self.by_address)
            self.assertNotIn(address, targets)
            self.assertIn(self.by_address[address - 4]['flow'], ('UNCONDITIONAL_JUMP', 'TERMINATOR'))
        # Between routines only zero padding.
        for address in set(range(*audit.MODULE, 4)) - addresses:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)

    def test_no_other_pointer_names_the_unit(self):
        code_end = self.segments[0][0] + len(self.segments[0][1])
        found = {}
        for base, contents in self.segments:
            for offset in range(0, len(contents) - 3, 4):
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                address = base + offset
                if audit.MODULE[0] <= target < audit.MODULE[1] and address >= code_end:
                    if JUMP_TABLES[0] <= address < JUMP_TABLES[1]:
                        continue
                    found[address] = target
        self.assertEqual(found, {address: code for address, code in DESCRIBED})

    def test_inventory_gap_is_the_added_and_removed_entries(self):
        inventory = ROOT / '.local/inventory/2026-11-21/inventory.json'
        if not inventory.exists():
            self.skipTest('the 2026-11-21 inventory is not kept')
        attributed = {int(r['entry'], 16) for r in json.loads(inventory.read_text())['routines']
                      if r['module'] == 'PLAYER_REGEN.CPP'}
        self.assertEqual(len(attributed), 52)
        self.assertEqual(set(ENTRIES) - attributed, ADDED)
        self.assertEqual(attributed - set(ENTRIES), REMOVED)
        self.assertEqual(REMOVED, set(LOADER))

    def test_the_neighbouring_routines_are_pinned_but_not_entries(self):
        self.assertEqual(len(audit.AMBIGUOUS_NEIGHBOURS), 11)
        self.assertEqual(sum(count for _, _, count, _ in audit.AMBIGUOUS_NEIGHBOURS[:7]), 2486)
        self.assertEqual(sum(count for _, _, count, _ in audit.AMBIGUOUS_NEIGHBOURS[7:]), 169)
        calls_into = {}
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for entry, end, count, expected in audit.AMBIGUOUS_NEIGHBOURS:
                self.assertNotIn(entry, ENTRIES)
                body = routine_records(db, entry, end)
                self.assertEqual((len(body), digest(body)), (count, expected), hex(entry))
                self.assertIn(body[-1]['flow'], ('TERMINATOR', 'UNCONDITIONAL_JUMP'), hex(entry))
                for record in body:
                    if record['flow'] == 'UNCONDITIONAL_CALL':
                        calls_into.setdefault(entry, set()).update(record['flows'])
                    self.assertFalse(any(audit.MODULE[0] <= t < audit.MODULE[1] for t in record['flows']
                                         if record['flow'] != 'UNCONDITIONAL_CALL'))
        # The loader group is closed: its helpers are called only from inside it; its way in is 0x102d5b30; it
        # asserts nothing and calls this unit's 0x102dee70; the ends lie between OFFICIALS_MANAGER.CPP's last assertion and our first.
        self.assertLess(0x102d5a50, LOADER[0])
        self.assertLess(LOADER[-1], audit.MODULE[0])
        callers = {}
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for entry in LOADER:
                callers[entry] = [a for a, in db.execute(
                    "SELECT i.address FROM instructions i,json_each(i.body,'$.flows') f WHERE f.value=? "
                    "AND json_extract(i.body,'$.flow')='UNCONDITIONAL_CALL' ORDER BY i.address", (entry,))]
        for entry in LOADER[1:]:
            self.assertTrue(callers[entry], hex(entry))
            self.assertTrue(all(any(s <= a < e for s, e, *_ in audit.AMBIGUOUS_NEIGHBOURS[:7]) for a in callers[entry]), hex(entry))
        self.assertTrue(any(not any(s <= a < e for s, e, *_ in audit.AMBIGUOUS_NEIGHBOURS[:7]) for a in callers[LOADER[0]]))
        self.assertIn(0x102dee70, calls_into[0x102d6770])
        self.assertEqual(self.assertion_sites(LOADER[0], audit.MODULE[0]), [(a, v) for a, v in
                         self.assertion_sites(LOADER[0], audit.MODULE[0]) if v is not None])
        self.assertEqual([r for r in self.assertion_sites(LOADER[0], audit.MODULE[0]) if r[1] == 0], [])
        # The search side: the four are named by descriptors 0x108b8e04..0x108b8e1c, whose slots only other units load.
        run = {self.word(a): a for a in range(0x108b8e04, 0x108b8e24, 8)}
        for entry in SEARCH_SIDE:
            self.assertIn(entry, run)
        for slot in ('-0x6304', '-0x6300'):
            found_loads = self.loads(slot)
            self.assertTrue(found_loads)
            self.assertFalse(any(audit.MODULE[0] <= a < audit.MODULE[1] for a in found_loads), slot)
        self.assertGreater(SEARCH_SIDE[0], audit.MODULE[1])
        self.assertEqual(SEARCH_SIDE[-1] + 0x80, NEXT_FIRST_LINE[0])

    def test_audited_units_keep_every_text_string_after_their_pool(self):
        """The rule behind the loader group's exclusion: a unit's code never loads a text string that lies before its
        own pool, in the audited units that keep their pool in the literal area (three load binary tables there)."""
        import game_inventory as gi
        export = gi.Export(EXPORT)
        code = next(data for start, data in export.segments if start == gi.CODE_START)
        pools = gi.source_pools(export)
        by_name = {}
        for offset, names in pools.items():
            if len(names) == 1:
                by_name.setdefault(names[0], []).append(offset)
        checked, odd = 0, set()
        for start, end, name in gi.audited_modules():
            if name not in by_name:
                continue
            base = export.word((gi.TOC + by_name[name][0]) & 0xFFFFFFFF)
            if not gi.CODE_END <= base < 0x10810000:
                continue
            checked += 1
            for address in range(start, end, 4):
                match = SLOT.search(export.assembly.get(address) or '')
                if not match or not export.assembly[address].startswith('lwz '):
                    continue
                value = export.word((gi.TOC + int(match.group(1), 0)) & 0xFFFFFFFF)
                if value and gi.CODE_END <= value < base - 0x40:
                    text = code[value - gi.CODE_START:value - gi.CODE_START + 4]
                    if re.fullmatch(rb'[\x20-\x7e]{3}[\x20-\x7e\x00]', text):
                        odd.add(name)
        self.assertGreaterEqual(checked, 24)
        self.assertEqual(odd, set())
        # CM3_DATA, a text string, is before this unit's pool, so the group that loads it is not this unit's.
        self.assertLess(LOADER_SLOT[1], POOL - 0x40)

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[10]
            line = f'PLAYER_REGEN_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_player_regen_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_player_regen_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing PLAYER_REGEN.CPP instructions'):
                    audit.require_player_regen_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)
            # A block's own .inc, included from the main list, is read too.
            block = Path(directory) / 'player_regen_routines_block.inc'
            block.write_text(line)
            candidate.write_text('#include "player_regen_routines_block.inc"\n')
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])


if __name__ == '__main__':
    unittest.main()
