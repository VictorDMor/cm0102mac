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
        records = self.records(0x10717)
        require_calendar_contract(0x10717, records)
        code = generate_page(0x10717, records, {r["address"] for r in records})
        for entry in (0x10717890, 0x10717B20):
            self.assertIn(f"case 0x{entry:x}: goto i_{entry:x};", code)
            self.assertIn(f"if (try_modern_calendar(s, 0x{entry:x})) return true;", code)
            self.assertIn(f"goto i_{entry:x};", code)

    def test_reject_changed_body_and_helper(self):
        for page, address in ((0x10717, 0x10717960), (0x10718, 0x10718480)):
            records = self.records(page)
            require_calendar_contract(page, records)
            next(r for r in records if r["address"] == address)["ops"] = []
            with self.assertRaisesRegex(ValueError, "calendar contract changed"):
                require_calendar_contract(page, records)

    def test_reject_incomplete_entry(self):
        records = [r for r in self.records(0x10717) if r["address"] == 0x10717890]
        with self.assertRaises(ValueError):
            require_calendar_contract(0x10717, records)


if __name__ == "__main__":
    unittest.main()
