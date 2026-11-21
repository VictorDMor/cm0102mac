// COMP_UTIL.CPP block `comparators` (comp_util_compare.cpp): the qsort comparators. Each answers a three-way
// result in r3 (QSORT4X reads only that).
COMP_UTIL_ROUTINE(0x10463580, 0x104635a4, R3, "cef5023dd107d8cb", [](Cpu& c) { c.set(3, std::uint32_t(comp_util::compare_score_descending(c, c.r(3), c.r(4)))); })
COMP_UTIL_ROUTINE(0x104647a0, 0x104647b8, R3, "e4856676bcc809bc", [](Cpu& c) { c.set(3, std::uint32_t(comp_util::compare_club_reputation(c, c.r(3), c.r(4)))); })
COMP_UTIL_ROUTINE(0x104647c0, 0x104647d8, R3, "782dc2b055d088ec", [](Cpu& c) { c.set(3, std::uint32_t(comp_util::compare_tag_ascending(c, c.r(3), c.r(4)))); })
COMP_UTIL_ROUTINE(0x104647e0, 0x10464830, R3, "15a230304f39331c", [](Cpu& c) { c.set(3, std::uint32_t(comp_util::compare_club_reputation_nulls_last(c, c.r(3), c.r(4)))); })
COMP_UTIL_ROUTINE(0x10464830, 0x10464880, R3, "8dfdbda40c785cb3", [](Cpu& c) { c.set(3, std::uint32_t(comp_util::compare_club_reputation_ascending(c, c.r(3), c.r(4)))); })
COMP_UTIL_ROUTINE(0x10477cd0, 0x10477d54, R3, "a82b1d5eb6c167a1", [](Cpu& c) { c.set(3, std::uint32_t(comp_util::compare_club_division(c, c.r(3), c.r(4)))); })
COMP_UTIL_ROUTINE(0x104777c0, 0x10477870, R3, "48cf3b6a794ed805", [](Cpu& c) { c.set(3, std::uint32_t(comp_util::compare_screen_rows(c, c.r(3), c.r(4)))); })
