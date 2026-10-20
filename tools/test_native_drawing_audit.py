"""Read-only native rendering hooks must survive direct generated gotos."""
from copy import deepcopy
from contextlib import closing
import json
from pathlib import Path
import sqlite3
import unittest
from compile_recomp import generate_page, operation
from native_drawing_audit import NATIVE_DRAWING_BOUNDARIES, require_native_drawing_contract

INDEX = Path(__file__).resolve().parent.parent / '.local/current/export/instructions.sqlite'

@unittest.skipUnless(INDEX.exists(), 'Private Mac export unavailable')
class NativeDrawingAuditTests(unittest.TestCase):
    def test_every_observer_is_at_its_destination_and_preserves_instructions(self):
        with closing(sqlite3.connect(INDEX.resolve().as_uri() + '?mode=ro', uri=True)) as db:
            for page in sorted({a >> 12 for a in NATIVE_DRAWING_BOUNDARIES}):
                records = [json.loads(row[0]) for row in db.execute(
                    'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                    (page << 12, (page + 1) << 12))]
                generated = generate_page(page, records, {r['address'] for r in records})
                for address in NATIVE_DRAWING_BOUNDARIES:
                    if address >> 12 != page:
                        continue
                    fragment = generated.split(f'i_{address:x}:', 1)[1]
                    self.assertLess(fragment.index(f'native_drawing_boundary(s, 0x{address:x});'), fragment.index(f's.step(0x{address:x});'))
                    self.assertNotIn('return', fragment[:fragment.index(f's.step(0x{address:x});')])
                    self.assertEqual(operation(['CALL', None, ['ram', str(address), 4]], address - 4, 0, page, {address}, 1), f'goto i_{address:x};')
                    changed = deepcopy(next(r for r in records if r['address'] == address))
                    changed['ops'] = []
                    with self.assertRaisesRegex(ValueError, 'Native drawing observation contract changed'):
                        require_native_drawing_contract([changed])

if __name__ == '__main__':
    unittest.main()
