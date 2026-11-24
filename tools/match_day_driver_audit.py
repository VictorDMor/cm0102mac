"""The background match day's driver (0x101cb9d0): its hook in the generated page.

The C++ owner (native/match_day_driver.cpp) takes the routine in its own orders: in_order
(the same work as the original, a check of the C++) and phases (TASKS.md, "Parallel play
phase"). In the order named original, the default, it declines and the generated body runs.
The hook has no switch of its own in the generator: every target links cm_aot.
"""
MATCH_DAY_DRIVER_ENTRIES = (0x101cb9d0,)


def match_day_driver_hook(entry):
    return f"if (try_modern_match_day_driver(s, 0x{entry:x})) return true;"


def require_match_day_driver_hooks(page, instructions, code):
    addresses = {record["address"] for record in instructions}
    for entry in MATCH_DAY_DRIVER_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label, hook, step = f"i_{entry:x}:", match_day_driver_hook(entry), f"s.step(0x{entry:x});"
        fragment = code.split(label, 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        if (label not in code or code.count(hook) != 1 or hook not in fragment or step not in fragment
                or fragment.index(hook) > fragment.index(step) or fragment.count("try_modern_") != 1):
            raise ValueError(f"Missing or competing match day driver hook at {entry:#x}")
