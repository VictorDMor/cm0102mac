"""Certify COMP_UTIL.CPP's 35 entries, the unit's bounds and (today) that nothing is routed to C++.

The private export is required evidence, rather than a reason to skip tests. Production ownership is
stated, not skipped: ZERO_ROUTED is true, so the tests assert that no entry is in the generator's owner
tables, that no hook is emitted and that the hook checker rejects a page without hooks. The hook step
flips ZERO_ROUTED and replaces those assertions with cup_audit's (hook before the first step, once).
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest

import comp_util_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
SLOT = re.compile(r'(-?0x[0-9a-f]+)\(r2\)')
CONTRACTS = (
    (0x10462cd0, 0x10462ffc, 203, 'e8c94ec7bb82f811'), (0x10463000, 0x10463578, 350, '5b936d325b255323'),
    (0x10463580, 0x104635a4, 9, 'cef5023dd107d8cb'), (0x104635b0, 0x10463988, 245, '25e0c09b62a258e4'),
    (0x10463990, 0x10463a90, 64, 'e4550804c03327b6'), (0x10463a90, 0x10463bd8, 81, '76ed4ef8e9f8ec08'),
    (0x10463be0, 0x10463dbc, 119, '3e7e3bd51d4149a8'), (0x10463dc0, 0x10464008, 146, '2e68df70fcb6f0d1'),
    (0x10464010, 0x10464154, 81, '2ec40f65e040580e'), (0x10464160, 0x104646c4, 345, '81b7a3a8a62d4dc0'),
    (0x104646d0, 0x104647a0, 52, 'cee4821e44300eee'), (0x104647a0, 0x104647b8, 6, 'e4856676bcc809bc'),
    (0x104647c0, 0x104647d8, 6, '782dc2b055d088ec'), (0x104647e0, 0x10464830, 20, '15a230304f39331c'),
    (0x10464830, 0x10464880, 20, '8dfdbda40c785cb3'), (0x10464880, 0x104648b8, 14, '7e8980ea389b9fb8'),
    (0x104648c0, 0x104648fc, 15, '0fea99944b73c6a3'), (0x10464900, 0x10464d00, 256, '6efd0d16e2a01a08'),
    (0x10464d00, 0x10469704, 4737, 'e5022cc4bb32fb42'), (0x10469710, 0x104698b4, 105, 'ee2f3edcb7bbd59c'),
    (0x104698c0, 0x10469bac, 187, '3b4f9db63fd824f6'), (0x10469bb0, 0x1046f1ec, 5519, '6edd52ac6392530e'),
    (0x1046f1f0, 0x10476774, 7521, '49730e9e942e113d'), (0x10476780, 0x10476898, 70, '2e52294b3d430d3b'),
    (0x104768a0, 0x104769d0, 76, '9f832acd2ea8f28e'), (0x104769d0, 0x10476ebc, 315, 'd2171274e59aa500'),
    (0x10476ec0, 0x10476f3c, 31, 'b256114e03909a5f'), (0x10476f40, 0x104770bc, 95, '187b8400ee18d8e9'),
    (0x104770c0, 0x1047726c, 106, '35632a134b67ad79'), (0x10477270, 0x10477484, 133, '9ea3e30b8e3fe8dd'),
    (0x10477490, 0x104777b8, 202, '69da00f94a7b0919'), (0x104777c0, 0x10477870, 44, '48cf3b6a794ed805'),
    (0x10477870, 0x10477b34, 177, 'a63f122e112e1c1f'), (0x10477b40, 0x10477cc4, 97, 'a3653680c024501b'),
    (0x10477cd0, 0x10477d54, 33, 'a82b1d5eb6c167a1'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# Calls from outside the unit, per entry: 723 sites, every one a `bl` of an entry.
INCOMING = {0x10462cd0: 44, 0x104635b0: 73, 0x10463990: 4, 0x10463a90: 104, 0x10463be0: 1, 0x10463dc0: 1,
            0x10464010: 11, 0x10464160: 12, 0x104646d0: 68, 0x10464880: 6, 0x104648c0: 13, 0x10464900: 1,
            0x10464d00: 53, 0x10469710: 22, 0x104698c0: 62, 0x10469bb0: 10, 0x1046f1f0: 1, 0x10476780: 76,
            0x104768a0: 6, 0x104769d0: 54, 0x10476ec0: 59, 0x10476f40: 23, 0x104770c0: 1, 0x10477490: 2,
            0x10477b40: 16}
# Entries another routine of the unit calls (0x10463000 also calls itself).
CROSS = {0x10463000, 0x104646d0, 0x10469bb0, 0x1046f1f0, 0x104769d0, 0x10476ec0, 0x10477870}
# The unit's descriptors, 0x108b9a04..0x108b9a44 (8 of them), and the code each names, in order.
DESCRIPTORS = (0x108b9a04, 0x108b9a44)
DESCRIBED = (0x10477cd0, 0x104777c0, 0x10477270, 0x10464830, 0x104647c0, 0x104647a0, 0x104647e0, 0x10463580)
# The descriptors' TOC slots with (loads, first load): the code that takes the address of an entry.
DESCRIPTOR_SLOTS = {'-0x5cfc': (1, 0x106d1f48, 0x10477cd0), '-0x6190': (1, 0x10477750, 0x104777c0),
                    '-0x4980': (1, 0x1047723c, 0x10477270), '-0x6134': (2, 0x104c0f08, 0x10464830),
                    '-0x60c4': (2, 0x104de274, 0x104647c0), '-0x6074': (2, 0x104f96c8, 0x104647a0),
                    '-0x6188': (56, 0x10463660, 0x104647e0), '-0x7464': (1, 0x10131798, 0x10463580)}
# The unit's TOC slot block -0x4980..-0x47e0: 105 slots, every load inside the unit. Of them 101 are
# jump-table slots (one per `bctr`); -0x47e8 is a global, -0x47e4 the string pool, -0x47e0 the constants.
SLOT_BLOCK = (-0x4980, -0x47e0)
POOL = 0x1083109c
JUMP_TABLES = (0x108cfa28, 0x108d7724)
JUMP_WORDS = {0x10469bb0: 1551, 0x1046f1f0: 6448}
BCTR = {0x10469bb0: 27, 0x1046f1f0: 74}
# Two words of an array of arbitrary constants that happen to fall inside the span.
CONSTANT_WORDS = (0x108e5918, 0x108e63d4)
# The neighbouring units' slots: (loads, first, last), all outside the unit.
NEIGHBOUR_SLOTS = {'-0x4984': (33, 0x104533b0, 0x1046287c),   # COMP_STATS.CPP's pool
                   '-0x498c': (54, 0x10456298, 0x10462aa4),   # its constants; the last load is 0x10462a40's
                   '-0x4990': (2, 0x104571c8, 0x10457220),    # its comparator 0x10462a30's descriptor
                   '-0x47dc': (2, 0x104782e8, 0x1047868c),    # CONMEBOL_SEEDING.CPP's descriptor run
                   '-0x47d8': (6, 0x10477d84, 0x10478c28)}    # its pool
# The entries the 2026-11-18 inventory did not attribute to the unit.
ADDED = {0x10463580, 0x10477cd0}
PREVIOUS_UNIT_COMPARATOR = (0x10462a30, 0x10462a40)   # COMP_STATS.CPP's, the previous descriptor
PREVIOUS_UNIT_LAST = (0x10462a40, 0x10462b78)         # COMP_STATS.CPP's last proven routine
NEXT_UNIT_FIRST = 0x10477d60                          # CONMEBOL_SEEDING.CPP
# Nothing is routed: the inventory step (audit.ZERO_ROUTED).
ROUTED = frozenset()
AMBIGUOUS_CALLERS = (0x1002b6d0, 0x10042704, 0x10043190, 0x1005fd2c, 0x102fe7e4, 0x1030b6e0, 0x1030d630)


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private COMP_UTIL.CPP export unavailable; inventory cannot be certified')


class CompUtilAuditTests(unittest.TestCase):
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
        if CompUtilAuditTests.slot_loads is None:
            found = {}
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                for address, body in db.execute("SELECT address, body FROM instructions WHERE body LIKE '%(r2)%' "
                                                'ORDER BY address'):
                    match = SLOT.search(json.loads(body)['assembly'])
                    if match:
                        found.setdefault(match.group(1), []).append(address)
            CompUtilAuditTests.slot_loads = found
        return CompUtilAuditTests.slot_loads.get(slot, [])

    def entry_of(self, address):
        return next((s for s, e in audit.ROUTINES if s <= address < e), None)

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.MODULE, (0x10462cd0, 0x10477d54))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.COMP_UTIL_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 35)
        self.assertEqual(len(self.records), 21480)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_comp_util_index(db, complete=False)

    def test_nothing_is_routed_yet_and_the_hook_checks_fail_when_they_should(self):
        # The inventory step: zero entries routed, stated rather than skipped.
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS, generate_page
        self.assertTrue(audit.ZERO_ROUTED)
        self.assertFalse(audit.COMPLETE)
        self.assertEqual({entry for entry, _, _ in audit.rewritten(drafts=True)}, ROUTED)
        self.assertEqual({entry for entry, _, _ in audit.rewritten()}, ROUTED)
        self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at in ENTRIES], [])
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_comp_util_index(db)
            with self.assertRaisesRegex(ValueError, 'requires all 35'):
                audit.require_comp_util_index(db, complete=True)
        # No page emits a hook, and the hook checker rejects such a page.
        entry = 0x10464160
        part = [r for r in self.records if r['address'] >> 12 == entry >> 12]
        page = generate_page(entry >> 12, part, set(self.by_address), userops=self.program['userops'])
        self.assertNotIn('try_modern_comp_util', page)
        self.assertNotIn('comp_util_adapter', page)
        with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
            audit.require_comp_util_hooks(entry >> 12, part, page)
        hooked = page.replace(f's.step(0x{entry:x});', audit.comp_util_hook(entry) + f'\ns.step(0x{entry:x});', 1)
        with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
            audit.require_comp_util_hooks(entry >> 12, part, hooked)   # the other entries of the page still lack it

    def test_unit_bounds_are_its_pool_descriptors_slots_and_neighbours(self):
        pool = self.word(TOC - 0x47e4)
        self.assertEqual(pool, POOL)
        self.assertEqual(self.original_bytes(pool, 14), b'COMP_UTIL.CPP\0')
        self.assertEqual(self.original_bytes(pool + 0xe, 13), b'v%s %s.%s %d\0')
        self.assertIn(b'are through to the', self.original_bytes(pool, 0x400))
        constants = self.word(TOC - 0x47e0)
        self.assertEqual(constants, 0x10839d00)
        self.assertEqual(self.original_bytes(constants, 16), bytes.fromhex('40efffe0000000004330000080000000'))
        # The pool's fourteen users all lie inside the span, first and last as in the docstring.
        users = self.loads('-0x47e4')
        self.assertEqual((len(users), users[0], users[-1]), (14, 0x104635c0, 0x10477b54))
        self.assertEqual(len({self.entry_of(a) for a in users}), 14)
        # The slot block: 105 slots, every load inside the unit, each slot loaded.
        slots = range(SLOT_BLOCK[0], SLOT_BLOCK[1] + 4, 4)
        self.assertEqual(len(slots), 105)
        for slot in slots:
            found = self.loads(f'-0x{-slot:x}')
            self.assertTrue(found, hex(slot))
            self.assertTrue(all(audit.MODULE[0] <= a < audit.MODULE[1] for a in found), hex(slot))
        # The neighbours' slots are outside it.
        for slot, expected in NEIGHBOUR_SLOTS.items():
            found = self.loads(slot)
            self.assertEqual((len(found), found[0], found[-1]), expected, slot)
            self.assertFalse(any(audit.MODULE[0] <= a < audit.MODULE[1] for a in found), slot)
        self.assertLess(PREVIOUS_UNIT_LAST[1], audit.MODULE[0])
        self.assertTrue(PREVIOUS_UNIT_LAST[0] <= 0x10462aa4 < PREVIOUS_UNIT_LAST[1])   # the last -0x498c load
        # The descriptors: one contiguous run of (code, TOC) pairs, between COMP_STATS.CPP's and
        # CONMEBOL_SEEDING.CPP's.
        start, end = DESCRIPTORS
        named = tuple(self.word(a) for a in range(start, end, 8))
        self.assertEqual(named, DESCRIBED)
        self.assertEqual([self.word(a + 4) for a in range(start, end, 8)], [TOC] * len(named))
        self.assertTrue(all(code in ENTRIES for code in named))
        self.assertEqual(self.word(start - 8), PREVIOUS_UNIT_COMPARATOR[0])   # COMP_STATS.CPP's comparator
        self.assertEqual(self.word(TOC - 0x4990), start - 8)
        self.assertEqual(self.word(end), 0x10478ea0)                           # CONMEBOL_SEEDING.CPP's comparator
        self.assertEqual(self.word(TOC - 0x47dc), end)
        for slot, (count, first, code) in DESCRIPTOR_SLOTS.items():
            found = self.loads(slot)
            self.assertEqual((len(found), found[0]), (count, first), slot)
            self.assertEqual(self.word(self.word(TOC + int(slot, 16))), code, slot)
        # The previous unit's comparator is a routine of its own, before its last one.
        first = self.by_address.get(PREVIOUS_UNIT_COMPARATOR[0])
        self.assertIsNone(first)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            previous = routine_records(db, *PREVIOUS_UNIT_COMPARATOR)
            self.assertEqual([r['assembly'] for r in previous][-1], 'blr')
            next_unit = routine_records(db, NEXT_UNIT_FIRST, NEXT_UNIT_FIRST + 8)
        self.assertEqual(next_unit[0]['assembly'], 'mfspr r0,LR')
        self.assertEqual(self.by_address[0x10462cd0]['assembly'], 'mfspr r0,LR')
        self.assertEqual(audit.ROUTINES[-1][1], 0x10477d54)
        # CONMEBOL_SEEDING.CPP's first pool load is in its first routine.
        self.assertTrue(NEXT_UNIT_FIRST <= NEIGHBOUR_SLOTS['-0x47d8'][1] < NEXT_UNIT_FIRST + 0x40)

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
        self.assertEqual(sum(incoming.values()), 723)
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
        # Every `bctr` has a slot of its own in the block, naming its table.
        tables = [self.word(TOC + slot) for slot in range(-0x497c, -0x47e8, 4)
                  if self.word(TOC + slot) and JUMP_TABLES[0] <= self.word(TOC + slot) < JUMP_TABLES[1]]
        self.assertEqual(len(tables), 101)
        self.assertEqual(sum(BCTR.values()), 101)

    def test_every_entry_is_called_by_a_descriptor_or_another_entry(self):
        named = set(DESCRIBED)
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
        start, end = DESCRIPTORS
        words = {}
        for base, contents in self.segments:
            for offset in range(0, len(contents) - 3, 4):
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if audit.MODULE[0] <= target < audit.MODULE[1] and base + offset >= code_end:
                    address = base + offset
                    if start <= address < end and (address - start) % 8 == 0:
                        continue
                    if JUMP_TABLES[0] <= address < JUMP_TABLES[1]:
                        # Jump tables: the word names the inside of one of the two big routines.
                        entry = self.entry_of(target)
                        self.assertIn(entry, JUMP_WORDS, hex(address))
                        words[entry] = words.get(entry, 0) + 1
                        self.assertNotIn(target, ENTRIES)
                        continue
                    self.assertIn(address, CONSTANT_WORDS, hex(address))
        self.assertEqual(words, JUMP_WORDS)
        self.assertEqual(sum(words.values()), 7999)
        # The two stray words sit among arbitrary constants: no neighbour points into code.
        for address in CONSTANT_WORDS:
            for near in (-8, -4, 4, 8):
                self.assertFalse(0x10000000 <= self.word(address + near) < 0x10a00000, hex(address + near))

    def test_inventory_gap_is_the_added_entries(self):
        inventory = ROOT / '.local/inventory/2026-11-18/inventory.json'
        if not inventory.exists():
            self.skipTest('the 2026-11-18 inventory is not kept')
        attributed = {int(r['entry'], 16) for r in json.loads(inventory.read_text())['routines']
                      if r['module'] == 'COMP_UTIL.CPP'}
        self.assertEqual(len(attributed), 33)
        self.assertEqual(set(ENTRIES) - attributed, ADDED)
        self.assertEqual(attributed - set(ENTRIES), set())

    def test_the_routine_between_the_neighbours_is_pinned_but_not_an_entry(self):
        entry, end, count, expected = audit.AMBIGUOUS_NEIGHBOUR
        self.assertEqual((entry, end), (0x10462b80, audit.MODULE[0]))
        self.assertNotIn(entry, ENTRIES)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            body = routine_records(db, entry, end)
            callers = [a for a, in db.execute(
                "SELECT i.address FROM instructions i,json_each(i.body,'$.flows') f WHERE f.value=? ORDER BY i.address",
                (entry,))]
            previous = routine_records(db, *PREVIOUS_UNIT_LAST)
        self.assertEqual((len(body), digest(body)), (count, expected))
        self.assertEqual(body[-1]['assembly'], 'blr')
        # No TOC load, no assertion; its callers are screens, none in COMP_STATS.CPP or this unit.
        self.assertFalse([r for r in body if '(r2)' in r['assembly']])
        self.assertEqual(tuple(callers), AMBIGUOUS_CALLERS)
        self.assertIn('bl 0x100c7d00', [r['assembly'] for r in body])
        self.assertEqual(previous[-1]['assembly'], 'blr')
        self.assertEqual(PREVIOUS_UNIT_LAST[1] + 8, entry)

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[10]
            line = f'COMP_UTIL_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_comp_util_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_comp_util_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing COMP_UTIL.CPP instructions'):
                    audit.require_comp_util_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)
            # A block's own .inc, included from the main list, is read too.
            block = Path(directory) / 'comp_util_routines_block.inc'
            block.write_text(line)
            candidate.write_text('#include "comp_util_routines_block.inc"\n')
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])


if __name__ == '__main__':
    unittest.main()
