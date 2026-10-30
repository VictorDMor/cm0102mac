INTERNATIONAL_AWARDS_ROUTINE(0x10440620, 0x104406d0, R3, "c59fc850c2cfde96", [](Cpu& c) { c.set(3, international_awards::construct(c, c.r(3), c.r(4))); })
INTERNATIONAL_AWARDS_ROUTINE(0x104406d0, 0x104409c0, NONE, "1084fe55ad2739a3", [](Cpu& c) { international_awards::initialize(c, c.r(3), c.r(4)); })
INTERNATIONAL_AWARDS_ROUTINE(0x104409c0, 0x10440c70, NONE, "4024b029b4feed35", [](Cpu& c) { international_awards::publish_due(c, c.r(3), c.r(4)); })
