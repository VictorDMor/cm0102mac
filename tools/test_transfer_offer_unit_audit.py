"""Certify TRANSFER_OFFER.CPP's 17 entries, the unit's bounds and its production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import transfer_offer_unit_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x10411e60, 0x10411ea0, 16, 'aba76d3d343f258f'),
    (0x10411ea0, 0x10411f08, 26, '67e9efd47574b0d5'),
    (0x10411f10, 0x10411f9c, 35, '27405f9734d173e5'),
    (0x10411fa0, 0x104123d4, 266, 'b40452e04363c1c9'),
    (0x104123e0, 0x104126cc, 185, 'd7fbc08702f7ff8f'),
    (0x104126d0, 0x10415e4c, 3551, 'f5d511c88a4a2a50'),
    (0x10415e50, 0x104163c8, 350, 'df703091d299e91b'),
    (0x104163d0, 0x104164e4, 69, '3ebcde79ebeedd60'),
    (0x104164f0, 0x10416710, 136, 'da7c9c29623e8506'),
    (0x10416710, 0x10416834, 73, 'd0877fd13f092a69'),
    (0x10416840, 0x10416d24, 312, '088a1a2f0e3f7ada'),
    (0x10416d30, 0x10416d5c, 11, '909734d1e65ccbf4'),
    (0x10416d60, 0x10416e04, 40, 'bda13f6bc2e119ce'),
    (0x10416e10, 0x104170f0, 183, '54cb712004280063'),
    (0x104170f0, 0x10417260, 92, 'ae3843d68f8e9bb8'),
    (0x10417260, 0x104172e4, 32, 'd39c1f4c57149130'),
    (0x104172f0, 0x10417590, 168, 'dccf0dc1d3e9728c'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# Calls from outside the unit, per entry: 118 sites, every one a `bl` of an entry.
INCOMING = {0x10411e60: 16, 0x10411ea0: 3, 0x10411f10: 46, 0x10411fa0: 2, 0x104123e0: 2, 0x104126d0: 2,
            0x104163d0: 2, 0x104164f0: 3, 0x10416710: 19, 0x10416840: 14, 0x10416d30: 1, 0x10416d60: 1,
            0x10416e10: 5, 0x104170f0: 1, 0x10417260: 1}
# The loads of the unit's three pools: strings (TOC -0x4b88), doubles (-0x4b8c) and floats (-0x4b90).
POOL_LOADS = {
    '-0x4b88': [0x10411fc0, 0x104123f8, 0x10412720, 0x104163dc, 0x10416520, 0x1041710c],
    '-0x4b8c': [0x104126e0, 0x10415e5c, 0x1041685c, 0x104172f8],
    '-0x4b90': [0x104157f0, 0x10415804, 0x10415858, 0x10415970, 0x10416854],
}
# The one pointer to the unit: a transition vector of the destructor (its code address, then the TOC),
# so the destructor is also called through a pointer. The hook is at its entry either way.
DESTRUCTOR_VECTOR = 0x108b9424
# Byte patterns elsewhere that read as an address in the unit; none is an entry or a vector.
POINTER_LOOKALIKES = {0x101b9e9a: 0x10414182, 0x10501262: 0x10413800, 0x10578b22: 0x10413800,
                      0x105d8852: 0x10414818, 0x108ec3ac: 0x104156f0}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private TRANSFER_OFFER export unavailable; inventory cannot be certified')


class TransferOfferUnitAuditTests(unittest.TestCase):
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

    def assert_hook(self, code, entry):
        hook = audit.transfer_offer_unit_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_exact_entries_original_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x10411e60, 0x10417590))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.TRANSFER_OFFER_UNIT_ENTRIES, ENTRIES)
        # Every entry is routed (no draft left).
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(self.records), 5545)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ['transfer_offer_unit_adapter'])
        # The register-level owners of the earlier process no longer route any of them.
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_transfer_offer_unit_index(db, complete=True)

    def test_unit_bounds_are_its_pools_and_its_neighbours(self):
        pool = self.word(TOC - 0x4b88)
        self.assertEqual(self.original_bytes(pool, 0x26).split(b'\0')[:3],
                         [b'TRANSFER_OFFER.CPP', b'v%s %s.%s %d', b'Error'])
        # The float and double pools follow the strings in the same data.
        self.assertEqual((self.word(TOC - 0x4b90), self.word(TOC - 0x4b8c)), (pool + 0x28, pool + 0x50))
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for slot, loads in POOL_LOADS.items():
                self.assertEqual([a for a, in db.execute(
                    'SELECT address FROM instructions WHERE body LIKE ? ORDER BY address', (f'%{slot}(r2)%',))], loads)
        for loads in POOL_LOADS.values():
            for load in loads:
                self.assertTrue(audit.MODULE[0] <= load < audit.MODULE[1])
        # Before: TRANSFER_MANAGER.CPP's last audited routine ends where this unit starts. After:
        # TRANSFER_SCREENS.CPP's first.
        import transfer_manager_audit
        import transfer_screens_audit
        self.assertEqual(transfer_manager_audit.MODULE[1], audit.MODULE[0])
        self.assertEqual(transfer_screens_audit.MODULE[0], audit.MODULE[1])
        # The three routines before the first pool load work on the record the others do: the two
        # constructors build its date (+0x14), one of them calls the builder 0x104126d0, and the
        # destructor releases its two lists.
        self.assertEqual(self.by_address[0x10411e70]['assembly'], 'addi r3,r31,0x14')
        self.assertEqual(self.by_address[0x10411eec]['assembly'], 'bl 0x104126d0')
        self.assertEqual(self.by_address[0x10411f5c]['assembly'], 'lwz r3,0x4(r30)')
        # 0x10417260, between two pool users, walks the same exchange list (+4).
        self.assertEqual(self.by_address[0x1041727c]['assembly'], 'lwz r3,0x4(r3)')

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
        self.assertEqual(fallthroughs, [])
        # 0x10415e50 and 0x104172f0 are called from inside the unit only.
        self.assertEqual(set(ENTRIES) - set(INCOMING), {0x10415e50, 0x104172f0})

    def test_cross_entry_edges_are_calls_of_entries_and_no_computed_flow(self):
        cross = set()
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                self.assertNotIn('COMPUTED', record['flow'])
                targets = record['flows'] + ([record['fallthrough']] if record.get('fallthrough') is not None else [])
                for target in targets:
                    if audit.MODULE[0] <= target < audit.MODULE[1] and not start <= target < end:
                        self.assertIn(target, ENTRIES)
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        cross.add(target)
        self.assertEqual(cross, {0x104126d0, 0x10415e50, 0x10416710, 0x104172f0})
        self.assertEqual([s for s in self.program['symbols']
                          if audit.MODULE[0] <= s['address'] < audit.MODULE[1]], [])

    def test_every_instruction_and_alignment_word_accounted(self):
        addresses = set()
        for start, end in audit.ROUTINES:
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual([r['address'] for r in records],
                             [a for a in range(start, end, 4) if a not in audit.UNREACHABLE_NOPS])
            self.assertEqual(records[-1]['assembly'], 'blr')
            addresses.update(range(start, end, 4))
        # The nine words the export leaves out inside routines are the compiler's alignment no-ops.
        for address in audit.UNREACHABLE_NOPS:
            self.assertEqual(self.word(address), 0x60000000)
            self.assertNotIn(address, self.by_address)
        for address in set(range(*audit.MODULE, 4)) - addresses:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)

    def test_the_only_pointer_to_the_unit_is_the_destructor_vector(self):
        pointers = {}
        for start, contents in self.segments:
            for match in re.finditer(b'\x10\x41', contents):
                offset = match.start()
                if offset + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if audit.MODULE[0] <= target < audit.MODULE[1]:
                    pointers[start + offset] = target
        self.assertEqual(pointers.pop(DESTRUCTOR_VECTOR), 0x10411f10)
        self.assertEqual(self.word(DESTRUCTOR_VECTOR + 4), TOC)
        self.assertEqual(pointers, POINTER_LOOKALIKES)
        for at, target in pointers.items():
            self.assertNotIn(target, ENTRIES)
            self.assertNotEqual(self.word(at + 4), TOC)

    def test_missing_export_and_stale_digest_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing TRANSFER_OFFER.CPP instructions'):
                audit.require_transfer_offer_unit_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten(drafts=True))]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_transfer_offer_unit_index(db)

    def test_duplicate_bad_boundary_and_draft_are_rejected(self):
        contents = audit.routine_lines()
        first = next(line for line in contents.splitlines() if 'TRANSFER_OFFER_UNIT_ROUTINE(' in line)
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('0x10411ea0, R3', '0x10411ea4, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace(first, first.replace('TRANSFER_OFFER_UNIT_ROUTINE(',
                                                                       'TRANSFER_OFFER_UNIT_DRAFT(')))
            self.assertEqual(len(audit.rewritten(candidate, drafts=True)), 17)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all 17 complete'):
                    audit.require_transfer_offer_unit_index(db, candidate, complete=True)

    def test_normal_hooks_precede_original_steps_and_detect_missing_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_transfer_offer_unit_hooks(page, part, normal)
            self.assertIn('#include "transfer_offer_unit_adapter.hpp"', normal)
            for old in ('try_modern_transfer_record(', 'try_modern_transfer_valuation(', 'try_modern_transfer_synthesis('):
                self.assertNotIn(old, normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.transfer_offer_unit_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_transfer_offer_unit_hooks(page, part, broken)


if __name__ == '__main__':
    unittest.main()
