// FORMATION.CPP: positions, shape and staff slots (see tactics_routines.inc).
TACTICS_ROUTINE(0x10111240, 0x10111310, R3, "80ee2c2a35efde64",
    [](Cpu& c) { c.set(3, std::uint32_t(formation::position(c, c.r(3), std::int8_t(c.r(4)), std::int8_t(c.r(5)) != 0))); })
TACTICS_ROUTINE(0x10111840, 0x10111890, R3, "60e760b0893ebc0e",
    [](Cpu& c) { c.set(3, std::uint32_t(formation::player_at(c, c.r(3), std::int16_t(c.r(4))))); })
TACTICS_ROUTINE(0x10114b10, 0x10114b50, R3, "70f93b2202c80b51",
    [](Cpu& c) { c.set(3, formation::staff_record(c, c.r(3), std::int8_t(c.r(4)))); })
TACTICS_ROUTINE(0x10114b50, 0x10114ba0, R3, "6ef61eb78b417c79",
    [](Cpu& c) { c.set(3, formation::set_staff(c, c.r(3), std::int8_t(c.r(4)), c.r(5))); })
TACTICS_DRAFT(0x10114ba0, 0x10114c00, R3, "7420005ce355e08f",
    [](Cpu& c) { c.set(3, formation::clear_staff(c, c.r(3))); })
TACTICS_ROUTINE(0x10114c00, 0x10114d40, NONE, "83d26f5cecae2572",
    [](Cpu& c) { formation::update_shape(c, c.r(3)); })
TACTICS_ROUTINE(0x10114d40, 0x10115000, R3, "9c04342f6b0de6e0",
    [](Cpu& c) { c.set(3, formation::shape_bit(std::int16_t(c.r(4)))); })
TACTICS_ROUTINE(0x10116780, 0x10116970, R3, "f181cd39f0249f46",
    [](Cpu& c) { c.set(3, formation::split_position(c, std::int16_t(c.r(3)), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x101171b0, 0x10117360, R3, "e48ccd6df700a73f",
    [](Cpu& c) { c.set(3, std::uint32_t(formation::position_order(c, std::int16_t(c.r(3))))); })
