import json
import sqlite3
import unittest
from pathlib import Path
from calendar_audit import require_calendar_contract
from compile_recomp import generate_page

INDEX = Path(__file__).resolve().parent.parent / ".local/recomp-input/instructions.sqlite"


@unittest.skipUnless(INDEX.exists(), "Private original calendar export is unavailable")
class CalendarAuditTests(unittest.TestCase):
    def records(self, page):
        with sqlite3.connect(f"file:{INDEX}?mode=ro", uri=True) as db:
            return [json.loads(row[0]) for row in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address",
                (page << 12, (page + 1) << 12))]

    def test_contract_and_same_page_entry_hooks(self):
        for entry in (0x10717890, 0x10717B20, 0x10718130):
            records = self.records(entry >> 12)
            require_calendar_contract(entry >> 12, records)
            code = generate_page(entry >> 12, records, {r["address"] for r in records})
            self.assertIn(f"case 0x{entry:x}: goto i_{entry:x};", code)
            self.assertIn(f"if (try_modern_calendar(s, 0x{entry:x})) return true;", code)
            self.assertIn(f"goto i_{entry:x};", code)

    def test_reject_changed_body_and_helper(self):
        for page, address in ((0x10717, 0x10717960), (0x10718, 0x10718480), (0x10718, 0x10718210)):
            records = self.records(page)
            require_calendar_contract(page, records)
            next(r for r in records if r["address"] == address)["ops"] = []
            with self.assertRaisesRegex(ValueError, "calendar contract changed"):
                require_calendar_contract(page, records)

    def test_reject_incomplete_entry(self):
        for entry in (0x10717890, 0x10717B20, 0x10718130, 0x10718440):
            records = [r for r in self.records(entry >> 12) if r["address"] == entry]
            with self.assertRaises(ValueError):
                require_calendar_contract(entry >> 12, records)

    def test_reject_missing_difference_entry(self):
        records = [r for r in self.records(0x10718) if r["address"] != 0x10718130]
        with self.assertRaises(ValueError):
            require_calendar_contract(0x10718, records)

    def test_same_page_difference_call_routes_to_hook(self):
        # No current direct difference caller shares its page. Exercise the
        # generator's call emission and its audited destination label.
        records = self.records(0x10718)
        code = generate_page(0x10718, records, {r["address"] for r in records})
        from compile_recomp import operation
        self.assertEqual(operation(["CALL", None, ["ram", str(0x10718130), 4]],
                                   0x10718000, 0, 0x10718, {0x10718130}, 1), "goto i_10718130;")
        self.assertIn("i_10718130:", code)
        self.assertIn("if (try_modern_calendar(s, 0x10718130)) return true;", code)


if __name__ == "__main__":
    unittest.main()
