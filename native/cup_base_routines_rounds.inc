// CUP.CPP, block `rounds` (cup_base_rounds*.cpp): the routines, all in production (CUP_BASE_ROUTINE). The routines call each other
// directly; 0x1047ef50 (79 callers outside the unit, r4-r10 read after it) calls 0x1047e980, 0x1047f3a0 and 0x10481e90,
// 0x1047e980 calls 0x10481ad0 and 0x10485d70, 0x1047f3a0 calls 0x104860a0: they stay together.
CUP_BASE_ROUTINE(0x1047e980, 0x1047ef48, R3, "3c58192e5aca444b", [](Cpu& c) { c.set(3, cup_base::rounds::make_round_draw(c, c.r(3), c.r(4))); })
CUP_BASE_ROUTINE(0x1047ef50, 0x1047f39c, R3, "1e6737fab40d9f04", [](Cpu& c) { c.set(3, cup_base::rounds::advance_round(c, c.r(3), c.r(4))); })
CUP_BASE_ROUTINE(0x1047f3a0, 0x1047fcf8, R3, "4290e87b00665fb9", [](Cpu& c) { c.set(3, cup_base::rounds::schedule_fixtures(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
CUP_BASE_ROUTINE(0x10481ad0, 0x10481e70, NONE, "38f050121683b7ce", [](Cpu& c) { cup_base::rounds::order_tie_sides(c, c.r(3), c.r(4), c.r(5)); })
CUP_BASE_ROUTINE(0x10481e90, 0x10482640, NONE, "f9ec63a8045484d7", [](Cpu& c) { cup_base::rounds::draw_news(c, c.r(3), c.r(4), c.r(5)); })
CUP_BASE_ROUTINE(0x10485d70, 0x10486050, NONE, "5b4e08465f8ba304", [](Cpu& c) { cup_base::rounds::separate_same_nation_ties(c, c.r(3), c.r(4)); })
CUP_BASE_ROUTINE(0x104860a0, 0x10486474, NONE, "5ea2a43eeac98ee7", [](Cpu& c) { cup_base::rounds::missing_side_news(c, c.r(3), c.r(4), c.r(5)); })
