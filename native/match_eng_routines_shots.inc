// Original entries of the shots block.
MATCH_ENG_ROUTINE(0x101df110, 0x101e15e0, NONE, "62cc031cc6fec57e",
    [](Cpu& c) { match_eng::update_side_tendencies(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101e1640, 0x101e1a50, R3, "08d090c63c70b099",
    [](Cpu& c) { match_eng::choose_candidate_weighted(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101e1a50, 0x101e1bd0, R3, "718c6a63447a6015",
    [](Cpu& c) { match_eng::choose_candidate_by_mode(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101e1bd0, 0x101e2100, R3, "776edb93b2a4259e",
    [](Cpu& c) { match_eng::choose_candidate_ranked(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101e2100, 0x101e22e0, R3, "d6943f06776bfe12",
    [](Cpu& c) { match_eng::choose_candidate_simple(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101e22e0, 0x101e25a0, R3, "01391f6e886fd300",
    [](Cpu& c) { match_eng::choose_candidate_by_traits(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101e25a0, 0x101e28b0, NONE, "58a398ba9fe71c80",
    [](Cpu& c) { match_eng::role_nibbles(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
