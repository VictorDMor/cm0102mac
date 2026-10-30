"""Certify DB_FILES.CPP's entries, original boundary and production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import db_files_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
CONTRACTS = (
    (0x105d71b0, 0x105d7200, 19, 'eb8d7d4fedede3b7'),
    (0x105d7200, 0x105d7250, 20, '1b29854d6049a64c'),
    (0x105d7250, 0x105d7530, 181, '09e15ca852dc0e42'),
    (0x105d7530, 0x105d76e0, 107, 'e52d79d1e4ce2f98'),
    (0x105d76e0, 0x105d7750, 28, 'dd679fe1386f27c7'),
    (0x105d7750, 0x105d7a30, 181, '0000bc666e17568f'),
    (0x105d7a30, 0x105d8560, 716, '488f8b2a517be96d'),
    (0x105d8560, 0x105d85d0, 28, 'e63cfd13f78a4ec5'),
    (0x105d85d0, 0x105d88b0, 183, 'd92e1ea85c00ea38'),
    (0x105d88b0, 0x105d8ad0, 136, 'b33b06d41c6c37fb'),
    (0x105d8ad0, 0x105d8b40, 28, 'df1f47ea9c7b3b42'),
    (0x105d8b40, 0x105d8e20, 181, '5646d9f853631111'),
    (0x105d8e20, 0x105d8ef0, 50, '10b3413f336b686a'),
    (0x105d8ef0, 0x105d8f60, 28, 'ae071a9859837a94'),
    (0x105d8f60, 0x105d9290, 202, '14f2bb84a65bb2c1'),
    (0x105d9290, 0x105d9710, 285, '1cceb5552c5bce56'),
    (0x105d9710, 0x105da670, 974, '1165fcb4d821a845'),
    (0x105da670, 0x105da6e0, 28, '293abe8473867e2d'),
    (0x105da6e0, 0x105da9c0, 181, 'cbfbc0fc4c18649f'),
    (0x105da9c0, 0x105dac10, 148, '5a6f83e8f6776a7a'),
    (0x105dac10, 0x105dac80, 28, '081353010d3ce5de'),
    (0x105dac80, 0x105daf60, 181, '740b224c3e05051e'),
    (0x105daf60, 0x105db230, 180, '068cd1556cce4ab4'),
    (0x105db230, 0x105db530, 190, '7c39661897d00a8b'),
    (0x105db530, 0x105db6c0, 97, '7ee146d4f8544ef1'),
    (0x105db6c0, 0x105dc160, 644, 'da19f51139e3e1c7'),
    (0x105dc160, 0x105dc1d0, 28, '620486951db4da7f'),
    (0x105dc1d0, 0x105dc4d0, 190, '2e8528692bd90e48'),
    (0x105dc4d0, 0x105dc710, 141, '62e607daaa0423e1'),
    (0x105dc710, 0x105dd910, 1088, 'b0a21229f12639ed'),
    (0x105dd910, 0x105dd980, 28, '426fc83ef720db59'),
    (0x105dd980, 0x105ddc60, 181, '3514f1398c7d20fa'),
    (0x105ddc60, 0x105ddd20, 45, '6bf117aae71a3581'),
    (0x105ddd20, 0x105ddd90, 28, '78840cee069d8c65'),
    (0x105ddd90, 0x105de070, 181, '2d15800f4133ceec'),
    (0x105de070, 0x105de0e0, 25, 'e86f26e6a0521bfb'),
    (0x105de0e0, 0x105de150, 28, '764992d94f1f5089'),
    (0x105de150, 0x105de450, 190, 'bb1e3d3a1f815486'),
    (0x105de450, 0x105de550, 63, 'a5bfff227007c43a'),
    (0x105de550, 0x105de9b0, 265, 'bcba966507e2f320'),
    (0x105de9b0, 0x105dea20, 28, '439ddc2b1fb80e76'),
    (0x105dea20, 0x105ded00, 181, '13fc2fcfb7b0bfa0'),
    (0x105ded00, 0x105ded70, 28, '9891d2839d091d1f'),
    (0x105ded70, 0x105dede0, 28, 'e81a52f697feda37'),
    (0x105dede0, 0x105df0c0, 181, '85a65d02a2dd3a8a'),
    (0x105df0c0, 0x105df1a0, 54, '3dcda24c0ffad53b'),
    (0x105df1a0, 0x105df210, 28, '0c5ab42ded5c4fa7'),
    (0x105df210, 0x105df4f0, 181, '8bc7519b31c9f82f'),
    (0x105df4f0, 0x105df5c0, 52, '6c1aaf255b84f602'),
    (0x105df5c0, 0x105df630, 28, 'b85043133ce209a4'),
    (0x105df630, 0x105df910, 181, 'b141935314514805'),
    (0x105df910, 0x105dfa10, 64, '02bdee193504ffb1'),
    (0x105dfa10, 0x105dfa80, 28, '8791979ac7928d81'),
    (0x105dfa80, 0x105dfd60, 181, 'b005bdbc54cfafed'),
    (0x105dfd60, 0x105dfdf0, 35, 'b44ae80a99c8d370'),
    (0x105dfdf0, 0x105dfe60, 28, 'a4184306af57c393'),
    (0x105dfe60, 0x105e0140, 181, '02e8866aded879a2'),
    (0x105e0140, 0x105e01d0, 35, '672698806c7a34ad'),
    (0x105e01d0, 0x105e0240, 28, '19230630c488700d'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
INCOMING = (
    (0x100a57b0, 0x105d71b0), (0x100a57f4, 0x105d71b0), (0x100a9070, 0x105d71b0), (0x100a90ec, 0x105d7250),
    (0x100a9188, 0x105d71b0), (0x100a9204, 0x105df630), (0x100a92a0, 0x105d71b0), (0x100a931c, 0x105dfe60),
    (0x100a93b8, 0x105d71b0), (0x100a9434, 0x105dfe60), (0x100a94d0, 0x105d71b0), (0x100a954c, 0x105dfe60),
    (0x100a95e8, 0x105d71b0), (0x100a9664, 0x105d85d0), (0x100a9700, 0x105d71b0), (0x100a977c, 0x105d8f60),
    (0x100a9818, 0x105d71b0), (0x100a9894, 0x105d8f60), (0x100a9930, 0x105d71b0), (0x100a99ac, 0x105dede0),
    (0x100a9a48, 0x105d71b0), (0x100a9ac4, 0x105de150), (0x100a9b60, 0x105d71b0), (0x100a9bdc, 0x105de150),
    (0x100a9c68, 0x105de150), (0x100a9d04, 0x105d71b0), (0x100a9dc4, 0x105df210), (0x100a9e60, 0x105d71b0),
    (0x100a9edc, 0x105dea20), (0x100a9f78, 0x105d71b0), (0x100a9ff4, 0x105dea20), (0x100aa090, 0x105d71b0),
    (0x100aa10c, 0x105dfa80), (0x100aa1a8, 0x105d71b0), (0x100aa224, 0x105d7750), (0x100aa2c0, 0x105d71b0),
    (0x100aa33c, 0x105d8b40), (0x100aa3d4, 0x105d71b0), (0x100aa44c, 0x105dac80), (0x100aa4e4, 0x105d71b0),
    (0x100aa55c, 0x105da6e0), (0x100aa5f4, 0x105d71b0), (0x100aa66c, 0x105dd980), (0x100aa704, 0x105d71b0),
    (0x100aa77c, 0x105db230), (0x100aa818, 0x105d71b0), (0x100aa894, 0x105dc1d0), (0x100aa930, 0x105d71b0),
    (0x100aaa08, 0x105ddd90), (0x100aaa84, 0x105d7200), (0x100ac16c, 0x105d7200), (0x100ac208, 0x105d76e0),
    (0x100ac3b4, 0x105d7200), (0x100ac450, 0x105dfa10), (0x100ac5fc, 0x105d7200), (0x100ac698, 0x105e01d0),
    (0x100ac844, 0x105d7200), (0x100ac8e0, 0x105e01d0), (0x100aca94, 0x105d7200), (0x100acb34, 0x105e01d0),
    (0x100acce8, 0x105d7200), (0x100acd88, 0x105d8ad0), (0x100acf3c, 0x105d7200), (0x100acfe8, 0x105da670),
    (0x100ad19c, 0x105d7200), (0x100ad23c, 0x105da670), (0x100ad3f0, 0x105d7200), (0x100ad490, 0x105df1a0),
    (0x100ad644, 0x105d7200), (0x100ad6f8, 0x105de9b0), (0x100ad8ac, 0x105d7200), (0x100ad94c, 0x105de9b0),
    (0x100ad9d4, 0x105de9b0), (0x100adb88, 0x105d7200), (0x100adc28, 0x105df5c0), (0x100adddc, 0x105d7200),
    (0x100ade88, 0x105ded70), (0x100ae03c, 0x105d7200), (0x100ae0dc, 0x105ded70), (0x100ae290, 0x105d7200),
    (0x100ae330, 0x105dfdf0), (0x100ae4e4, 0x105d7200), (0x100ae584, 0x105d8560), (0x100ae738, 0x105d7200),
    (0x100ae7d8, 0x105d8ef0), (0x100ae990, 0x105d7200), (0x100aea30, 0x105dac10), (0x100aec30, 0x105d7200),
    (0x100aecf8, 0x105ddd20), (0x100aeee4, 0x105d7200), (0x100aef9c, 0x105dc160), (0x100af188, 0x105d7200),
    (0x100af240, 0x105dd910), (0x100af3f4, 0x105d7200), (0x100af494, 0x105de0e0), (0x100af54c, 0x105d7200),
)
# Each loader's conversion, and the four version 1 readers.
OWNED_CALLS = {
    (0x105d7460, 0x105d7530), (0x105d7960, 0x105d7a30), (0x105d87e8, 0x105d88b0), (0x105d8d50, 0x105d8e20),
    (0x105d917c, 0x105d9290), (0x105d9210, 0x105d9710), (0x105da8f0, 0x105da9c0), (0x105dae90, 0x105daf60),
    (0x105db44c, 0x105db530), (0x105db4b0, 0x105db6c0), (0x105dc3ec, 0x105dc4d0), (0x105dc450, 0x105dc710),
    (0x105ddb90, 0x105ddc60), (0x105ddfa0, 0x105de070), (0x105de36c, 0x105de450), (0x105de3d0, 0x105de550),
    (0x105dec30, 0x105ded00), (0x105deff0, 0x105df0c0), (0x105df420, 0x105df4f0), (0x105df840, 0x105df910),
    (0x105dfc90, 0x105dfd60), (0x105e0070, 0x105e0140),
}
NOP = 0x60000000


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private DB_FILES export unavailable; inventory cannot be certified')


class DbFilesAuditTests(unittest.TestCase):
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

    def assert_production(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(audit.ROUTINES))
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in owners if at == entry], ['db_files_adapter'])

    def assert_hook(self, code, entry):
        hook = audit.db_files_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_exact_entries_original_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x105d71b0, 0x105e0240))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.DB_FILES_ENTRIES, ENTRIES)
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(ENTRIES), 59)
        self.assertEqual(len(self.records), 9055)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        self.assert_production(audit.rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_db_files_index(db)

    def test_source_pool_and_neighbors_close_the_boundary(self):
        toc = 0x108bba90
        pool = 0x108435dc
        self.assertEqual(self.word(toc - 0x42e8), pool)
        self.assertEqual(self.word(toc - 0x42e4), 0x1084371c)
        text = self.original_bytes(pool, 0x140)
        self.assertTrue(text.startswith(b'rb\0wb\0cm3_continents\0 \0.dam\0CM3_DATA\0DB_FILES.CPP\0v%s %s.%s %d\0Error\0'))
        self.assertIn(b'cm3_names\0', text)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            def assembly(address):
                return json.loads(db.execute('SELECT body FROM instructions WHERE address=?',
                                             (address,)).fetchone()[0])['assembly']
            # The previous module ends in an empty routine; the next one
            # (USA_RULES.CPP) reads its own pool through the following slot.
            self.assertEqual(assembly(0x105d71a0), 'blr')
            self.assertEqual(assembly(0x105e0240), 'mfspr r0,LR')
            self.assertEqual(assembly(0x105e0298), 'lwz r31,-0x42e4(r2)')
            users = [row[0] for row in db.execute(
                'SELECT address FROM instructions WHERE body LIKE ? ORDER BY address', ('%-0x42e8(r2)%',))]
        self.assertEqual(len(users), 22)
        self.assertTrue(all(audit.MODULE[0] <= address < audit.MODULE[1] for address in users))

    def test_all_incoming_edges_have_named_entry_and_no_external_fallthrough(self):
        incoming = []
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
                        incoming.append((address, target))
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*audit.MODULE, *audit.MODULE)))
        self.assertEqual(tuple(incoming), INCOMING)
        self.assertEqual(fallthroughs, [])

    def test_all_cross_entry_edges_are_the_direct_owned_calls(self):
        cross = set()
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                self.assertNotIn('COMPUTED', record['flow'])
                targets = record['flows'] + ([record['fallthrough']]
                                              if record.get('fallthrough') is not None else [])
                for target in targets:
                    if audit.MODULE[0] <= target < audit.MODULE[1] and not start <= target < end:
                        self.assertIn(target, ENTRIES)
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        cross.add((record['address'], target))
        self.assertEqual(cross, OWNED_CALLS)
        self.assertEqual([s for s in self.program['symbols']
                          if audit.MODULE[0] <= s['address'] < audit.MODULE[1]], [])

    def test_padding_is_zero_or_nop_and_nothing_reaches_it(self):
        addresses = set(self.by_address)
        padding = set(range(*audit.MODULE, 4)) - addresses
        self.assertEqual(len(padding), 197)
        self.assertEqual(sum(self.word(a) == 0 for a in padding), 80)
        self.assertEqual(sum(self.word(a) == NOP for a in padding), 117)
        for record in self.records:
            for target in record['flows'] + ([record['fallthrough']]
                                              if record.get('fallthrough') is not None else []):
                self.assertNotIn(target, padding)

    def test_every_routine_ends_in_blr_and_no_aligned_pointer_reaches_the_module(self):
        for start, end in audit.ROUTINES:
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(records[-1]['assembly'], 'blr')
        for start, contents in self.segments:
            for match in re.finditer(b'\x10[\x5d\x5e]', contents):
                offset = match.start()
                if (start + offset) % 4 or offset + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                self.assertFalse(audit.MODULE[0] <= target < audit.MODULE[1], f'{start + offset:x}')

    def test_missing_export_and_stale_digest_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_db_files_reference.py'),
                                     directory, str(Path(directory) / 'reference.cpp')],
                                    text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((Path(directory) / 'reference.cpp').exists())
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten(drafts=True))]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_db_files_index(db)

    def test_duplicate_bad_boundary_draft_and_missing_owner_are_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'DB_FILES_ROUTINE(' in line)
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('0x105d7200, R3', '0x105d7210, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('DB_FILES_ROUTINE(', 'DB_FILES_DRAFT(', 1))
            self.assertEqual(len(audit.rewritten(candidate, drafts=True)), 59)
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all 59 complete'):
                    audit.require_db_files_index(db, candidate)
        missing = [(at, owner) for at, owner in OWNER_REPLACEMENTS if at != ENTRIES[0]]
        with self.assertRaises(AssertionError):
            self.assert_production(audit.rewritten(), missing)

    def test_normal_hooks_precede_original_steps_and_detect_disabled_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_db_files_hooks(page, part, normal)
            self.assertIn('#include "db_files_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.db_files_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_db_files_hooks(page, part, broken)

    def test_generated_original_reference_has_every_label_and_no_rewrite(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_db_files_reference.py'),
                                     str(EXPORT), str(target)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            code = target.read_text()
        self.assertNotIn('try_modern_', code)
        self.assertNotIn('db_files::', code)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE),
                         [f'{r["address"]:x}' for r in self.records])


if __name__ == '__main__':
    unittest.main()
