FINANCE_ROUTINE(0x100fd210, 0x100fe638, NONE, "555c3980eaa39bac", [](Cpu& c) { finance::review_club(c, c.r(3), c.r(4)); })
FINANCE_ROUTINE(0x100fe640, 0x100fefa0, R3, "88deef72427a9901", [](Cpu& c) { c.set(3, finance::donation(c, c.r(3), c.r(4), c.r(5))); })
