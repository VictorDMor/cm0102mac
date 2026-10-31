// Original entries of the setup block.
MATCH_ENG_ROUTINE(0x101d16a0, 0x101d1760, R3, "625664953717fe84",
    [](Cpu& c) { match_eng::construct_match(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101d1760, 0x101d3ac0, R3, "8c479d0ddeb590d8",
    [](Cpu& c) { match_eng::set_up_match(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101d3b60, 0x101d3bf0, R3, "de0278406902df8c",
    [](Cpu& c) { match_eng::side_player(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101d3c90, 0x101d3e50, R3, "3b626f1043a1c2e3",
    [](Cpu& c) { match_eng::random_below(c, std::int32_t(c.r(3))); })
MATCH_ENG_ROUTINE(0x101d3e50, 0x101d3f60, R3, "81fa6f92594e8f8e",
    [](Cpu& c) { match_eng::destroy_match(c, c.r(3), c.r(4)); })
