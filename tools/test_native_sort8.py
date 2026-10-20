"""Audit the fused sort against the private original instruction export."""
from copy import deepcopy
import inspect
from pathlib import Path
import sqlite3
import sys
import unittest

from native_sort8 import CONTRACTS, ENTRY, generate_sort8, load_sort8, require_fragment


class NativeSortAudit(unittest.TestCase):
    records = []

    @classmethod
    def setUpClass(cls):
        if not cls.records:
            raise unittest.SkipTest('Pass the private instructions.sqlite')

    def test_every_instruction_is_required_and_immutable(self):
        for first, (last, _, _) in CONTRACTS.items():
            rows = [r for r in self.records if first <= r['address'] <= last]
            chosen = {r['address'] for r in rows}
            for index in range(len(rows)):
                for changed, selected in ((rows[:index] + rows[index + 1:], chosen),
                                          (rows + [rows[index]], chosen),
                                          (rows, chosen - {rows[index]['address']})):
                    with self.assertRaises(ValueError):
                        require_fragment(first, changed, selected)
                for field, value in (('size', 8), ('assembly', 'nop'), ('flow', 'TEST'),
                                     ('fallthrough', 0), ('flows', [0]), ('ops', [])):
                    if rows[index].get(field) == value:
                        continue
                    changed = deepcopy(rows)
                    changed[index][field] = value
                    with self.assertRaises(ValueError):
                        require_fragment(first, changed, chosen)

    @staticmethod
    def render(page, records, chosen):
        from compile_recomp import generate_page
        hooks = {name: False for name in inspect.signature(generate_page).parameters
                 if name.endswith('_hooks')}
        return generate_page(page, records, chosen, **hooks, native_leaf_lowering=True)

    def test_lowering_and_intra_page_entry_hook(self):
        from compile_recomp import generate_page
        result = generate_sort8(self.records, self.render)
        for row in self.records:
            self.assertIn(f'frame.step(0x{row["address"]:x})', result)
        self.assertNotIn('try_modern_', result)
        self.assertIn('if(frame.count>UINT64_MAX-5)', result)
        self.assertIn('if(count++==UINT64_MAX)', result)
        self.assertIn('frame.category_enter(3)', result)
        self.assertIn('frame.category_enter(4)', result)
        self.assertIn('if(frame.pc==0x10768774)goto', result)
        self.assertIn('s.budget!=UINT64_MAX', result)
        self.assertIn('d.entry_original[3]', result)
        self.assertIn('s.memory.read_trace', result)
        self.assertIn('match_pc_profile_enabled()', result)
        rows = [r for r in self.records if ENTRY <= r['address'] <= CONTRACTS[ENTRY][0]]
        chosen = {r['address'] for r in rows}
        page = generate_page(ENTRY >> 12, rows, chosen)
        self.assertIn('i_10768660: // mfspr r0,LR\nif (try_native_sort8(s)) return true;', page)
        self.assertNotIn('s.pc==0x10768660', page)

    def test_changed_lowering_and_unexpected_closure_fail(self):
        def changed(*args):
            return self.render(*args).replace('s.step(0x10781320);', '')
        with self.assertRaises(ValueError):
            generate_sort8(self.records, changed)
        with self.assertRaises(ValueError):
            generate_sort8(self.records + [dict(self.records[0], address=0)], self.render)

    def test_empty_bounded_build_is_a_declining_stub(self):
        self.assertIn('bool try_native_sort8(State&){return false;}', generate_sort8([], self.render))


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    database = Path(sys.argv.pop()).resolve().as_uri() + '?mode=ro'
    with sqlite3.connect(database, uri=True) as connection:
        NativeSortAudit.records = load_sort8(connection)
        assert load_sort8(connection, {0}) == []
        try:
            load_sort8(connection, {ENTRY})
        except ValueError:
            pass
        else:
            raise AssertionError('Partial selected closure was accepted')
    unittest.main()
