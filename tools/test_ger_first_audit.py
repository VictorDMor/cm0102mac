import unittest

import ger_first_audit as league


class GermanFirstAudit(unittest.TestCase):
    def test_entries_and_virtual_roots(self):
        self.assertEqual(len(league.ENTRIES), 11)
        self.assertEqual(league.DIRECT_ROOTS | set(league.VECTORS.values()), set(league.ENTRIES))
        for entry, end, _ in league.ROUTINES:
            self.assertEqual(league.owner(entry), entry)
            self.assertEqual(league.owner(end - 4), entry)
            self.assertLess(entry, end)
        self.assertIsNone(league.owner(league.MODULE[1]))

    def test_reject_registry_and_external_interior(self):
        for entry in (league.MODULE[0] + 4, league.MODULE[1]):
            with self.assertRaisesRegex(ValueError, "production registry"):
                league.production_entries(f"GER_FIRST_ROUTINE({entry:#x}, bad)")
        with self.assertRaisesRegex(ValueError, "production registry"):
            league.production_entries("GER_FIRST_ROUTINE(0x10539590, duplicate)\n" * 2)
        with self.assertRaisesRegex(ValueError, "Cross-entry"):
            league.validate_flows({}, [(0x10536d44, 0x10536d54, "UNCONDITIONAL_JUMP")])

    def test_original_inventory(self):
        self.assertEqual(league.validate_export(league.EXPORT), 3916)


if __name__ == "__main__":
    unittest.main()
