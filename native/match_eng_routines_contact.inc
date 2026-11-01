// Original entries of the contact block.
MATCH_ENG_ROUTINE(0x101f5050, 0x101f50c0, NONE, "19baeffb3bb5f001",
    [](Cpu& c) { match_eng::discipline_c260(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)); })
MATCH_ENG_ROUTINE(0x101f50c0, 0x101f5110, NONE, "e8f46f0868d4b7da",
    [](Cpu& c) { match_eng::discipline_b860(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101f5110, 0x101f5180, NONE, "898a57da85b4e82b",
    [](Cpu& c) { match_eng::discipline_1e00(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)); })
MATCH_ENG_ROUTINE(0x101f5180, 0x101f51e0, NONE, "d6db030528709fed",
    [](Cpu& c) { match_eng::discipline_bec0(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
MATCH_ENG_ROUTINE(0x101f51e0, 0x101f5230, NONE, "8b70ae718681f0a2",
    [](Cpu& c) { match_eng::discipline_3100(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101f5230, 0x101f5280, NONE, "f83f3109c0596a78",
    [](Cpu& c) { match_eng::discipline_2290(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101f5280, 0x101f5350, R3, "4693e8c852acc409",
    [](Cpu& c) { match_eng::is_ball_holder(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101f5350, 0x101f6940, NONE, "39ec180a5112f251",
    [](Cpu& c) { match_eng::describe_contact(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); })
MATCH_ENG_ROUTINE(0x101f6940, 0x101f6c60, NONE, "b03ee673943c0261",
    [](Cpu& c) { match_eng::apply_contact_injury(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101f6c60, 0x101f7100, NONE, "5179c1dad7758994",
    [](Cpu& c) { match_eng::handle_injured_players(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101f7100, 0x101f7280, NONE, "6faa6876ecfac40b",
    [](Cpu& c) { match_eng::choose_man_of_the_match(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101f7280, 0x101f7500, NONE, "671abcd5cc876d61",
    [](Cpu& c) { match_eng::flag_star_performers(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101f7500, 0x101f75a0, NONE, "0b6718cb1f14c309",
    [](Cpu& c) { match_eng::flag_players_on_pitch(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101f75a0, 0x101f7c10, NONE, "d066c570ab561fe2",
    [](Cpu& c) { match_eng::process_contact_events(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101f7c10, 0x101f84e0, NONE, "ad386c9894d02603",
    [](Cpu& c) { match_eng::fill_contact_grid(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101f84e0, 0x101f93d0, NONE, "26bb30a9ca70df6e",
    [](Cpu& c) { match_eng::update_contacts(c, c.r(3)); })
