// PLAYER_STATS.CPP: the ratings and values the match engine, recruitment and the screens ask of a player's season records (player_stats_queries.hpp).
PLAYER_STATS_ROUTINE(0x10303560, 0x10303980, F1, "e26f1c6219a35b01", [](Cpu& c) { c.set_f(1, player_stats::player_value(c, c.r(3), c.r(4), c.r(5), c.r(6), c.f(1))); })
PLAYER_STATS_ROUTINE(0x10303980, 0x10303ff0, F1, "2da561811abd39f9", [](Cpu& c) { c.set_f(1, player_stats::season_value(c, c.r(4), c.r(5), c.f(1))); })
PLAYER_STATS_ROUTINE(0x103054e0, 0x10305700, R3, "a5a3c925a8331218", [](Cpu& c) { player_stats::season_record(c, c.r(3), c.r(4), std::int8_t(c.r(5))); })
PLAYER_STATS_ROUTINE(0x10306a80, 0x10306c00, F1, "3ac231abf65987a7", [](Cpu& c) { c.set_f(1, player_stats::entry_figure(c, c.r(3), c.r(4), std::int8_t(c.r(5)))); })
PLAYER_STATS_ROUTINE(0x10303090, 0x103031f0, R3, "13a794c99893e4fd", [](Cpu& c) { player_stats::packed_ratings_average(c, c.r(3), c.r(4)); })
PLAYER_STATS_ROUTINE(0x103031f0, 0x103032e0, F1, "e78d43617572c860", [](Cpu& c) { c.set_f(1, player_stats::fifth_rating(c, c.r(3), c.r(4))); })
