// tactics_setup: routines written by an author, drafts until verified (see tactics_routines.inc).
// FORMATION.CPP: the .tct formation files (formation_files.hpp).
TACTICS_ROUTINE(0x10115000, 0x10115cd0, R3, "c646c1aef1c9f1d6",
    [](Cpu& c) { c.set(3, formation::load_from_disk(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x10115cd0, 0x10116560, R3, "16d18bb2ffdb69ae",
    [](Cpu& c) { c.set(3, formation::save_to_disk(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x10117fa0, 0x10118180, R3, "c86fc8a8e36b5527",
    [](Cpu& c) { c.set(3, formation::read_lock(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x10118180, 0x10118210, R3, "04a2d2e7f32bfaf7",
    [](Cpu& c) { c.set(3, formation::lock_state(c, 0x10118180, c.r(3), 0)); })
TACTICS_DRAFT(0x10118210, 0x101182a0, R3, "747a4dc0d5f27804",
    [](Cpu& c) { c.set(3, formation::lock_state(c, 0x10118210, c.r(3), 1)); })
// TACTICS.CPP: setting up CTACTICS and the saved game's tactics.dat (tactics_setup.hpp).
TACTICS_ROUTINE(0x103acf80, 0x103af060, R3, "0505078baac4b91a",
    [](Cpu& c) { c.set(3, tactics::setup::construct(c, c.r(3), c.r(4))); })
TACTICS_ROUTINE(0x103b1d00, 0x103b3290, R3, "a711f71e2dd1aeb9",
    [](Cpu& c) { c.set(3, tactics::setup::load(c, c.r(3), c.r(4))); })
TACTICS_DRAFT(0x103b3290, 0x103b3830, R3, "858a4925997e1d09",
    [](Cpu& c) { c.set(3, tactics::setup::save(c, c.r(3), c.r(4))); })
