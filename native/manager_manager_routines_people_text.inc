// MANAGER_MANAGER.CPP people text block (manager_manager_routines.inc).
MANAGER_MANAGER_ROUTINE(0x10197e00, 0x10197e80, R3, "02fd04f217214bb0", [](Cpu& c) { manager_manager::club_first_band(c, c.r(3), c.r(4)); })
MANAGER_MANAGER_ROUTINE(0x10197e80, 0x10197f00, R3, "43688c9a5fd36ddc", [](Cpu& c) { manager_manager::club_second_band(c, c.r(3), c.r(4)); })
MANAGER_MANAGER_ROUTINE(0x10197f00, 0x101980a0, NONE, "0135381648802850", [](Cpu& c) { manager_manager::club_upcoming_entry(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MANAGER_MANAGER_ROUTINE(0x101980a0, 0x1019c584, NONE, "84886375fb12e8b2", [](Cpu& c) { manager_manager::club_people_text(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
