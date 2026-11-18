// COMP.CPP calendar block (comp_calendar.cpp, comp_regions.cpp): in production, except 0x10450ee0, whose allocation-failure
// outcomes (the 1000-row block and the resize) have no check: the game keeps running the original for that entry.
COMP_DRAFT(0x10450ee0, 0x10451470, R3, "2eb7fd6b33b09aa5", [](Cpu& c) { c.set(3, comp::fixture_dates(c, c.r(3), c.r(4))); })
COMP_ROUTINE(0x10451470, 0x10451570, R3, "c4ad7f0959154a78", [](Cpu& c) { c.set(3, std::uint32_t(comp::compare_fixture_dates(c, c.r(3), c.r(4)))); })
COMP_ROUTINE(0x10453190, 0x104532b8, NONE, "da1706ee5f7cd1ba", [](Cpu& c) { comp::first_date(c, c.r(3), c.r(4)); })
COMP_ROUTINE(0x10451720, 0x104517f4, R3, "5b94bed0fc98b9fb", [](Cpu& c) { c.set(3, comp::descriptor_index(c, std::int8_t(c.r(3)))); })
COMP_ROUTINE(0x10451800, 0x10451990, R3, "2376315e4051e176", [](Cpu& c) { c.set(3, comp::fixture_descriptor_code(c, c.r(3), c.r(4))); })
COMP_ROUTINE(0x10451990, 0x10451a2c, R3, "7a62f42fa70ca923", [](Cpu& c) { c.set(3, comp::nation_descriptor_code(c, c.r(3))); })
