// LEAGUE.CPP, block `news` (league_base_news*.cpp): prizes, news items and the remaining queries.
LEAGUE_BASE_ROUTINE(0x1049f540, 0x1049f6d0, R3, "fe44d8e3251760a3", [](Cpu& c) { c.set(3, league_base::prize_total(c, c.r(3), c.r(4))); })
LEAGUE_BASE_ROUTINE(0x1049f6d0, 0x1049f77c, NONE, "849da473ab4f8281", [](Cpu& c) { league_base::file_prize_news(c, c.r(3), c.r(4), c.r(5)); })
LEAGUE_BASE_ROUTINE(0x104a0e40, 0x104a10d0, NONE, "6fe39c29dc533cd5", [](Cpu& c) { league_base::file_first_round_news(c, c.r(3)); })
LEAGUE_BASE_ROUTINE(0x104a1350, 0x104a14e8, NONE, "a6266495c1b7b6aa", [](Cpu& c) { league_base::settle_places(c, c.r(3)); })  // +0x38
LEAGUE_BASE_ROUTINE(0x104a44f0, 0x104a47f0, R3, "dc56b9c455ebe2e9", [](Cpu& c) { c.set(3, league_base::file_result_news(c, c.r(3), c.r(4), c.r(5), c.r(6))); })  // +0x48
LEAGUE_BASE_ROUTINE(0x104a47f0, 0x104a4864, R3, "287cc728ab79f2a9", [](Cpu& c) { c.set(3, league_base::record_place(c, c.r(3), c.r(4), c.r(5))); })  // +0x4c
LEAGUE_BASE_ROUTINE(0x104aa0c0, 0x104aa23c, NONE, "dd35f038135e8482", [](Cpu& c) { league_base::record_positions(c, c.r(3)); })  // +0x64
LEAGUE_BASE_ROUTINE(0x104aa3a0, 0x104aa42c, NONE, "9d58a7e9363111d9", [](Cpu& c) { league_base::touch_clubs(c, c.r(3)); })
LEAGUE_BASE_ROUTINE(0x104aa620, 0x104aaae4, R3 | F1, "2e0a265409b0d3bb", [](Cpu& c) { c.set(3, league_base::team_points(c, c.r(3), c.r(4))); })  // +0x8c
