// Original entries of the advance block.
MATCH_ENG_ROUTINE(0x101d3f60, 0x101d5660, R3, "bf3eaf008c6adf0b",
    [](Cpu& c) { match_eng::advance(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101d5760, 0x101d57c0, R3, "c201c3d534a0ddad",
    [](Cpu& c) { match_eng::side_flag_1000(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101d5810, 0x101d5870, NONE, "246b69370c7d661a",
    [](Cpu& c) { match_eng::post_event(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101d5880, 0x101d5980, R3, "b7d7a8f11dd7594a",
    [](Cpu& c) { match_eng::restart_still_valid(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101d5980, 0x101d6690, R3, "cef4f43584f17c7f",
    [](Cpu& c) { match_eng::resolve_restart(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101d6710, 0x101d6770, NONE, "b7c2bb0d00025195",
    [](Cpu& c) { match_eng::post_player_event(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); })
MATCH_ENG_ROUTINE(0x101d6780, 0x101d6860, NONE, "cada48f5d7ff7783",
    [](Cpu& c) { match_eng::adjust_side_around(c, c.r(3), c.r(4), c.f(1)); })
MATCH_ENG_ROUTINE(0x101d6860, 0x101d68f0, NONE, "da01060a3b6fbfab",
    [](Cpu& c) { match_eng::adjust_side(c, c.r(3), c.r(4), c.f(1)); })
MATCH_ENG_ROUTINE(0x101d69c0, 0x101d6a90, R3, "605978f928d71424",
    [](Cpu& c) { match_eng::restart_owner(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101d6a90, 0x101d6da0, NONE, "fbf4514ca3888bd3",
    [](Cpu& c) { match_eng::side_pressure(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101d6da0, 0x101d6f90, NONE, "302f51e4d8a19278",
    [](Cpu& c) { match_eng::drift_ball(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101d6f90, 0x101d70d0, NONE, "e35da828c1fcd940",
    [](Cpu& c) { match_eng::update_teams_players(c, c.r(3)); })
