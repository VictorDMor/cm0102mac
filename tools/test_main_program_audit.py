"""The registers the C++ MAIN.CPP routines do not reproduce are never read."""
from contextlib import closing
import os
from pathlib import Path
import sqlite3
import unittest
from main_program_audit import RESULT, SCRATCH, read_before_write, volatile_reads

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get("CM_MAIN_PROGRAM_INPUT", ROOT / ".local/current/export"))


def record(address, assembly, ops, flow="FALL_THROUGH", flows=(), fallthrough=None):
    return {"address": address, "assembly": assembly, "ops": ops, "flow": flow, "flows": list(flows),
            "fallthrough": address + 4 if fallthrough is None else fallthrough}


def register(number):
    return ["register", str(number * 4), 4]


class ScanTests(unittest.TestCase):
    def test_a_read_before_a_write_is_found(self):
        records = {0x100: record(0x100, "or r4,r0,r0", [["COPY", register(4), register(0)]]),
                   0x104: record(0x104, "blr", [], flow="TERMINATOR")}
        self.assertEqual(read_before_write(records, 0x100, SCRATCH["r0"]), 0x100)

    def test_a_write_first_hides_the_register(self):
        records = {0x100: record(0x100, "li r0,0x1", [["COPY", register(0), ["const", "1", 4]]]),
                   0x104: record(0x104, "or r4,r0,r0", [["COPY", register(4), register(0)]])}
        self.assertIsNone(read_before_write(records, 0x100, SCRATCH["r0"]))

    def test_a_branch_target_is_followed(self):
        records = {0x100: record(0x100, "beq 0x200", [["CBRANCH", None]], flow="CONDITIONAL_JUMP", flows=[0x200]),
                   0x104: record(0x104, "blr", [], flow="TERMINATOR"),
                   0x200: record(0x200, "mtspr CTR,r12", [["COPY", ["register", "4132", 4], register(12)]])}
        self.assertEqual(read_before_write(records, 0x100, SCRATCH["r12"]), 0x200)

    def test_r3_passed_to_a_callee_is_read(self):
        records = {0x100: record(0x100, "bl 0x200", [["CALL", None]], flow="UNCONDITIONAL_CALL", flows=[0x200]),
                   0x200: record(0x200, "stw r3,0x0(r1)", [["STORE", None, register(1), register(3)]])}
        self.assertEqual(read_before_write(records, 0x100, RESULT[1]), 0x200)


@unittest.skipUnless((INPUT / "instructions.sqlite").exists(), "Private export unavailable")
class ProgramTests(unittest.TestCase):
    def test_no_code_around_main_cpp_reads_what_the_cpp_leaves_different(self):
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            self.assertEqual(volatile_reads(db), [])


if __name__ == "__main__":
    unittest.main()
