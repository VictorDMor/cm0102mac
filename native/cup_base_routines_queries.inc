// CUP.CPP block `queries` (cup_base_queries*.cpp): the news, the prizes, the ranks and the remaining virtuals.
// The comment after a virtual is its slot in the vtable 0x108d7724.
CUP_BASE_ROUTINE(0x1047b360, 0x1047b5a4, R3, "33b77b1651a945ae", [](Cpu& c) { c.set(3, cup_base::queries::file_result_news(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8))); })  // +0x48
CUP_BASE_ROUTINE(0x1047b5b0, 0x1047b7d0, R3, "39fe8e4a7648cb25", [](Cpu& c) { c.set(3, cup_base::queries::record_place(c, c.r(3), c.r(4), c.r(5), c.r(8), c.r(9))); })  // +0x4c
CUP_BASE_ROUTINE(0x1047b7d0, 0x1047ba48, NONE, "45a26844176fc94a", [](Cpu& c) { cup_base::queries::record_final_places(c, c.r(3)); })  // +0x38
CUP_BASE_ROUTINE(0x1047ba50, 0x1047bba8, R3, "b6bc7eb577cd6f92", [](Cpu& c) { c.set(3, std::uint32_t(cup_base::queries::tie_index(c, c.r(3), c.r(4)))); })
CUP_BASE_ROUTINE(0x1047bde0, 0x1047bfa4, NONE, "8fa1d8e64ba03d1a", [](Cpu& c) { cup_base::queries::file_uefa_seeding(c, c.r(3)); })
CUP_BASE_ROUTINE(0x10484b90, 0x10484e08, R3, "f36aa01fcdf8d970", [](Cpu& c) { c.set(3, cup_base::queries::side_entrants(c, c.r(3), std::int32_t(c.r(4)), std::int32_t(c.r(5)), c.r(6), c.r(7))); })
CUP_BASE_ROUTINE(0x10484e10, 0x10484fcc, R3, "4f3aefeb565908a8", [](Cpu& c) { c.set(3, cup_base::queries::append_entrants(c, c.r(3), std::int32_t(c.r(4)), std::int32_t(c.r(5)), c.r(6), c.r(7))); })
CUP_BASE_ROUTINE(0x10484fd0, 0x10485334, NONE, "a738b2589a5db677", [](Cpu& c) { cup_base::queries::record_entrant_places(c, c.r(3), c.r(4), c.r(5)); })
CUP_BASE_ROUTINE(0x10485340, 0x10485404, NONE, "94fa8bda601df0c8", [](Cpu& c) { cup_base::queries::settle_match_flags(c, c.r(3), c.r(4), c.r(5)); })
CUP_BASE_ROUTINE(0x104858b0, 0x10485930, R3, "4338f07412f538ec", [](Cpu& c) { c.set(3, std::uint32_t(cup_base::queries::round_difference(c, c.r(3), c.r(4), c.r(5), c.r(6)))); })  // +0x5c
CUP_BASE_ROUTINE(0x10485930, 0x10485ab0, NONE, "b4619df2967e1c1e", [](Cpu& c) { cup_base::queries::pay_prizes(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
CUP_BASE_ROUTINE(0x10485ab0, 0x10485b0c, R3, "68a8f013996c3dd7", [](Cpu& c) { c.set(3, cup_base::queries::lower_record(c, c.r(3), c.r(4), c.r(5))); })  // +0x98
CUP_BASE_ROUTINE(0x10485b10, 0x10485d70, NONE, "4d1a01bea49dc5b5", [](Cpu& c) { cup_base::queries::rank_teams(c, c.r(3)); })  // +0x64
CUP_BASE_ROUTINE(0x10486480, 0x104864c4, R3, "a44de5a06b5e48d1", [](Cpu& c) { c.set(3, cup_base::queries::record_group_places(c, c.r(3), c.r(4))); })  // +0x8c
