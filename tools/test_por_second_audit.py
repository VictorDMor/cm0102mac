import unittest

import por_second_audit as league


class PortugueseSecondAudit(unittest.TestCase):
    def test_entries_and_virtual_roots(self):
        self.assertEqual(len(league.ENTRIES), 6)
        self.assertEqual(league.DIRECT_ROOTS | set(league.VECTORS.values()), set(league.ENTRIES))
        for entry, end, _ in league.ROUTINES:
            self.assertEqual(league.owner(entry), entry)
            self.assertEqual(league.owner(end - 4), entry)
            self.assertLess(entry, end)
        self.assertIsNone(league.owner(league.MODULE[1]))

    def test_closed_production_registry(self):
        self.assertEqual(league.POR_SECOND_ENTRIES, league.ENTRIES)

    def test_reject_registry_and_external_interior(self):
        for entry in (league.MODULE[0] + 4, league.MODULE[1]):
            with self.assertRaisesRegex(ValueError, "production registry"):
                league.production_entries(f"POR_SECOND_ROUTINE({entry:#x}, bad)")
        with self.assertRaisesRegex(ValueError, "production registry"):
            league.production_entries("POR_SECOND_ROUTINE(0x10578680, duplicate)\n" * 2)
        with self.assertRaisesRegex(ValueError, "Cross-entry"):
            league.validate_flows({}, [(0x105773b0, 0x105773c4, "UNCONDITIONAL_JUMP")])

    def test_original_inventory(self):
        self.assertEqual(league.validate_export(league.EXPORT), 1531)


if __name__ == "__main__":
    unittest.main()
