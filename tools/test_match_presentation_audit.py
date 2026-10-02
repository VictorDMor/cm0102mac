"""Mutation checks for the original Mac presentation-duration contract.

Usage: python3 tools/test_match_presentation_audit.py <instructions.sqlite>
The original export remains private and is never copied into a test fixture.
"""
from copy import deepcopy
import json
from pathlib import Path
import sqlite3
import sys
import unittest

from match_presentation_audit import require_match_presentation_context
from compile_recomp import generate_page


class MatchPresentationAuditTests(unittest.TestCase):
    pages = {}

    @classmethod
    def setUpClass(cls):
        if not cls.pages:
            raise unittest.SkipTest(
                "Original Mac fixture not loaded; run: python3 "
                "tools/test_match_presentation_audit.py <instructions.sqlite>")

    def rejected(self, page, records, selected=None):
        if selected is None:
            selected = {item["address"] for item in records}
        with self.assertRaises(ValueError):
            require_match_presentation_context(page, records, selected)

    def test_original_export(self):
        for page, records in self.pages.items():
            require_match_presentation_context(page, records,
                                               {item["address"] for item in records})

    def test_partial_export_without_relevant_call_or_load(self):
        for page, anchor in ((0x101cc, 0x101cce38), (0x10203, 0x10203738)):
            records = [r for r in self.pages[page] if r["address"] != anchor]
            generated = generate_page(page, records, {r["address"] for r in records})
            self.assertNotIn("commentary_presentation_duration(s", generated)
            for item in records:
                self.assertIn(f"i_{item['address']:x}: // {item['assembly']}", generated)

    def test_selected_call_or_load_requires_complete_audited_context(self):
        for page, anchor in ((0x101cc, 0x101cce38), (0x10203, 0x10203738)):
            originals = self.pages[page]
            for missing in range(len(originals)):
                records = originals[:missing] + originals[missing + 1:]
                selected = {r["address"] for r in records} | {anchor}
                with self.subTest(page=hex(page), missing=hex(originals[missing]["address"])):
                    with self.assertRaises(ValueError):
                        generate_page(page, records, selected)

    def test_complete_callee_keeps_duration_lowering(self):
        records = self.pages[0x10203]
        selected = {r["address"] for r in records}
        generated = generate_page(0x10203, records, selected)
        self.assertIn("commentary_presentation_duration(s", generated)
        changed = deepcopy(records)
        changed[-1]["assembly"] = "nop"
        with self.assertRaises(ValueError):
            generate_page(0x10203, changed, selected)

    def test_all_instructions_required_once_and_selected(self):
        for page, records in self.pages.items():
            for index, item in enumerate(records):
                with self.subTest(address=hex(item["address"])):
                    self.rejected(page, records[:index] + records[index + 1:])
                    self.rejected(page, records + [item])
                    self.rejected(page, records,
                                  {r["address"] for r in records} - {item["address"]})

    def test_assembly_flow_and_pcode_pinned(self):
        for page, originals in self.pages.items():
            for index, item in enumerate(originals):
                for field, value in (("assembly", "nop"), ("size", 8),
                                     ("flow", "COMPUTED_JUMP"),
                                     ("fallthrough", 0), ("flows", [0]), ("ops", [])):
                    records = deepcopy(originals)
                    records[index][field] = value
                    if item.get(field) != value:
                        with self.subTest(address=hex(item["address"]), field=field):
                            self.rejected(page, records)
                # Same assembly with changed P-code must also fail closed.
                records = deepcopy(originals)
                records[index]["ops"][0][0] = "CALLOTHER"
                self.rejected(page, records)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    database = Path(sys.argv.pop()).resolve().as_uri() + "?mode=ro"
    with sqlite3.connect(database, uri=True) as connection:
        for page, first, last in ((0x101cc, 0x101cce28, 0x101cce70),
                                 (0x10203, 0x102036f0, 0x102037f4)):
            MatchPresentationAuditTests.pages[page] = [json.loads(row[0]) for row in
                connection.execute("SELECT body FROM instructions WHERE address BETWEEN ? AND ? ORDER BY address",
                                   (first, last))]
    unittest.main()
