// COMP_UTIL.CPP block `round_codes` (comp_util_rounds.cpp): the round code's class and the competition's gender.
COMP_UTIL_ROUTINE(0x1046f1f0, 0x10476774, R3, "49730e9e942e113d", [](Cpu& c) { comp_util::round_gender(c, c.r(3), c.r(4)); })
COMP_UTIL_ROUTINE(0x10476ec0, 0x10476f3c, R3, "b256114e03909a5f", [](Cpu& c) { comp_util::competition_gender(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
