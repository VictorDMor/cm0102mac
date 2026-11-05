import unittest

import hall_of_fame_audit as hall


class HallOfFameAudit(unittest.TestCase):
    def test_source_inventory(self):
        self.assertEqual(len(hall.ENTRIES), 20)
        self.assertEqual(len(hall.LABELS), 27)
        self.assertTrue(set(hall.LABELS).isdisjoint(hall.ENTRIES))
        self.assertEqual(hall.validate_export(hall.EXPORT), 2736)

    def test_pilot_hook_precedes_the_original(self):
        for entry in hall.HALL_OF_FAME_ENTRIES:
            label, step = f"i_{entry:x}:\n", f"s.step({entry:#x});\n"
            hook = hall.hall_of_fame_hook(entry) + "\n"
            records = [{"address": entry}]
            hall.require_hall_of_fame_hooks(entry >> 12, records, label + hook + step)
            for malformed in (label + step, label + step + hook, label + hook + hook + step):
                with self.assertRaisesRegex(ValueError, "Missing or competing"):
                    hall.require_hall_of_fame_hooks(entry >> 12, records, malformed)

    def test_external_jump_cannot_become_an_interior_root(self):
        calls = [(site, target, "UNCONDITIONAL_CALL") for site, target in hall.DIRECT_CALLS.items()]
        calls.append((0x10140720, hall.ENTRIES[0] + 4, "UNCONDITIONAL_JUMP"))
        with self.assertRaisesRegex(ValueError, "cross-entry"):
            hall.validate_flows({}, calls)


if __name__ == "__main__":
    unittest.main()
