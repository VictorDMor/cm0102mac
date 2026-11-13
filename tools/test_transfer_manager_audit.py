"""Transfer-manager scope and takeover checks against the private Mac export."""
from contextlib import closing
import json
from pathlib import Path
import sqlite3
import tempfile
import unittest

from compile_recomp import DirectOwnerCalls, OWNER_REPLACEMENTS, generate_page
from tactics_audit import routine_records
from transfer_offer_audit import TRANSFER_OFFER_ENTRIES
from transfer_manager_audit import (
    BOUNDS, ENTRIES, EXPORT, MODULE, TRANSFER_MANAGER_ENTRIES, registered,
    require_transfer_manager_hooks, require_transfer_manager_index, transfer_manager_hook, validate_export,
)


class TransferManagerAudit(unittest.TestCase):
    def test_original_scope_and_indirect_targets(self):
        self.assertEqual((ENTRIES[0], BOUNDS[ENTRIES[-1]]), MODULE)
        self.assertEqual(len(ENTRIES), 214)
        self.assertEqual(len(ENTRIES), len(set(ENTRIES)))
        for first, second in zip(ENTRIES, ENTRIES[1:]):
            self.assertLessEqual(BOUNDS[first], second)
        self.assertEqual(validate_export(EXPORT), 51656)

    def test_sole_new_owners_and_legacy_owners(self):
        for entry in TRANSFER_MANAGER_ENTRIES:
            self.assertEqual([name for address, name in OWNER_REPLACEMENTS if address == entry],
                             ["transfer_manager_adapter"])
        for entry in (0x10411e60, 0x10411ea0, 0x10411f10):
            self.assertNotIn(entry, ENTRIES)
            self.assertIn((entry, "transfer_offer_unit_adapter"), OWNER_REPLACEMENTS)
        self.assertTrue(set(TRANSFER_OFFER_ENTRIES) <= set(TRANSFER_MANAGER_ENTRIES))
        self.assertNotIn("transfer_offer_adapter", {name for _, name in OWNER_REPLACEMENTS})

    def test_duplicate_unknown_and_changed_original_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            registry = Path(directory) / "routines.inc"
            registry.write_text("TRANSFER_MANAGER_ROUTINE(0x103ea250, hook)\n" * 2)
            with self.assertRaisesRegex(ValueError, "duplicate"):
                registered(registry)
            registry.write_text("TRANSFER_MANAGER_ROUTINE(0x10411f10, hook)\n")
            with self.assertRaisesRegex(ValueError, "Invalid"):
                registered(registry)
        with closing(sqlite3.connect(":memory:")) as db:
            db.execute("CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)")
            with self.assertRaisesRegex(ValueError, "original differs"):
                require_transfer_manager_index(db)

    def test_normal_hook_and_original_bypass(self):
        page = 0x103ea
        program = json.loads((EXPORT / "program.json").read_text())
        with closing(sqlite3.connect((EXPORT / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)) as db:
            records = routine_records(db, page << 12, (page + 1) << 12)
        selected = {r["address"] for r in records}
        normal = generate_page(page, records, selected, userops=program["userops"], block_accounting=True)
        legacy = generate_page(page, records, selected, userops=program["userops"], transfer_manager_hooks=False)
        for entry in (0x103ea250, 0x103ea270, 0x103ea290):
            hook = transfer_manager_hook(entry)
            self.assertIn(hook, normal)
            self.assertNotIn(hook, legacy)
            self.assertIn(f"try_modern_transfer_offer(s, 0x{entry:x})", legacy)
            with self.assertRaisesRegex(ValueError, "Missing or misplaced"):
                require_transfer_manager_hooks(page, records, normal.replace(hook, ""))

    def test_direct_and_same_page_calls_keep_the_entry_route(self):
        callee = 0x103ea250
        for site in (0x10001000, 0x103ea000):
            ret = site + 4
            record = {"address": site, "size": 4, "assembly": f"bl 0x{callee:x}",
                      "flow": "UNCONDITIONAL_CALL", "flows": [callee], "fallthrough": ret,
                      "ops": [["COPY", ["register", "2440", 4], ["register", "8", 4]],
                              ["COPY", ["register", "4128", 4], ["const", str(ret), 4]],
                              ["CALL", None, ["ram", str(callee), 4]]]}
            tail = {"address": ret, "size": 4, "assembly": "blr", "flow": "TERMINATOR", "flows": [],
                    "ops": [["RETURN", None, ["register", "4128", 4]]]}
            records = [record, tail]
            if site >> 12 == callee >> 12:
                records.append(dict(tail, address=callee))
            code = generate_page(site >> 12, records, {r["address"] for r in records},
                                 direct_owner_calls=DirectOwnerCalls(), transfer_offer_hooks=False)
            self.assertIn(f"try_modern_transfer_manager(s, 0x{callee:x}) && continues_after_owner", code)


if __name__ == "__main__":
    unittest.main()
