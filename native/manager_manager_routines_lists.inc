// MANAGER_MANAGER.CPP lists block (manager_manager_routines.inc).
MANAGER_MANAGER_ROUTINE(0x101b1620, 0x101b171c, R3, "8b3e2c4cdca68565", [](Cpu& c) { manager_manager::list_contains(c, c.r(3), c.r(4)); })
