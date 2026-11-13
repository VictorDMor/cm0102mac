"""Certify DATE.CPP's closed inventory and production routing.

Private original evidence is mandatory: a missing export fails rather than skips.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import date_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
PRIVATE = ROOT / '.local/rewrites/date'
CONTRACTS = (
    (0x107156b0, 0x107156f0, 16, 'eb4e6c047bf4654b'), (0x107156f0, 0x1071587c, 99, '61bbf98b187b15ed'),
    (0x10715880, 0x1071589c, 7, '8114b9442a84dbdd'), (0x107158a0, 0x10715a28, 98, '9d48baefcf3c0d79'),
    (0x10715a30, 0x10715b1c, 59, '57eddb71e3fefbe1'), (0x10715b20, 0x10715bc0, 40, 'aaf5689e8439cfcc'),
    (0x10715bc0, 0x10715df4, 141, '20e5dcdc136bd147'), (0x10715e00, 0x1071612c, 203, 'fad23ddccff8430c'),
    (0x10716130, 0x107164ec, 239, '0964cb38344761e3'), (0x107164f0, 0x1071689c, 235, '2a4725e817af8646'),
    (0x107168a0, 0x10716afc, 151, '20286c611d3dc12c'), (0x10716b00, 0x10716ea4, 233, '70d9e81ac834b555'),
    (0x10716eb0, 0x10717268, 238, '31a9b2a0522a8782'), (0x10717270, 0x10717620, 236, '3695e6059bcfea6f'),
    (0x10717620, 0x10717888, 154, 'bc27f5450199ca5a'), (0x10717890, 0x10717a0c, 95, '84e975a62706d146'),
    (0x10717a10, 0x10717b1c, 67, '1c656d8487a1e52e'), (0x10717b20, 0x10717ca8, 98, '851dd1def01feaff'),
    (0x10717cb0, 0x10717dc4, 69, '66d2161513bec505'), (0x10717dd0, 0x10718130, 216, 'd2371c1d86107d80'),
    (0x10718130, 0x107182f0, 111, 'f8040a3dae70994f'), (0x107182f0, 0x10718354, 24, 'ad0726e4884cc04e'),
    (0x10718360, 0x10718438, 54, '0d513862f7d764c8'), (0x10718440, 0x107184cc, 35, '144fc37ca054d864'),
    (0x107184d0, 0x10718570, 40, '7c232a56230fc89c'), (0x10718570, 0x1071864c, 55, 'f86f82a48e94b088'),
    (0x10718650, 0x107186f8, 42, 'da59a839700884a7'), (0x10718700, 0x107187cc, 51, '17faf112d5504eb9'),
    (0x107187d0, 0x1071889c, 51, 'a7bdb992e00810e2'), (0x107188a0, 0x107189c4, 73, '317149b1b96f9527'),
    (0x107189d0, 0x10718af0, 72, '01fd7766f17f9d90'), (0x10718af0, 0x10718b38, 18, 'aef29aca20c76773'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
NOPS = {0x1071820c, 0x1071833c}
TOC = 0x108bba90
UNIT_END = 0x10718af0  # where DATE.CPP's own routines end and the neighbour begins
# The unit's data: the pool, the month starts, the static dates and text buffers, the jump tables
# (TOC slots), and the guard bytes of the static dates (bytes relative to r2).
UNIT_SLOTS = range(-0x3fe8, -0x3fa0 + 1, 4)
GUARD_BYTES = range(-0x35c0, -0x35bb + 1)
# Each name routine's switch: TOC slot of its table of code addresses, and the case label of each
# index (native/date_text.cpp finds the case by its address).
JUMP_TABLES = {
    # Weekday 0 is Monday; the code has Sunday's case first.
    -0x3fd0: [0x10718738 + 0x10 * ((n + 1) % 7) for n in range(7)],
    -0x3fd8: [0x10718808 + 0x10 * ((n + 1) % 7) for n in range(7)],
    -0x3fe0: [0x107188d8, 0x107188e8, 0x107188f8, 0x1071890c, 0x1071891c, 0x1071892c, 0x1071893c, 0x1071894c,
              0x1071895c, 0x1071896c, 0x1071897c, 0x1071898c],
    -0x3fe8: [0x10718a08, 0x10718a18, 0x10718a28, 0x10718a38, 0x10718a48, 0x10718a5c, 0x10718a6c, 0x10718a7c,
              0x10718a8c, 0x10718a9c, 0x10718aac, 0x10718abc],
}
# Calls the unit makes to other units.
LEAVING = {0x101fe9f0, 0x10331860, 0x10727880, 0x10727890, 0x10739890, 0x107467a0, 0x10768810, 0x10769b60,
           0x1077b720, 0x1077dbf0}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file() or not (path / 'program.json').is_file():
        raise RuntimeError('Private export unavailable; the DATE.CPP inventory cannot be certified')


class DateAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        PRIVATE.mkdir(parents=True, exist_ok=True)
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

    def test_exact_entries_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x107156b0, 0x10718b38))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.DATE_ENTRIES, ENTRIES)
        self.assertEqual(audit.NEIGHBOUR, UNIT_END)
        self.assertEqual(audit.UNREACHABLE_NOPS, frozenset(NOPS))
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(CONTRACTS), 32)
        self.assertEqual(len(self.records), 3320)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ['date_adapter'])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_date_index(db)

    def test_source_name_and_data_close_the_boundary(self):
        pool = self.word(TOC - 0x3fa0)
        self.assertEqual(self.original_bytes(pool, 9), b'DATE.CPP\0')
        users, guards = {}, {}
        slot = re.compile(r'(-0x[0-9a-f]+)\(r2\)')
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, body in db.execute("SELECT address,body FROM instructions WHERE body LIKE '%(r2)%'"):
                found = slot.search(json.loads(body)['assembly'])
                if not found:
                    continue
                offset = int(found.group(1), 16)
                if offset in UNIT_SLOTS:
                    users.setdefault(offset, []).append(address)
                elif offset in GUARD_BYTES:
                    guards.setdefault(offset, []).append(address)
            before = [json.loads(body)['assembly'] for body, in db.execute(
                'SELECT body FROM instructions WHERE address IN (?,?) ORDER BY address', (0x107156a0, 0x107156a4))]
        # Every slot and every guard byte is used, and only by the 31 routines before the neighbour.
        self.assertEqual(sorted(users), list(UNIT_SLOTS))
        self.assertEqual(sorted(guards), list(GUARD_BYTES))
        for addresses in (*users.values(), *guards.values()):
            for address in addresses:
                self.assertTrue(audit.MODULE[0] <= address < UNIT_END, hex(address))
        # The routine before the unit is a two-instruction stub; the neighbour uses no slot at all.
        self.assertEqual(before, ['li r3,0x0', 'blr'])
        for record in self.records:
            if record['address'] >= UNIT_END:
                self.assertNotIn('(r2)', record['assembly'])

    def test_every_caller_enters_at_an_entry(self):
        incoming, callers = set(), set()
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    'WHERE f.value>=? AND f.value<? ORDER BY i.address', audit.MODULE):
                if audit.MODULE[0] <= address < audit.MODULE[1]:
                    continue
                record = json.loads(body)
                self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                incoming.update(t for t in record['flows'] if audit.MODULE[0] <= t < audit.MODULE[1])
                callers.add(address)
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*audit.MODULE, *audit.MODULE)))
        # The short time of day and the short weekday name are called only inside the unit.
        self.assertEqual(set(ENTRIES) - incoming, {0x107184d0, 0x10718700})
        self.assertEqual(incoming - set(ENTRIES), set())
        self.assertEqual(len(callers), 2472)
        self.assertEqual(fallthroughs, [])

    def test_flows_leave_only_through_calls_and_the_four_switches(self):
        leaving, computed = set(), []
        entries = set(ENTRIES)
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                if 'COMPUTED' in record['flow']:
                    computed.append(record['address'])
                for target in record['flows']:
                    if start <= target < end:
                        continue
                    if audit.MODULE[0] <= target < audit.MODULE[1]:
                        self.assertIn(target, entries)
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                    else:
                        leaving.add(target)
        self.assertEqual(leaving, LEAVING)
        self.assertEqual(computed, [0x10718734, 0x10718804, 0x107188d4, 0x10718a04])

    def test_jump_tables_hold_the_case_labels_the_rewrite_expects(self):
        for slot, cases in JUMP_TABLES.items():
            table = self.word(TOC + slot)
            self.assertEqual([self.word(table + 4 * n) for n in range(len(cases))], cases)
            for case in cases:
                self.assertIn(case, self.by_address)

    def test_body_words_padding_and_unreachable_nops_accounted(self):
        exported = set(self.by_address)
        bodies = set()
        for start, end in audit.ROUTINES:
            bodies.update(range(start, end, 4))
        for address in range(*audit.MODULE, 4):
            if address in exported:
                self.assertIn(address, bodies)
                continue
            self.assertEqual(self.word(address), 0x60000000 if address in NOPS else 0)
            self.assertEqual(address in NOPS, address in bodies)
        self.assertEqual(exported & NOPS, set())

    def test_stale_incomplete_and_duplicate_lists_fail(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing DATE.CPP instructions'):
                audit.require_date_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten())]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_date_index(db)
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'DATE_ROUTINE(' in line)
        with tempfile.TemporaryDirectory(dir=PRIVATE) as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('0x107156b0, 0x107156f0', '0x107156b4, 0x107156f0', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            for changed in (contents.replace('DATE_ROUTINE(', 'DATE_DRAFT(', 1), contents.replace(first, '')):
                candidate.write_text(changed)
                with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                    with self.assertRaisesRegex(ValueError, 'all 32 complete'):
                        audit.require_date_index(db, candidate)

    def test_generated_hooks_precede_original_steps_with_negative_controls(self):
        from compile_recomp import generate_page
        for page in (0x10715, 0x10716, 0x10717, 0x10718):
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                part = [json.loads(body) for body, in db.execute(
                    'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                    (page << 12, (page + 1) << 12))]
            selected = {r['address'] for r in part} | {t for r in part for t in r['flows']} | \
                {r['fallthrough'] for r in part if r.get('fallthrough')}
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_date_hooks(page, part, normal)
            self.assertIn('#include "date_adapter.hpp"', normal)
            self.assertNotIn('try_modern_date_subsystem', normal)
            self.assertNotIn('try_modern_calendar', normal)
            original = generate_page(page, part, selected, userops=self.program['userops'], date_hooks=False)
            self.assertNotIn('try_modern_date', original)
            for entry in (e for e in ENTRIES if e >> 12 == page):
                hook, step = audit.date_hook(entry), f's.step(0x{entry:x});'
                fragment = normal.split(f'i_{entry:x}:', 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
                self.assertLess(fragment.index(hook), fragment.index(step))
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_date_hooks(page, part, broken)


if __name__ == '__main__':
    unittest.main()
