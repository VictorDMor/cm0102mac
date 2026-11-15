FINANCE_ROUTINE(0x100f7270, 0x100f7ce4, NONE, "f32e5b9fe36b53be", [](Cpu& c) { finance::matchday_takings(c, c.r(3), c.r(4)); })
FINANCE_ROUTINE(0x100f7cf0, 0x100f84d4, R3, "6ec70f6f99bf4e84", [](Cpu& c) { c.set(3, finance::expand_ground(c, c.r(3), std::int32_t(c.r(4)), std::int32_t(c.r(5)), c.r(6), c.r(7)) ? 1 : 0); })
FINANCE_ROUTINE(0x100f8780, 0x100f95d8, NONE, "ddc3ee3efaf88e50", [](Cpu& c) { finance::split_fixture_income(c, c.r(3), c.r(4), c.r(5)); })
FINANCE_ROUTINE(0x100f95e0, 0x100f9afc, R3, "550058a60d7fcef9", [](Cpu& c) { c.set(3, std::uint32_t(finance::fixture_income(c, c.r(3), c.r(4), std::int32_t(c.r(5))))); })
