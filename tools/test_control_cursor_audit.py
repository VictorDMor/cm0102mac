"""Mutation checks against the original exported Mac control-cursor block.

Usage: python3 tools/test_control_cursor_audit.py <instructions.sqlite>
The original input stays local; no game-derived fixture is copied into Git.
"""
from copy import deepcopy
import json
from pathlib import Path
import sqlite3
import sys
import unittest

from compile_recomp import generate_page, require_control_cursor_block


class ControlCursorAuditTests(unittest.TestCase):
    records = []

    @classmethod
    def setUpClass(cls):
        if not cls.records:
            raise unittest.SkipTest(
                "Original Mac fixture not loaded; run: python3 "
                "tools/test_control_cursor_audit.py <instructions.sqlite>")

    def assert_rejected(self, records, selected=None):
        if selected is None:
            selected = {item["address"] for item in records}
        with self.assertRaises(ValueError):
            require_control_cursor_block(records, selected)

    def test_original_export_and_lowering(self):
        selected = {item["address"] for item in self.records}
        require_control_cursor_block(self.records, selected)
        generated = generate_page(0x1071a, self.records, selected)
        self.assertIn("next_control_cursor_entry(s, counter);", generated)
        self.assertIn("goto i_1071a864;", generated)

    def test_every_instruction_and_continuation_required_once(self):
        for index, original in enumerate(self.records):
            with self.subTest(address=hex(original["address"])):
                self.assert_rejected(self.records[:index] + self.records[index + 1:])
                self.assert_rejected(self.records + [original])
                selected = {item["address"] for item in self.records} - {original["address"]}
                self.assert_rejected(self.records, selected)

    def test_every_assembly_size_and_flow_annotation_pinned(self):
        for index, original in enumerate(self.records[:-1]):
            for key, value in (("assembly", "nop"), ("size", 8),
                               ("flow", "COMPUTED_JUMP"), ("flows", [0x1071a868]),
                               ("fallthrough", 0x1071a868)):
                records = deepcopy(self.records)
                records[index][key] = value
                with self.subTest(address=hex(original["address"]), field=key):
                    self.assert_rejected(records)
            for key in ("size", "assembly", "flow", "flows", "ops"):
                records = deepcopy(self.records)
                del records[index][key]
                with self.subTest(address=hex(original["address"]), missing=key):
                    self.assert_rejected(records)
            if "fallthrough" in original:
                records = deepcopy(self.records)
                del records[index]["fallthrough"]
                self.assert_rejected(records)

    def test_every_pcode_operation_and_node_pinned(self):
        for index, original in enumerate(self.records[:-1]):
            for op_index, operation in enumerate(original["ops"]):
                records = deepcopy(self.records)
                records[index]["ops"][op_index][0] = "CALLOTHER"
                with self.subTest(address=hex(original["address"]), op=op_index):
                    self.assert_rejected(records)
                records = deepcopy(self.records)
                del records[index]["ops"][op_index]
                self.assert_rejected(records)
                for node_index, node in enumerate(operation[1:], start=1):
                    replacements = [None] if node is not None else [["register", "0", 4]]
                    if node is not None:
                        replacements += [["ram", node[1], node[2]],
                                         [node[0], str(int(node[1]) + 1), node[2]],
                                         [node[0], node[1], node[2] + 1]]
                    for changed in replacements:
                        if changed == node:
                            continue
                        records = deepcopy(self.records)
                        records[index]["ops"][op_index][node_index] = changed
                        self.assert_rejected(records)

    def test_generation_rejects_changed_pcode_despite_same_assembly(self):
        records = deepcopy(self.records)
        records[0]["ops"][0][2][1] = "116"
        with self.assertRaises(ValueError):
            generate_page(0x1071a, records, {item["address"] for item in records})


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    database = Path(sys.argv.pop()).resolve().as_uri() + "?mode=ro"
    with sqlite3.connect(database, uri=True) as connection:
        ControlCursorAuditTests.records = [json.loads(row[0]) for row in connection.execute(
            "SELECT body FROM instructions WHERE address >= ? AND address <= ? ORDER BY address",
            (0x1071a828, 0x1071a864))]
    if len(ControlCursorAuditTests.records) != 16:
        raise SystemExit("Original control-cursor instruction fixture is incomplete")
    unittest.main()
