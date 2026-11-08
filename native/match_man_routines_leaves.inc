// MATCH_MAN.CPP leaves block (match_man_leaves.hpp).
MATCH_MAN_ROUTINE(0x102065e0, 0x102065e4, NONE, "f0b063ea75240311", [](Cpu& c) { match_man::empty_callback(c); })
MATCH_MAN_ROUTINE(0x102107c0, 0x102107fc, R3, "fcd5116f70b33228", [](Cpu& c) { match_man::side_player_for_person(c, c.r(3), c.r(4)); })
