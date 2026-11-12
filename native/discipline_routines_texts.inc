// DISCIPLINE.CPP texts block (discipline_routines.inc).
DISCIPLINE_ROUTINE(0x100e7210, 0x100e7490, NONE, "3f1a5f417f077100", [](Cpu& c) { discipline::ban_line(c, c.r(3), c.r(4), c.r(5), std::int32_t(c.r(6))); c.finish(); })
DISCIPLINE_ROUTINE(0x100e7490, 0x100e7780, R3, "6d4ed60ed63310c0", [](Cpu& c) { c.set(3, discipline::describe_ban_bodies(c, c.r(3), c.r(4), c.r(5), std::int32_t(c.r(6)), c.r(7), c.r(8))); c.finish(); })
DISCIPLINE_ROUTINE(0x100e7780, 0x100e7918, NONE, "aca2d84ab1b1f67f", [](Cpu& c) { discipline::status_text(c, c.r(3), c.r(4), c.r(5), std::int32_t(c.r(6)), {c.r(7), c.r(8), c.r(9), c.r(10)}); c.finish(); })
