"""Certify FONT.CPP's 19 entries (the game's text drawing layer over the font engine: tools/font_cpp_audit.py): bounds, the
string pool, the callers from outside, the call into the engine, the indirect callback and which entries are routed to C++.

The private export is required evidence, rather than a reason to skip tests. Production ownership is asserted, not skipped:
exactly the routed entries are listed, owned and hooked, and the hook checker rejects a page that carries a hook for an entry
that is not hooked or lacks one for an entry that is.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import struct
import tempfile
import unittest
from unittest import mock

import font_cpp_audit as audit
import font_engine_audit as engine
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CODE_END = 0x10781330
CONTRACTS = (
    (0x1074d380, 0x1074d448, 50, 'd7cdd3fef96cbc27'),
    (0x1074d450, 0x1074d4bc, 27, '40b3b9c5a51e0ece'),
    (0x1074f170, 0x1074f2c0, 84, 'c93c19a2bba8caac'),
    (0x1074f2c0, 0x1074f384, 49, '8bc1a89955310544'),
    (0x1074f390, 0x1074f404, 29, '355f199a76b04e11'),
    (0x1074f410, 0x1074f468, 22, 'b9a0a85123bfcc14'),
    (0x1074f470, 0x1074f800, 228, '79058f936de15667'),
    (0x1074f800, 0x107500ec, 570, '9c2376c35dbc123e'),
    (0x107500f0, 0x10750200, 67, '271adda3ba688ac9'),
    (0x10750200, 0x10750320, 71, '9a0017e4f42254bb'),
    (0x10750320, 0x10750580, 152, 'e5009ac1e6528e08'),
    (0x10750580, 0x107509f4, 283, '195c9e9be340eb63'),
    (0x10750a00, 0x10750a54, 21, 'be3a203bc3b5faaa'),
    (0x10750a60, 0x10750afc, 39, '31764446752079fd'),
    (0x10750b00, 0x10750b5c, 23, '145c131c9d7c2a2f'),
    (0x10750b60, 0x10750c9c, 79, '15f45265c366e6d0'),
    (0x10750ca0, 0x10750d28, 34, '0b002f8b8c824187'),
    (0x10750d30, 0x10750f00, 116, 'a9274a5386d7d2fe'),
    (0x10750f00, 0x10751348, 273, 'b78bd64134e1204a'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# The routed entries: block fontlist (the font list's five entries: the directory finder, the lazy initialisation wrappers, the
# count, the selection setter and the name getter).
ROUTED = frozenset({0x10750a00, 0x10750a60, 0x10750b00, 0x10750b60, 0x10750ca0})
# Blocks selector (the style selector) and glyphs (the glyph cache and the string width).
ROUTED = ROUTED | frozenset({0x10750320, 0x10750580, 0x107500f0})
# Block drawer (the text drawer).
ROUTED = ROUTED | frozenset({0x1074f800})
# Blocks fontfiles (the resource file helpers, the font data finder and file opener, the name reader, the list builder) and record (the font record's
# constructor and destructor, its release and the loader).
ROUTED = ROUTED | frozenset({0x1074d380, 0x1074d450, 0x1074f170, 0x1074f2c0, 0x10750d30, 0x10750f00, 0x1074f390, 0x1074f410, 0x10750200, 0x1074f470})
ASSEMBLY_TARGET = re.compile(r'^(b|bl|b[a-z]+) (0x[0-9a-f]+)$')
SLOT = re.compile(r'^lwz r\d+,(-?0x[0-9a-f]+)\(r2\)$')
REGISTER = re.compile(r'\br(\d+)\b')


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private FONT.CPP export unavailable; inventory cannot be certified')


def inside(address):
    return any(low <= address < high for low, high in audit.SEGMENTS)


class FontCppAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        cls.program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes()) for s in cls.program['segments'] if 'file' in s]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            cls.records = [r for low, high in audit.SEGMENTS for r in routine_records(db, low, high)]
            cls.branches = [(a, m.group(1), int(m.group(2), 16)) for a, text in db.execute(
                "SELECT address, json_extract(body, '$.assembly') FROM instructions WHERE json_extract(body, '$.assembly') LIKE 'b% 0x%'")
                for m in [ASSEMBLY_TARGET.match(text)] if m]
            cls.loads = {}
            for a, text in db.execute("SELECT address, json_extract(body, '$.assembly') FROM instructions "
                                      "WHERE json_extract(body, '$.assembly') LIKE 'lwz r%(r2)'"):
                if m := SLOT.match(text):
                    cls.loads.setdefault(int(m.group(1), 16), []).append(a)
            cls.glue_sites = [a for (a,) in db.execute("SELECT address FROM instructions WHERE "
                                                       "json_extract(body, '$.assembly') = 'bl 0x10781320'") if inside(a)]
            cls.assembly = {a: text for a, text in db.execute(
                "SELECT address, json_extract(body, '$.assembly') FROM instructions WHERE address BETWEEN ? AND ?",
                (0x1076ca00, 0x1076d000))}
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

    def string_position(self, text):
        needle = text.encode() + b'\0'
        for start, contents in self.segments:
            at = contents.find(needle)
            if at >= 0 and 0x108a0000 <= start + at < 0x108a1000:
                return start + at
        raise AssertionError(text)

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.FONT_CPP_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 19)
        self.assertEqual(len(self.records), 2217)
        self.assertEqual(sum(count for _, _, count, _ in CONTRACTS), 2217)
        self.assertEqual(sum(count for a, _, count, _ in CONTRACTS if a >= 0x1074f170), 2140)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_font_cpp_index(db, complete=False)

    def test_the_pool_names_the_file_and_every_load_of_it_is_inside_the_module(self):
        self.assertEqual(self.word(TOC + audit.POOL_SLOT), audit.POOL)
        self.assertEqual(self.string(audit.POOL + audit.NAME_OFFSET), 'FONT.CPP')
        self.assertEqual(self.string(audit.POOL), '.ttf')
        # Between SIPRINT.CPP's pool (before) and the font engine's first unit's pool (after): the link order of the pools.
        self.assertLess(self.string_position('SIPRINT.CPP'), audit.POOL)
        first_engine_string = engine.POOLS['AUTOGRID'][3]
        self.assertGreater(first_engine_string, audit.POOL + audit.NAME_OFFSET)
        self.assertEqual(self.string(first_engine_string), 'shift < 96')
        users = {slot: self.loads[slot] for slot in (audit.POOL_SLOT, audit.FLOAT_SLOT)}
        self.assertEqual((len(users[audit.POOL_SLOT]), len(users[audit.FLOAT_SLOT])), (11, 1))
        for slot, sites in users.items():
            self.assertTrue(all(inside(a) for a in sites), slot)
        loaders = {max(e for e in ENTRIES if e <= a) for sites in users.values() for a in sites}
        self.assertEqual(sorted(loaders), [0x1074f2c0, 0x1074f390, 0x1074f470, 0x1074f800, 0x107500f0, 0x10750200, 0x10750320,
                                           0x10750580, 0x10750b60, 0x10750ca0, 0x10750d30])
        self.assertEqual(min(a for sites in users.values() for a in sites), 0x1074f2c8)
        self.assertEqual(max(a for sites in users.values() for a in sites), 0x10750d40)
        # The module's strings are the ones the assertions and the font list name.
        for offset, text in ((0xd, 'data'), (0x61, 'window'), (0x68, 'window->back_buffer'), (0x7c, 'string'), (0x45, 'SI_DATA'),
                             (0xe7, 'FONTS'), (0xed, 'CM3_DATA')):
            self.assertEqual(self.string(audit.POOL + offset), text)
        # No routine of the module loads a pool slot of the font engine.
        for slot in {slot for _, _, slot, _ in engine.POOLS.values()}:
            self.assertFalse([a for a in self.loads.get(slot, []) if inside(a)], slot)

    def test_every_instruction_and_unreachable_word_accounted(self):
        covered = set()
        for start, end in audit.ROUTINES:
            covered.update(range(start, end, 4))
        self.assertEqual(set(self.by_address), covered - audit.UNREACHABLE)
        self.assertEqual(len(audit.UNREACHABLE), 6)
        self.assertTrue(audit.UNREACHABLE <= covered)
        # An unreachable word follows an unconditional branch or a return (an alignment no-op).
        for address in audit.UNREACHABLE:
            before = address - 4
            while before in audit.UNREACHABLE:
                before -= 4
            self.assertRegex(self.by_address[before]['assembly'], r'^(b |blr)')
        for (_, end), (start, _) in zip(audit.ROUTINES, audit.ROUTINES[1:]):
            if end >= audit.SEGMENTS[1][0] or start < audit.SEGMENTS[0][1]:
                self.assertLessEqual(start - end, 0x10)  # padding between routines only
        # The stretches' ends: the helpers run into the shared inline block, the module into the font engine's first entry; the
        # helpers follow SIPRINT.CPP's last routine.
        self.assertEqual(audit.SEGMENTS[0][1] + 4, 0x1074d4c0)
        self.assertEqual(audit.MODULE[1] + 8, engine.MODULE[0])
        self.assertEqual(audit.SEGMENTS[0][0], 0x1074d37c + 4)
        # Every routine ends in a return, no branch leaves its routine (no tail call) and there is no jump table.
        ends = dict(audit.ROUTINES)
        for start, end in audit.ROUTINES:
            last = max(a for a in self.by_address if start <= a < end)
            self.assertEqual(self.by_address[last]['assembly'], 'blr', hex(start))
        for site, kind, target in self.branches:
            if not inside(site) or kind == 'bl':
                continue
            routine = max(e for e in ENTRIES if e <= site)
            self.assertTrue(routine <= target < ends[routine], (hex(site), hex(target)))
        self.assertFalse([a for a, r in self.by_address.items() if r['assembly'] in ('bctr', 'bctrl')])

    def test_the_callers_from_outside_and_the_descriptors(self):
        outside = {}
        for site, kind, target in self.branches:
            if inside(target) and not inside(site):
                self.assertEqual(kind, 'bl', hex(site))
                outside.setdefault(target, []).append(site)
        self.assertEqual({t: tuple(sorted(s)) for t, s in outside.items()}, audit.EXTERNAL)
        called = {target for site, kind, target in self.branches if kind == 'bl' and inside(target)}
        self.assertEqual(set(ENTRIES) - called, audit.POINTER_ONLY)
        # Descriptors (code word, TOC word) naming an entry: exactly the pointer-only ones.
        found = {}
        for start, contents in self.segments:
            for address in range((max(start, CODE_END) + 3) & ~3, start + len(contents) - 7, 4):
                code, toc = struct.unpack_from('>II', contents, address - start)
                if toc == TOC and code in ENTRIES:
                    found[code] = address
        self.assertEqual(found, {0x1074d450: 0x108bd63c, 0x1074f410: 0x108bd674, 0x1074f390: 0x108bd67c})
        # The record constructor and destructor are loaded from TOC slots by the static array construction 0x1076ea20 only.
        loaded = {self.word(TOC + slot): slot for slot in self.loads}
        self.assertEqual(self.loads[loaded[found[0x1074f390]]], [0x1076ea30])
        self.assertEqual(self.loads[loaded[found[0x1074f410]]], [0x1076ea3c, 0x1076ea80])
        self.assertNotIn(found[0x1074d450], loaded)  # nothing loads the resource file destructor's descriptor
        # The resource file helpers are called from the loader's first routine only.
        helpers = [(s, t) for s, k, t in self.branches if t in (0x1074d380, 0x1074d450)]
        self.assertEqual(len(helpers), 3)
        self.assertTrue(all(0x1074f170 <= s < 0x1074f2c0 for s, _ in helpers))

    def test_the_calls_into_the_engine_are_its_api(self):
        into = {}
        for site, kind, target in self.branches:
            if inside(site) and engine.MODULE[0] <= target < engine.MODULE[1]:
                self.assertEqual(kind, 'bl')
                into.setdefault(target, set()).add(max(e for e in ENTRIES if e <= site))
        self.assertEqual({t: tuple(sorted(c)) for t, c in into.items()}, {t: tuple(sorted(c)) for t, c in engine.API.items()})
        # The style functions the selector loads from TOC slots -0x5b78 and -0x5b74.
        self.assertEqual(self.loads[-0x5b78], [0x10750330])
        self.assertEqual(self.loads[-0x5b74], [0x10750338])

    def test_the_only_indirect_calls_are_the_string_drawers_callback_and_it_reads_one_argument(self):
        self.assertEqual(self.glue_sites, [0x1074f9d0, 0x1074fa84])
        callback = audit.CALLBACK
        # The ninth argument of 0x1074f800 is the stack word at 0xd8(r1); each caller stores r6 at 0x38(r1) before the call.
        self.assertEqual(self.by_address[0x1074f834]['assembly'], 'lwz r29,0xd8(r1)')
        sites = [s for s, k, t in self.branches if t == 0x1074f800 and k == 'bl']
        self.assertEqual(sorted(sites), sorted(audit.EXTERNAL[0x1074f800] + (0x1074f8fc,)))
        for site in audit.EXTERNAL[0x1074f800]:
            block = []
            at = site - 4
            while self.assembly[at] != 'extsh r4,r21':
                block.append(self.assembly[at])
                at -= 4
            self.assertIn('stw r6,0x38(r1)', block, hex(site))
            # the block's instructions run from the call back to its start: nothing writes r6 before the store (later in the list)
            written = [n for n, text in enumerate(block) if re.match(r'^[a-z]+\.? r6,', text) and not text.startswith('stw')]
            self.assertTrue(all(n < block.index('stw r6,0x38(r1)') for n in written), hex(site))
        # r6 is set before the jump table: the callback's descriptor or zero.
        self.assertEqual([a for a, text in self.assembly.items() if 0x1076cb34 <= a < 0x1076cb54 and re.match(r'^[a-z]+\.? r6,', text)], [])
        self.assertEqual(self.assembly[callback['load']], 'lwz r6,-0x3be4(r2)')
        self.assertEqual(self.assembly[0x1076cb30], 'li r6,0x0')
        self.assertEqual(self.loads[callback['slot']], [callback['load']])
        # The recursive call passes a zero ninth argument.
        self.assertEqual([self.by_address[a]['assembly'] for a in (0x1074f8cc, 0x1074f8d4)], ['li r0,0x0', 'stw r0,0x38(r1)'])
        # The descriptor, and the routine it names, which reads r3 and nothing else of r4-r10 (r4 is written first).
        self.assertEqual(self.word(TOC + callback['slot']), callback['descriptor'])
        self.assertEqual((self.word(callback['descriptor']), self.word(callback['descriptor'] + 4)), (callback['code'], TOC))
        body = [self.assembly[a] for a in range(callback['code'], callback['end'], 4)]
        self.assertEqual(len(body), 11)
        self.assertEqual(sorted({int(n) for text in body for n in REGISTER.findall(text)}), [0, 2, 3, 4])
        self.assertEqual(body[0], 'lwz r4,-0x3578(r2)')
        self.assertEqual([t for t in body if 'r3' in t], ['extsh r0,r3', 'li r3,0x0'])

    def test_exactly_the_routed_blocks_are_routed_and_hooked_and_the_hook_checks_fail_when_they_should(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS, generate_page
        pinned = {a: (b, d) for a, b, _, d in CONTRACTS}
        self.assertTrue(audit.COMPLETE)
        DRAFTS = frozenset()  # (2026-11-23: the drawer, the glyph cache and the string width were drafts for the host's HD drawing; routed again)
        listed = audit.rewritten(drafts=True)
        for entry, end, expected in listed:
            self.assertEqual(pinned[entry], (end, expected))
        self.assertEqual({entry for entry, _, _ in listed}, ROUTED)
        self.assertEqual({entry for entry, _, _ in audit.rewritten()}, ROUTED - DRAFTS)
        self.assertEqual(set(audit.FONT_CPP_HOOKED), ROUTED)
        for entry in ENTRIES:
            owners = [owner for at, owner in OWNER_REPLACEMENTS if at == entry]
            self.assertEqual(owners, [audit.OWNER] if entry in ROUTED else [], hex(entry))
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_font_cpp_index(db)
            # The completeness check refuses a set with one entry short of production (a draft).
            with tempfile.TemporaryDirectory() as directory:
                candidate = Path(directory) / 'routines.inc'
                first = CONTRACTS[0][0]
                lines = ''.join(f'FONT_CPP_ROUTINE(0x{a:x}, 0x{b:x}, R3, "{d}", name)\n' for a, b, _, d in CONTRACTS)
                candidate.write_text(lines)
                audit.require_font_cpp_index(db, candidate, complete=True)
                # The three routines the host's HD drawing observes inside or through a typed call: refused when the module stops listing what its
                # C++ passes (tools/host_observed.py; tools/test_host_observed.py removes each listing in turn).
                with mock.patch.object(audit, 'HANDLED_POINTS', {}):
                    with self.assertRaisesRegex(ValueError, 'host-observed instructions'):
                        audit.require_font_cpp_index(db, candidate, complete=True)
                with mock.patch.object(audit, 'HANDLED_CALLS', {}):
                    with self.assertRaisesRegex(ValueError, 'HANDLED_CALLS'):
                        audit.require_font_cpp_index(db, candidate, complete=True)
                candidate.write_text(lines.replace(f'FONT_CPP_ROUTINE(0x{first:x},', f'FONT_CPP_DRAFT(0x{first:x},', 1))
                with self.assertRaisesRegex(ValueError, 'requires all 19'):
                    audit.require_font_cpp_index(db, candidate, complete=True)
        selected = set(self.by_address)
        pages = sorted({entry >> 12 for entry in ROUTED} | {0x1074f})  # the routed pages and one with unrouted entries only
        for page in pages:
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_font_cpp_hooks(page, part, normal)
            on_page = [entry for entry in ENTRIES if entry >> 12 == page]
            hooked = [entry for entry in on_page if entry in ROUTED]
            self.assertEqual('#include "font_cpp_adapter.hpp"' in normal, bool(hooked), hex(page))
            for entry in on_page:
                hook = audit.font_cpp_hook(entry)
                fragment = normal.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
                step = f's.step(0x{entry:x});'
                if entry not in ROUTED:
                    self.assertNotIn(hook, normal)
                    with self.assertRaisesRegex(ValueError, 'Unexpected'):
                        audit.require_font_cpp_hooks(page, part, normal.replace(step, hook + '\n' + step, 1))
                    with mock.patch.object(audit, 'FONT_CPP_HOOKED', audit.FONT_CPP_HOOKED + (entry,)):
                        with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                            audit.require_font_cpp_hooks(page, part, normal)
                    continue
                self.assertEqual(normal.count(hook), 1)
                self.assertIn(hook, fragment)
                self.assertLess(fragment.index(hook), fragment.index(step))
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_font_cpp_hooks(page, part, broken)
                with mock.patch.object(audit, 'FONT_CPP_HOOKED', tuple(e for e in audit.FONT_CPP_HOOKED if e != entry)):
                    with self.assertRaisesRegex(ValueError, 'Unexpected'):
                        audit.require_font_cpp_hooks(page, part, normal)

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[10]
            line = f'FONT_CPP_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_font_cpp_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_font_cpp_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing FONT.CPP instructions'):
                    audit.require_font_cpp_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)
            block = Path(directory) / 'font_cpp_routines_block.inc'
            block.write_text(line)
            candidate.write_text('#include "font_cpp_routines_block.inc"\n')
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])


if __name__ == '__main__':
    unittest.main()
