// MATCH_MAN.CPP callbacks block (match_man_callbacks.hpp).
MATCH_MAN_ROUTINE(0x10206cb0, 0x10206d78, R3, "6f520ae915e0b5c9", [](Cpu& c) { match_man::release_side(c, c.r(3), c.r(4)); })
MATCH_MAN_ROUTINE(0x10206d80, 0x10206e9c, R3, "ab150310b3c7e425", [](Cpu& c) { match_man::prepare_squad_actors(c, c.r(3)); })
MATCH_MAN_ROUTINE(0x1020e8f0, 0x1020e9f8, R3, "192173d5171d1e85", [](Cpu& c) { match_man::prepare_actor(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MATCH_MAN_ROUTINE(0x1020ea00, 0x1020edf4, R3|F1, "0b8e004fcde0f14f", [](Cpu& c) { match_man::compare_by_scaled_score(c, c.r(3), c.r(4)); })
MATCH_MAN_ROUTINE(0x1020ee00, 0x1020ef64, R3|F1, "efeffa4cab9c31b9", [](Cpu& c) { match_man::compare_by_score(c, c.r(3), c.r(4)); })
