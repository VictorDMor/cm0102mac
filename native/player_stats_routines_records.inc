// PLAYER_STATS.CPP: a person's career table and the sums the game reads from it (player_stats_records.hpp).
PLAYER_STATS_ROUTINE(0x10305700, 0x103058f0, R3, "5f8c0e0281c599ab", [](Cpu& c) { c.set(3, player_stats::club_appearances(c, c.r(4), c.r(5))); })
PLAYER_STATS_ROUTINE(0x103058f0, 0x10305ae0, R3, "9e312dfb30edd225", [](Cpu& c) { c.set(3, player_stats::club_goals(c, c.r(4), c.r(5))); })
PLAYER_STATS_ROUTINE(0x10305ae0, 0x10305c60, R3, "89eaab5753815bcd", [](Cpu& c) { c.set(3, player_stats::career_goals(c, c.r(4))); })
PLAYER_STATS_ROUTINE(0x10305c60, 0x10306a80, R3, "700834a092ea0375", [](Cpu& c) { c.set(3, player_stats::career_table(c, c.r(3), c.r(4), c.r(5))); })
PLAYER_STATS_ROUTINE(0x10309cc0, 0x10309ee0, R3, "2ced5ce417834a4e", [](Cpu& c) { c.set(3, player_stats::seasons_since_club(c, c.r(4), c.r(5))); })
