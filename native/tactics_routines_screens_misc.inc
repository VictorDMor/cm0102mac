// screens_misc: routines written by an author, drafts until verified (see tactics_routines.inc).
// TACTICS_SCREENS.CPP: slot accessors and edits, squad names, applying edits,
// history callbacks, the player-instructions dialog (tactics_screens_misc.hpp).
TACTICS_DRAFT(0x103b3b20, 0x103b3b50, R3, "5480f959137c5bd0",
    [](Cpu& c) { c.set(3, tactics_screens::misc::in_match(c, c.r(3))); })
TACTICS_DRAFT(0x103bac20, 0x103bac60, R3, "9253370bd4410c50",
    [](Cpu& c) { c.set(3, tactics_screens::misc::slot_role(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103bac60, 0x103baca0, R3_R4, "ad9b9ed8ed99028e", [](Cpu& c) {
        const auto value = tactics_screens::misc::slot_instructions(c, c.r(3), c.r(4));
        c.set({{3, std::uint32_t(value >> 32)}, {4, std::uint32_t(value)}});
    })
TACTICS_DRAFT(0x103c4830, 0x103c4870, R3, "ae3e2895aa7fd09c",
    [](Cpu& c) { c.set(3, tactics_screens::misc::slot_staff(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103beae0, 0x103beb40, NONE, "968cb29edb332513",
    [](Cpu& c) { tactics_screens::misc::leave_read_only(c); })
TACTICS_DRAFT(0x103beb40, 0x103bebb0, NONE, "88bd3ac3cd962f1c",
    [](Cpu& c) { tactics_screens::misc::leave_editable(c); })
TACTICS_DRAFT(0x103c82a0, 0x103c8360, R3, "7493b829260fc937",
    [](Cpu& c) { c.set(3, tactics_screens::misc::set_slot_instructions(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_DRAFT(0x103c8c70, 0x103c8d80, R3, "3ea792851c3be386",
    [](Cpu& c) { c.set(3, tactics_screens::misc::set_slot_role(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_DRAFT(0x103c8d80, 0x103c8e70, R3, "d6d1e258a10336b3",
    [](Cpu& c) { c.set(3, tactics_screens::misc::set_slot_staff(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_DRAFT(0x103c9720, 0x103c9810, R3, "8dd97fbf32be62d3",
    [](Cpu& c) { c.set(3, tactics_screens::misc::slot_in_use(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103cac10, 0x103cad40, R3, "4516e3729a17cfd9",
    [](Cpu& c) { c.set(3, tactics_screens::misc::match_player(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_DRAFT(0x103c9260, 0x103c95b0, R3, "ff60cb54da9098ec", [](Cpu& c) {
        c.set(3, tactics_screens::misc::slot_name(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)));
    })
TACTICS_DRAFT(0x103c95b0, 0x103c9720, R3, "cdfafc3f9058b708", [](Cpu& c) {
        c.set(3, tactics_screens::misc::slot_label(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)));
    })
TACTICS_DRAFT(0x103c5b70, 0x103c5e60, R3, "8ad7134fdf94331b",
    [](Cpu& c) { c.set(3, tactics_screens::misc::apply(c, c.r(3))); })
TACTICS_DRAFT(0x103cb760, 0x103cba60, R3, "d12bf4cd987e6d3e",
    [](Cpu& c) { c.set(3, tactics_screens::misc::substitute(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_DRAFT(0x103c4870, 0x103c4b10, R3, "854ddda4f9d4705b",
    [](Cpu& c) { c.set(3, tactics_screens::misc::instructions_event(c, c.r(3))); })
TACTICS_DRAFT(0x103c2010, 0x103c4830, R3, "4b0e1a362d733fc8",
    [](Cpu& c) { c.set(3, tactics_screens::misc::instructions_build(c)); })
