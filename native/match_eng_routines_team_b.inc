// Original entries of the team_b block.
MATCH_ENG_ROUTINE(0x101ec890, 0x101ee510, R3, "7ce3c82042edd99a",
    [](Cpu& c) { match_eng::resolve_contest(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101ee530, 0x101ee610, R3, "a9a8750f482a5a28",
    [](Cpu& c) { match_eng::protected_action(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101ee680, 0x101ee770, R3, "1f4f4f2bd2412973",
    [](Cpu& c) { match_eng::count_players_ahead(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MATCH_ENG_ROUTINE(0x101ee770, 0x101ee9c0, R3, "65d04fbd1636cd89",
    [](Cpu& c) { match_eng::lane_status(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101eeac0, 0x101eecb0, R3, "2cdeab5844153a7e",
    [](Cpu& c) { match_eng::run_allowed(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MATCH_ENG_ROUTINE(0x101eecb0, 0x101eee20, R3, "c65d8d56567cd147",
    [](Cpu& c) { match_eng::adjacent_cell_free(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101eee20, 0x101eeeb0, R3, "90619c43798d5aa4",
    [](Cpu& c) { match_eng::count_side_players(c, c.r(3), c.r(4)); })
