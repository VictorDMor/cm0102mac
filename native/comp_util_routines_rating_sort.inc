// COMP_UTIL.CPP block `rating_sort` (comp_util_rating.cpp): the club rating and the sort by it.
COMP_UTIL_ROUTINE(0x10477870, 0x10477b34, R3, "a63f122e112e1c1f", [](Cpu& c) { c.set(3, std::uint32_t(comp_util::club_strength(c, c.r(3)))); })
COMP_UTIL_ROUTINE(0x10477b40, 0x10477cc4, NONE, "a3653680c024501b", [](Cpu& c) { comp_util::sort_by_strength(c, c.r(3), std::int32_t(c.r(4))); })
