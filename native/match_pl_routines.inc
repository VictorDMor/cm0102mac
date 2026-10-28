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

// Tactical context and strategy queries.
MATCH_PL_ROUTINE(0x102373a0, 0x102373c0, R3, "5559ca3a48a48fd4",
    [](Cpu& c) { match_pl::larger_value(c, std::int32_t(c.r(3)), std::int32_t(c.r(4))); })
MATCH_PL_ROUTINE(0x102373c0, 0x10237420, R3, "75ee49061684f182",
    [](Cpu& c) { match_pl::marking_trait(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10237b00, 0x10237b10, R3, "23fcb7f81b151ea9",
    [](Cpu& c) { match_pl::heading(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102775c0, 0x102775e0, R3, "731686a5bc14dc76",
    [](Cpu& c) { match_pl::has_possession(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10283540, 0x10283590, R3, "32acc34d9366af03",
    [](Cpu& c) { match_pl::contains_marker(c, c.r(3), c.r(4)); })
MATCH_PL_ROUTINE(0x10283590, 0x10283620, R3, "86411115cf6bd64f",
    [](Cpu& c) { match_pl::remove_marker(c, c.r(3), c.r(4)); })
MATCH_PL_ROUTINE(0x10283690, 0x102836a0, R3, "e3c4cade64c322c0",
    [](Cpu& c) { match_pl::side(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10285750, 0x10285790, R3, "0f2e8a18c6b6e758",
    [](Cpu& c) { match_pl::team_line(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10285790, 0x102857c0, R3, "6d76edf0ec8aa066",
    [](Cpu& c) { match_pl::match_context(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10285ed0, 0x10285ee0, R3, "5a304991323a0d32",
    [](Cpu& c) { match_pl::add_pressure(c, c.r(3), c.r(4)); })
MATCH_PL_ROUTINE(0x10285ee0, 0x10285ef0, R3, "130df3dbd61ba572",
    [](Cpu& c) { match_pl::set_pressure(c, c.r(3), c.r(4)); })
MATCH_PL_ROUTINE(0x102860c0, 0x102861b0, R3, "fc4903e0a58bbfd4",
    [](Cpu& c) { match_pl::forward_option(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102861b0, 0x102862b0, R3, "e1489c04752370a7",
    [](Cpu& c) { match_pl::wide_option(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102862b0, 0x10286320, R3, "230339173acc73a3",
    [](Cpu& c) { match_pl::strategy_bit21(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10286320, 0x10286390, R3, "86402670701f8c0e",
    [](Cpu& c) { match_pl::strategy_bit25(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10286390, 0x10286400, R3, "96f4297b2e012314",
    [](Cpu& c) { match_pl::strategy_bit29(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10286400, 0x10286470, R3, "dd3a233c96dbf99d",
    [](Cpu& c) { match_pl::strategy_bit27(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10286470, 0x102864b0, R3, "e67fbee1a6bb7c13",
    [](Cpu& c) { match_pl::strategy_flags(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102864b0, 0x102864f0, R3, "d669feb2d4d2278e",
    [](Cpu& c) { match_pl::alternate_strategy_flags(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102864f0, 0x102865a0, R3, "54a8b3935b5cefd4",
    [](Cpu& c) { match_pl::team_or_slot_status(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10289550, 0x10289560, R3, "37ba5ab0ecdd9b25",
    [](Cpu& c) { match_pl::cached_role(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10289560, 0x102895a0, R3, "eef5843d04f65a71",
    [](Cpu& c) { match_pl::current_role(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102895a0, 0x102895e0, R3, "7c054bd4df2c4175",
    [](Cpu& c) { match_pl::previous_role(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102895e0, 0x10289620, R3, "7bed26fd08fa5736",
    [](Cpu& c) { match_pl::secondary_role(c, c.r(3)); })

// Geometry, intensity and one-draw condition recovery.
MATCH_PL_ROUTINE(0x1022e760, 0x1022e7e0, F1, "a29c182df5561795",
    [](Cpu& c) { match_pl::distance_to(c, c.r(3), std::int8_t(c.r(4)), std::int8_t(c.r(5))); })
MATCH_PL_ROUTINE(0x10235db0, 0x10235ea0, R3|F1, "df03e33747b7a155",
    [](Cpu& c) { match_pl::improve_condition(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10245050, 0x10245160, R3, "ef1f4ff8f6db8159",
    [](Cpu& c) { match_pl::can_face(c, c.r(3), c.r(4), c.r(5)); })
MATCH_PL_ROUTINE(0x10245160, 0x10245220, F1, "e8656bbbde63591c",
    [](Cpu& c) { match_pl::distance_to_goal(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10270ba0, 0x10270c70, R3, "bc86e3ea793e5196",
    [](Cpu& c) { match_pl::near_own_goal(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102865a0, 0x10286650, R3|F1, "ca7cb93edc9abb87",
    [](Cpu& c) { match_pl::scale_intensity(c, c.r(3), c.f(1)); })
MATCH_PL_ROUTINE(0x102875f0, 0x102876c0, R3|F1, "02510e3272e14414",
    [](Cpu& c) { match_pl::selection_score(c, c.r(3), c.r(4)); })

MATCH_PL_ROUTINE(0x10226520, 0x102270f0, R3|F1, "682a50b1abbedbb1",
    [](Cpu& c) { match_pl::sample_range(c, c.r(3)); })

// Transient attributes and live condition maintenance.
MATCH_PL_ROUTINE(0x10235ea0, 0x102373a0, R3|F1, "d0d1be29af04de46",
    [](Cpu& c) { match_pl::refresh_attributes(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102393c0, 0x10239590, R3|F1, "bca74f263a6e8fcd",
    [](Cpu& c) { match_pl::refresh_condition(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10239590, 0x10239790, R3|F1, "6db3598e8618a19e",
    [](Cpu& c) { match_pl::consume_condition(c, c.r(3), std::int16_t(c.r(5)), c.f(1)); })
MATCH_PL_ROUTINE(0x10239790, 0x10239d10, R3|F1, "91a71ce89d91068e",
    [](Cpu& c) { match_pl::restore_condition(c, c.r(3), std::int8_t(c.r(4))); })

// Coordinate, heading and marking decisions.
MATCH_PL_ROUTINE(0x102374f0, 0x10237830, R3, "2f5d2fee693f48f2",
    [](Cpu& c) { match_pl::set_coordinates(c, c.r(3), c.r(4), c.r(5)); })
MATCH_PL_ROUTINE(0x10237830, 0x10237b00, R3, "d30e61b3045d1c73",
    [](Cpu& c) { match_pl::set_heading(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_PL_ROUTINE(0x10281780, 0x102818f0, R3|F1, "6cd1c5a0fba9968f",
    [](Cpu& c) { match_pl::can_cover_goal(c, c.r(3), c.r(4), c.r(5)); })
MATCH_PL_ROUTINE(0x102818f0, 0x10281af0, R3|F1, "5f95d1d1cfbc11f8",
    [](Cpu& c) { match_pl::turn_direction(c, c.r(3), std::int16_t(c.r(4)), c.f(1), c.r(10)); })
MATCH_PL_ROUTINE(0x10285ef0, 0x102860c0, R3, "58d18f4f935227ef",
    [](Cpu& c) { match_pl::marked_opponent(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10227140, 0x10227460, R3|F1, "b4cf921550e45315",
    [](Cpu& c) { match_pl::readiness_rating(c, c.r(3)); })
MATCH_PL_ROUTINE(0x10288230, 0x102884d0, R3, "b197398a8a58b3a1",
    [](Cpu& c) { match_pl::publish_incident(c, c.r(3)); })
MATCH_PL_ROUTINE(0x102857c0, 0x10285ed0, R3, "02d03e677ade5746",
    [](Cpu& c) { match_pl::adjust_action_bias(c, c.r(3), c.r(4)); })
MATCH_PL_ROUTINE(0x10288f30, 0x10289550, R3|F1, "7caa4433c52787bc",
    [](Cpu& c) { match_pl::adjust_morale(c, c.r(3), c.r(4)); })

MATCH_PL_ROUTINE(0x10244c60, 0x10245050, R3|F1, "c4a854add6bd1dcd",
    [](Cpu& c) { match_pl::accepts_target(c, c.r(3), c.r(4), c.r(5), std::int8_t(c.r(6)), c.r(9), c.f(1), c.f(2)); })
