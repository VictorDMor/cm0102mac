// MATCH_MAN.CPP lineup_b block (match_man_lineup_b.hpp).
MATCH_MAN_ROUTINE(0x102065f0, 0x10206cac, R3, "9102ccc2e5365ab9", [](Cpu& c) { match_man::choose_lineup(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)); })
MATCH_MAN_ROUTINE(0x10209d10, 0x1020a288, R3|F1, "349442f2f4704f79", [](Cpu& c) { match_man::scaled_random(c, std::int32_t(c.r(3))); })
