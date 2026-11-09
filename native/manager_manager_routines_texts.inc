// MANAGER_MANAGER.CPP manager texts block (manager_manager_routines.inc).
MANAGER_MANAGER_ROUTINE(0x101b76b0, 0x101b782c, NONE, "0ed7988a12492ad8", [](Cpu& c) { manager_manager::post_text(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MANAGER_MANAGER_ROUTINE(0x101b7960, 0x101b7bc0, NONE, "792be2ffa9444717", [](Cpu& c) { manager_manager::post_people_text(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MANAGER_MANAGER_ROUTINE(0x101b5370, 0x101b5b18, NONE, "9b45ee1519f1569c", [](Cpu& c) { manager_manager::appointment_text(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
