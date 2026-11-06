"""Check the Gaúcho source boundary, original reference and production ownership."""
from contextlib import closing
import json

import re
import sqlite3
import unittest
from unittest.mock import patch

import bra_reg_gaucho_audit as audit
from tactics_audit import routine_records

EXPORT = audit.ROOT / '.local/current/export'
TOC = 0x108bba90
VTABLE = 0x108e0a24
VIRTUAL_METHODS = {
    0x08: 0x106f1100, 0x10: 0x106f1f80, 0x30: 0x106f2210, 0x38: 0x106f2810,
    0x44: 0x106f1200, 0x4c: 0x106f24d0, 0x50: 0x106f2630, 0x64: 0x106f2850,
    0x70: 0x106f2a30, 0x94: 0x106f1eb0, 0x98: 0x106f2ac0,
}
OWNED_CALLS = {(0x106f0f10, 0x106f1d60), (0x106f20ac, 0x106f1d60),
               (0x106f22b0, 0x106f22d0)}


def open_index():
    return closing(sqlite3.connect((EXPORT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True))


class GauchoAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes())
                        for s in cls.program['segments'] if 'file' in s]
        with open_index() as db:
            cls.records = routine_records(db, *audit.MODULE)
        cls.index = {r['address']: r for r in cls.records}

    def bytes_at(self, address, size):
        for base, data in self.segments:
            if base <= address and address + size <= base + len(data):
                return data[address - base:address - base + size]
        self.fail(f'Unmapped original data at {address:#x}')

    def word(self, address):
        return int.from_bytes(self.bytes_at(address, 4), 'big')

    def test_source_pool_neighbours_and_all_transition_vectors(self):
        start, end = audit.MODULE
        self.assertEqual((start, end), (0x106f0dc0, 0x106f2ae0))
        pool = self.word(TOC - 0x407c)
        self.assertEqual(self.bytes_at(pool, 0x26), b'bra_reg_gaucho.cpp\0v%s %s.%s %d\0Error\0')
        self.assertEqual(self.word(TOC - 0x5ca8), VTABLE)
        vectors = {}
        for slot, target in VIRTUAL_METHODS.items():
            at = self.word(VTABLE + slot)
            self.assertEqual((self.word(at), self.word(at + 4)), (target, TOC))
            vectors[at] = target
        self.assertEqual(self.word(0x108bd124), 0x106ef3a0)
        self.assertEqual(self.word(0x108bd184), 0x106f47c0)
        pointers = {}
        for base, data in self.segments:
            for match in re.finditer(b'\x10\x6f', data):
                offset = match.start()
                target = int.from_bytes(data[offset:offset + 4], 'big')
                if start <= target < end:
                    pointers[base + offset] = target
        self.assertEqual(pointers, {**vectors, 0x1035d977: 0x106f2380})
        # The unaligned match straddles two unrelated instruction encodings.
        self.assertEqual(self.bytes_at(0x1035d974, 8)[3:7], bytes.fromhex('106f2380'))
        self.assertFalse(any(start <= symbol['address'] < end for symbol in self.program['symbols']))
        with open_index() as db:
            for at, expected in ((start - 4, 'blr'), (end, 'mfspr r0,LR'),
                                 (0x106f2af8, 'lwz r31,-0x4074(r2)')):
                row = db.execute('SELECT body FROM instructions WHERE address=?', (at,)).fetchone()
                self.assertEqual(json.loads(row[0])['assembly'], expected)
            references = db.execute('SELECT address FROM instructions WHERE body LIKE ? ORDER BY address',
                                    ('%-0x407c(r2)%',))
            self.assertEqual([a for a, in references], [0x106f0dd8, 0x106f1214, 0x106f1d7c, 0x106f1f98,
                                                       0x106f2218, 0x106f22e8, 0x106f2648, 0x106f2864])

    def test_entry_and_internal_flow_closes_inventory(self):
        start, end = audit.MODULE
        with open_index() as db:
            incoming = list(db.execute(
                "SELECT i.address, f.value, json_extract(i.body,'$.flow') FROM instructions i,"
                "json_each(i.body,'$.flows') f WHERE f.value>=? AND f.value<? AND (i.address<? OR i.address>=?)",
                (*audit.MODULE, *audit.MODULE)))
            self.assertEqual(incoming, [(0x10350914, start, 'UNCONDITIONAL_CALL')])
            self.assertFalse(list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*audit.MODULE, *audit.MODULE))))
        expected_entries = {incoming[0][1], *VIRTUAL_METHODS.values(), *(to for _, to in OWNED_CALLS)}
        self.assertEqual(expected_entries, set(audit.BRA_REG_GAUCHO_ENTRIES))
        owned, dependencies = set(), set()
        for first, last in audit.ROUTINES:
            self.assertEqual(self.index[first]['assembly'], 'mfspr r0,LR')
            self.assertEqual(self.index[last - 4]['assembly'], 'blr')
            for address in range(first, last, 4):
                if address in audit.UNREACHABLE_NOPS:
                    self.assertEqual(self.word(address), 0x60000000)
                    self.assertTrue(self.index[address - 4]['assembly'].startswith('b '))
                    continue
                record = self.index[address]
                self.assertNotIn('COMPUTED', record['flow'])
                for target in record['flows'] + [record.get('fallthrough')]:
                    if target is None:
                        continue
                    if start <= target < end:
                        self.assertIn(target, self.index)
                        if not first <= target < last:
                            self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                            owned.add((address, target))
                    else:
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        dependencies.add(target)
        self.assertEqual(owned, OWNED_CALLS)
        self.assertEqual(sorted(dependencies), list(audit.EXTERNAL_DEPENDENCIES))
        self.assertEqual(len(self.records), 1837)
        self.assertEqual({a for a in range(start, end, 4) if a not in self.index and self.word(a)},
                         audit.UNREACHABLE_NOPS)

    def test_digests_and_generator_ownership(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS, generate_page
        with open_index() as db:
            audit.require_bra_reg_gaucho_index(db)
            with patch.object(audit, 'rewritten', return_value=[
                (a, b, '0' * 16) for a, b in audit.ROUTINES]):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_bra_reg_gaucho_index(db)
        self.assertEqual(len(audit.BRA_REG_GAUCHO_ENTRIES), 14)
        for entry in audit.BRA_REG_GAUCHO_ENTRIES:
            self.assertEqual([owner for at, owner in OWNER_REPLACEMENTS if at == entry], ['bra_reg_gaucho_adapter'])
            self.assertFalse(any(at == entry for at, _ in SEMANTIC_REPLACEMENTS))
        for page in sorted({a >> 12 for a in self.index}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, self.index.keys(), userops=self.program['userops'])
            audit.require_bra_reg_gaucho_hooks(page, part, normal)
            for entry in audit.BRA_REG_GAUCHO_ENTRIES:
                if entry >> 12 != page:
                    continue
                hook = audit.bra_reg_gaucho_hook(entry)
                for replacement in ('', hook + '\n' + hook):
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_bra_reg_gaucho_hooks(page, part, normal.replace(hook, replacement))


if __name__ == '__main__':
    unittest.main()
