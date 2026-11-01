// Original entries of the rating block: candidate picking and small helpers (0x101fce60..0x101fd100, 0x101fea10..0x101fee90).
MATCH_ENG_ROUTINE(0x101fce60, 0x101fcf20, R3, "df16c178ad3cd859",
    [](Cpu& c) { match_eng::kind_82_free(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101fcf20, 0x101fd000, R3, "aa943de2748d69ef",
    [](Cpu& c) { match_eng::kind_96_free(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101fd000, 0x101fd100, R3, "42caa1ad2d03324d",
    [](Cpu& c) { match_eng::side_flagged(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101fd100, 0x101fd6b0, R3, "896c651c95392cca",
    [](Cpu& c) { match_eng::pick_opponent(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101fea10, 0x101fea90, R3, "7c9e33d041908bcd",
    [](Cpu& c) { match_eng::ball_cell_side(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101feae0, 0x101feba0, R3, "de9a9d2f7476aafd",
    [](Cpu& c) { match_eng::totals_not_ahead(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101feba0, 0x101fec50, R3, "ff848f4e9eae7778",
    [](Cpu& c) { match_eng::count_matching_players(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101fec50, 0x101fed00, R3, "7540d216a78cfd5d",
    [](Cpu& c) { match_eng::first_matching_player(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101fed00, 0x101fedd0, R3, "dc126182f41a8459",
    [](Cpu& c) { match_eng::first_eligible_player(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101fedd0, 0x101fee30, NONE, "5b860e248862dcef",
    [](Cpu& c) { match_eng::apply_record_a(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101fee30, 0x101fee90, NONE, "ea1ccc07a05bdc80",
    [](Cpu& c) { match_eng::apply_record_b(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101fee90, 0x101fef60, NONE, "c04a0852d36bbf12",
    [](Cpu& c) { match_eng::free_record_tables(c); })
