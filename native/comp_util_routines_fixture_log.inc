// COMP_UTIL.CPP block `fixture_log` (comp_util_log.cpp): the history log table. The appender and the lookup leave r4..r10
// as the original does (callers read them); the collector is read for r3 only.
COMP_UTIL_ROUTINE(0x10463990, 0x10463a90, NONE, "e4550804c03327b6", [](Cpu& c) { comp_util::append_history(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
COMP_UTIL_ROUTINE(0x10463a90, 0x10463bd8, R3, "76ed4ef8e9f8ec08", [](Cpu& c) { c.set(3, comp_util::history_club(c, c.r(3), c.r(4))); })
COMP_UTIL_ROUTINE(0x104770c0, 0x1047726c, R3, "35632a134b67ad79", [](Cpu& c) { c.set(3, comp_util::club_history(c, c.r(3), c.r(4), c.r(5))); })
COMP_UTIL_ROUTINE(0x10477270, 0x10477484, R3, "9ea3e30b8e3fe8dd", [](Cpu& c) { c.set(3, std::uint32_t(comp_util::compare_history_rows(c, c.r(3), c.r(4)))); })
