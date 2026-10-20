"""Audit the native record-field specialization using the private export."""
from copy import deepcopy
from pathlib import Path
import sqlite3
import sys
import unittest

from native_record_fields import CONTRACTS, load_leaves, require_leaf, generate_native_record_fields


class NativeRecordFieldAudit(unittest.TestCase):
    leaves = {}

    @classmethod
    def setUpClass(cls):
        if not cls.leaves:
            raise unittest.SkipTest("Pass the private instructions.sqlite")

    def test_complete_original_contracts(self):
        for first, records in self.leaves.items():
            require_leaf(first, records, {row["address"] for row in records})

    def test_missing_duplicate_and_unselected_instructions(self):
        for first, records in self.leaves.items():
            selected = {row["address"] for row in records}
            for index in range(len(records)):
                for changed, chosen in ((records[:index] + records[index + 1:], selected),
                                        (records + [records[index]], selected),
                                        (records, selected - {records[index]["address"]})):
                    with self.assertRaises(ValueError):
                        require_leaf(first, changed, chosen)

    def test_changed_contract_fields(self):
        for first, records in self.leaves.items():
            for index in (0, len(records) // 2, len(records) - 1):
                for field, value in (("size", 8), ("assembly", "nop"), ("flow", "COMPUTED_JUMP"),
                                     ("fallthrough", 0), ("flows", [0]), ("ops", [])):
                    if records[index].get(field) == value:
                        continue
                    changed = deepcopy(records)
                    changed[index][field] = value
                    with self.assertRaises(ValueError):
                        require_leaf(first, changed, {row["address"] for row in records})

    def test_bounded_render_rejects_unhandled_state_effect(self):
        def changed_render(*_):
            return "s.platform.reset();"
        with self.assertRaises(ValueError):
            generate_native_record_fields(self.leaves, changed_render)

    def test_full_lowering_and_entry_hooks(self):
        from compile_recomp import generate_page
        result = generate_native_record_fields(self.leaves, lambda page, records, selected:
            generate_page(page, records, selected, native_record_field_hooks=False, native_leaf_lowering=True))
        self.assertIn("s.budget!=UINT64_MAX", result)
        self.assertIn("s.memory.date_observer", result)
        for first, records in self.leaves.items():
            self.assertIn(f"case 0x{first:x}:return native_record_fields_{first:x}(s)", result)
            self.assertIn(f"frame.step(0x{records[-1]['address']:x})", result)
            self.assertIn(f"pc(0x{first:x})", result)
        # The cross-page first leaf must remain in one native function.
        self.assertIn("goto i_100c6000;", result)
        self.assertNotIn("try_native_record_fields(s,", result)
        for first, records in self.leaves.items():
            page = generate_page(first >> 12, records, {row["address"] for row in records})
            self.assertIn('#include "native_record_fields.hpp"', page)
            self.assertIn(f"i_{first:x}: //", page)
            self.assertIn(f"if (try_native_record_fields(s, 0x{first:x})) return true;", page)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    database = Path(sys.argv.pop()).resolve().as_uri() + "?mode=ro"
    with sqlite3.connect(database, uri=True) as connection:
        NativeRecordFieldAudit.leaves = load_leaves(connection)
    unittest.main()
