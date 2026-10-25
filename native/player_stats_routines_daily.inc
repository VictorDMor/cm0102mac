// PLAYER_STATS.CPP: the daily and match-day updates of the season records and the history files (player_stats_daily.hpp).
PLAYER_STATS_ROUTINE(0x103024a0, 0x10302b20, R3, "6be0783c0168ea6b", [](Cpu& c) { c.set(3, player_stats::record_match(c, c.r(3), c.r(4), c.r(5))); })
PLAYER_STATS_ROUTINE(0x10302b20, 0x10302cb0, R3, "f56636e147d60e1b", [](Cpu& c) { c.set(3, player_stats::archive_nation(c, c.r(3), c.r(4))); })
PLAYER_STATS_ROUTINE(0x10308090, 0x10308350, R3, "7139f2ed06b805ba", [](Cpu& c) { c.set(3, player_stats::add_season_record(c, c.r(3), c.r(4), c.r(5))); })
PLAYER_STATS_ROUTINE(0x10308350, 0x10308de0, R3, "45eae6252e8e256d", [](Cpu& c) { c.set(3, player_stats::archive_seasons(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9))); })
PLAYER_STATS_ROUTINE(0x10308de0, 0x10309040, R3, "276a197771ce7475", [](Cpu& c) { c.set(3, player_stats::competition_type(c, c.r(3), c.r(4))); })
PLAYER_STATS_ROUTINE(0x10309040, 0x103092f0, NONE, "84055b1f489a7bd3", [](Cpu& c) { player_stats::clear_seasons(c, c.r(3), c.r(4)); })
