// COMP_UTIL.CPP block `round_text` (comp_util_rounds.cpp, comp_util_rounds_text.cpp): a round's name and the
// sentences built around it. The two sentence builders (0x10469710, 0x104698c0) answer nothing: r3 is whatever the
// last call left.
COMP_UTIL_ROUTINE(0x10469bb0, 0x1046f1ec, R3, "6edd52ac6392530e", [](Cpu& c) { c.set(3, comp_util::round_text(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
COMP_UTIL_ROUTINE(0x10476f40, 0x104770bc, R3, "187b8400ee18d8e9", [](Cpu& c) { c.set(3, comp_util::round_pair_text(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
COMP_UTIL_ROUTINE(0x104768a0, 0x104769d0, R3, "9f832acd2ea8f28e", [](Cpu& c) { c.set(3, comp_util::fixture_name_text(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
COMP_UTIL_ROUTINE(0x104769d0, 0x10476ebc, R3, "d2171274e59aa500", [](Cpu& c) { c.set(3, comp_util::competition_name_text(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8))); })
COMP_UTIL_ROUTINE(0x10469710, 0x104698b4, R3, "ee2f3edcb7bbd59c", [](Cpu& c) { comp_util::competition_round_news(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
COMP_UTIL_ROUTINE(0x104698c0, 0x10469bac, R3, "3b4f9db63fd824f6", [](Cpu& c) { comp_util::fixture_round_news(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); })
