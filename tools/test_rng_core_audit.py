import json
from contextlib import closing
import sqlite3
import unittest
from pathlib import Path
from compile_recomp import generate_page, operation
from rng_core_audit import CONTRACTS, RNG_CORE_ENTRIES, require_rng_core_contract

INDEX = Path(__file__).resolve().parent.parent / '.local/recomp-input/instructions.sqlite'


@unittest.skipUnless(INDEX.exists(), 'Private Mac export unavailable')
class RngAuditTests(unittest.TestCase):
    def records(self, page):
        with closing(sqlite3.connect(INDEX.resolve().as_uri() + '?mode=ro', uri=True)) as db:
            return [json.loads(row[0]) for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                (page << 12, (page + 1) << 12))]

    def test_complete_contracts_and_destination_hooks(self):
        self.assertEqual(len(RNG_CORE_ENTRIES), 3)
        for start, end, _ in CONTRACTS:
            records = self.records(start >> 12)
            require_rng_core_contract(start >> 12, records)
            code = generate_page(start >> 12, records, {r['address'] for r in records})
            self.assertIn(f'i_{start:x}:', code)
            self.assertIn(f'if (try_modern_rng_core(s, 0x{start:x})) return true;', code)
            self.assertEqual(operation(['CALL', None, ['ram', str(start), 4]],
                start - 4, 0, start >> 12, {start}, 1), f'goto i_{start:x};')
            for victim in (start, end - 4):
                with self.assertRaisesRegex(ValueError, 'RNG core contract changed'):
                    require_rng_core_contract(start >> 12, [r for r in records if r['address'] != victim])
            next(r for r in records if r['address'] == start)['ops'] = []
            with self.assertRaisesRegex(ValueError, 'RNG core contract changed'):
                require_rng_core_contract(start >> 12, records)

    def test_consumer_remains_generated(self):
        records = self.records(0x10712)
        code = generate_page(0x10712, records, {r['address'] for r in records})
        self.assertNotIn('try_modern_rng_core', code)
        self.assertIn('s.pc = 0x1077b770; return true;', code)


if __name__ == '__main__':
    unittest.main()
