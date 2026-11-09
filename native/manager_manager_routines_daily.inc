// MANAGER_MANAGER.CPP daily update block (manager_manager_routines.inc).
MANAGER_MANAGER_DRAFT(0x10196390, 0x10197648, NONE, "6128ccdb2447b3cc", [](Cpu& c) { manager_manager::daily(c, c.r(3)); })
MANAGER_MANAGER_DRAFT(0x10197650, 0x101976a4, R3, "1ce385392ca42954", [](Cpu& c) { manager_manager::add_applicant(c, c.r(3), c.r(4), c.r(5)); })
MANAGER_MANAGER_DRAFT(0x101976b0, 0x1019795c, NONE, "5fe80e781e5055a1", [](Cpu& c) { manager_manager::end_post(c, c.r(3), c.r(4), c.r(5)); })
MANAGER_MANAGER_DRAFT(0x10197960, 0x10197df4, NONE, "80f1ecf7149d885e", [](Cpu& c) { manager_manager::clear_post(c, c.r(3), c.r(4), c.r(5)); })
MANAGER_MANAGER_DRAFT(0x101b5090, 0x101b5368, NONE, "eea1ce80c179a1b6", [](Cpu& c) { manager_manager::announce(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
