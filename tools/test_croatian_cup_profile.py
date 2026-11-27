"""The authoring contract for the native Croatian Cup profiles."""
import copy
import json
from pathlib import Path
import unittest

from compile_croatian_cup import compile_profiles, unique_object


SOURCE = Path(__file__).resolve().parents[1] / "rules/competitions/croatian-cup.json"


class CroatianCupProfileTests(unittest.TestCase):
    def setUp(self):
        self.data = json.loads(SOURCE.read_text(), object_pairs_hook=unique_object)

    def test_named_editions_can_share_or_switch_profiles(self):
        normal = compile_profiles(self.data)
        changed = copy.deepcopy(self.data)
        changed["editions"]["si_3968"] = changed["editions"]["cm0001_389"]
        self.assertNotEqual(compile_profiles(changed), normal)
        # Editing one date in an override changes only the matching stage/profile.
        self.data["profiles"]["national"]["round_overrides"]["final"]["gap"] = 12
        new = compile_profiles(self.data)
        self.assertEqual(sum(a != b for a, b in zip(normal.splitlines(), new.splitlines())), 1)

    def test_unknown_and_ambiguous_inputs(self):
        with self.assertRaisesRegex(ValueError, "duplicate"):
            json.loads('{"gap": 7, "gap": 14}', object_pairs_hook=unique_object)
        self.data["editions"]["cm0102"] = "absent"
        with self.assertRaisesRegex(ValueError, "unknown profile"):
            compile_profiles(self.data)

    def test_malformed_profiles_fail_before_publication(self):
        cases = [
            (("profiles", "regional", "first_places"), 15, "admit every entrant"),
            (("profiles", "national", "lower_pool_limit"), 17, "lower_pool_limit"),
            (("profiles", "national", "second_order"), "random", "ordering policy"),
            (("profiles", "national", "round_overrides", "replay"), {}, "unknown fields"),
            (("profiles", "national", "round_overrides", "final", "gap"), 255, "next edition"),
            (("rounds", 2, "legs"), 2, "leg count"),
            (("rounds", 2, "new_entrants"), 1, "cannot pair"),
            (("rounds", 0, "fixture"), "02-30 Wed", "day"),
            (("seed_range",), 129, "seed_range"),
        ]
        for keys, value, message in cases:
            with self.subTest(keys=keys):
                data = copy.deepcopy(self.data)
                parent = data
                for key in keys[:-1]:
                    parent = parent[key]
                parent[keys[-1]] = value
                with self.assertRaisesRegex(ValueError, message):
                    compile_profiles(data)

    def test_calendar_checks_use_the_actual_weekdays(self):
        self.data["rounds"][0].update(draw="08-15 Tue", fixture="08-16 Sun")
        with self.assertRaisesRegex(ValueError, "before its draw"):
            compile_profiles(self.data)


if __name__ == "__main__":
    unittest.main()
