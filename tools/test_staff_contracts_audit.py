import unittest

import staff_contracts_audit as staff
from contract_display_audit import CONTRACT_DISPLAY_ENTRIES
from contract_draft_audit import CONTRACT_DRAFT_ENTRIES
from contract_manager_audit import ROUTINES as MANAGER_ROUTINES
from contract_query_audit import CONTRACT_QUERY_ENTRIES
from contract_review_audit import CONTRACT_REVIEW_ENTRIES
from contract_screens_audit import CONTRACTS as SCREEN_ROUTINES
from contract_storage_audit import CONTRACT_STORAGE_ENTRIES
from contract_terms_audit import CONTRACT_TERMS_ENTRIES


class StaffContractsAuditTest(unittest.TestCase):
    def test_bounds_and_legacy_owners(self):
        self.assertEqual(len(staff.ENTRIES), 47)
        self.assertEqual(len(set(staff.ENTRIES)), 47)
        self.assertEqual(staff.ENTRIES[0], staff.MODULE[0])
        for (entry, end, _), next_entry in zip(staff.ROUTINES, (*staff.ENTRIES[1:], staff.MODULE[1])):
            self.assertLess(entry, end)
            self.assertLessEqual(end, next_entry)
            self.assertEqual(staff.owner(entry), entry)
            self.assertEqual(staff.owner(end - 4), entry)

        legacy = (*CONTRACT_DISPLAY_ENTRIES, *CONTRACT_DRAFT_ENTRIES, *CONTRACT_REVIEW_ENTRIES,
                  *CONTRACT_TERMS_ENTRIES, 0x103727c0, 0x1037b6f0, 0x103810d0)
        self.assertEqual(len(set(legacy)), len(legacy))
        self.assertLessEqual(set(legacy), set(staff.ENTRIES))
        neighbors = {*CONTRACT_QUERY_ENTRIES, *CONTRACT_STORAGE_ENTRIES} - {0x103727c0}
        for routines in (MANAGER_ROUTINES, SCREEN_ROUTINES):
            neighbors.update(entry for entry, *_ in routines)
        self.assertTrue(neighbors.isdisjoint(staff.ENTRIES))

    def test_rejects_interior_entries(self):
        entry = staff.ENTRIES[0]
        with self.assertRaisesRegex(ValueError, "Cross-entry branch"):
            staff.validate_ownership({}, [(entry - 4, entry + 4, "UNCONDITIONAL_JUMP")], {})
        with self.assertRaisesRegex(ValueError, "Direct call roots"):
            staff.validate_ownership({}, [(entry - 4, entry + 4, "UNCONDITIONAL_CALL")], {})

    def test_export(self):
        self.assertEqual(staff.validate_export(staff.EXPORT), 15214)


if __name__ == "__main__":
    unittest.main()
