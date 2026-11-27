"""Croatian Cup source ownership, original entry contracts and indirect resumes."""
from contextlib import closing
import json
import sqlite3
import unittest

import croatian_cup_audit as cup


class CroatianCupAudit(unittest.TestCase):
    def test_all_six_roots_are_accounted_for(self):
        entries = cup.CROATIAN_CUP_ENTRIES
        self.assertEqual(len(entries), 6)
        self.assertEqual(entries, tuple(sorted(set(entries))))
        roots = {target for _, target in cup.DIRECT_CALLS} | {target for _, _, target in cup.SLOTS}
        self.assertEqual(set(entries), roots)
        for (start, end, _), next_start in zip(cup.ROUTINES, (*entries[1:], cup.MODULE[1])):
            self.assertLess(start, end)
            self.assertLessEqual(end, next_start)

    def test_reject_unknown_branch_and_interior_call(self):
        known = [(source, target, "UNCONDITIONAL_CALL") for source, target in cup.DIRECT_CALLS]
        entry = cup.CROATIAN_CUP_ENTRIES[0]
        for flow, message in (("CONDITIONAL_JUMP", "interior branch"), ("UNCONDITIONAL_CALL", "direct callers"),
                              ("CONDITIONAL_CALL", "Unexpected.*call")):
            with self.subTest(flow=flow), self.assertRaisesRegex(ValueError, message):
                cup.validate_edges({}, [*known, (0x10350ddc, entry + 4, flow)])
        with self.assertRaisesRegex(ValueError, "direct callers"):
            cup.validate_edges({}, known[1:])

    def test_reject_original_body_in_production(self):
        code = "\n".join(
            f"i_{entry:x}: // entry\ns.step(0x{entry:x});\nreturn croatia_cup::dispatch(s, 0x{entry:x});"
            for entry in cup.CROATIAN_CUP_ENTRIES)
        cup.validate_production_page(code)
        with self.assertRaisesRegex(ValueError, "six Croatian Cup ABI entries"):
            cup.validate_production_page(code + "\ni_106bf0f4: // original body")
        with self.assertRaisesRegex(ValueError, "original instructions"):
            cup.validate_production_page(code.replace("return croatia_cup::dispatch", "if (croatia_cup::dispatch", 1))

    def test_reject_missing_pinned_instruction(self):
        with closing(sqlite3.connect(":memory:")) as db:
            db.execute("CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)")
            with self.assertRaisesRegex(ValueError, "0x106bf0f0 changed"):
                cup.require_croatian_cup_index(db)

    @unittest.skipUnless((cup.EXPORT / "instructions.sqlite").is_file(), "Private export unavailable")
    def test_original_export(self):
        self.assertEqual(cup.validate_export(cup.EXPORT), 860)

    @unittest.skipUnless((cup.EXPORT / "program.json").is_file(), "Private export unavailable")
    def test_reject_secondary_descriptor_unaligned_pointer_and_changed_vtable(self):
        program = json.loads((cup.EXPORT / "program.json").read_text())
        segments = [(s["address"], (cup.EXPORT / s["file"]).read_bytes()) for s in program["segments"]
                    if s.get("file")]
        for target in (cup.CROATIAN_CUP_ENTRIES[0], cup.CROATIAN_CUP_ENTRIES[0] + 4):
            with self.subTest(target=target), self.assertRaisesRegex(ValueError, "unclassified entry pointer"):
                cup.validate_image(segments + [(0x12000000, b"\x00" + target.to_bytes(4, "big"))])
        # Keep every vector intact, but install the destructor in the calendar slot.
        table = 0x108df314
        with self.assertRaisesRegex(ValueError, "vtable slots"):
            cup.validate_image([(table + 0x44, cup.SLOTS[0][1].to_bytes(4, "big")), *segments])


if __name__ == "__main__":
    unittest.main()
