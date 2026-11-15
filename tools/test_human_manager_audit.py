"""Certify HUMAN_MANAGER.CPP's 62 entries, the unit's bounds and its production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest

import human_manager_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x10145af0, 0x10145d64, 156, '6907c35dc3c94a5b'), (0x10145d70, 0x10145dcc, 23, '20ebf70bacff44fe'),
    (0x10145dd0, 0x10145e0c, 15, '8f38a1c94dadda0c'), (0x10145e10, 0x10145e6c, 23, 'fe9bf773d2c0ee85'),
    (0x10145e70, 0x10145ea0, 12, 'da19cd6cdd2971e1'), (0x10145ea0, 0x10145fd0, 76, 'c84e7a8c93215ad7'),
    (0x10145fd0, 0x101466b8, 442, '3554b831aa14fd8d'), (0x101466c0, 0x10146828, 90, '3e5465fd2914e75a'),
    (0x10146830, 0x10146a20, 124, '237afb9f40e85489'), (0x10146a20, 0x10146b8c, 91, 'd5283451a9c6abff'),
    (0x10146b90, 0x10146cfc, 91, '7e87faa5474b7274'), (0x10146d00, 0x10146e6c, 91, '8a3b8607261b770d'),
    (0x10146e70, 0x10146fdc, 91, '0770912b6761ef0d'), (0x10146fe0, 0x1014714c, 91, '095a2a16c623b00a'),
    (0x10147150, 0x1014730c, 111, '26cb79aa8f105857'), (0x10147310, 0x101476e0, 244, '1a357eb24211a882'),
    (0x101476e0, 0x10147bbc, 311, '3c8406c64067906c'), (0x10147bc0, 0x10147d18, 86, '8e93e694482dded4'),
    (0x10147d20, 0x10147e70, 84, 'bb955df90d6f7a38'), (0x10147e70, 0x10147fc8, 86, '4f9424a3327605f5'),
    (0x10147fd0, 0x101481d0, 128, 'fd81c1d330af1ced'), (0x101481d0, 0x1014833c, 91, '06248ef26e36e5b6'),
    (0x10148340, 0x101484a4, 89, 'ca58a3aa852b2b93'), (0x101484b0, 0x10148660, 108, '228dbc309a99b33e'),
    (0x10148660, 0x101487c4, 89, '187914b429d13870'), (0x101487d0, 0x10148928, 86, '245cca9211d04245'),
    (0x10148930, 0x10148a88, 86, '94d808b59259bef0'), (0x10148a90, 0x10148be8, 86, 'e0da9c458d914a1b'),
    (0x10148bf0, 0x10148d48, 86, 'd9b3379c3edaefa4'), (0x10148d50, 0x10148eac, 87, 'f9bfcc9ff1067e2c'),
    (0x10148eb0, 0x10149008, 86, '03ec84d473f6dac5'), (0x10149010, 0x10149168, 86, '3045abf7d95d2d5b'),
    (0x10149170, 0x101492c8, 86, '9ebccfe149c30519'), (0x101492d0, 0x101494a4, 117, '511384e1f70aaf67'),
    (0x101494b0, 0x10149690, 120, '427b796d490bd496'), (0x10149690, 0x10149790, 63, 'a6e22ec1faf913d5'),
    (0x10149790, 0x101499e4, 149, 'ecbe166e2310e44a'), (0x101499f0, 0x10149bd0, 120, '4ae7ff95af4ebbf6'),
    (0x10149bd0, 0x10149c8c, 47, 'd16f063e30c35fdb'), (0x10149c90, 0x10149de8, 86, '5a064d2e1700e283'),
    (0x10149df0, 0x10149f50, 88, 'd4dc7a5cd5613052'), (0x10149f50, 0x1014a1a0, 148, '0d0922b9744413f3'),
    (0x1014a1a0, 0x1014a2f8, 86, '9b72f373ee143c7f'), (0x1014a300, 0x1014b094, 868, '9e4d66b6d8ccf15c'),
    (0x1014b0a0, 0x1014b68c, 379, '0f2a244a973df112'), (0x1014b690, 0x1014b808, 94, '349478d7a1dafa0d'),
    (0x1014b810, 0x1014bb4c, 207, 'a2cb1a05b83a8d74'), (0x1014bb50, 0x1014bdbc, 155, 'ebf3cba12cbf3ee1'),
    (0x1014bdc0, 0x1014bf74, 109, '78b4699a66cb451d'), (0x1014bf80, 0x1014c0d8, 86, '59590d75c5a4a52a'),
    (0x1014c0e0, 0x1014c2dc, 127, '9169708bf07a6bbd'), (0x1014c2e0, 0x1014c48c, 107, '37a49c47dba3ac1a'),
    (0x1014c490, 0x1014c5e4, 85, '29c951040c2dd349'), (0x1014c5f0, 0x1014c678, 34, '008fe1ce872ac8f5'),
    (0x1014c680, 0x1014c88c, 131, 'd0ca378ef58f887b'), (0x1014c890, 0x1014caa0, 132, '1ca7bf91a7a8acfb'),
    (0x1014caa0, 0x1014cce4, 145, '771321a279eb5b50'), (0x1014ccf0, 0x1014cef8, 130, 'f76f0abe48bcdfe7'),
    (0x1014cf00, 0x1014d0e8, 122, '857f3d0f517fc9e5'), (0x1014d0f0, 0x1014d2fc, 131, '2bab19d845e8a788'),
    (0x1014d300, 0x1014d514, 132, '445b341c440e4cb6'), (0x1014d520, 0x1014d560, 16, 'ecaa227f571ed0c4'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# Calls from outside the unit, per entry: 878 sites, every one a `bl` of an entry.
INCOMING = {0x10145af0: 2, 0x10145ea0: 1, 0x10145fd0: 2, 0x101466c0: 10, 0x10146830: 8, 0x10146a20: 2, 0x10146b90: 2,
            0x10146d00: 2, 0x10146e70: 2, 0x10146fe0: 2, 0x10147150: 39, 0x10147310: 11, 0x101476e0: 6, 0x10147bc0: 14,
            0x10147d20: 3, 0x10147e70: 35, 0x10147fd0: 20, 0x101481d0: 17, 0x10148340: 4, 0x101484b0: 20, 0x10148660: 2,
            0x101487d0: 9, 0x10148930: 2, 0x10148a90: 2, 0x10148bf0: 1, 0x10148d50: 6, 0x10148eb0: 39, 0x10149010: 10,
            0x10149170: 1, 0x101492d0: 5, 0x101494b0: 6, 0x10149690: 1, 0x10149790: 2, 0x10149bd0: 1, 0x10149c90: 3,
            0x10149df0: 37, 0x10149f50: 13, 0x1014a1a0: 2, 0x1014b0a0: 1, 0x1014b690: 2, 0x1014b810: 1, 0x1014bb50: 1,
            0x1014bdc0: 3, 0x1014bf80: 6, 0x1014c0e0: 1, 0x1014c2e0: 418, 0x1014c490: 74, 0x1014c5f0: 7, 0x1014c680: 1,
            0x1014c890: 8, 0x1014caa0: 2, 0x1014ccf0: 1, 0x1014cf00: 2, 0x1014d0f0: 1, 0x1014d300: 5}
# Entries another routine of the unit calls.
CROSS = {0x10145fd0, 0x10147e70, 0x101481d0, 0x101484b0, 0x10149170, 0x101494b0, 0x101499f0, 0x1014a300, 0x1014c2e0}
# The unit's private TOC slots with (loads, first load, last load). No other code loads them.
PRIVATE_SLOTS = {
    '-0x54a4': (1, 0x10147174, 0x10147174),    # a data word
    '-0x54a0': (53, 0x10145b0c, 0x1014d324),   # the string pool
    '-0x549c': (1, 0x10145b10, 0x10145b10),    # two scrambled names (the CD check)
    '-0x5498': (1, 0x10145b14, 0x10145b14),
    '-0x73f8': (6, 0x1014b69c, 0x1014c298),    # the comparator's vector
    '-0x73f4': (1, 0x1014a310, 0x1014a310),    # a data word
    '-0x73e8': (1, 0x10145e14, 0x10145e14),    # the tactic slot's constructor vector
    '-0x73e4': (2, 0x10145d90, 0x10145e20),    # the tactic slot's destructor vector
    '-0x73e0': (3, 0x10145c6c, 0x1014a4bc),    # the record's destructor vector
    '-0x73dc': (2, 0x10145c68, 0x1014a4b8),    # the record's constructor vector
}
# The neighbouring units' slots on either side of the runs: (loads, first, last), all outside the unit.
NEIGHBOUR_SLOTS = {'-0x54a8': (1, 0x10143298, 0x10143298), '-0x5494': (1, 0x10167eb0, 0x10167eb0),
                   '-0x73fc': (8, 0x1014363c, 0x10144cb0)}
# The five pointers to the unit: transition vectors (code address, then the TOC), each loaded
# through its own TOC slot.
VECTORS = {0x108b8b1c: (0x1014d520, -0x73f8), 0x108b8b24: (0x10145e70, -0x73e8), 0x108b8b2c: (0x10145dd0, -0x73e4),
           0x108b8b34: (0x10145d70, -0x73e0), 0x108b8b3c: (0x10145e10, -0x73dc)}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private HUMAN_MANAGER export unavailable; inventory cannot be certified')


class HumanManagerAuditTests(unittest.TestCase):
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

    def routine_of(self, address):
        return next((start for start, end in audit.ROUTINES if start <= address < end), None)

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.MODULE, (0x10145af0, 0x1014d560))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.HUMAN_MANAGER_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 62)
        self.assertEqual(len(self.records), 7746)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_human_manager_index(db, complete=False)

    def test_every_entry_is_routed_to_the_units_owner_alone(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        # Every entry is routed (no draft left), and nothing else owns one.
        self.assertTrue(audit.COMPLETE)
        self.assertEqual(audit.rewritten(), [(a, b, pinned) for a, b, _, pinned in CONTRACTS])
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ['human_manager_adapter'])
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_human_manager_index(db)
            audit.require_human_manager_index(db, complete=True)

    def assert_hook(self, code, entry):
        hook = audit.human_manager_hook(entry)
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
            audit.require_human_manager_hooks(page, part, normal)
            self.assertIn('#include "human_manager_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.human_manager_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_human_manager_hooks(page, part, broken)

    def test_unit_bounds_are_its_private_toc_slots_and_its_neighbours(self):
        pool = self.word(TOC - 0x54a0)
        self.assertEqual(self.original_bytes(pool + 0x37, 0x30).split(b'\0')[:3], [b'HUMAN_MANAGER.CPP', b'v%s %s.%s %d', b'Error'])
        self.assertEqual(self.original_bytes(pool, 9), b'Insert CD')
        # HISTORY.CPP's pool ends the run before; INDEX.CPP's starts the one after.
        self.assertIn(b'HISTORY.CPP\0', self.original_bytes(self.word(TOC - 0x54b4), 0x60))
        self.assertEqual(self.original_bytes(self.word(TOC - 0x547c), 9), b'CM3_TEMP\0')

        def loads(slot):
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                found = [a for a, in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                (f'%{slot}(r2)%',))]
            return len(found), found[0], found[-1]
        for slot, expected in PRIVATE_SLOTS.items():
            self.assertEqual(loads(slot), expected, slot)
            self.assertTrue(audit.MODULE[0] <= expected[1] <= expected[2] < audit.MODULE[1])
        self.assertEqual({int(slot, 16) for slot in PRIVATE_SLOTS},
                         set(range(-0x54a4, -0x5494, 4)) | {-0x73f8, -0x73f4, -0x73e8, -0x73e4, -0x73e0, -0x73dc})
        for slot, expected in NEIGHBOUR_SLOTS.items():
            self.assertEqual(loads(slot), expected, slot)
            self.assertFalse(audit.MODULE[0] <= expected[1] < audit.MODULE[1])
            self.assertFalse(audit.MODULE[0] <= expected[2] < audit.MODULE[1])
        # Before: the two routines at 0x10145a30 and 0x10145aa0 are HISTORY.CPP's (their vectors sit in
        # HISTORY.CPP's slots, loaded by HISTORY.CPP code). After: INDEX.CPP starts at the unit's end.
        self.assertEqual((self.word(self.word(TOC - 0x54c4)), self.word(self.word(TOC - 0x54c8))), (0x10145a30, 0x10145aa0))
        for slot in ('-0x54c4', '-0x54c8'):
            count, first, last = loads(slot)
            self.assertTrue(first >= 0x10143000 and last < audit.MODULE[0])
        self.assertEqual(self.routine_of(0x10145b0c), ENTRIES[0])
        self.assertEqual(self.routine_of(0x1014d324), 0x1014d300)
        self.assertEqual(self.routine_of(0x1014b69c), 0x1014b690)

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
        self.assertEqual(sum(incoming.values()), 878)
        self.assertEqual(fallthroughs, [])
        # Every entry is reached: by an outside call, a call inside the unit, or a vector.
        vectored = {target for target, _ in VECTORS.values()}
        self.assertEqual(set(INCOMING) | CROSS | vectored, set(ENTRIES))
        self.assertEqual(vectored & (set(INCOMING) | CROSS), set())

    def test_cross_entry_edges_are_calls_of_entries_and_computed_flow_is_the_two_tables(self):
        cross, computed = set(), {}
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                if 'COMPUTED' in record['flow']:
                    self.assertEqual((record['assembly'], record['flow']), ('bctr', 'COMPUTED_JUMP'))
                    computed[record['address']] = start
                targets = record['flows'] + ([record['fallthrough']] if record.get('fallthrough') is not None else [])
                for target in targets:
                    if not start <= target < end:
                        # No tail call and no branch out of a routine: only calls leave it.
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        if audit.MODULE[0] <= target < audit.MODULE[1]:
                            self.assertIn(target, ENTRIES)
                            cross.add(target)
        self.assertEqual(cross, CROSS)
        self.assertEqual(computed, {})
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
            self.assertEqual(self.by_address[address - 4]['flow'], 'UNCONDITIONAL_JUMP')
        for address in set(range(*audit.MODULE, 4)) - addresses:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)

    def test_pointers_to_the_unit_are_its_five_vectors(self):
        code_end = self.segments[0][0] + len(self.segments[0][1])
        seen = set()
        for start, contents in self.segments:
            for match in re.finditer(b'\x10\x14', contents):
                at = start + match.start()
                if match.start() + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[match.start():match.start() + 4], 'big')
                if not audit.MODULE[0] <= target < audit.MODULE[1]:
                    continue
                if at in VECTORS:
                    entry, slot = VECTORS[at]
                    self.assertEqual((target, self.word(at + 4), self.word(TOC + slot)), (entry, TOC, at))
                    seen.add(at)
                elif at % 4:
                    # Bytes of instructions or data that read as an address: never a word-aligned pointer.
                    self.assertNotIn(target, ENTRIES)
                else:
                    # No jump table: any other aligned word naming the unit is code, not a pointer.
                    self.assertLess(at, code_end)
        self.assertEqual(seen, set(VECTORS))

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[1]
            line = f'HUMAN_MANAGER_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_human_manager_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_human_manager_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing HUMAN_MANAGER.CPP instructions'):
                    audit.require_human_manager_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)


if __name__ == '__main__':
    unittest.main()
