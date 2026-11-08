// MATCH_MAN.CPP selection block (match_man_selection.hpp).
MATCH_MAN_ROUTINE(0x102102d0, 0x1021042c, R3|F1, "44823dc24eb7d860", [](Cpu& c) { match_man::best_person(c, c.r(3), c.r(4), std::int8_t(c.r(5))); })
MATCH_MAN_ROUTINE(0x10210430, 0x102104c4, R3, "535de3db3b91526e", [](Cpu& c) { match_man::best_player_index(c, c.r(3)); })
MATCH_MAN_ROUTINE(0x10210800, 0x10210c44, R3|F1, "059a4f97c678bfa7", [](Cpu& c) { match_man::selection_rating(c, c.r(3), c.r(4), c.r(5)); })
MATCH_MAN_ROUTINE(0x10210c50, 0x10211e64, R3, "5d2671f2b2fdedf3", [](Cpu& c) { match_man::select_places(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)); })
MATCH_MAN_ROUTINE(0x1021a240, 0x1021b010, R3, "aae2a23dc38ec0be", [](Cpu& c) { match_man::refill_places(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)); })
