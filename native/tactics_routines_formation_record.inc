// formation_record: routines written by an author, drafts until verified (see tactics_routines.inc).
// FORMATION.CPP: building records, team options, player instructions and roles, position names.
TACTICS_ROUTINE(0x10110c80, 0x10110f40, R3, "70f97a07a98dbd74",
    [](Cpu& c) { c.set(3, formation::initialise(c, c.r(3))); })
TACTICS_ROUTINE(0x10110f40, 0x10111240, R3, "648f2b4e6218bc68",
    [](Cpu& c) { c.set(3, formation::initialise_from_tactics(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x10113b60, 0x10113e10, R3, "15563483a548771c",
    [](Cpu& c) { c.set(3, formation::set_team_option(c, c.r(3), std::int32_t(c.r(4)))); })
TACTICS_ROUTINE(0x10113e30, 0x101149f0, R3, "05939c9a39185f36",
    [](Cpu& c) { c.set(3, formation::set_instruction(c, c.r(3), std::int8_t(c.r(4)), std::uint64_t(c.r(5)) << 32 | c.r(6))); })
TACTICS_ROUTINE(0x10114a00, 0x10114b10, R3, "c5409ef9044e2810",
    [](Cpu& c) { c.set(3, formation::set_role(c, c.r(3), std::int8_t(c.r(4)), c.r(5))); })
TACTICS_ROUTINE(0x10116970, 0x10116c90, R3, "8f4676f9e6436fe1",
    [](Cpu& c) { c.set(3, formation::position_name(c, std::int16_t(c.r(3)), c.r(4), std::int32_t(c.r(5)), c.r(6))); })
TACTICS_ROUTINE(0x10116c90, 0x10116f80, R3, "2c50cb11b6adcb8e",
    [](Cpu& c) { c.set(3, formation::position_abbreviation(c, std::int16_t(c.r(3)), c.r(4), std::int32_t(c.r(5)), c.r(6))); })
TACTICS_ROUTINE(0x10116f80, 0x101171b0, R3, "22dc570e69a26d69",
    [](Cpu& c) { c.set(3, formation::position_part_name(c, std::int16_t(c.r(3)), c.r(4), std::int32_t(c.r(5)))); })
TACTICS_ROUTINE(0x101182a0, 0x10118a40, R3, "8d9cbf9e7ba56fa9",
    [](Cpu& c) { c.set(3, formation::no_wide_midfield_or_attack(c, c.r(3))); })
TACTICS_ROUTINE(0x10118a40, 0x10118ec0, R3, "3c44ea059293f4b5",
    [](Cpu& c) { c.set(3, formation::no_wide_defence(c, c.r(3))); })
