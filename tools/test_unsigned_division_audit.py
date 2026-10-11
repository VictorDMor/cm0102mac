"""Mutation checks using the local original Mac unsigned division export.

Usage: python3 tools/test_unsigned_division_audit.py <instructions.sqlite>
Original instruction data stays local and is never copied into this repository.
"""
from copy import deepcopy
import json
from pathlib import Path
import sqlite3
import sys
import unittest

from unsigned_division_audit import require_unsigned_division_routine
from compile_recomp import generate_page


class UnsignedDivisionAuditTests(unittest.TestCase):
    records = []

    @classmethod
    def setUpClass(cls):
        if not cls.records:
            raise unittest.SkipTest(
                "Original Mac fixture not loaded; run: python3 "
                "tools/test_unsigned_division_audit.py <instructions.sqlite>")

    def assert_rejected(self, records, selected=None):
        if selected is None:
            selected = {item["address"] for item in records if "address" in item}
        with self.assertRaises(ValueError):
            require_unsigned_division_routine(records, selected)

    def test_original_export(self):
        require_unsigned_division_routine(self.records, {r["address"] for r in self.records})

    def test_partial_export_without_entry_preserves_original_instructions(self):
        records = self.records[1:]
        generated = generate_page(0x10773, records, {r["address"] for r in records})
        self.assertNotIn("try_fast_unsigned_division", generated)
        for item in records:
            self.assertIn(f"i_{item['address']:x}: // {item['assembly']}", generated)

    def test_partial_export_with_entry_requires_complete_context(self):
        for missing in range(len(self.records)):
            records = self.records[:missing] + self.records[missing + 1:]
            # Keep the entry selected even if its record itself is missing.
            selected = {r["address"] for r in records} | {0x10773870}
            with self.subTest(missing=hex(self.records[missing]["address"])):
                with self.assertRaises(ValueError):
                    generate_page(0x10773, records, selected)

    def test_complete_export_keeps_specialization_and_fingerprint(self):
        selected = {r["address"] for r in self.records}
        generated = generate_page(0x10773, self.records, selected)
        self.assertIn("try_modern_compiler_arithmetic(s, 0x10773870)", generated)
        self.assertNotIn("try_fast_unsigned_division(s)", generated)
        legacy = generate_page(0x10773, self.records, selected, compiler_arithmetic_hooks=False)
        self.assertIn("try_fast_unsigned_division(s)", legacy)
        records = deepcopy(self.records)
        records[-1]["assembly"] = "nop"
        with self.assertRaises(ValueError):
            generate_page(0x10773, records, selected)

    def test_every_instruction_required_once_and_selected(self):
        for index, record in enumerate(self.records):
            with self.subTest(address=hex(record["address"])):
                self.assert_rejected(self.records[:index] + self.records[index + 1:])
                self.assert_rejected(self.records + [record])
                self.assert_rejected(self.records,
                                     {r["address"] for r in self.records} - {record["address"]})

    def test_every_record_field_pinned_including_returns(self):
        for index, record in enumerate(self.records):
            for key in record:
                records = deepcopy(self.records)
                del records[index][key]
                with self.subTest(address=hex(record["address"]), missing=key):
                    self.assert_rejected(records)
            for key, replacement in (("assembly", "nop"), ("size", 8),
                                     ("flow", "COMPUTED_JUMP"), ("flows", [0x1077395C]),
                                     ("fallthrough", 0x1077395C)):
                records = deepcopy(self.records)
                records[index][key] = replacement
                if records != self.records:
                    self.assert_rejected(records)

    def test_every_pcode_operation_and_operand_pinned(self):
        for index, record in enumerate(self.records):
            for op_index, operation in enumerate(record["ops"]):
                records = deepcopy(self.records)
                records[index]["ops"][op_index][0] = "CALLOTHER"
                self.assert_rejected(records)
                records = deepcopy(self.records)
                del records[index]["ops"][op_index]
                self.assert_rejected(records)
                for node_index, node in enumerate(operation[1:], start=1):
                    replacements = [["register", "0", 4]] if node is None else [
                        None, ["ram", node[1], node[2]],
                        [node[0], str(int(node[1]) + 1), node[2]],
                        [node[0], node[1], node[2] + 1]]
                    for changed in replacements:
                        if changed == node:
                            continue
                        records = deepcopy(self.records)
                        records[index]["ops"][op_index][node_index] = changed
                        self.assert_rejected(records)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    database = Path(sys.argv.pop()).resolve().as_uri() + "?mode=ro"
    with sqlite3.connect(database, uri=True) as connection:
        UnsignedDivisionAuditTests.records = [json.loads(row[0]) for row in connection.execute(
            "SELECT body FROM instructions WHERE address BETWEEN ? AND ? ORDER BY address",
            (0x10773870, 0x10773958))]
    if len(UnsignedDivisionAuditTests.records) != 59:
        raise SystemExit("Original unsigned division fixture is incomplete")
    unittest.main()
