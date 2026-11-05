import unittest

import simulated_stats_audit as stats
from compile_recomp import OWNER_REPLACEMENTS
from shortlist_manager_audit import MODULE as SHORTLIST
from staff_contracts_audit import MODULE as CONTRACTS


class SimulatedStatsAuditTest(unittest.TestCase):
    def test_inventory_and_neighbors(self):
        self.assertEqual(stats.MODULE, (SHORTLIST[1], CONTRACTS[0]))
        self.assertEqual(len(stats.ENTRIES), 7)
        for (entry, end, _), next_entry in zip(stats.ROUTINES, (*stats.ENTRIES[1:], stats.MODULE[1])):
            self.assertLess(entry, end)
            self.assertLessEqual(end, next_entry)
            self.assertEqual(stats.owner(entry), entry)
            self.assertEqual(stats.owner(end - 4), entry)

    def test_invalid_registry_and_interior_flows(self):
        for bad in (0x1036ff64, stats.MODULE[1]):
            with self.assertRaisesRegex(ValueError, "production registry"):
                stats.production_entries(f"SIMULATED_STATS_ROUTINE({bad:#x}, bad)")
        with self.assertRaisesRegex(ValueError, "production registry"):
            stats.production_entries("SIMULATED_STATS_ROUTINE(0x10370db0, first)\n" * 2)
        with self.assertRaisesRegex(ValueError, "Cross-entry"):
            stats.validate_ownership({}, [(0x1036ff5c, 0x1036ff64, "UNCONDITIONAL_JUMP")])
        with self.assertRaisesRegex(ValueError, "Direct call roots"):
            stats.validate_ownership({}, [(0x1036ff5c, 0x1036ff64, "UNCONDITIONAL_CALL")])

    def test_only_registered_entries_are_owned(self):
        self.assertEqual(stats.SIMULATED_STATS_ENTRIES, stats.ENTRIES)
        for entry in stats.ENTRIES:
            owners = [name for address, name in OWNER_REPLACEMENTS if address == entry]
            expected = ["simulated_stats_adapter"] if entry in stats.SIMULATED_STATS_ENTRIES else []
            self.assertEqual(owners, expected)

    def test_export(self):
        self.assertEqual(stats.validate_export(stats.EXPORT), 2201)


if __name__ == "__main__":
    unittest.main()
