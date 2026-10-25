// PLAYER_STATS.CPP: the stats screens' texts: competition names, column titles and formatted values (player_stats_text.hpp).
PLAYER_STATS_ROUTINE(0x10302cb0, 0x10302e70, R3, "954bc3d8f0f92c88", [](Cpu& c) { c.set(3, player_stats::competition_name(c, std::int8_t(c.r(4)), c.r(5), std::int32_t(c.r(6)))); })
PLAYER_STATS_ROUTINE(0x10302e70, 0x10303090, NONE, "b26ad8c036208b8d", [](Cpu& c) { player_stats::recent_ratings_text(c, c.r(3), c.r(4), c.r(5), std::int32_t(c.r(6))); })
PLAYER_STATS_ROUTINE(0x103032e0, 0x10303560, R3, "d738489714488963", [](Cpu& c) { c.set(3, player_stats::column_title(c, std::int8_t(c.r(4)), c.r(5), std::int32_t(c.r(6)))); })
PLAYER_STATS_ROUTINE(0x10303ff0, 0x10304190, R3, "57c703fff117c0e4", [](Cpu& c) { c.set(3, player_stats::player_value_text(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), std::int32_t(c.r(8)), c.r(9), c.r(10), c.u8(c.r(1) + 0x3b))); })
PLAYER_STATS_ROUTINE(0x10304190, 0x10304b10, R3, "30cc6f9abc7ee7fc", [](Cpu& c) { c.set(3, player_stats::season_value_text(c, c.r(4), std::int8_t(c.r(5)), c.r(6), std::int32_t(c.r(7)), c.r(8), c.r(9), c.r(10))); })
PLAYER_STATS_ROUTINE(0x10304b10, 0x103054e0, R3, "eb984652fdc37976", [](Cpu& c) { c.set(3, player_stats::totals_value_text(c, c.r(4), std::int8_t(c.r(5)), c.r(6), std::int32_t(c.r(7)), c.r(8), c.r(9), c.r(10))); })
