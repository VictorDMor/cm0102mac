// screens_instructions: routines written by an author, drafts until verified (see tactics_routines.inc).
// TACTICS_SCREENS.CPP: the team-instructions screen (tactics_screens_instructions.hpp).
TACTICS_DRAFT(0x103b3b50, 0x103b3bd0, R3, "aae98a1b19ee6919",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::destroy(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103b3bd0, 0x103b3c40, R3, "4db247daac0360bb",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::destroy_tactics(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103baca0, 0x103bacd0, R3, "ea6354a8612faa3d",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::club_query(c, c.r(3))); })
TACTICS_DRAFT(0x103c0cc0, 0x103c0cf0, R3, "96060fa973964874",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::team_options(c, c.r(3))); })
TACTICS_DRAFT(0x103c0d10, 0x103c11b0, R3, "881d1d6c5f7f747e",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::handle(c, c.r(3))); })
TACTICS_DRAFT(0x103c11b0, 0x103c1340, R3, "23cf65b6da3ff215",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::copy_screen(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103c1340, 0x103c14a0, R3, "38a5e12e76a267bd",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::copy_tactics(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103c14a0, 0x103c1980, R3, "03c04efeedd76d84",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::copy_player(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103c1980, 0x103c1c00, R3, "946f3d16373e3cca",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::copy_formation(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103c1c00, 0x103c1d40, R3, "595e1eed11c65ab2",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::copy_settings(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103c1d40, 0x103c1da0, R3, "73c61b5d1ca19ba7",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::copy_ten(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103c1da0, 0x103c1f20, R3, "79bdd314a64bfa9c",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::copy_teams(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103c6d80, 0x103c6e80, R3, "bb4adf6c1da42312",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::taker(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_DRAFT(0x103c6e80, 0x103c7050, R3, "b5b83ea12cefe90f",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::set_taker(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_DRAFT(0x103c7050, 0x103c7120, R3, "e30866acbd18e616",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::playmaker(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103c7120, 0x103c7260, R3, "2e597cfe6ff0b51d",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::set_playmaker(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_DRAFT(0x103c80b0, 0x103c8160, R3, "e82e2ec825455c59",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::set_team_option(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103c8e70, 0x103c9260, R3, "f7803a92807412c5",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::slot_number(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8))); })
TACTICS_DRAFT(0x103c9810, 0x103c9960, R3, "efea85e84add116a",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::slot_available(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_DRAFT(0x103ca4e0, 0x103ca730, R3, "dacb3e9b61959a48",
    [](Cpu& c) { c.set(3, tactics_screens::instructions::slot_player(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_DRAFT(0x103c0cf0, 0x103c0d10, R3, "defe725c5ac54c74",
    [](Cpu& c) { tactics_screens::instructions::open(c); })
TACTICS_DRAFT(0x103becb0, 0x103c0cc0, R3, "1ec10a861f0ef77b",
    [](Cpu& c) { tactics_screens::instructions::build(c); })
TACTICS_DRAFT(0x103cbb30, 0x103ce330, R3, "1e286c88c11a6fff",
    [](Cpu& c) {
        c.set(3, tactics_screens::instructions::draw_pitch(c, c.f(1), c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8),
                                                            c.r(9), c.r(10)));
    })
