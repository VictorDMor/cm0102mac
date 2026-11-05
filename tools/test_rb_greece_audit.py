import unittest

import rb_greece_audit as rulebook


class GreekRulebookAudit(unittest.TestCase):
    def test_source_inventory(self):
        self.assertEqual(len(rulebook.ENTRIES), 4)
        self.assertTrue(set(rulebook.RB_GREECE_ENTRIES) <= set(rulebook.ENTRIES))
        self.assertEqual(rulebook.validate_export(rulebook.EXPORT), 397)

    def test_pilot_hook_is_unique_and_precedes_generated_code(self):
        for entry in rulebook.RB_GREECE_ENTRIES:
            label, step = f"i_{entry:x}:\n", f"s.step({entry:#x});\n"
            hook = rulebook.rb_greece_hook(entry) + "\n"
            records = [{"address": entry}]
            rulebook.require_rb_greece_hooks(entry >> 12, records, label + hook + step)
            for malformed in (label + step, label + step + hook, label + hook + hook + step):
                with self.assertRaisesRegex(ValueError, "Missing or competing"):
                    rulebook.require_rb_greece_hooks(entry >> 12, records, malformed)


if __name__ == "__main__":
    unittest.main()
