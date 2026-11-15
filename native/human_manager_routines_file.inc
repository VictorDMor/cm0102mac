// HUMAN_MANAGER.CPP block "file": the humans' records read from and written to a saved game's file.
HUMAN_MANAGER_ROUTINE(0x1014a300, 0x1014b094, R3, "9e4d66b6d8ccf15c", [](Cpu& c) { c.set(3, human_manager::read_humans(c, c.r(3), c.r(4))); })
HUMAN_MANAGER_ROUTINE(0x1014b0a0, 0x1014b68c, R3, "0f2a244a973df112", [](Cpu& c) { c.set(3, human_manager::write_humans(c, c.r(3), c.r(4))); })
