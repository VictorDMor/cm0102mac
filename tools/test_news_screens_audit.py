"""Certify NEWS_SCREENS.CPP's entries, original boundary and production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import struct
import unittest

import news_screens_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
GENERATED = ROOT / '.local/current/generated'
CONTRACTS = (
    (0x102ce710, 0x102cea2c, 199, '736f3c5b9c9129ed'),
    (0x102cea30, 0x102cfcd0, 1191, '528990297196ea8e'),
    (0x102cfcd0, 0x102d022c, 343, '636316d1f7526738'),
    (0x102d0230, 0x102d02a4, 29, '9d18648de584528b'),
    (0x102d02b0, 0x102d036c, 47, '5425244fe584d342'),
    (0x102d0370, 0x102d0500, 100, '0721647d32749af3'),
    (0x102d0500, 0x102d07b0, 172, '2a1f8d0329d0e80b'),
    (0x102d07b0, 0x102d0a50, 168, '5d1d90a5eb8d89c4'),
)
TOC = 0x108bba90
# Incoming `bl`s: open_news from the eight callers, the dialog from the one.
INCOMING = ((0x1011b4a0, 0x102ce710), (0x1011b574, 0x102ce710), (0x1011b614, 0x102ce710), (0x101cc0f4, 0x102ce710),
            (0x102a5ef4, 0x102ce710), (0x102a6dfc, 0x102ce710), (0x102a74a8, 0x102ce710), (0x102a77e4, 0x102d02b0),
            (0x102a7d80, 0x102ce710))
VECTORS = ((0x108b8d6c, 0x102d0370), (0x108b8d74, 0x102d07b0), (0x108b8d7c, 0x102d0500),
           (0x108b8d84, 0x102d0230), (0x108b8d8c, 0x102cfcd0), (0x108b8d94, 0x102cea30))
JUMP_TABLE = 0x108ce92c
VOWEL, OTHER = 0x102ced24, 0x102ced38


def word(segment, base, address):
    return struct.unpack('>I', segment[address - base:address - base + 4])[0]


class NewsScreensAudit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.code = (EXPORT / 'segment-0.bin').read_bytes()
        cls.data = (EXPORT / 'segment-1.bin').read_bytes()
        cls.db = sqlite3.connect(EXPORT / 'instructions.sqlite')

    @classmethod
    def tearDownClass(cls):
        cls.db.close()

    def data_word(self, address):
        return word(self.data, 277559952, address)

    def code_text(self, address):
        offset = address - 0x10000000
        return self.code[offset:self.code.index(b'\0', offset)]

    def test_contracts_match_routines_and_list(self):
        self.assertEqual(tuple((a, b) for a, b, _, _ in CONTRACTS), audit.ROUTINES)
        self.assertEqual(sum(n for _, _, n, _ in CONTRACTS), 2249)
        for entry, end, count, fingerprint in CONTRACTS:
            records = routine_records(self.db, entry, end)
            self.assertEqual(len(records), count)
            self.assertEqual(digest(records), fingerprint)
            self.assertEqual(records[0]['assembly'], 'mfspr r0,LR')
            self.assertEqual(records[-1]['assembly'], 'blr')
        audit.require_news_screens_index(self.db)

    def test_module_boundary(self):
        at = lambda address: json.loads(self.db.execute('SELECT body FROM instructions WHERE address=?', (address,)).fetchone()[0])['assembly']
        self.assertEqual(at(0x102ce704), 'blr')
        self.assertEqual(at(audit.MODULE[0]), 'mfspr r0,LR')
        self.assertEqual(at(0x102d0a4c), 'blr')
        self.assertEqual(at(audit.MODULE[1]), 'mfspr r0,LR')
        for slot, address, text in ((-0x5100, 0x107f14bc, b'NEWS.CPP'), (-0x50f0, 0x107f54c8, b'NEWS_SCREENS.CPP'),
                                    (-0x50cc, 0x107f5734, b'Insert CD')):
            self.assertEqual(self.data_word(TOC + slot), address)
            self.assertEqual(self.code_text(address), text)
        loads = lambda slot: [a for a, in self.db.execute(
            "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", (f'%{slot}(r2)%',))]
        self.assertEqual(loads('-0x5100')[-1], 0x102ce32c)
        self.assertEqual(loads('-0x50f0'), [0x102ce728, 0x102cea50, 0x102d0520])
        self.assertEqual(loads('-0x50f4'), [0x102ced10])
        self.assertEqual(loads('-0x50e4'), [0x102ce730])
        self.assertEqual(loads('-0x50e8'), [0x102ce738])
        self.assertEqual(loads('-0x50ec'), [0x102ce740])
        self.assertEqual(loads('-0x50f8'), [0x102d02c8, 0x102d0380])
        self.assertEqual(loads('-0x50fc'), [0x102d02d8, 0x102d037c])
        self.assertGreaterEqual(loads('-0x50cc')[0], audit.MODULE[1])

    def test_entries_are_closed(self):
        entries = {entry for entry, _ in audit.ROUTINES}
        incoming = []
        for address, body in self.db.execute('SELECT address, body FROM instructions'):
            assembly = json.loads(body)['assembly']
            found = re.match(r'(\w+)\s+(?:cr\d,)?0x([0-9a-f]+)$', assembly)
            if not found:
                continue
            target = int(found.group(2), 16)
            inside = audit.MODULE[0] <= address < audit.MODULE[1]
            if audit.MODULE[0] <= target < audit.MODULE[1] and not inside:
                incoming.append((address, target))
                self.assertEqual(found.group(1), 'bl')
            if inside and found.group(1) == 'bl' and audit.MODULE[0] <= target < audit.MODULE[1]:
                self.fail(f'internal bl at {address:x}')
        self.assertEqual(tuple(sorted(incoming)), INCOMING)
        self.assertEqual({target for _, target in INCOMING} | {target for _, target in VECTORS}, entries)
        # Every data word pointing into the module is one of the vectors or the jump table.
        base = 277559952
        pointing = {base + offset: struct.unpack('>I', self.data[offset:offset + 4])[0]
                    for offset in range(0, len(self.data) - 3, 4)
                    if audit.MODULE[0] <= struct.unpack('>I', self.data[offset:offset + 4])[0] < audit.MODULE[1]}
        table = {JUMP_TABLE + 4 * n for n in range(0x39)}
        self.assertEqual(set(pointing), set(a for a, _ in VECTORS) | table)
        for address, target in VECTORS:
            self.assertEqual(pointing[address], target)
            self.assertEqual(self.data_word(address + 4), TOC)
        vowels = set('AEIOUYaeiouy')
        for n in range(0x39):
            self.assertEqual(pointing[JUMP_TABLE + 4 * n], VOWEL if chr(n + 0x41) in vowels else OTHER)
        self.assertEqual(self.data_word(TOC - 0x50f4), JUMP_TABLE)

    def test_gaps_are_padding_or_unreachable(self):
        addresses = {a for a, in self.db.execute('SELECT address FROM instructions WHERE address>=? AND address<?', audit.MODULE)}
        gaps = [a for a in range(*audit.MODULE, 4) if a not in addresses]
        self.assertEqual(word(self.code, 0x10000000, 0x102cf4cc), 0x60000000)  # after `b`, nothing reaches it
        self.assertEqual({a for a in gaps if word(self.code, 0x10000000, a) != 0}, set(audit.UNREACHABLE))

    def test_volatile_reads_are_the_pointer_glue_only(self):
        # The rewrite sets r12 for its one pointer call, in news_command.
        self.assertEqual(audit.volatile_reads(self.db), ['r12 read at 10781320 in 10781320, called by the module at 102d01dc'])

    def test_source_is_not_a_builder(self):
        for path in ROOT.glob('native/news_screens_*.cpp'):
            if not path.name.endswith('_checks.cpp'):
                self.assertNotIn('SemanticGuestCall', path.read_text(), path.name)

    def test_every_entry_is_hooked_in_generated_pages(self):
        for entry, _ in audit.ROUTINES:
            page = GENERATED / f'page_{entry >> 12:x}.cpp'
            code = page.read_text()
            self.assertEqual(code.count(audit.news_screens_hook(entry)), 1, f'{entry:x}')

    def test_call_sites_follow_the_listing(self):
        # Each return address the source names follows a `bl` to that routine in the original.
        names = dict(re.findall(r'(\w+) = (0x10[0-9a-f]{6})', (ROOT / 'native/news_screens_detail.hpp').read_text()))
        calls = {}
        for address, body in self.db.execute('SELECT address, body FROM instructions WHERE address>=? AND address<?', audit.MODULE):
            found = re.match(r'bl 0x([0-9a-f]+)$', json.loads(body)['assembly'])
            if found:
                calls[address + 4] = int(found.group(1), 16)
        source = ''.join(path.read_text() for path in ROOT.glob('native/news_screens_*.cpp')
                         if not path.name.endswith(('_checks.cpp', '_adapter.cpp')))
        seen = 0
        for name, back in re.findall(r'\bbl\(c, (\w+), (?:\w+ \? )?(0x102[0-9a-f]{5})', source):
            self.assertEqual(calls.get(int(back, 16)), int(names[name], 16), f'{name} {back}')
            seen += 1
        for kind, back in re.findall(r'history\.(value|set)\([^;]*?(0x102[0-9a-f]{5})\)', source):
            self.assertEqual(calls.get(int(back, 16)), 0x1073ef70 if kind == 'value' else 0x1073f1e0, back)
            seen += 1
        self.assertGreater(seen, 150)


if __name__ == '__main__':
    unittest.main()
