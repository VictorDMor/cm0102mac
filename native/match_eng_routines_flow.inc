// Original entries of the flow block: period ends and the penalty shoot-out.
MATCH_ENG_ROUTINE(0x101d95a0, 0x101da020, R3, "460372a0c5aa4151",
    [](Cpu& c) { match_eng::period_ended(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101da020, 0x101da3c0, NONE, "8456716aa6843d94",
    [](Cpu& c) { match_eng::check_lineup_balance(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101da3c0, 0x101da5a0, NONE, "5fe2a1491f13e7f2",
    [](Cpu& c) { match_eng::choose_shootout_order(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101da5a0, 0x101da6d0, R3, "dd60101cbce0f630",
    [](Cpu& c) { match_eng::side_can_supply_kicker(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101da760, 0x101da820, R3, "9fa39535c42ae81c",
    [](Cpu& c) { match_eng::player_index(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101da820, 0x101db320, NONE, "310736aaf122f9e4",
    [](Cpu& c) { match_eng::shootout_step(c, c.r(3), c.r(4)); })
