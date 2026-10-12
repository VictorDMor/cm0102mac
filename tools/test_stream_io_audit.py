"""StreamIo whole-body/page pins and destination-hook negatives."""
from contextlib import closing
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import sqlite3
import unittest

from stream_io_audit import (CONTRACTS, STREAM_IO_ENTRIES, INSTRUCTION_COUNTS,
    require_stream_io_contract, require_stream_io_bodies, require_stream_io_edges,
    require_stream_io_index, require_stream_io_hooks)
from compile_recomp import SEMANTIC_REPLACEMENTS, generate_page, operation

INDEX = Path(__file__).resolve().parent.parent / '.local/recomp-input/instructions.sqlite'


class StreamIoManifestTests(unittest.TestCase):
    def test_exact_prior_140_and_9_ordered_entries(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS), 649)
        self.assertEqual(len({a for a, _ in SEMANTIC_REPLACEMENTS}), 649)
        self.assertEqual(SEMANTIC_REPLACEMENTS[140:149], tuple((e, 'stream_io_adapter') for e in STREAM_IO_ENTRIES))
        self.assertEqual(STREAM_IO_ENTRIES, tuple(sorted(STREAM_IO_ENTRIES)))
        self.assertEqual(len(CONTRACTS), 9)
        self.assertEqual(sum(INSTRUCTION_COUNTS), 682)
        prior = [{'entry': f'0x{entry:x}', 'module': module} for entry, module in SEMANTIC_REPLACEMENTS[:140]]
        self.assertEqual(hashlib.sha256(json.dumps(prior, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         '5c4dfac37d687da95ad3c8c4ee0cc14ecd0a4c71d88618a5a79d270c3600d121')


@unittest.skipUnless(INDEX.exists(), 'Private Mac export unavailable')
class StreamIoAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect(INDEX.resolve().as_uri()+'?mode=ro', uri=True)) as db:
            cls.records = [json.loads(row[0]) for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                (0x10775ad0, 0x1077fad8))]
        cls.pages = {page: [r for r in cls.records if r['address'] >> 12 == page] for page in (0x10775, 0x10776, 0x1077f)}
        cls.codes = {page: generate_page(page, records, {r['address'] for r in records})
                     for page, records in cls.pages.items()}

    def test_complete_bodies_fragments_and_destination_hooks(self):
        require_stream_io_bodies(self.records, complete=True)
        for (start, end, _), count in zip(CONTRACTS, INSTRUCTION_COUNTS):
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(len({r['address'] for r in body}), count)
            self.assertEqual(body[0]['address'], start)
            self.assertEqual(body[-1]['address'], end-4)
            self.assertEqual(body[-1]['assembly'], 'blr')
            for page in range(start >> 12, ((end-1) >> 12)+1):
                require_stream_io_contract(page, self.pages[page])
            code = self.codes[start >> 12]
            fragment = code.split(f'i_{start:x}:', 1)[1].split(f'i_{start+4:x}:', 1)[0]
            hook = f'if (try_modern_stream_io(s, 0x{start:x})) return true;'
            self.assertIn('#include "stream_io_adapter.hpp"', code)
            self.assertIn(f'case 0x{start:x}: goto i_{start:x};', code)
            self.assertEqual(fragment.count(hook), 1)
            self.assertLess(fragment.index(hook), fragment.index(f's.step(0x{start:x});'))
            for kind in ('CALL', 'BRANCH'):
                self.assertEqual(operation([kind, None, ['ram', str(start), 4]], start-4, 0, start >> 12, {start}, 1), f'goto i_{start:x};')
                self.assertEqual(operation([kind, None, ['ram', str(start), 4]], start-4, 0, (start >> 12)+1, {start}, 1), f's.pc = 0x{start:x}; return true;')
            for kind in ('CALLIND', 'BRANCHIND'):
                self.assertEqual(operation([kind, None, ['register', '4200', 4]], start-4, 0, start >> 12, {start}, 1), 's.pc = std::uint32_t(s.r<4>(4200)); return true;')

    def test_changed_partial_duplicate_and_missing_fragments_reject(self):
        for start, end, _ in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            for victim in (start, start+4, end-4):
                partial = [r for r in body if r['address'] != victim]
                with self.assertRaisesRegex(ValueError, 'stream_io contract changed'):
                    require_stream_io_bodies(partial)
                page = victim >> 12
                with self.assertRaisesRegex(ValueError, 'stream_io contract changed'):
                    require_stream_io_contract(page, [r for r in partial if r['address'] >> 12 == page])
            for field, value in (('assembly', 'changed'), ('ops', []), ('flows', [-1]), ('fallthrough', -1), ('address', start-4)):
                changed = deepcopy(body); changed[0][field] = value
                with self.assertRaisesRegex(ValueError, 'stream_io contract changed'):
                    require_stream_io_bodies(changed)
            with self.assertRaisesRegex(ValueError, 'stream_io contract changed'):
                require_stream_io_bodies(body + [deepcopy(body[0])])

    def test_missing_bodies_cannot_emit_full_manifest(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            for start, end, _ in CONTRACTS:
                db.execute('DELETE FROM instructions')
                db.executemany('INSERT INTO instructions VALUES (?,?)',
                    ((r['address'], json.dumps(r)) for r in self.records if not start <= r['address'] < end))
                with self.assertRaisesRegex(ValueError, 'Missing complete Mac stream_io body'):
                    require_stream_io_index(db)
            for page in (0x10775, 0x10776, 0x1077f):
                db.execute('DELETE FROM instructions')
                db.executemany('INSERT INTO instructions VALUES (?,?)',
                    ((r['address'], json.dumps(r)) for r in self.records if r['address'] >> 12 != page))
                with self.assertRaises(ValueError): require_stream_io_index(db)

    def test_all_edges_and_discriminating_interior_negatives(self):
        with closing(sqlite3.connect(INDEX.resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_stream_io_index(db)
        for start, end, _ in CONTRACTS:
            for target in (start+4, end-4):
                with self.assertRaisesRegex(ValueError, 'External stream_io interior edge'):
                    require_stream_io_edges([(start-4, target)])
            require_stream_io_edges([(start-4, start), (start, start+4)])

    def test_missing_duplicate_and_misplaced_hooks_reject(self):
        for start in STREAM_IO_ENTRIES:
            page = start >> 12; code = self.codes[page]
            hook = f'if (try_modern_stream_io(s, 0x{start:x})) return true;'
            for changed in (code.replace(hook, ''), code+hook,
                            code.replace(hook, '').replace(f's.step(0x{start:x});', f's.step(0x{start:x});\n'+hook)):
                with self.assertRaisesRegex(ValueError, 'stream_io hook'):
                    require_stream_io_hooks(page, self.pages[page], changed)
        for page, records in self.pages.items():
            code = generate_page(page, records, {r['address'] for r in records}, local_temporaries=False)
            require_stream_io_hooks(page, records, code)

    def test_io_block_excludes_storage_and_other_file_services(self):
        for entry in (0x10775240, 0x107757d0, 0x107758a0, 0x107766f0,
                      0x107767b0, 0x10776940, 0x10777cc0, 0x10777d20):
            self.assertNotIn(entry, STREAM_IO_ENTRIES)


if __name__ == '__main__':
    unittest.main()
