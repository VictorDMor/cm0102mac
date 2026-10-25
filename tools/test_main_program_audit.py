"""The registers the C++ MAIN.CPP routines do not reproduce are never read."""
from contextlib import closing
import json
import os
from pathlib import Path
import re
import shutil
import sqlite3
import subprocess
import unittest
from main_program_audit import RESULT, SCRATCH, read_before_write, volatile_reads

ROOT = Path(__file__).resolve().parent.parent
INPUT = Path(os.environ.get("CM_MAIN_PROGRAM_INPUT", ROOT / ".local/current/export"))
# The installed original (read only), for its resources.
GAME = Path(os.environ.get("CM_MAIN_PROGRAM_GAME", ""))


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


def calls_to(db, target):
    """The call sites of `target` in the whole program."""
    return sorted(address for address, body in db.execute("SELECT address, body FROM instructions")
                  if (record := json.loads(body)).get("flow") == "UNCONDITIONAL_CALL" and record.get("flows", [None])[0] == target)


@unittest.skipUnless((INPUT / "instructions.sqlite").exists(), "Private export unavailable")
class DebugMenuTests(unittest.TestCase):
    """debug_menu (0x10770a10) handles menu 4. The program installs one menu
    bar, MBAR 1, and adds no menu to it, so menu 4 is never in the bar and
    neither MenuSelect nor MenuKey can choose it."""

    def test_the_only_menu_bar_is_mbar_1_and_no_menu_is_added(self):
        symbols = json.loads((INPUT / "program.json").read_text())["symbols"]
        glue = {symbol["address"]: symbol["name"] for symbol in symbols
                if symbol.get("space") == "ram" and symbol["name"].startswith(".glue::")}
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            by_name = {}
            for address, body in db.execute("SELECT address, body FROM instructions"):
                record = json.loads(body)
                if record.get("flow") == "UNCONDITIONAL_CALL" and record.get("flows", [None])[0] in glue:
                    by_name.setdefault(glue[record["flows"][0]].removeprefix(".glue::"), []).append(address)
            self.assertEqual(by_name.get("GetNewMBar"), [0x1076f8a0])
            self.assertEqual(by_name.get("SetMenuBar"), [0x1076f8b0])
            for adds in ("InsertMenu", "MacInsertMenu", "AppendMenu", "NewMenu", "CreateNewMenu", "InsertMenuItem"):
                self.assertNotIn(adds, by_name)
            # r3 = 1 at 0x1076f894 (nothing writes r3 before the call).
            argument = json.loads(db.execute("SELECT body FROM instructions WHERE address = ?", (0x1076f894,)).fetchone()[0])
            self.assertEqual(argument["assembly"], "li r3,0x1")
            # menu 4's routine has one caller: menu_command, reached from
            # MenuSelect (the press) and MenuKey (key_down) only.
            self.assertEqual(calls_to(db, 0x10770a10), [0x10770924])
            self.assertEqual(calls_to(db, 0x107708c0), [0x107700fc, 0x1077040c])

    @unittest.skipUnless((GAME / "CM 01-02").exists() and shutil.which("DeRez"), "Installed game unavailable")
    def test_mbar_1_holds_menus_1_to_3(self):
        text = subprocess.run(["DeRez", "-only", "MBAR", str(GAME / "CM 01-02")], check=True,
                              capture_output=True, text=True).stdout
        found = re.search(r"data 'MBAR' \(1[,)].*?\$\"([0-9A-F ]+)\"", text, re.S)
        self.assertIsNotNone(found)
        words = [int(word, 16) for word in found.group(1).split()]
        self.assertEqual(words, [3, 1, 2, 3])  # count, then menus 1, 2, 3


if __name__ == "__main__":
    unittest.main()
