import json
import sqlite3
import unittest
from pathlib import Path
from calendar_audit import require_calendar_contract, DATE_SUBSYSTEM_ENTRIES, CONTRACTS
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
        for entry in (0x103317F0, 0x10331860, 0x10715B20, 0x10717890, 0x10717B20, 0x10718130, 0x107182F0, 0x10718360):
            records = self.records(entry >> 12)
            require_calendar_contract(entry >> 12, records)
            code = generate_page(entry >> 12, records, {r["address"] for r in records})
            self.assertIn(f"case 0x{entry:x}: goto i_{entry:x};", code)
            self.assertIn(f"if (try_modern_calendar(s, 0x{entry:x})) return true;", code)
            self.assertIn(f"goto i_{entry:x};", code)

    def test_reject_changed_body_and_helper(self):
        for page, address in ((0x10331, 0x10331840), (0x10331, 0x103318b0), (0x10717, 0x10717960), (0x10718, 0x10718480), (0x10718, 0x10718210), (0x10718, 0x1071842c), (0x10718, 0x10718330)):
            records = self.records(page)
            require_calendar_contract(page, records)
            next(r for r in records if r["address"] == address)["ops"] = []
            with self.assertRaisesRegex(ValueError, "calendar contract changed"):
                require_calendar_contract(page, records)

    def test_reject_incomplete_entry(self):
        for entry in (0x103317F0, 0x10331860, 0x10715B20, 0x10717890, 0x10717B20, 0x10718130, 0x107182F0, 0x10718360, 0x10718440):
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

    def test_same_page_month_call_and_missing_entry(self):
        records = self.records(0x10718)
        from compile_recomp import operation
        self.assertEqual(operation(["CALL", None, ["ram", str(0x10718360), 4]],
                                   0x10718000, 0, 0x10718, {0x10718360}, 1), "goto i_10718360;")
        with self.assertRaises(ValueError):
            require_calendar_contract(0x10718, [r for r in records if r["address"] != 0x10718360])

    def test_same_page_day_of_month_call_and_fragments(self):
        records = self.records(0x10718)
        from compile_recomp import operation
        self.assertEqual(operation(["CALL", None, ["ram", str(0x107182F0), 4]],
                                   0x10718000, 0, 0x10718, {0x107182F0}, 1), "goto i_107182f0;")
        for missing in (0x107182F0, 0x1071832C, 0x10718338, 0x10718348, 0x10718350):
            with self.assertRaises(ValueError):
                require_calendar_contract(0x10718, [r for r in records if r["address"] != missing])

    def test_weekday_all_exits_and_same_page_caller(self):
        records = self.records(0x10715)
        self.assertEqual(next(r for r in records if r["address"] == 0x10715A54)["assembly"], "bl 0x10715b20")
        code = generate_page(0x10715, records, {r["address"] for r in records})
        self.assertIn("goto i_10715b20;", code.split("i_10715a54:")[1].split("i_10715a58:")[0])
        self.assertIn("if (try_modern_calendar(s, 0x10715b20)) return true;", code)
        for missing in (0x10715B20, 0x10715B5C, 0x10715B74, 0x10715BA0, 0x10715BAC, 0x10715BB4, 0x10715BBC):
            with self.assertRaises(ValueError):
                require_calendar_contract(0x10715, [r for r in records if r["address"] != missing])
        for address in (0x10715BAC, 0x10715BB4, 0x10715BBC):
            altered = self.records(0x10715)
            next(r for r in altered if r["address"] == address)["ops"] = []
            with self.assertRaises(ValueError):
                require_calendar_contract(0x10715, altered)

    def test_real_same_page_ordering_call_routes_to_hook(self):
        records = self.records(0x10331)
        call = next(r for r in records if r["address"] == 0x10331794)
        self.assertEqual(call["assembly"], "bl 0x10331860")
        code = generate_page(0x10331, records, {r["address"] for r in records})
        call_code = code.split("i_10331794:")[1].split("i_10331798:")[0]
        self.assertIn("goto i_10331860;", call_code)
        self.assertIn("if (try_modern_calendar(s, 0x10331860)) return true;", code)
        with self.assertRaises(ValueError):
            require_calendar_contract(0x10331, [r for r in records if r["address"] != 0x10331860])

    def test_real_same_page_before_call_routes_to_hook(self):
        records = self.records(0x10331)
        self.assertEqual(next(r for r in records if r["address"] == 0x103317b8)["assembly"], "bl 0x103317f0")
        code = generate_page(0x10331, records, {r["address"] for r in records})
        self.assertIn("goto i_103317f0;", code.split("i_103317b8:")[1].split("i_103317bc:")[0])
        self.assertIn("if (try_modern_calendar(s, 0x103317f0)) return true;", code)
        with self.assertRaises(ValueError):
            require_calendar_contract(0x10331, [r for r in records if r["address"] != 0x103317f0])

    def test_all_pending_destinations_and_body_fragments(self):
        self.assertEqual(len(DATE_SUBSYSTEM_ENTRIES), 26)
        self.assertEqual(len({start for start, _, _ in CONTRACTS}), 34)
        for page in (0x10715, 0x10716, 0x10717, 0x10718):
            records = self.records(page)
            require_calendar_contract(page, records)
            code = generate_page(page, records, {r["address"] for r in records})
            for entry in DATE_SUBSYSTEM_ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assertIn(f"case 0x{entry:x}: goto i_{entry:x};", code)
                self.assertIn(f"if (try_modern_date_subsystem(s, 0x{entry:x})) return true;", code)
            for start, end, _ in CONTRACTS:
                if not start >> 12 <= page <= (end - 1) >> 12:
                    continue
                body = [r for r in records if start <= r["address"] < end]
                for victim in (body[0]["address"], body[-1]["address"]):
                    with self.assertRaises(ValueError):
                        require_calendar_contract(page, [r for r in records if r["address"] != victim])

    def test_cross_page_formatter_contracts(self):
        for page, address in ((0x10715, 0x10715ffc), (0x10716, 0x10716000),
                              (0x10716, 0x10716ffc), (0x10717, 0x10717000)):
            records = self.records(page)
            next(r for r in records if r["address"] == address)["ops"] = []
            with self.assertRaisesRegex(ValueError, "calendar contract changed"):
                require_calendar_contract(page, records)


if __name__ == "__main__":
    unittest.main()
