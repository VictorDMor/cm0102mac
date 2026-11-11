// MANAGER_MANAGER.CPP assessment block (manager_manager_routines.inc).
MANAGER_MANAGER_ROUTINE(0x101c3860, 0x101c3b08, NONE, "cc550c660aadf166", [](Cpu& c) { manager_manager::post_assessment_message(c, c.r(4), c.r(5), std::int8_t(c.r(6)), std::int8_t(c.r(7)), std::int8_t(c.r(8)), std::int8_t(c.r(9))); c.finish(); })
MANAGER_MANAGER_ROUTINE(0x101c1e20, 0x101c3218, NONE, "fae445abd7126bf9", [](Cpu& c) { manager_manager::assess_verdict(c, c.r(3), c.r(4), c.r(5), c.r(6)); c.finish(); })
MANAGER_MANAGER_ROUTINE(0x101c3220, 0x101c385c, NONE, "85b8bce82636cad6", [](Cpu& c) { manager_manager::assess_manager(c, c.r(3), c.r(4), c.r(5), c.r(6)); c.finish(); })
