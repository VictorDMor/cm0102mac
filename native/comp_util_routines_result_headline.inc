// COMP_UTIL.CPP block `result_headline` (comp_util_headline.cpp, comp_util_headline_tables.inc): the news item about a club's
// result in a fixture. Answers nothing: r3 is whatever the last call left.
COMP_UTIL_ROUTINE(0x10464d00, 0x10469704, R3, "e5022cc4bb32fb42", [](Cpu& c) { comp_util::result_headline(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); })
