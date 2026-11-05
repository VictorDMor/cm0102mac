import unittest

import national_teams_audit as national


class NationalTeamsAudit(unittest.TestCase):
    def test_closed_inventory(self):
        self.assertEqual(len(national.ENTRIES), 73)
        self.assertEqual(len(national.VECTORS), 3)
        self.assertEqual(national.validate_export(national.EXPORT), 17621)

    def test_production_owner_before_original(self):
        for entry in national.NATIONAL_TEAMS_ENTRIES:
            label = f"i_{entry:x}:\n"
            hook = national.national_teams_hook(entry) + "\n"
            step = f"s.step({entry:#x});\n"
            instructions = [{"address": entry}]
            national.require_national_teams_hooks(entry >> 12, instructions, label + hook + step)
            for body in (label + step, label + step + hook, label + hook + hook + step):
                with self.assertRaisesRegex(ValueError, "Missing or competing"):
                    national.require_national_teams_hooks(entry >> 12, instructions, body)

    def test_external_jump_does_not_become_a_routine_root(self):
        with self.assertRaisesRegex(ValueError, "cross-entry branch"):
            national.validate_flows({}, [(0x102aada0, national.ENTRIES[0] + 4, "UNCONDITIONAL_JUMP")])


if __name__ == "__main__":
    unittest.main()
