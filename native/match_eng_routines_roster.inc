// Original entries of the roster block.
MATCH_ENG_ROUTINE(0x101f1530, 0x101f1860, NONE, "66df54b480f08982",
    [](Cpu& c) { match_eng::player_arrives(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101f1880, 0x101f1930, NONE, "ad79d340b4dbae98",
    [](Cpu& c) { match_eng::put_off_pitch(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101f1950, 0x101f1dc0, NONE, "1c8fc32053303d7a",
    [](Cpu& c) { match_eng::leave_cell(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101f1e10, 0x101f21c0, R3, "4b648fe99d152565",
    [](Cpu& c) { match_eng::side_goalkeeper(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101f21d0, 0x101f2400, R3, "9e83bdfb2b110a9d",
    [](Cpu& c) { match_eng::tie_totals(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)); })
MATCH_ENG_ROUTINE(0x101f2400, 0x101f3480, NONE, "ecfa99dba0356baf",
    [](Cpu& c) { match_eng::period_statistics(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101f3480, 0x101f3a30, NONE, "bb112d80589bbcd5",
    [](Cpu& c) { match_eng::share_percentages(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101f3a30, 0x101f4860, R3, "9a6593d20ae0d3c9",
    [](Cpu& c) { match_eng::pick_player(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MATCH_ENG_ROUTINE(0x101f4860, 0x101f4900, NONE, "99ac88ff60a1997b",
    [](Cpu& c) { match_eng::adjust_side_ratings(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101f4900, 0x101f5050, NONE, "c0934395859e5e67",
    [](Cpu& c) { match_eng::count_event(c, c.r(3), c.r(4), c.r(5)); })
