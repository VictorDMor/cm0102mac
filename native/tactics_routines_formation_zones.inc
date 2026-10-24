// formation_zones: routines written by an author, drafts until verified (see tactics_routines.inc).
// FORMATION.CPP: players' spots per pitch cell, the zone grids and position helpers.
TACTICS_ROUTINE(0x10111310, 0x10111630, R3, "cfe30978746f4198",
    [](Cpu& c) { c.set(3, formation::set_position(c, c.r(3), c.r(4), c.r(5), std::int8_t(c.r(6)))); })
TACTICS_ROUTINE(0x10111630, 0x10111840, R3, "24bc135e31a2c6d4",
    [](Cpu& c) { c.set(3, formation::central_in_line(c, c.r(3), std::int16_t(c.r(4)), c.r(5))); })
TACTICS_ROUTINE(0x10111890, 0x10111ba0, R3, "d87719f10838f060",
    [](Cpu& c) { c.set(3, formation::swap_players(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x10111ba0, 0x10111d30, R3, "6261b6e96f274a3d",
    [](Cpu& c) { c.set(3, formation::order_players(c, c.r(3), c.r(4), std::int32_t(c.r(5)))); })
TACTICS_ROUTINE(0x10111d30, 0x10111e60, R3, "b8548a9ab137d7ea",
    [](Cpu& c) { c.set(3, formation::displaced(c, c.r(3), std::int8_t(c.r(4)), c.r(5))); })
TACTICS_ROUTINE(0x10111e60, 0x101124c0, R3, "6df0b943733c260e",
    [](Cpu& c) { c.set(3, formation::move_spot(c, c.r(3), c.r(4), std::int8_t(c.r(5)), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x101124c0, 0x10112690, R3, "34d0cd7bac2fad8a",
    [](Cpu& c) { c.set(3, formation::spot(c, c.r(3), std::int8_t(c.r(4)), std::int8_t(c.r(5)), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x10112690, 0x10112850, R3, "2d76c645cde7739f",
    [](Cpu& c) { c.set(3, formation::move_spots(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x10112850, 0x10112a00, R3, "a8570ae8a13d8af0",
    [](Cpu& c) { c.set(3, formation::spots(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x10112a00, 0x10112b90, R3, "0ba87f725cec92a6",
    [](Cpu& c) { c.set(3, formation::move_all_spots(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x10112b90, 0x10112d20, R3, "fdbc6be41b5ff8c4",
    [](Cpu& c) { c.set(3, formation::all_spots(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x10112d20, 0x10112f00, R3, "2ca99dba0d0efe68",
    [](Cpu& c) { c.set(3, std::uint32_t(formation::player_at_spot(c, c.r(3), c.r(4), c.r(5), c.r(6)))); })
TACTICS_ROUTINE(0x10112f00, 0x10113160, R3, "edef36b06aeb3781",
    [](Cpu& c) { c.set(3, formation::swap_spots(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x10113160, 0x10113430, R3, "d53de2f1cd1aded5",
    [](Cpu& c) { c.set(3, formation::mirror_spots(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x10113430, 0x10113550, R3, "269c05ba04d2f07f",
    [](Cpu& c) { c.set(3, formation::has_custom_spot(c, c.r(3), c.r(4))); })
TACTICS_ROUTINE(0x10113550, 0x10113940, R3, "1782d73a2124b3c7",
    [](Cpu& c) { c.set(3, formation::custom_spot(c, c.r(3), std::int8_t(c.r(4)), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x10113940, 0x10113b60, R3, "08635ef08fff64e6",
    [](Cpu& c) { c.set(3, formation::shared_spot(c, c.r(3), std::int8_t(c.r(4)), c.r(5), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x10116560, 0x10116780, R3, "a52a647c1702dcba",
    [](Cpu& c) { c.set(3, formation::update_zone_grid(c, c.r(3))); })
// Reads the caller's r0 / r5 too (line indices of codes without a line bit).
TACTICS_ROUTINE(0x10117360, 0x10117490, R3, "413837786d0acfcc",
    [](Cpu& c) { c.set(3, std::uint32_t(formation::between_positions(c, c.r(3), c.r(4), c.r(0), c.r(5)))); })
TACTICS_ROUTINE(0x10117490, 0x10117690, R3, "c30b87b8e46be302",
    [](Cpu& c) { c.set(3, formation::natural_positions(c, c.r(3))); })
TACTICS_ROUTINE(0x10117690, 0x10117fa0, R3, "8d24ae3b7c2d9de9",
    [](Cpu& c) { c.set(3, formation::default_spot(c, std::int16_t(c.r(3)), c.r(4), c.r(5), c.r(6))); })
