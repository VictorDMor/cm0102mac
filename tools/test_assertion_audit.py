"""Certify the assertion module (tools/assertion_audit.py): the file-name splitter 0x10739890 and the C assertion reporter 0x107755c0.

The private export is required evidence, rather than a reason to skip tests. Pinned: the entries, ends and digests, who calls them (only
`bl`, no tail jump, no pointer in code or data), what they call, the TOC slots they load (and that nothing else loads the splitter's), the
format string, that no other audit's routines contain them, and which entries are routed: exactly the listed ones are owned and hooked, and
the hook checker rejects a page that carries a hook for an entry that is not hooked or lacks one for an entry that is.
"""
from contextlib import closing
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

import assertion_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
SPLIT, REPORT = 0x10739890, 0x107755c0
CONTRACTS = (
    (0x10739890, 0x10739b3c, 170, '6fb8f344147b63fc'),
    (0x107755c0, 0x107755fc, 15, '9e32e6cda6f2ff85'),
)
ROUTED = {SPLIT, REPORT}
SLOT = re.compile(r'(?:lwz|lbz|lhz|lha|stw|stb|sth|addi) r\d+,(-0x[0-9a-f]+)\(r2\)')
BRANCH = re.compile(r'(bl?) (0x[0-9a-f]+)')


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private export unavailable; the assertion module cannot be certified')


def routine_of(address):
    return next((entry for entry, end in audit.ROUTINES if entry <= address < end), None)


class AssertionAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        cls.program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes()) for s in cls.program['segments'] if 'file' in s]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            cls.assembly = {a: t for a, t in db.execute("SELECT address, json_extract(body, '$.assembly') FROM instructions")}
            cls.records = {entry: routine_records(db, entry, end) for entry, end in audit.ROUTINES}
        cls.loads = {}
        for a, text in cls.assembly.items():
            if m := SLOT.match(text):
                cls.loads.setdefault(int(m.group(1), 16), []).append(a)

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

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        for entry, end, count, expected in CONTRACTS:
            body = self.records[entry]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
            self.assertEqual(body[-1]['assembly'], 'blr')
            self.assertEqual(body[-1]['address'], end - 4)
            self.assertEqual([r['address'] for r in body], [a for a in range(entry, end, 4) if a not in audit.UNREACHABLE])
        # The one word the export lacks is the alignment nop after an unconditional jump (0x10739a20 is `b 0x10739a30`).
        self.assertEqual(self.assembly[0x10739a20], 'b 0x10739a30')
        self.assertNotIn(0x10739a24, self.assembly)

    def test_nothing_enters_the_bodies_but_their_entries_and_the_callers_are_calls(self):
        for entry, end in audit.ROUTINES:
            sites = [a for a, t in self.assembly.items() if (m := BRANCH.fullmatch(t)) and int(m.group(2), 16) == entry]
            self.assertTrue(all(self.assembly[a].startswith('bl ') for a in sites), hex(entry))  # no tail jump
            self.assertEqual(len(sites), {SPLIT: 6486, REPORT: 136}[entry])
            # No branch from outside lands inside a body, and no word of the code or data holds the entry (no descriptor, no pointer).
            for a, t in self.assembly.items():
                if (m := BRANCH.fullmatch(t)) and entry < int(m.group(2), 16) < end:
                    self.assertTrue(entry <= a < end, hex(a))
            for start, contents in self.segments:
                self.assertEqual(contents.count(struct.pack('>I', entry)), 0, hex(entry))

    def test_the_splitter_is_the_first_call_of_every_assertion_report(self):
        sites = sorted(a for a, t in self.assembly.items() if t == 'bl 0x10739890')
        # 6,428 of the 6,486 sites are followed at once by the version text getter (the report's second call, guest_abi.hpp report_assertion).
        followed = [a for a in sites if self.assembly.get(a + 4) == 'bl 0x10768810']
        self.assertEqual(len(followed), 6428)
        # The reporter's callers are the C library's users: the font engine, FONT.CPP, POL_CUP, QSORT4X, LEAGUE and DATABASE.
        self.assertEqual(len([a for a, t in self.assembly.items() if t == 'bl 0x107755c0']), 136)

    def test_what_each_routine_calls_and_loads(self):
        callees = {entry: sorted({int(m.group(2), 16) for r in self.records[entry] if (m := BRANCH.fullmatch(r['assembly']))
                                  and not entry <= int(m.group(2), 16) < dict(audit.ROUTINES)[entry]}) for entry, _ in audit.ROUTINES}
        # The splitter's branches that leave it are all `bl` to strlen and strcpy; the reporter's to fprintf and abort.
        self.assertEqual(callees[SPLIT], [0x1077dbd0, 0x1077dbf0])
        self.assertEqual(callees[REPORT], [0x10773fb0, 0x1077b610])
        for entry, _ in audit.ROUTINES:
            for r in self.records[entry]:
                if m := BRANCH.fullmatch(r['assembly']):
                    if not entry <= int(m.group(2), 16) < dict(audit.ROUTINES)[entry]:
                        self.assertEqual(m.group(1), 'bl')
                self.assertNotIn(r['assembly'].split(' ')[0], ('bctr', 'bctrl', 'blrl', 'bcctr'))
        # The four slots the splitter's frame buffers copy from: loaded only there, addresses 0x100 apart in the zero-filled statics.
        slots = (-0x3e78, -0x3e7c, -0x3e80, -0x3e84)
        for slot in slots:
            self.assertEqual({routine_of(a) for a in self.loads[slot]}, {SPLIT}, hex(slot))
            self.assertEqual(len(self.loads[slot]), 1)
        values = [self.word(TOC + slot) for slot in slots]
        self.assertEqual(values, [0x1098ef94, 0x1098ee94, 0x1098ed94, 0x1098ec94])
        # They sit next to the one TOC -0x3e70 points at, which only 0x107390e0 and the engine initializer 0x10739f30 load: a unit that no
        # audit owns (UTILS.CPP's slots stop at -0x3e4c, its pool is -0x3e34).
        self.assertEqual(self.word(TOC - 0x3e70), 0x1098f094)
        for a in self.loads[-0x3e70]:
            self.assertTrue(0x107390e0 <= a < 0x107393e8 or 0x10739f30 <= a < 0x10739f68, hex(a))
        # The reporter: its format string is loaded by it alone; -0x5a68 is the C library's FILE array that the stream routines load too.
        self.assertEqual({routine_of(a) for a in self.loads[-0x3b6c]}, {REPORT})
        self.assertEqual(self.string(self.word(TOC - 0x3b6c)), 'Assertion (%s) failed in "%s" on line %d\n')
        self.assertIn(REPORT, {routine_of(a) for a in self.loads[-0x5a68]})
        self.assertGreater(len({a for a in self.loads[-0x5a68] if routine_of(a) is None}), 3)

    def test_no_other_audit_contains_the_two_routines(self):
        import stdlib_core_audit, string_core_audit, utils_audit, graph_carbon_audit, memory_primitives_audit
        ranges = [(c[0], c[1]) for c in stdlib_core_audit.CONTRACTS] + [(c[0], c[1]) for c in string_core_audit.CONTRACTS]
        ranges += [(c[0], c[1]) for c in memory_primitives_audit.CONTRACTS] + [utils_audit.MODULE, graph_carbon_audit.MODULE]
        for entry, end in audit.ROUTINES:
            for start, stop in ranges:
                self.assertTrue(end <= start or entry >= stop, f'{entry:x} overlaps {start:x}-{stop:x}')
        # 0x107755c0 lies between stdlib_core's 0x10775560 body and 0x10775600 (a pinned bsearch) without being either.
        self.assertIn((0x10775560, 0x107755bc), [(s, e) for s, e, *_ in stdlib_core_audit.CONTRACTS])
        self.assertIn(0x10775600, [s for s, *_ in stdlib_core_audit.CONTRACTS])

    def test_exactly_the_listed_routines_are_routed_and_hooked_and_the_hook_checks_fail_when_they_should(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS, generate_page
        pinned = {a: (b, d) for a, b, _, d in CONTRACTS}
        self.assertTrue(audit.COMPLETE)
        self.assertEqual(set(audit.ENTRIES), ROUTED)
        listed = audit.rewritten(drafts=True)
        for entry, end, expected in listed:
            self.assertEqual(pinned[entry], (end, expected))
        self.assertEqual({entry for entry, _, _ in listed}, ROUTED)
        self.assertEqual({entry for entry, _, _ in audit.rewritten()}, ROUTED)
        self.assertEqual(set(audit.ASSERTION_HOOKED), ROUTED)
        for entry in audit.ENTRIES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], [audit.OWNER])
            self.assertEqual([at for at, _ in SEMANTIC_REPLACEMENTS if at == entry], [])
        # Both are compared on the dead stack (native/assertion_stack_exact.inc).
        text = (ROOT / 'native/assertion_stack_exact.inc').read_text()
        self.assertEqual({int(w, 16) for w in re.findall(r'0x[0-9a-f]+', re.sub(r'//.*', '', text))}, set(audit.STACK_EXACT))
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_assertion_index(db)
            with tempfile.TemporaryDirectory() as directory:
                candidate = Path(directory) / 'routines.inc'
                lines = ''.join(f'ASSERTION_ROUTINE(0x{a:x}, 0x{b:x}, R3, "{d}", name)\n' for a, b, _, d in CONTRACTS)
                candidate.write_text(lines)
                audit.require_assertion_index(db, candidate, complete=True)
                candidate.write_text(lines.replace('ASSERTION_ROUTINE(0x10739890,', 'ASSERTION_DRAFT(0x10739890,', 1))
                with self.assertRaisesRegex(ValueError, 'requires all 2'):
                    audit.require_assertion_index(db, candidate, complete=True)
        selected = {r['address'] for records in self.records.values() for r in records}
        for page in (SPLIT >> 12, REPORT >> 12):
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                part = routine_records(db, page << 12, (page + 1) << 12)
            normal = generate_page(page, part, selected | {r['address'] for r in part}, userops=self.program['userops'])
            audit.require_assertion_hooks(page, part, normal)
            self.assertIn('#include "assertion_adapter.hpp"', normal)
            entry = SPLIT if page == SPLIT >> 12 else REPORT
            hook = audit.assertion_hook(entry)
            step = f's.step(0x{entry:x});'
            fragment = normal.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
            self.assertEqual(normal.count(hook), 1)
            self.assertLess(fragment.index(hook), fragment.index(step))
            moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
            for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                    audit.require_assertion_hooks(page, part, broken)
            with mock.patch.object(audit, 'ASSERTION_HOOKED', tuple(e for e in audit.ASSERTION_HOOKED if e != entry)):
                with self.assertRaisesRegex(ValueError, 'Unexpected'):
                    audit.require_assertion_hooks(page, part, normal)

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[1]
            line = f'ASSERTION_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_assertion_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_assertion_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing assertion module instructions'):
                    audit.require_assertion_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)
            block = Path(directory) / 'assertion_routines_block.inc'
            block.write_text(line)
            candidate.write_text('#include "assertion_routines_block.inc"\n')
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])

    def test_the_entries_listing_and_the_digest_printer(self):
        text = subprocess.run([sys.executable, str(ROOT / 'tools/assertion_audit.py'), '--entries'], check=True, capture_output=True,
                              text=True).stdout.split()
        self.assertEqual(text, [f'0x{a:x}' for a in audit.ENTRIES])
        printed = subprocess.run([sys.executable, str(ROOT / 'tools/assertion_audit.py'), '0x107755c0'], check=True,
                                 capture_output=True, text=True).stdout.split()
        self.assertEqual(printed[:3], ['0x107755c0', '0x107755fc', '9e32e6cda6f2ff85'])


if __name__ == '__main__':
    unittest.main()
