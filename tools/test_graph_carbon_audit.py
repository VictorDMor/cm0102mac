"""Certify graph_carbon.cpp's 55 routines (the game's drawing layer over the Mac: tools/graph_carbon_audit.py): bounds, the string
pool, the TOC slots and small data, the descriptors and jump tables, the callers from outside, the routines and Toolbox imports it
calls, the native drawing observation points, and which entries are routed to C++.

The private export is required evidence, rather than a reason to skip tests. Production ownership is asserted, not skipped:
exactly the routed entries are listed, owned and hooked, and the hook checker rejects a page that carries a hook for an entry
that is not hooked or lacks one for an entry that is.
"""
from contextlib import closing
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

import engine_initialization_audit as initializers
import font_cpp_audit as font_cpp
import graph_carbon_audit as audit
import main_program_audit as main_program
import qsort4x_audit as qsort
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CODE_END = 0x10781330
GLUE = (0x1077fad8, CODE_END)
CONTRACTS = (
(0x10768810, 0x10768818, 2, 'b9902cc980c9bd4d'),
(0x10768820, 0x10768990, 92, '61c96a73c17b0c4c'),
(0x10768990, 0x107689dc, 19, '44ebb83e38ded294'),
(0x107689e0, 0x10768b38, 86, '6f3f1c89ccb1d216'),
(0x10768b40, 0x10768c4c, 67, '9cf8cd2ec4d71bc7'),
(0x10768c50, 0x10768c84, 13, '91c36aead22a2351'),
(0x10768c90, 0x10768dc0, 76, '178f3da5679a29ce'),
(0x10768dc0, 0x10768e04, 17, '36c671a372db0452'),
(0x10768e10, 0x107690e4, 181, '28537d531d132b77'),
(0x107690f0, 0x107691a4, 45, 'd17c2adef0ffa3b9'),
(0x107691b0, 0x107692e4, 77, '0307d148a78c1be6'),
(0x107692f0, 0x10769460, 92, '1b05a9f008c23cca'),
(0x10769460, 0x107697b4, 213, '881fa9aa7ebf4ccf'),
(0x107697c0, 0x107698c8, 66, 'e61581f5f79439b5'),
(0x107698d0, 0x1076991c, 19, 'ee2787a784de2f12'),
(0x10769920, 0x10769980, 24, 'e7a36124f47d281e'),
(0x10769980, 0x10769b4c, 115, '1eec4bbbbb7b148f'),
(0x10769b50, 0x10769b5c, 3, '64f4cd09be131057'),
(0x10769b60, 0x1076a140, 375, '6c4d68e0be4bb501'),
(0x1076a140, 0x1076a574, 269, '6a01d94fc10f5354'),
(0x1076a580, 0x1076a788, 130, 'bd277c818bf68a33'),
(0x1076a790, 0x1076a8ec, 87, 'bc1e5a50f43e18ca'),
(0x1076a8f0, 0x1076ad50, 280, 'e96c24745594dbe4'),
(0x1076ad50, 0x1076b018, 176, '43a01da42fd3602b'),
(0x1076b020, 0x1076b13c, 70, '3903ee95792fd27a'),
(0x1076b140, 0x1076b238, 62, '083cc089b8461f76'),
(0x1076b240, 0x1076b2e8, 42, '45881a05f2c66910'),
(0x1076b2f0, 0x1076b450, 88, '3b7f8a0dc9b6a2f6'),
(0x1076b450, 0x1076b6d4, 161, '35dfe2b129351d54'),
(0x1076b6e0, 0x1076b7dc, 63, 'ac6755dbc5dfb28c'),
(0x1076b7e0, 0x1076b824, 17, '95103b089ab2b420'),
(0x1076b830, 0x1076bb14, 185, 'f31f29ddd3259622'),
(0x1076bb20, 0x1076bb34, 5, '0add5158886b3c2f'),
(0x1076bb40, 0x1076bbe8, 42, 'aeaebf792535f274'),
(0x1076bbf0, 0x1076c010, 264, 'a64b4045fbc56640'),
(0x1076c010, 0x1076c04c, 15, '47698214f9ee72ac'),
(0x1076c050, 0x1076c0a4, 21, 'a3bfa2b2a923423e'),
(0x1076c0b0, 0x1076c974, 561, '90d0ff207d06b72f'),
(0x1076c980, 0x1076ca18, 38, 'ce454977b33b0dfb'),
(0x1076ca20, 0x1076ca4c, 11, '1c46c47760272e82'),
(0x1076ca50, 0x1076d198, 466, '550366aed59dd02f'),
(0x1076d1a0, 0x1076d1e0, 16, '17f2f0ee5a9cfecd'),
(0x1076d1e0, 0x1076d470, 164, '724899b89b67ad5d'),
(0x1076d470, 0x1076d598, 74, '4347141db3022171'),
(0x1076d5a0, 0x1076d6e0, 80, '281ed6eaf553cb81'),
(0x1076d6e0, 0x1076e158, 670, '679813c80923d8bb'),
(0x1076e160, 0x1076e5e4, 289, 'dee94778e4147ffc'),
(0x1076e5f0, 0x1076e7c0, 116, '7a0c8ac972bb7227'),
(0x1076e7c0, 0x1076e868, 42, '262f16b7b7a0f676'),
(0x1076e870, 0x1076e904, 37, 'f7e36e71ee758c0b'),
(0x1076e910, 0x1076e918, 2, '76a58357dcf88f07'),
(0x1076e920, 0x1076e9d0, 44, '7f4ea3db074b52cb'),
(0x1076e9d0, 0x1076ea20, 20, '4e1c851471a8ee52'),
(0x1076ea20, 0x1076ea64, 17, '3bd55bb156c26566'),
(0x1076ea70, 0x1076eaa0, 12, '22ca3fc74ef7a65e'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# The routed entries: block leaves (the leaf routines of rectangles and colours, two flag readers and the text callback).
ROUTED = frozenset({0x10769b50, 0x1076e910, 0x107698d0, 0x10769920, 0x1076bb20, 0x1076bb40, 0x1076ca20, 0x10768820, 0x10768990, 0x107689e0, 0x10768b40, 0x10768c50, 0x10768c90, 0x10768dc0, 0x10768e10, 0x107690f0, 0x107691b0, 0x107692f0, 0x1076e7c0, 0x1076e870, 0x1076e920, 0x1076e9d0})
ROUTED = ROUTED | frozenset({0x107697c0, 0x10769980, 0x1076a140, 0x1076a580, 0x1076a790, 0x1076a8f0, 0x1076ad50})  # pixels (lines, fills)
ROUTED = ROUTED | frozenset({0x1076b830, 0x1076b6e0, 0x1076b7e0, 0x1076b020, 0x1076b140, 0x1076b240, 0x1076b2f0, 0x1076b450})  # colours (colour variables, bitmaps, .rgm files)
ROUTED = ROUTED | frozenset({0x10769460, 0x1076d6e0})  # remap (the screen fade), frames (the framed rectangle)
ROUTED = ROUTED | frozenset({0x1076d5a0, 0x1076d470, 0x1076d1e0, 0x1076ca50, 0x1076bbf0, 0x1076c0b0, 0x1076c980, 0x1076c050, 0x1076c010, 0x1076d1a0, 0x1076ea70})  # text (fonts and text, but the text box 0x1076e160 and the message panel 0x1076e5f0)
ROUTED = ROUTED | frozenset({0x1076e160, 0x1076e5f0})  # text box and message panel (the panel carries the profiling markers)
ROUTED = ROUTED | frozenset({0x10768810, 0x10769b60})  # box (the version-text getter and the assertion box: 990 pages' call sites)
ASSEMBLY_TARGET = re.compile(r'^(b|bl|b[a-z]+) (0x[0-9a-f]+)$')
SLOT = re.compile(r'^lwz r\d+,(-?0x[0-9a-f]+)\(r2\)$')
SMALL = re.compile(r'^(?:l[bhwf][zas]?|st[bhwf][sd]?|lfs|lfd|stfs|stfd|lha) \w+,(-?0x[0-9a-f]+)\(r2\)$')


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private graph_carbon.cpp export unavailable; inventory cannot be certified')


def inside(address):
    return audit.MODULE[0] <= address < audit.MODULE[1]


def site_digest(sites):
    return hashlib.sha256(','.join(f'{s:x}' for s in sorted(sites)).encode()).hexdigest()[:16]


class GraphCarbonAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        cls.program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes()) for s in cls.program['segments'] if 'file' in s]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            cls.records = routine_records(db, *audit.MODULE)
            cls.branches = [(a, m.group(1), int(m.group(2), 16)) for a, text in db.execute(
                "SELECT address, json_extract(body, '$.assembly') FROM instructions WHERE json_extract(body, '$.assembly') LIKE 'b% 0x%'")
                for m in [ASSEMBLY_TARGET.match(text)] if m]
            cls.loads = {}
            cls.small = {}
            for a, text in db.execute("SELECT address, json_extract(body, '$.assembly') FROM instructions "
                                      "WHERE json_extract(body, '$.assembly') LIKE '%(r2)'"):
                if m := SLOT.match(text):
                    cls.loads.setdefault(int(m.group(1), 16), []).append(a)
                if m := SMALL.match(text):
                    cls.small.setdefault(int(m.group(1), 16), []).append(a)
            cls.assembly = {a: text for a, text in db.execute(
                "SELECT address, json_extract(body, '$.assembly') FROM instructions WHERE address BETWEEN ? AND ?",
                (audit.MODULE[0], audit.MODULE[1]))}
        cls.by_address = {r['address']: r for r in cls.records}

    def word(self, address):
        for start, contents in self.segments:
            if start <= address and address + 4 <= start + len(contents):
                return struct.unpack_from('>I', contents, address - start)[0]
        raise AssertionError(f'no data at {address:x}')

    def string(self, address):
        for start, contents in self.segments:
            if start <= address < start + len(contents):
                return contents[address - start:contents.index(b'\0', address - start)].decode()
        raise AssertionError(f'no data at {address:x}')

    def glue_name(self, glue):
        match = SLOT.match(self.assembly_at(glue))
        symbols = {s['address']: s['name'] for s in self.program['symbols'] if s['space'] == 'EXTERNAL'}
        imports = next(s for s in self.program['segments'] if s['name'] == 'IMPORTS')
        value = self.word(TOC + int(match.group(1), 16))
        self.assertTrue(imports['address'] <= value < imports['address'] + imports['size'])
        return symbols[(value - imports['address']) // 4 + 1].removeprefix('CarbonLib::')

    def assembly_at(self, address):
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            return db.execute("SELECT json_extract(body, '$.assembly') FROM instructions WHERE address = ?", (address,)).fetchone()[0]

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.GRAPH_CARBON_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 55)
        self.assertEqual(len(audit.OWNED), 54)
        self.assertEqual(len(self.records), 6218)
        self.assertEqual(sum(count for _, _, count, _ in CONTRACTS), 6218)
        self.assertEqual(sum(count for a, _, count, _ in CONTRACTS if a in audit.OWNED), 6201)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_graph_carbon_index(db, complete=False)

    def test_the_pool_names_the_file_and_every_load_of_it_is_inside_the_module(self):
        self.assertEqual(self.word(TOC + audit.POOL_SLOT), audit.POOL)
        self.assertEqual(self.string(audit.POOL), 'graph_carbon.cpp')
        sites = self.loads[audit.POOL_SLOT]
        self.assertEqual(len(sites), 21)
        self.assertTrue(all(inside(a) for a in sites))
        self.assertEqual((min(sites), max(sites)), (0x10768838, 0x1076e614))
        loaders = {max(e for e in ENTRIES if e <= a) for a in sites}
        self.assertEqual(len(loaders), 18)
        self.assertEqual(min(loaders), 0x10768820)  # the first loader; 0x10768810 before it has none
        # Between QSORT8X.C's pool (before) and MAIN.CPP's (after): the link order of the pools.
        self.assertEqual(self.string(0x108a0abf), 'qsort8x.c')
        self.assertLess(0x108a0abf, audit.POOL)
        main_pool = self.word(TOC - 0x3bb4)
        self.assertEqual(main_pool, 0x108a0c60)
        self.assertEqual(self.string(0x108a0c80), 'MAIN.CPP')
        self.assertGreater(main_pool, audit.POOL + 0x14e)
        self.assertTrue(all(not inside(a) and a >= main_program.MODULE[0] for a in self.loads[-0x3bb4]))
        # The module's texts: the assertion format, the message box, the bitmap font files and the font loader's complaint.
        for offset, text in ((0x11, 'v%s %s.%s %d'), (0x1e, 'Error'), (0x34, 'MESSAGE %s %s\n'), (0x43, 'Ok'), (0x78, '.rgm'),
                             (0x95, 'SI_DATA'), (0x9d, 'arial_narrow_10.fnt'), (0xec, 'trade_cond_24_bold.fnt'),
                             (0x11a, "load_font() : unable to open file '%s' for input"), (0x14b, '%c'), (0x14e, 'code: %d')):
            self.assertEqual(self.string(audit.POOL + offset), text)
        # The byte table the font loader reads sits before the pool and its slot is loaded in the module only.
        self.assertEqual(self.word(TOC + audit.TABLE_SLOT), 0x108a0acc)
        self.assertEqual(self.loads[audit.TABLE_SLOT], [0x1076c754])

    def test_the_toc_slots_and_the_small_data_belong_to_the_module(self):
        for slot in audit.OWN_SLOTS:
            self.assertTrue(self.loads[slot] and all(inside(a) for a in self.loads[slot]), hex(slot))
        # The neighbours' slots: QSORT8X.C's pool slot before, MAIN.CPP's descriptor slot after.
        self.assertTrue(all(not inside(a) for a in self.loads[-0x3c00]))
        self.assertTrue(all(not inside(a) for a in self.loads[-0x3bcc]))
        self.assertGreaterEqual(min(self.loads[-0x3bcc]), main_program.MODULE[0])
        low, high = audit.SMALL_DATA
        span = {offset: sites for offset, sites in self.small.items() if low <= offset <= high}
        self.assertEqual(min(span), -0x3580)
        self.assertEqual(max(span), -0x3550 + 1)
        self.assertTrue(all(inside(a) for sites in span.values() for a in sites))
        # No access just outside the span from the module, and the next offsets belong to MAIN.CPP.
        self.assertTrue(all(not inside(a) and a >= main_program.MODULE[0] for o in (-0x354e, -0x354c) for a in self.small[o]))
        self.assertTrue(all(not inside(a) for o in (-0x3582, -0x3584) for a in self.small[o]))

    def test_every_instruction_and_unreachable_word_accounted(self):
        covered = set()
        for start, end in audit.ROUTINES:
            covered.update(range(start, end, 4))
        self.assertEqual(set(self.by_address), covered - audit.UNREACHABLE)
        self.assertEqual(len(audit.UNREACHABLE), 4)
        self.assertTrue(audit.UNREACHABLE <= covered)
        for address in audit.UNREACHABLE:
            self.assertRegex(self.by_address[address - 4]['assembly'], r'^b ')
        for (_, end), (start, _) in zip(audit.ROUTINES, audit.ROUTINES[1:]):
            self.assertLessEqual(start - end, 0xc)  # padding between routines only
        for address in range(audit.MODULE[0], audit.MODULE[1], 4):
            if address not in covered:
                self.assertNotIn(address, self.by_address)
        # The stretch follows QSORT8X.C (end 0x10768808, alignment) and runs into MAIN.CPP's helpers (0x1076eaa0).
        self.assertEqual(qsort.MODULE[1] + 8, audit.MODULE[0])
        self.assertEqual(audit.ROUTINES[0][0], audit.MODULE[0])
        self.assertEqual(audit.ROUTINES[-1][1], audit.MODULE[1])
        self.assertIn('MoreFiles', main_program.__doc__)
        self.assertLess(audit.MODULE[1], main_program.MODULE[0])
        # Every routine ends in a return and no branch leaves its routine (no tail call).
        ends = dict(audit.ROUTINES)
        for start, end in audit.ROUTINES:
            last = max(a for a in self.by_address if start <= a < end)
            self.assertEqual(self.by_address[last]['assembly'], 'blr', hex(start))
        for site, kind, target in self.branches:
            if not inside(site) or kind == 'bl':
                continue
            routine = max(e for e in ENTRIES if e <= site)
            self.assertTrue(routine <= target < ends[routine], (hex(site), hex(target)))
        # Only the text drawers' three jump tables are indirect jumps, and their entries stay inside the routine.
        self.assertEqual(sorted(a for a, r in self.by_address.items() if r['assembly'] in ('bctr', 'bctrl')), [0x1076cb50, 0x1076d234, 0x1076d4b8])
        for slot, (table, routine) in audit.JUMP_TABLES.items():
            self.assertEqual(self.word(TOC + slot), table)
            self.assertEqual(len(self.loads[slot]), 1)
            self.assertEqual(max(e for e in ENTRIES if e <= self.loads[slot][0]), routine)
            for n in range(8):
                self.assertTrue(routine <= self.word(table + 4 * n) < ends[routine])

    def test_the_callers_from_outside_and_the_descriptors(self):
        outside = {}
        for site, kind, target in self.branches:
            if inside(target) and not inside(site):
                self.assertEqual(kind, 'bl', hex(site))
                outside.setdefault(target, []).append(site)
        self.assertEqual({t: (len(s), site_digest(s)) for t, s in outside.items()}, audit.EXTERNAL)
        called = {target for site, kind, target in self.branches if kind == 'bl' and inside(target)}
        self.assertEqual(set(ENTRIES) - called, audit.POINTER_ONLY)
        # The descriptors (code word, TOC word) naming an entry sit in one block between the font engine's and MAIN.CPP's.
        found = {}
        for start, contents in self.segments:
            for address in range((max(start, CODE_END) + 3) & ~3, start + len(contents) - 7, 4):
                code, toc = struct.unpack_from('>II', contents, address - start)
                if toc == TOC and audit.MODULE[0] <= code < audit.MODULE[1]:
                    found[address] = code
        self.assertEqual(found, {d: entry for d, (entry, _) in audit.DESCRIPTORS.items()})
        self.assertEqual(sorted(found), list(range(0x108bda9c, 0x108bdac4, 8)))
        self.assertEqual(self.word(0x108bda94 + 4), TOC)
        self.assertTrue(0x10751350 <= self.word(0x108bda94) < 0x10768080)  # the engine's last descriptor
        # The next descriptors (0x108bdac4 on) name MAIN.CPP's routines.
        self.assertTrue(main_program.MODULE[0] <= self.word(0x108bdac4) < main_program.MODULE[1])
        self.assertEqual(self.word(0x108bdac4 + 4), TOC)
        for descriptor, (entry, slot) in audit.DESCRIPTORS.items():
            if slot is not None:
                self.assertEqual(self.word(TOC + slot), descriptor)
                self.assertTrue(self.loads[slot] and all(inside(a) for a in self.loads[slot]), hex(descriptor))
            else:  # named only by the packed exception tables, at no alignment: the descriptor's address bytes
                needle = struct.pack('>I', descriptor)
                self.assertTrue(any(needle in contents[:] for start, contents in self.segments if start >= CODE_END), hex(descriptor))
        # The text callback (FONT.CPP's ninth argument) is one of them.
        self.assertEqual(font_cpp.CALLBACK['code'], 0x1076ca20)
        self.assertEqual(audit.DESCRIPTORS[font_cpp.CALLBACK['descriptor']][0], 0x1076ca20)
        # The font array's static initializer is pinned by the other audit and loads this module's slot.
        self.assertIn(0x1076ea20, [start for start, _, _, _ in initializers.CONTRACTS])
        self.assertEqual(audit.OTHER_AUDIT, {0x1076ea20: 'engine_initialization_audit'})
        self.assertEqual(self.loads[-0x3bd8].count(0x1076ea24), 1)
        self.assertEqual(self.loads[-0x3bd8].count(0x1076ea74), 1)
        self.assertTrue(all(inside(a) for a in self.loads[-0x3bd8]))
        self.assertEqual(self.assembly_at(0x1076ea40), 'bl 0x10773320')
        self.assertEqual(self.assembly_at(0x1076ea50), 'bl 0x10772df0')
        self.assertEqual(self.assembly_at(0x1076ea80), 'lwz r4,-0x5b1c(r2)')  # the destructor's slot of the font record's descriptor

    def test_what_the_module_calls_outside_itself(self):
        lib, glue = set(), set()
        for site, kind, target in self.branches:
            if inside(site) and not inside(target):
                self.assertEqual(kind, 'bl', hex(site))
                (glue if GLUE[0] <= target < GLUE[1] else lib).add(target)
        self.assertEqual(tuple(sorted(lib)), audit.CALLEES)
        self.assertEqual(tuple(sorted(self.glue_name(g) for g in glue)), audit.GLUE_NAMES)
        self.assertEqual(len(audit.GLUE_NAMES), 49)
        # FONT.CPP is called by the loader, the selection setter, the drawer and the width routine (font_cpp_audit.EXTERNAL).
        self.assertEqual(lib & set(font_cpp.FONT_CPP_ENTRIES), {0x1074f470, 0x1074f800, 0x107500f0, 0x10750a60})
        self.assertTrue({0x1074f470, 0x1074f800, 0x107500f0, 0x10750a60} <= set(font_cpp.EXTERNAL))
        # The assertion path every module calls: the version text, the version address, the reporter and the dialog.
        for site in (0x100000b4, 0x100000b8, 0x100000e4):
            self.assertIn(self.assembly_at(site), ('bl 0x10739890', 'bl 0x10768810', 'bl 0x10769b60'))
        # No routine of the module draws from the pooled random generator or the game's database (no call into an owner that does).
        self.assertFalse({0x10705f50, 0x1070e1a0} & set(audit.CALLEES))

    def test_the_native_drawing_observation_points_and_the_leavers(self):
        from native_drawing_audit import NATIVE_DRAWING_BOUNDARIES
        observed = sorted(a for a in NATIVE_DRAWING_BOUNDARIES if inside(a))
        self.assertEqual(len(observed), 22)
        entries = [a for a in observed if a in ENTRIES]
        self.assertEqual(len(entries), 12)
        self.assertEqual(sorted(set(observed) - set(entries)), [0x107696cc, 0x107697b0, 0x107698c4, 0x10769b48, 0x1076a570, 0x1076a784,
                                                                0x1076b014, 0x1076b138, 0x1076b234, 0x1076d194])
        # A routed entry may not hold one inside its body: the guard names it.
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text('GRAPH_CARBON_DRAFT(0x10769460, 0x107697b4, R3, "0000000000000000", name)\n')
            # (every entry with observation points inside is now one whose C++ runs them: the guard is shown with none named.)
            with mock.patch.object(audit, 'OBSERVES_INTERIOR', frozenset()):
                with self.assertRaisesRegex(ValueError, 'observation points'):
                    audit.rewritten(candidate, drafts=True)
        # The profiling markers (tools/profiling_boundaries.py): the message panel holds two inside its body and the entry's, so its C++ passes the two
        # inside (PROFILES_INTERIOR) and the generator keeps it out of the in-place owner table; any other routed entry holding one is refused.
        self.assertEqual(audit.profiled_inside(0x1076e5f0, 0x1076e7c0), [0x1076e7a4, 0x1076e7bc])
        self.assertEqual([entry for entry, end in audit.ROUTINES if audit.profiled_inside(entry, end)], [0x1076e5f0])
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text('GRAPH_CARBON_DRAFT(0x1076e5f0, 0x1076e7c0, R3, "0000000000000000", name)\n')
            with mock.patch.object(audit, 'PROFILES_INTERIOR', frozenset()):
                with self.assertRaisesRegex(ValueError, 'profiling markers'):
                    audit.rewritten(candidate, drafts=True)
            audit.rewritten(candidate, drafts=True)
        from compile_recomp import direct_owner_entries
        self.assertNotIn(0x1076e5f0, direct_owner_entries())
        self.assertIn(0x1076e160, direct_owner_entries())
        # The leavers: each prologue saves registers from the first named one to r31 (stmw), or r31 and r30 one by one.
        self.assertEqual(len(audit.LEAVERS), 12)
        for entry, first in audit.LEAVERS.items():
            prologue = [self.by_address[a]['assembly'] for a in range(entry, entry + 0x40, 4) if a in self.by_address][:14]
            saves = [text for text in prologue if text.startswith(('stmw', 'stw r')) and not text.startswith('stw r0,')]
            registers = [int(re.match(r'(?:stmw|stw) r(\d+),', text).group(1)) for text in saves]
            self.assertEqual(min(registers), first, hex(entry))
        # The one reader of stale stack inside the module, in the allocator wrapper.
        self.assertIn('lwz', self.by_address[0x1076b760]['assembly'])

    def test_exactly_the_routed_blocks_are_routed_and_hooked_and_the_hook_checks_fail_when_they_should(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS, generate_page
        pinned = {a: (b, d) for a, b, _, d in CONTRACTS}
        self.assertTrue(audit.COMPLETE)
        self.assertEqual(ROUTED, set(audit.OWNED))
        listed = audit.rewritten(drafts=True)
        for entry, end, expected in listed:
            self.assertEqual(pinned[entry], (end, expected))
        self.assertEqual({entry for entry, _, _ in listed}, ROUTED)
        self.assertEqual({entry for entry, _, _ in audit.rewritten()}, ROUTED)
        self.assertEqual(set(audit.GRAPH_CARBON_HOOKED), ROUTED)
        for entry in ENTRIES:
            owners = [owner for at, owner in OWNER_REPLACEMENTS if at == entry and owner == audit.OWNER]
            self.assertEqual(owners, [audit.OWNER] if entry in ROUTED else [], hex(entry))
        # Only the static initializer is owned elsewhere (the engine initialization audit).
        self.assertEqual([(at, owner) for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [(0x1076ea20, 'engine_initialization_adapter')])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_graph_carbon_index(db)
            # The completeness check refuses a set with one owned entry short of production (a draft), and accepts all of them.
            with tempfile.TemporaryDirectory() as directory:
                candidate = Path(directory) / 'routines.inc'
                owned = [c for c in CONTRACTS if c[0] in audit.OWNED]
                lines = ''.join(f'GRAPH_CARBON_ROUTINE(0x{a:x}, 0x{b:x}, R3, "{d}", name)\n' for a, b, _, d in owned)
                candidate.write_text(lines)
                # A routed entry with observation points inside is refused unless its C++ runs them (OBSERVES_INTERIOR).
                # (with every block in, no owned entry is left without its C++ running the points: the refusal is shown with none named.)
                with mock.patch.object(audit, 'OBSERVES_INTERIOR', frozenset()):
                    with self.assertRaisesRegex(ValueError, 'observation points'):
                        audit.require_graph_carbon_index(db, candidate, complete=True)
                with mock.patch.object(audit, 'OBSERVES_INTERIOR', frozenset(owned_entry for owned_entry, *_ in owned)):
                    audit.require_graph_carbon_index(db, candidate, complete=True)
                    first = owned[0][0]
                    candidate.write_text(lines.replace(f'GRAPH_CARBON_ROUTINE(0x{first:x},', f'GRAPH_CARBON_DRAFT(0x{first:x},', 1))
                    with self.assertRaisesRegex(ValueError, 'requires all 54'):
                        audit.require_graph_carbon_index(db, candidate, complete=True)
        selected = set(self.by_address)
        pages = sorted({entry >> 12 for entry in ROUTED} | {0x10768})  # the routed pages and one with unrouted entries only
        for page in pages:
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_graph_carbon_hooks(page, part, normal)
            on_page = [entry for entry in ENTRIES if entry >> 12 == page]
            hooked = [entry for entry in on_page if entry in ROUTED]
            self.assertEqual('#include "graph_carbon_adapter.hpp"' in normal, bool(hooked), hex(page))
            for entry in on_page:
                hook = audit.graph_carbon_hook(entry)
                fragment = normal.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
                step = f's.step(0x{entry:x});'
                if entry not in ROUTED:
                    self.assertNotIn(hook, normal)
                    with self.assertRaisesRegex(ValueError, 'Unexpected'):
                        audit.require_graph_carbon_hooks(page, part, normal.replace(step, hook + '\n' + step, 1))
                    with mock.patch.object(audit, 'GRAPH_CARBON_HOOKED', audit.GRAPH_CARBON_HOOKED + (entry,)):
                        with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                            audit.require_graph_carbon_hooks(page, part, normal)
                    continue
                self.assertEqual(normal.count(hook), 1)
                self.assertIn(hook, fragment)
                self.assertLess(fragment.index(hook), fragment.index(step))
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_graph_carbon_hooks(page, part, broken)
                with mock.patch.object(audit, 'GRAPH_CARBON_HOOKED', tuple(e for e in audit.GRAPH_CARBON_HOOKED if e != entry)):
                    with self.assertRaisesRegex(ValueError, 'Unexpected'):
                        audit.require_graph_carbon_hooks(page, part, normal)

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[9]
            line = f'GRAPH_CARBON_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_graph_carbon_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_graph_carbon_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing graph_carbon.cpp instructions'):
                    audit.require_graph_carbon_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)
            # The static initializer belongs to the engine initialization audit: listing it here is refused.
            candidate.write_text('GRAPH_CARBON_DRAFT(0x1076ea20, 0x1076ea64, NONE, "0000000000000000", name)\n')
            with self.assertRaisesRegex(ValueError, 'belongs to engine_initialization_audit'):
                audit.rewritten(candidate, drafts=True)
            block = Path(directory) / 'graph_carbon_routines_block.inc'
            block.write_text(line)
            candidate.write_text('#include "graph_carbon_routines_block.inc"\n')
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])

    def test_the_entries_listing_and_the_digest_printer(self):
        text = subprocess.run([sys.executable, str(ROOT / 'tools/graph_carbon_audit.py'), '--entries'], check=True, capture_output=True,
                              text=True).stdout.split()
        self.assertEqual(text, [f'0x{a:x}' for a in ENTRIES])
        printed = subprocess.run([sys.executable, str(ROOT / 'tools/graph_carbon_audit.py'), '0x1076bb40'], check=True,
                                 capture_output=True, text=True).stdout.split()
        self.assertEqual(printed[:3], ['0x1076bb40', '0x1076bbe8', dict((a, d) for a, _, _, d in CONTRACTS)[0x1076bb40]])


if __name__ == '__main__':
    unittest.main()
