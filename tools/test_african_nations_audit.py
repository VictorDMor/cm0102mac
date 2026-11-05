import unittest

import african_nations_audit as league


class AfricanNationsAudit(unittest.TestCase):
    def test_entries_and_virtual_roots(self):
        self.assertEqual(len(league.ENTRIES), 15)
        self.assertEqual(league.DIRECT_ROOTS | set(league.VECTORS.values()), set(league.ENTRIES))
        for entry, end, _ in league.ROUTINES:
            self.assertEqual(league.owner(entry), entry)
            self.assertEqual(league.owner(end - 4), entry)
            self.assertLess(entry, end)
        self.assertIsNone(league.owner(league.MODULE[1]))

    def test_reject_registry_and_external_interior(self):
        for entry in (league.MODULE[0] + 4, league.MODULE[1]):
            with self.assertRaisesRegex(ValueError, "production registry"):
                league.production_entries(f"AFRICAN_NATIONS_ROUTINE({entry:#x}, bad)")
        with self.assertRaisesRegex(ValueError, "production registry"):
            league.production_entries("AFRICAN_NATIONS_ROUTINE(0x104f5910, duplicate)\n" * 2)
        with self.assertRaisesRegex(ValueError, "Cross-entry"):
            league.validate_flows({}, [(0x104f4c78, 0x104f4c84, "UNCONDITIONAL_JUMP")])

    def test_original_inventory(self):
        self.assertEqual(league.validate_export(league.EXPORT), 2309)


if __name__ == "__main__":
    unittest.main()
