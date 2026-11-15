// HUMAN_MANAGER.CPP block "lifecycle".
HUMAN_MANAGER_ROUTINE(0x10145af0, 0x10145d64, R3, "6907c35dc3c94a5b", [](Cpu& c) { c.set(3, human_manager::build_table(c, c.r(3), c.r(4))); })
HUMAN_MANAGER_ROUTINE(0x10145d70, 0x10145dcc, R3, "20ebf70bacff44fe", [](Cpu& c) { c.set(3, human_manager::destroy_record(c, c.r(3), c.r(4))); })
HUMAN_MANAGER_ROUTINE(0x10145dd0, 0x10145e0c, R3, "8f38a1c94dadda0c", [](Cpu& c) { c.set(3, human_manager::destroy_tactic(c, c.r(3), c.r(4))); })
HUMAN_MANAGER_ROUTINE(0x10145e10, 0x10145e6c, R3, "fe9bf773d2c0ee85", [](Cpu& c) { c.set(3, human_manager::construct_record(c, c.r(3))); })
HUMAN_MANAGER_ROUTINE(0x10145e70, 0x10145ea0, R3, "da19cd6cdd2971e1", [](Cpu& c) { c.set(3, human_manager::construct_tactic(c, c.r(3))); })
HUMAN_MANAGER_ROUTINE(0x10145ea0, 0x10145fd0, R3, "c84e7a8c93215ad7", [](Cpu& c) { c.set(3, human_manager::destroy_table(c, c.r(3), c.r(4))); })
HUMAN_MANAGER_ROUTINE(0x10145fd0, 0x101466b8, R3, "3554b831aa14fd8d", [](Cpu& c) { c.set(3, human_manager::start_human(c, c.r(3), c.r(4))); })
