// COMP_UTIL.CPP block `comp_screens_list` (comp_util_screens.cpp): the sorted list of a COMP_SCREENS record.
COMP_UTIL_ROUTINE(0x10477490, 0x104777b8, R3, "69da00f94a7b0919", [](Cpu& c) { c.set(3, comp_util::sort_screen_list(c, c.r(3)) ? 1u : 0u); })
