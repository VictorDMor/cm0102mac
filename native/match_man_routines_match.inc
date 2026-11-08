// MATCH_MAN.CPP match block (match_man_match.hpp).
MATCH_MAN_ROUTINE(0x1020d7d0, 0x1020e8e4, R3, "768ced4efc9cc64a", [](Cpu& c) { match_man::pick_lineup(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_MAN_ROUTINE(0x1020ef70, 0x102102c8, NONE, "375f8643313749e9", [](Cpu& c) { match_man::manage_substitutions(c, c.r(3), c.r(4)); })
MATCH_MAN_ROUTINE(0x102104d0, 0x102107c0, NONE, "9f7f74d65c871ba5", [](Cpu& c) { match_man::apply_lineup(c, c.r(3)); })
