// PLAYER_REGEN.CPP block `candidate_score` (player_regen_random.cpp, player_regen_candidates.cpp): the pooled Random(n), the
// score of a replacement candidate and the pick of the best one.
PLAYER_REGEN_ROUTINE(0x102da630, 0x102daba8, R3, "b2cd027f36d6559a", [](Cpu& c) { c.set(3, std::uint32_t(player_regen::draw_below(c, std::int32_t(c.r(3))))); })
PLAYER_REGEN_ROUTINE(0x102dc410, 0x102dcb20, R3, "14e349132e6479a5", [](Cpu& c) { c.set(3, std::uint32_t(player_regen::candidate_score(c, c.r(4)))); })
PLAYER_REGEN_ROUTINE(0x102dbf00, 0x102dbff8, R3, "9a4cb7a4297923cb", [](Cpu& c) { c.set(3, player_regen::best_candidate(c, c.r(4))); })
