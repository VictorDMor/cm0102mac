// Original entries of the events block.
MATCH_ENG_ROUTINE(0x101db340, 0x101de040, R3, "49374e3fcb439a37",
    [](Cpu& c) { match_eng::simulate_match_events(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101de0a0, 0x101de340, R3, "6f6e33550a720cbf",
    [](Cpu& c) { match_eng::adjust_players_a(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101de340, 0x101de610, R3, "ba82580f98146ed5",
    [](Cpu& c) { match_eng::adjust_players_b(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101de610, 0x101de8b0, R3, "1e4fff7604f9998c",
    [](Cpu& c) { match_eng::adjust_players_c(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101de8b0, 0x101deb50, R3, "71c3243971846c14",
    [](Cpu& c) { match_eng::adjust_players_d(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101deb50, 0x101dee30, R3, "ff4dbe00c0b2b8bb",
    [](Cpu& c) { match_eng::adjust_players_e(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101dee30, 0x101df110, R3, "7b65f5cfc7085099",
    [](Cpu& c) { match_eng::adjust_players_f(c, c.r(3), c.r(4), c.r(5)); })
