"""Guard sparse profiling hooks without depending on private game exports."""
import unittest

from profiling_boundaries import DAY_CALLS, PROFILE_BOUNDARIES, add_boundary, profiling_hook


class ProfilingBoundaries(unittest.TestCase):
    def test_guards_reject_every_changed_instruction(self):
        for address, (assembly, names) in PROFILE_BOUNDARIES.items():
            with self.subTest(address=hex(address)):
                self.assertEqual(profiling_hook(address, assembly), "\n".join(
                    f'CM_PROFILE_EVENT("{name}");' for name in names))
                with self.assertRaisesRegex(ValueError, "boundary changed"):
                    profiling_hook(address, "changed instruction")

    def test_each_call_has_ordered_begin_and_return(self):
        self.assertEqual(len(DAY_CALLS), len({address for address, *_ in DAY_CALLS}))
        self.assertEqual(list(DAY_CALLS), sorted(DAY_CALLS))
        for address, target, return_assembly, name in DAY_CALLS:
            label = f"Day: {name} [{address:x}]"
            self.assertEqual(PROFILE_BOUNDARIES[address][0], f"bl 0x{target:x}")
            self.assertEqual(PROFILE_BOUNDARIES[address][1][-1], f"{label} begin")
            self.assertEqual(PROFILE_BOUNDARIES[address + 4][0], return_assembly)
            self.assertEqual(PROFILE_BOUNDARIES[address + 4][1][0], f"{label} after")

    def test_adjacent_call_order_and_conflicts(self):
        boundaries = {}
        add_boundary(boundaries, 4, "bl 0x1234", "previous end")
        add_boundary(boundaries, 4, "bl 0x1234", "next begin")
        self.assertEqual(boundaries[4][1], ("previous end", "next begin"))
        with self.assertRaisesRegex(ValueError, "Conflicting profiling"):
            add_boundary(boundaries, 4, "blr", "invalid")


if __name__ == "__main__":
    unittest.main()
