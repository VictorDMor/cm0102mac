// Original entries of the first MATCH_PL state/spatial-query block.
MATCH_PL_ROUTINE(0x102238a0, 0x10223ac0, R3, "f723a2754ee72021",
    [](Cpu& c) { match_pl::initialize(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102270f0, 0x10227140, R3, "bfddc345d197acea",
    [](Cpu& c) { match_pl::destroy(c, c.r(3), std::int16_t(c.r(4))); })
MATCH_PL_ROUTINE(0x1022e710, 0x1022e760, NONE, "4b24fef090e818ac",
    [](Cpu& c) { match_pl::clear_match_target(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10237420, 0x102374f0, R3, "88dd6dee2d2edc13",
    [](Cpu& c) { match_pl::at_attacking_goal(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10237b10, 0x10237b50, R3, "f71ae0fa59ccd7ac",
    [](Cpu& c) { match_pl::team_instruction(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10239d10, 0x10239dc0, R3, "3a0cdcdb8808ff80",
    [](Cpu& c) { match_pl::in_goal_area(c, c.r(3), std::int8_t(c.r(4))); })
MATCH_PL_ROUTINE(0x10239dc0, 0x10239e50, R3, "669a8ae62eb80937",
    [](Cpu& c) { match_pl::coordinates_in_goal_area(c, std::int8_t(c.r(4)), std::int8_t(c.r(5)), std::int8_t(c.r(6))); })
MATCH_PL_ROUTINE(0x10239e50, 0x10239f10, NONE, "10751487df5b132b",
    [](Cpu& c) { match_pl::advance_row(c, c.r(3), std::int8_t(c.r(4))); })
MATCH_PL_ROUTINE(0x10251240, 0x10251270, R3, "3c0b299daf9ce161",
    [](Cpu& c) { match_pl::on_goal_line(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10272050, 0x102720a0, R3, "682ab8d1fd5d1aef",
    [](Cpu& c) { match_pl::in_defensive_third(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102720a0, 0x102720d0, R3, "fa1a7866fe4b74ec",
    [](Cpu& c) { match_pl::wide_column(c, std::int8_t(c.r(4))); })
MATCH_PL_ROUTINE(0x102720d0, 0x10272100, R3, "1e8f52c55bd68faa",
    [](Cpu& c) { match_pl::in_wide_column(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10272100, 0x10272150, R3, "38cc74365987e3fb",
    [](Cpu& c) { match_pl::in_attacking_third(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10272150, 0x102721a0, R3, "89509a4e39327ef2",
    [](Cpu& c) { match_pl::in_attacking_band(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102721a0, 0x102721f0, R3, "2287300b48615787",
    [](Cpu& c) { match_pl::in_own_half(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102721f0, 0x10272240, R3, "d0b8d64e3d1be30e",
    [](Cpu& c) { match_pl::row_in_own_half(c, c.r(3), std::int8_t(c.r(4))); })
MATCH_PL_ROUTINE(0x10283620, 0x10283690, R3, "0853efa40dfb85bb",
    [](Cpu& c) { match_pl::team_crossed_half(c, c.r(3)); })
MATCH_PL_ROUTINE(0x1028abc0, 0x1028ac50, R3, "198352f7db2f1518",
    [](Cpu& c) { match_pl::both_roles_masked(c, c.r(3)); })
MATCH_PL_ROUTINE(0x1028ac50, 0x1028acf0, R3, "c47ef5917085e55c",
    [](Cpu& c) { match_pl::role_distance(c, c.r(3)); })
MATCH_PL_ROUTINE(0x1028acf0, 0x1028ad60, R3, "8f865608b13eb5f9",
    [](Cpu& c) { match_pl::role_rank(c, c.r(4)); })

MATCH_PL_ROUTINE(0x1022e3a0, 0x1022e710, R3, "ac962997d13f2944",
    [](Cpu& c) { match_pl::acquire_ball(c, c.r(3)); })
