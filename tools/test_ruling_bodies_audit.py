"""Certify the ruling-body routines in C++: bounds, the class each belongs to, and the production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
import json
from pathlib import Path
import sqlite3
import unittest

import ruling_bodies_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CONTRACTS = (
    (0x106abe20, 0x106abed4, 45, 'a90f4757ce5ea339'),
    (0x106abee0, 0x106abf58, 30, '83aee9d44c3f7916'),
    (0x106abf60, 0x106abfe0, 32, 'f0c7619aed365abc'),
    (0x106abfe0, 0x106ac21c, 142, 'c948794206216ea9'),
)
# Class -> (TOC slot of its virtual table, table, {offset: (transition vector, routine)}).
# +0x10 is the report, +0x18 the nation text, +0x1c the query.
CLASSES = {
    'rb_ireland.cpp': (-0x5ddc, 0x108decec, {0x10: (0x108bc524, 0x106ab970), 0x18: (0x108bc51c, 0x106abe20),
                                            0x1c: (0x108bc514, 0x106abee0)}),
    'rb_northern_ireland.cpp': (-0x5dd8, 0x108ded14, {0x10: (0x108bc544, 0x106abfe0), 0x18: (0x108bc53c, 0x106ac220),
                                                     0x1c: (0x108bc534, 0x106ac2e0)}),
}
# The constructor that stores each table, and its one caller (the rulebook factory).
CONSTRUCTORS = {'rb_ireland.cpp': (0x106ab8f0, 0x106ab908, 0x100e80bc),
                'rb_northern_ireland.cpp': (0x106abf60, 0x106abf78, 0x100e8500)}
# The nation text names its source file: TOC slot of the pool, the name, the one load of the slot.
SOURCES = {'rb_ireland.cpp': (-0x41d0, 0x106abe2c), 'rb_northern_ireland.cpp': (-0x41cc, 0x106ac22c)}
PADDING = {0x106abed4: 0, 0x106abed8: 0, 0x106abedc: 0, 0x106abf58: 0, 0x106abf5c: 0, 0x106ac068: 0x60000000,
           0x106ac21c: 0}


class RulingBodiesAudit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (EXPORT / 'instructions.sqlite').is_file():
            raise RuntimeError('Private export unavailable; the ruling-body inventory cannot be certified')
        program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
        cls.db = sqlite3.connect((EXPORT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)

    @classmethod
    def tearDownClass(cls):
        cls.db.close()

    def read(self, address, size):
        for base, data in self.segments:
            if base <= address <= base + len(data) - size:
                return data[address - base:address - base + size]
        raise AssertionError(f'no original bytes at {address:#x}')

    def word(self, address):
        return int.from_bytes(self.read(address, 4), 'big')

    def assembly(self, address):
        return routine_records(self.db, address, address + 4)[0]['assembly']

    def test_bodies_and_production_lines(self):
        self.assertEqual(audit.ROUTINES, tuple((entry, end) for entry, end, _, _ in CONTRACTS))
        self.assertEqual(audit.rewritten(), [(entry, end, pinned) for entry, end, _, pinned in CONTRACTS])
        self.assertEqual(audit.RULING_BODIES_ENTRIES, tuple(entry for entry, _, _, _ in CONTRACTS))
        for entry, end, count, pinned in CONTRACTS:
            body = routine_records(self.db, entry, end)
            self.assertEqual((len(body), digest(body)), (count, pinned), hex(entry))
            self.assertEqual(body[-1]['assembly'], 'blr')
            # No flow leaves the routine except calls and the return.
            for record in body:
                if 'CALL' not in record['flow']:
                    self.assertTrue(all(entry <= target < end for target in record['flows']), hex(record['address']))
        audit.require_ruling_bodies_index(self.db)
        for address, value in PADDING.items():
            self.assertEqual(self.word(address), value)

    def test_classes_tie_the_routines_together(self):
        for unit, (slot, table, slots) in CLASSES.items():
            self.assertEqual(self.word(TOC + slot), table, unit)
            for offset, (vector, routine) in slots.items():
                self.assertEqual((self.word(table + offset), self.word(vector), self.word(vector + 4)),
                                 (vector, routine, TOC), f'{unit} +{offset:#x}')
            constructor, store, caller = CONSTRUCTORS[unit]
            self.assertEqual(self.assembly(store), f'lwz r0,-{-slot:#x}(r2)')
            self.assertLess(constructor, store)
            self.assertEqual(self.assembly(caller), f'bl {constructor:#x}')
            callers = [a for a, in self.db.execute('SELECT address FROM instructions WHERE body LIKE ?',
                                                   (f'%bl {constructor:#x}"%',))]
            self.assertEqual(callers, [caller])

    def test_source_names(self):
        for unit, (slot, load) in SOURCES.items():
            at = self.word(TOC + slot)
            self.assertEqual(self.read(at, len(unit) + 1), unit.encode() + b'\0')
            loads = [a for a, in self.db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                                 (f'%-{-slot:#x}(r2)%',))]
            self.assertEqual(loads, [load])
        # The Irish text and query are the ones the Irish class's table names; every pointer to the
        # four routines is one of the vectors above or the factory's direct call.
        vectors = {vector: routine for _, _, slots in CLASSES.values() for vector, routine in slots.values()}
        for entry, _, _, _ in CONTRACTS:
            found = []
            for base, data in self.segments:
                at = data.find(entry.to_bytes(4, 'big'))
                while at >= 0:
                    if at % 4 == 0:
                        found.append(base + at)
                    at = data.find(entry.to_bytes(4, 'big'), at + 1)
            expected = [vector for vector, routine in vectors.items() if routine == entry]
            self.assertEqual(found, expected, hex(entry))

    def test_generator_hooks(self):
        for entry in audit.RULING_BODIES_ENTRIES:
            hook = audit.ruling_bodies_hook(entry)
            good = f'i_{entry:x}:\n{hook}\ns.step(0x{entry:x});\ni_{entry + 4:x}:\n'
            audit.require_ruling_bodies_hooks(entry >> 12, [{'address': entry}], good)
            with self.assertRaises(ValueError):
                audit.require_ruling_bodies_hooks(entry >> 12, [{'address': entry}], good.replace(hook, ''))


if __name__ == '__main__':
    unittest.main()
