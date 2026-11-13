"""Certify GAME_CONFIG.CPP's twelve entries, the unit's bounds and its production owner.

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

import game_config_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x1013f030, 0x1013f404, 245, '3fd3aaa3fb0d2c9c'),
    (0x1013f410, 0x1013f4c4, 45, 'd751fdd1dc15c5f8'),
    (0x1013f4d0, 0x1013f564, 37, 'f14583780105afdb'),
    (0x1013f570, 0x1013f67c, 67, '6f8efcc4d4b58a71'),
    (0x1013f680, 0x1013fb58, 310, '81cfdde4046345f5'),
    (0x1013fb60, 0x1013fc44, 57, 'a8adb922ba9e7666'),
    (0x1013fc50, 0x1013fcfc, 43, '110b2be3b63248f1'),
    (0x1013fd00, 0x1013fdf4, 61, 'a44f971e429f385e'),
    (0x1013fe00, 0x1013feac, 43, '4b29baa1449d9eb7'),
    (0x1013feb0, 0x101406d4, 521, '3d6c78a790d7cee6'),
    (0x101406e0, 0x101406e8, 2, '6d00413f899a2979'),
    (0x101406f0, 0x10140724, 13, '1ee31f70870136d7'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# Every call from outside the unit: 40 sites in DATABASE.CPP, GAME.CPP, MENUBAR.CPP and SETUP.CPP.
INCOMING = (
    (0x100a56b8, 0x1013fb60), (0x100a5764, 0x1013fb60), (0x10118f44, 0x1013f030), (0x1011903c, 0x1013f4d0),
    (0x102a65e0, 0x1013f680), (0x102a660c, 0x1013feb0), (0x10334b70, 0x1013feb0), (0x10334cb4, 0x1013feb0),
    (0x10334d28, 0x1013feb0), (0x1033541c, 0x1013feb0), (0x10337ea8, 0x1013feb0), (0x10337ff0, 0x1013feb0),
    (0x1033cecc, 0x1013f410), (0x1033cf80, 0x1013f410), (0x1033cfe0, 0x1013f410), (0x1033da68, 0x101406e0),
    (0x1033f8d8, 0x1013f570), (0x1033f930, 0x1013feb0), (0x1033f93c, 0x1013f4d0), (0x1033f990, 0x1013feb0),
    (0x1033f99c, 0x1013f4d0), (0x1033fa18, 0x1013fb60), (0x1033fab4, 0x101406f0), (0x1033fb30, 0x1013fc50),
    (0x1033fba8, 0x1013f680), (0x1033fc00, 0x1013f680), (0x1033fc3c, 0x1013fe00), (0x10341944, 0x1013feb0),
    (0x10341b34, 0x1013feb0), (0x10342120, 0x1013f680), (0x10342144, 0x1013f680), (0x10342168, 0x1013f680),
    (0x103421a8, 0x1013f680), (0x103421cc, 0x1013f680), (0x103421f0, 0x1013f680), (0x1034228c, 0x1013feb0),
    (0x10342518, 0x1013feb0), (0x10342720, 0x1013fb60), (0x10342728, 0x1013feb0), (0x10343864, 0x1013feb0),
)
# The loads of the unit's string pool (TOC -0x54e8): one per routine that reports or names a file.
POOL_LOADS = [0x1013f048, 0x1013f68c, 0x1013fb68, 0x1013fc58, 0x1013fe08, 0x1013fed0]
# Byte patterns elsewhere that read as an address in the unit. None is an entry, none is word-aligned
# data followed by the TOC (a transition vector), so no pointer to the unit exists.
POINTER_LOOKALIKES = {0x102ae3fd: 0x1014057c, 0x108a2c61: 0x10140080, 0x108c266c: 0x101402c0}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private GAME_CONFIG export unavailable; inventory cannot be certified')


class GameConfigAuditTests(unittest.TestCase):
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

    def assembly(self, db, address):
        row = db.execute('SELECT body FROM instructions WHERE address=?', (address,)).fetchone()
        return json.loads(row[0])['assembly'] if row else None

    def assert_production(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(audit.ROUTINES))
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in owners if at == entry], ['game_config_adapter'])

    def assert_hook(self, code, entry):
        hook = audit.game_config_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_exact_entries_original_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x1013f030, 0x10140724))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.GAME_CONFIG_ENTRIES, ENTRIES)
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(self.records), 1444)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        self.assert_production(audit.rewritten(), OWNER_REPLACEMENTS)
        # The register-level owner of the earlier process no longer routes any of them.
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_game_config_index(db)

    def test_unit_bounds_are_its_string_pool_and_its_neighbours(self):
        pool = self.word(TOC - 0x54e8)
        strings = self.original_bytes(pool, 0x105).split(b'\0')
        self.assertEqual(strings[:5], [b'CM3_DATA', b'language.ldb', b'GAME_CONFIG.CPP', b'v%s %s.%s %d', b'Error'])
        self.assertEqual(strings[-1], b'wb')
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            self.assertEqual([a for a, in db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                     ('%-0x54e8(r2)%',))], POOL_LOADS)
            # Before: the static initializer that closes GAME.CPP. After: HALL_OF_FAME.CPP's first routine.
            self.assertEqual(self.assembly(db, 0x1013f000), 'mfspr r0,LR')
            self.assertEqual(self.assembly(db, 0x1013f028), 'blr')
            self.assertIsNone(self.assembly(db, 0x1013f02c))
            self.assertEqual(self.assembly(db, 0x10140730), 'mfspr r0,LR')
        from engine_initialization_audit import CONTRACTS as INITIALIZERS
        self.assertIn((0x1013f000, 0x1013f02c), [(a, b) for a, b, *_ in INITIALIZERS])
        import hall_of_fame_audit
        self.assertEqual(hall_of_fame_audit.MODULE[0], 0x10140730)
        # The last two routines have no pool load of their own: they read and write the record's font
        # (+0x12c, +0x130), which 0x1013f030 and 0x1013feb0 read, write and store.
        self.assertEqual(self.by_address[0x101406e0]['assembly'], 'lwz r3,0x12c(r3)')
        self.assertEqual(self.by_address[0x10140700]['assembly'], 'stw r4,0x12c(r3)')
        self.assertEqual(self.by_address[0x10140708]['assembly'], 'addi r4,r5,0x130')

    def test_all_incoming_edges_are_calls_of_entries_and_no_external_fallthrough(self):
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
        # 0x1013fd00 is called from inside the unit only.
        self.assertEqual({target for _, target in INCOMING}, set(ENTRIES) - {0x1013fd00})

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
        self.assertEqual(cross, {0x1013f570, 0x1013f680, 0x1013fd00, 0x101406e0})
        self.assertEqual([s for s in self.program['symbols']
                          if audit.MODULE[0] <= s['address'] < audit.MODULE[1]], [])

    def test_every_instruction_and_alignment_word_accounted(self):
        addresses = set()
        for start, end in audit.ROUTINES:
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual([r['address'] for r in records], list(range(start, end, 4)))
            self.assertEqual(records[-1]['assembly'], 'blr')
            addresses.update(range(start, end, 4))
        padding = set(range(*audit.MODULE, 4)) - addresses
        self.assertEqual(len(padding), 25)
        for address in padding:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)

    def test_no_pointer_to_the_unit(self):
        pointers = {}
        for start, contents in self.segments:
            for match in re.finditer(b'\x10[\x13\x14]', contents):
                offset = match.start()
                if offset + 4 > len(contents):
                    continue
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if audit.MODULE[0] <= target < audit.MODULE[1]:
                    pointers[start + offset] = target
        self.assertEqual(pointers, POINTER_LOOKALIKES)
        for at, target in pointers.items():
            self.assertNotIn(target, ENTRIES)
            self.assertNotEqual(self.word(at + 4), TOC)

    def test_missing_export_and_stale_digest_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_game_config_reference.py'),
                                     directory, str(Path(directory) / 'reference.cpp')],
                                    text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((Path(directory) / 'reference.cpp').exists())
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with self.assertRaisesRegex(ValueError, 'Missing GAME_CONFIG.CPP instructions'):
                audit.require_game_config_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten(drafts=True))]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_game_config_index(db)

    def test_duplicate_bad_boundary_draft_and_missing_owner_are_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = audit.INCLUDES.read_text()
        first = next(line for line in contents.splitlines() if 'GAME_CONFIG_ROUTINE(' in line)
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('0x1013f404, R3', '0x1013f408, R3', 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace(first, first.replace('GAME_CONFIG_ROUTINE(', 'GAME_CONFIG_DRAFT(')))
            self.assertEqual(len(audit.rewritten(candidate, drafts=True)), 12)
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                with self.assertRaisesRegex(ValueError, 'all twelve complete'):
                    audit.require_game_config_index(db, candidate)
        missing = [(at, owner) for at, owner in OWNER_REPLACEMENTS if at != ENTRIES[0]]
        with self.assertRaises(AssertionError):
            self.assert_production(audit.rewritten(), missing)

    def test_normal_hooks_precede_original_steps_and_detect_missing_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_game_config_hooks(page, part, normal)
            self.assertIn('#include "game_config_adapter.hpp"', normal)
            self.assertNotIn('try_modern_preferences(', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.game_config_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_game_config_hooks(page, part, broken)

    def test_generated_original_reference_has_every_label_and_no_rewrite(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'reference.cpp'
            result = subprocess.run([sys.executable, str(ROOT / 'tools/make_game_config_reference.py'),
                                     str(EXPORT), str(target)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            code = target.read_text()
        self.assertNotIn('try_modern_', code)
        self.assertNotIn('game_config::', code)
        self.assertEqual(re.findall(r'^i_([0-9a-f]+):', code, re.MULTILINE),
                         [f'{r["address"]:x}' for r in self.records])


if __name__ == '__main__':
    unittest.main()
