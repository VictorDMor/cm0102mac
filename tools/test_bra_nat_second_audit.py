import unittest

import bra_nat_second_audit as league


class BrazilianSecondAudit(unittest.TestCase):
    def test_inventory(self):
        self.assertEqual(len(league.ENTRIES), 13)
        self.assertEqual(league.BRA_NAT_SECOND_ENTRIES, league.ENTRIES)
        self.assertEqual(set(league.ENTRIES), set(league.VECTORS.values()) | set(league.DIRECT_CALLS.values()))
        for entry, end, _ in league.ROUTINES:
            self.assertEqual(league.owner(entry), entry)
            self.assertEqual(league.owner(end - 4), entry)
        self.assertEqual(league.validate_export(league.EXPORT), 1916)

    def test_reject_nonentry_duplicate_and_external_interior(self):
        for registry in (
            "BRA_NAT_SECOND_ROUTINE(0x105177b4, bad)",
            "BRA_NAT_SECOND_ROUTINE(0x105195f0, bad)",
            "BRA_NAT_SECOND_ROUTINE(0x105195d0, twice)\n" * 2,
        ):
            with self.assertRaisesRegex(ValueError, "production registry"):
                league.production_entries(registry)
        with self.assertRaisesRegex(ValueError, "Cross-entry"):
            league.validate_flows({}, [(0x105177ac, 0x105177b4, "UNCONDITIONAL_JUMP")])

    def test_reject_competing_and_late_hooks(self):
        for entry in league.BRA_NAT_SECOND_ENTRIES:
            hook = league.bra_nat_second_hook(entry)
            label, step = f"i_{entry:x}:\n", f"s.step({entry:#x});\n"
            records = [{"address": entry}]
            league.require_bra_nat_second_hooks(entry >> 12, records, label + hook + "\n" + step)
            for bad in (label + step + hook, label + hook + "\n" + hook + step,
                        label + hook + "\nif (try_modern_other(s, 0)) return true;\n" + step):
                with self.assertRaisesRegex(ValueError, "Missing or competing"):
                    league.require_bra_nat_second_hooks(entry >> 12, records, bad)


if __name__ == "__main__":
    unittest.main()
