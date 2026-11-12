"""Certify the sort family's closed inventory and production routing.

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

import qsort4x_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
PRIVATE = ROOT / '.local/rewrites/qsort4x'
CONTRACTS = (
    (0x10768300, 0x10768340, 16, 'f9ff2e514c82235a'),
    (0x10768340, 0x107684b8, 92, '7d0b9c27d4bf7f20'),
    (0x107684c0, 0x10768658, 100, '03b276fd57bf4a61'),
    (0x10768660, 0x10768808, 106, '3e9365c6efabaf31'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
NOPS = {0x107683ac, 0x1076846c, 0x10768550, 0x10768610}
PADDING = {0x107684b8, 0x107684bc, 0x10768658, 0x1076865c}
# The entry's three calls, by the remainder of the row width.
OWNED_CALLS = {(0x10768314, 0x10768660), (0x10768324, 0x107684c0), (0x1076832c, 0x10768340)}
TOC = 0x108bba90
# Bytes of two neighbouring calls (0x1063220c, 0x10632210) that read as an
# address inside the word sort when taken from an odd offset; not a word of data.
FALSE_POINTERS = {0x1063220d: 0x10768548}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file() or not (path / 'program.json').is_file():
        raise RuntimeError('Private export unavailable; the sort family inventory cannot be certified')


class Qsort4xAuditTests(unittest.TestCase):
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

    def assert_production(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(audit.ROUTINES))
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in owners if at == entry], ['qsort4x_adapter'])

    def test_exact_entries_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x10768300, 0x10768808))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.QSORT4X_ENTRIES, ENTRIES)
        self.assertEqual(audit.UNREACHABLE_NOPS, frozenset(NOPS))
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(self.records), 314)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        self.assert_production(audit.rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_qsort4x_index(db)

    def test_source_names_and_neighbors_close_boundary(self):
        # The word and pair sorts name their source units in their assertions;
        # the entry and the byte sort carry none.
        for slot, text in ((-0x3c04, b'(member_size & 3) == 0\0qsort4x.c\0'),
                           (-0x3c00, b'(member_size & 7) == 0\0qsort8x.c\0')):
            self.assertEqual(self.original_bytes(self.word(TOC + slot), len(text)), text)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            users = {slot: [a for a, in db.execute(
                'SELECT address FROM instructions WHERE body LIKE ? ORDER BY address', (f'%"lwz r4,{slot}(r2)"%',))]
                for slot in ('-0x3c04', '-0x3c00')}
            self.assertEqual(users, {'-0x3c04': [0x107684cc], '-0x3c00': [0x1076866c]})
            assembly = {a: json.loads(b)['assembly'] for a, b in db.execute(
                'SELECT address,body FROM instructions WHERE address IN (?,?,?,?)',
                (0x107682f8, 0x10768300, 0x10768804, 0x10768810))}
        self.assertEqual(assembly, {0x107682f8: 'blr', 0x10768300: 'mfspr r0,LR',
                                    0x10768804: 'blr', 0x10768810: 'lwz r3,-0x5b20(r2)'})
        for address in (0x107682fc, 0x10768808, 0x1076880c, *PADDING):
            self.assertEqual(self.word(address), 0)

    def test_every_caller_enters_at_the_entry(self):
        incoming, callers = [], set()
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    'WHERE f.value>=? AND f.value<? ORDER BY i.address', audit.MODULE):
                if audit.MODULE[0] <= address < audit.MODULE[1]:
                    continue
                record = json.loads(body)
                self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                incoming.extend(t for t in record['flows'] if audit.MODULE[0] <= t < audit.MODULE[1])
                callers.add(address)
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*audit.MODULE, *audit.MODULE)))
        self.assertEqual(set(incoming), {0x10768300})
        self.assertGreater(len(callers), 300)
        self.assertEqual(fallthroughs, [])

    def test_flows_between_routines_are_the_entrys_three_calls(self):
        cross, leaving = set(), set()
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                self.assertNotIn('COMPUTED', record['flow'])
                for target in record['flows']:
                    if start <= target < end:
                        continue
                    if audit.MODULE[0] <= target < audit.MODULE[1]:
                        cross.add((record['address'], target))
                    else:
                        leaving.add(target)
        self.assertEqual(cross, OWNED_CALLS)
        # The comparator glue and the assertion report.
        self.assertEqual(leaving, {0x10781320, 0x107755c0})

    def test_body_words_padding_and_unreachable_nops_accounted(self):
        exported = set(self.by_address)
        for address in range(*audit.MODULE, 4):
            if address in exported:
                continue
            self.assertIn(address, NOPS | PADDING)
            self.assertEqual(self.word(address), 0x60000000 if address in NOPS else 0)
        self.assertEqual(exported & (NOPS | PADDING), set())

    def test_no_word_of_the_program_points_into_the_family(self):
        # Every byte offset of every segment, overlapping matches included.
        pointers = {}
        for start, contents in self.segments:
            for match in re.finditer(b'(?=\x10\x76[\x83-\x88])', contents):
                offset = match.start()
                if offset + 4 <= len(contents):
                    target = int.from_bytes(contents[offset:offset + 4], 'big')
                    if audit.MODULE[0] <= target < audit.MODULE[1]:
                        pointers[start + offset] = target
        self.assertEqual(pointers, FALSE_POINTERS)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address in FALSE_POINTERS:
                self.assertEqual(len(list(db.execute('SELECT 1 FROM instructions WHERE address IN (?,?)',
                                                     (address - 1, address + 3)))), 2)

    def test_stale_incomplete_and_duplicate_lists_fail(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing sort family instructions'):
                audit.require_qsort4x_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten())]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_qsort4x_index(db)
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'QSORT4X_ROUTINE(' in line)
        with tempfile.TemporaryDirectory(dir=PRIVATE) as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('0x10768340, R3', '0x10768344, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            for changed in (contents.replace('QSORT4X_ROUTINE(', 'QSORT4X_DRAFT(', 1), contents.replace(first, '')):
                candidate.write_text(changed)
                with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                    with self.assertRaisesRegex(ValueError, 'all four complete'):
                        audit.require_qsort4x_index(db, candidate)

    def test_generated_hooks_precede_original_steps_with_negative_controls(self):
        from compile_recomp import generate_page
        page = 0x10768
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            part = [json.loads(body) for body, in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                (page << 12, (page + 1) << 12))]
        selected = {r['address'] for r in part} | {0x10781320, 0x107755c0}
        normal = generate_page(page, part, selected, userops=self.program['userops'])
        audit.require_qsort4x_hooks(page, part, normal)
        self.assertIn('#include "qsort4x_adapter.hpp"', normal)
        original = generate_page(page, part, selected, userops=self.program['userops'], qsort4x_hooks=False)
        self.assertNotIn('try_modern_qsort4x', original)
        for entry in ENTRIES:
            hook, step = audit.qsort4x_hook(entry), f's.step(0x{entry:x});'
            fragment = normal.split(f'i_{entry:x}:', 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
            self.assertLess(fragment.index(hook), fragment.index(step))
            moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
            for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                    audit.require_qsort4x_hooks(page, part, broken)
        # The fused 8-byte sort keeps the first word at the pair sort's entry.
        pair = normal.split('i_10768660:', 1)[1].split('i_10768664:', 1)[0]
        self.assertLess(pair.index('try_native_sort8(s)'), pair.index(audit.qsort4x_hook(0x10768660)))


if __name__ == '__main__':
    unittest.main()
