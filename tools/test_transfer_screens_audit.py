"""Original bounds/callbacks and incremental TRANSFER_SCREENS ownership checks."""
from contextlib import closing
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import transfer_screens_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/current/export/instructions.sqlite'


class TransferScreensAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not INPUT.is_file():
            raise RuntimeError('Private TRANSFER_SCREENS export unavailable; inventory cannot be certified')
        cls.program = json.loads((INPUT.parent / 'program.json').read_text())
        cls.segments = [(s['address'], (INPUT.parent / s['file']).read_bytes())
                        for s in cls.program['segments'] if 'file' in s]

    def original_bytes(self, address, size):
        for start, contents in self.segments:
            if start <= address and address + size <= start + len(contents):
                return contents[address - start:address - start + size]
        self.fail(f'Original bytes unavailable at {address:x}')

    def word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), 'big')

    def test_complete_original_inventory_and_reference_fingerprints(self):
        self.assertEqual(len(audit.ROUTINES), 75)
        self.assertEqual(sum(count for _, _, count, _ in audit.ORIGINAL), 25003)
        self.assertEqual((audit.ROUTINES[0][0], audit.ROUTINES[-1][1]), audit.MODULE)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(audit.ROUTINES, audit.ROUTINES[1:])))
        with closing(sqlite3.connect(INPUT)) as db:
            audit.require_transfer_screens_index(db)

    def test_source_pool_and_adjacent_module_boundaries(self):
        toc = 0x108bba90
        for slot, address, source in ((-0x4b88, 0x10827500, b'TRANSFER_OFFER.CPP\0'),
                                      (-0x4ad0, 0x108276c0, b'TRANSFER_SCREENS.CPP\0'),
                                      (-0x4ab0, 0x1082ac98, b'Insert CD\0')):
            self.assertEqual(self.word(toc + slot), address)
            self.assertEqual(self.original_bytes(address, len(source)), source)
        self.assertEqual(self.original_bytes(0x1082accf, 18), b'VIRTUAL_STAFF.CPP\0')
        with closing(sqlite3.connect(INPUT)) as db:
            loads = [json.loads(body) for body, in db.execute(
                'SELECT body FROM instructions WHERE body LIKE ? ORDER BY address', ('%-0x4ad0(r2)%',))]
            self.assertEqual(len(loads), 28)
            self.assertEqual((loads[0]['address'], loads[-1]['address']), (0x10417b98, 0x1042fb44))
            self.assertTrue(all(audit.MODULE[0] <= r['address'] < audit.MODULE[1] for r in loads))
            at = lambda a: json.loads(db.execute('SELECT body FROM instructions WHERE address=?', (a,)).fetchone()[0])['assembly']
            self.assertEqual(at(0x1041758c), 'blr')
            self.assertEqual(at(audit.MODULE[0]), 'mfspr r0,LR')
            self.assertEqual(at(audit.MODULE[1]), 'mfspr r0,LR')
            self.assertEqual(at(audit.MODULE[1] + 8), 'lwz r29,-0x4ab0(r2)')
            preceding = [json.loads(body)['address'] for body, in db.execute(
                'SELECT body FROM instructions WHERE body LIKE ? ORDER BY address', ('%-0x4b88(r2)%',))]
            self.assertEqual(preceding[-1], 0x1041710c)

    def test_each_body_and_every_alignment_word_are_accounted_for(self):
        with closing(sqlite3.connect(INPUT)) as db:
            for entry, end in audit.ROUTINES:
                with self.subTest(entry=hex(entry)):
                    records = routine_records(db, entry, end)
                    self.assertEqual(records[0]['assembly'], 'blr' if entry == 0x1041e760 else 'mfspr r0,LR')
                    self.assertEqual(records[-1]['assembly'], 'blr')
                    self.assertEqual(sum(r['assembly'] == 'mfspr r0,LR' for r in records), int(entry != 0x1041e760))
                    self.assertEqual({r['address'] for r in records}, set(range(entry, records[-1]['address'] + 4, 4)))
                    last = records[-1]['address'] + 4
                    self.assertLess(end - last, 16)
                    self.assertEqual(self.original_bytes(last, end - last), bytes(end - last))

    def test_whole_export_incoming_targets_and_internal_flows(self):
        internal, external, targets = [], [], set()
        with closing(sqlite3.connect(INPUT)) as db:
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    'WHERE f.value>=? AND f.value<?', audit.MODULE):
                record = json.loads(body)
                for target in record['flows']:
                    if not audit.MODULE[0] <= target < audit.MODULE[1]:
                        continue
                    if record['flow'] == 'UNCONDITIONAL_CALL':
                        self.assertIn(target, dict(audit.ROUTINES))
                        targets.add(target)
                        (internal if audit.MODULE[0] <= address < audit.MODULE[1] else external).append((address, target))
                    else:
                        self.assertTrue(audit.MODULE[0] <= address < audit.MODULE[1], 'external interior resume')
                        entry, end = next((a, b) for a, b in audit.ROUTINES if a <= address < b)
                        self.assertTrue(entry <= target < end, 'cross-entry tail branch')
            for entry, end in audit.ROUTINES:
                for record in routine_records(db, entry, end):
                    self.assertNotIn('COMPUTED', record['flow'])
                    self.assertNotIn(record['assembly'], ('bctr', 'bctrl'))
        self.assertEqual(len(internal), 49)
        self.assertEqual(len(external), 55)
        self.assertEqual(hashlib.sha256(json.dumps(sorted(internal)).encode()).hexdigest(),
                         'd2fe711b4647e0b09ddbf4d76ce6052e6d010cb879658070fa61ec8c5aab6597')
        self.assertEqual(hashlib.sha256(json.dumps(sorted(external)).encode()).hexdigest(),
                         'ef558357fda2e73cae6ea25999a5a49cf9f79ca97d3f859dca7f3cba83caf3da')
        self.assertEqual(targets | {entry for _, entry in audit.CALLBACKS}, set(dict(audit.ROUTINES)))

    def test_transition_vectors_and_all_byte_pointer_false_positives(self):
        self.assertEqual(len(audit.CALLBACKS), 51)
        self.assertEqual([a for a, _ in audit.CALLBACKS], list(range(0x108b942c, 0x108b95c4, 8)))
        callbacks = dict(audit.CALLBACKS)
        for descriptor, entry in audit.CALLBACKS:
            self.assertEqual(self.word(descriptor), entry)
            self.assertEqual(self.word(descriptor + 4), 0x108bba90)
        found_callbacks = {}
        rng_patterns = {}
        instruction_patterns = []
        for start, contents in self.segments:
            for match in re.finditer(b'\x10[\x41\x42]', contents):
                offset = match.start()
                if offset + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if not audit.MODULE[0] <= target < audit.MODULE[1]:
                    continue
                address = start + offset
                if address in callbacks:
                    found_callbacks[address] = target
                elif start == 0x10000000:
                    self.assertNotEqual(address % 4, 0)
                    instruction_patterns.append(address)
                else:
                    rng_patterns[address] = target
        self.assertEqual(found_callbacks, callbacks)
        self.assertEqual(rng_patterns, {0x108e54d2: 0x1042f9be, 0x108ed54f: 0x10424ed2, 0x108f4058: 0x1042ac2c})
        pool = self.word(0x108bba90 - 0x7b8c)
        self.assertEqual(pool, 0x108e3140)
        self.assertTrue(all(pool <= address and address + 4 <= pool + 51000 * 2 for address in rng_patterns))
        with closing(sqlite3.connect(INPUT)) as db:
            for address in instruction_patterns:
                for word in (address & ~3, (address + 3) & ~3):
                    self.assertIsNotNone(db.execute('SELECT 1 FROM instructions WHERE address=?', (word,)).fetchone())

    def test_incremental_production_owners_are_exact(self):
        from compile_recomp import OWNER_REPLACEMENTS
        actual = [(entry, owner) for entry, owner in OWNER_REPLACEMENTS if audit.MODULE[0] <= entry < audit.MODULE[1]]
        expected = [(entry, 'transfer_screens_adapter') for entry, _, _ in audit.rewritten()]
        self.assertEqual(sorted(actual), expected)

    def test_production_completeness_gate_rejects_missing_disabled_and_conflicting(self):
        full = [(entry, end, fingerprint) for entry, end, _, fingerprint in audit.ORIGINAL]
        owners = [(entry, 'transfer_screens_adapter') for entry, _ in audit.ROUTINES]
        audit.require_transfer_screens_production(owners, full)
        for incomplete in (full[:-1], []):
            with self.assertRaisesRegex(ValueError, 'incomplete'):
                audit.require_transfer_screens_production(owners, incomplete)
        for bad_owners in (owners[:-1], owners + [owners[0]], [(audit.ROUTINES[0][0], 'legacy_builder')] + owners[1:]):
            with self.assertRaisesRegex(ValueError, 'owner'):
                audit.require_transfer_screens_production(bad_owners, full)

    def test_registration_rejects_duplicates_bounds_and_stale_drafts(self):
        entry, end, _, fingerprint = audit.ORIGINAL[0]
        line = f'TRANSFER_SCREENS_DRAFT(0x{entry:x}, 0x{end:x}, R3, "{fingerprint}", hook)\n'
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'routines.inc'
            path.write_text(line)
            self.assertEqual(audit.rewritten(path), [])
            self.assertEqual(audit.rewritten(path, drafts=True), [(entry, end, fingerprint)])
            path.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(path)
            path.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'not a routine'):
                audit.rewritten(path)
        stale = [(entry, end, '0' * 16)]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(audit, 'rewritten', return_value=stale):
            with self.assertRaisesRegex(ValueError, 'stale digest'):
                audit.require_transfer_screens_index(db)

    def test_whole_inventory_fingerprint_rejects_missing_or_corrupted_original(self):
        entry, end, count, fingerprint = audit.ORIGINAL[0]
        with closing(sqlite3.connect(INPUT)) as db:
            for bad in ((entry, end, count + 1, fingerprint), (entry, end, count, '0' * 16)):
                with patch.object(audit, 'ORIGINAL', (bad,)), self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_transfer_screens_index(db)
        with sqlite3.connect(':memory:') as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'changed in the export'):
                audit.require_transfer_screens_index(db)

    def test_explicit_missing_include_is_not_silently_skipped(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'routines.inc'
            path.write_text('#include "transfer_screens_routines_missing.inc"\n')
            with self.assertRaises(FileNotFoundError):
                audit.rewritten(path)


if __name__ == '__main__':
    unittest.main()
