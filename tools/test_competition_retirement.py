"""Pin the deliberate historical policy outside original-equivalence owners."""
import inspect
import json
from pathlib import Path
import sqlite3
import unittest

from compile_recomp import generate_page

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"


@unittest.skipUnless((EXPORT / "instructions.sqlite").exists(), "Private export unavailable")
class CompetitionRetirementTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with sqlite3.connect((EXPORT / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True) as db:
            cls.pages = {page: [json.loads(row[0]) for row in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address",
                (page << 12, (page + 1) << 12))]
                for page in (0x1034f, 0x1034a, 0x10482, 0x10131, 0x1048a, 0x1049d)}
            cls.records = cls.pages[0x1034f]
        cls.userops = json.loads((EXPORT / "program.json").read_text())["userops"]
        cls.disabled = {name: False for name in inspect.signature(generate_page).parameters
                        if name.endswith("_hooks") or name == "guest_context_transfers"}

    def generate(self, records, retirement=True):
        hooks = dict(self.disabled, competition_retirement_hooks=retirement)
        return generate_page(records[0]["address"] >> 12, records, {r["address"] for r in records},
                             userops=self.userops, **hooks)

    def test_both_factory_decisions_use_policy_and_original_can_disable_it(self):
        production = self.generate(self.records)
        original = self.generate(self.records, retirement=False)
        self.assertEqual(production.count("inter_amer_cup_creation_id(s,"), 2)
        self.assertNotIn("inter_amer_cup_creation_id", original)
        self.assertIn('#include "inter_amer_cup_retirement.hpp"', production)

    def test_each_audited_factory_contract_rejects_changed_original(self):
        for address in (0x1034f1f8, 0x1034f200, 0x1034f214,
                        0x1034f318, 0x1034f320, 0x1034f34c, 0x1034f354):
            records = [dict(record, assembly="nop") if record["address"] == address else record
                       for record in self.records]
            with self.subTest(address=hex(address)), self.assertRaisesRegex(ValueError, "factory no longer matches"):
                self.generate(records)


    def test_update_reader_and_new_career_hooks_are_optional_and_targeted(self):
        for page, expression in ((0x1034a, "inter_amer_cup_initialize_new_career(s)"),
                                 (0x10482, "inter_amer_cup_skip_daily(s)"),
                                 (0x10131, "inter_amer_cup_retired_object(s,"),
                                 (0x1048a, "inter_amer_cup_cancel_pending(s,"),
                                 (0x1049d, "inter_amer_cup_skip_host_announcement(s,")):
            with self.subTest(page=hex(page)):
                self.assertIn(expression, self.generate(self.pages[page]))
                self.assertNotIn(expression, self.generate(self.pages[page], retirement=False))
        self.assertIn("goto i_101313a4;", self.generate(self.pages[0x10131]))
        self.assertIn("goto i_1048acd8;", self.generate(self.pages[0x1048a]))

    def test_update_reader_and_call_contracts_reject_changed_original(self):
        for page, address in ((0x1034a, 0x1034a8e4), (0x10482, 0x10482ca0),
                              (0x10131, 0x101312b4), (0x10131, 0x101313a4),
                              (0x1048a, 0x1048aa94), (0x1048a, 0x1048acd8),
                              (0x1049d, 0x1049dfa8), (0x1049d, 0x1049dfa4)):
            records = [dict(record, assembly="nop") if record["address"] == address else record
                       for record in self.pages[page]]
            with self.subTest(address=hex(address)), self.assertRaisesRegex(ValueError, "no longer matches"):
                self.generate(records)


    def test_host_skip_resumes_at_original_next_item(self):
        self.assertIn("s.pc = 0x1049e1cc; return true;", self.generate(self.pages[0x1049d]))
        with sqlite3.connect((EXPORT / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True) as db:
            for address, assembly in ((0x1049e1cc, "or r3,r25,r25"),
                                      (0x1049e1d0, "bl 0x10738e00"),
                                      (0x1049e1d8, "bne 0x1049dfa8")):
                record = json.loads(db.execute("SELECT body FROM instructions WHERE address=?", (address,)).fetchone()[0])
                self.assertEqual(record["assembly"], assembly)


if __name__ == "__main__":
    unittest.main()
