FINANCE_ROUTINE(0x100f2680, 0x100f478c, R3, "9855fa5c2a993402", [](Cpu& c) { c.set(3, finance::club_budget(c, c.r(3), c.r(4), c.r(5))); })
FINANCE_ROUTINE(0x100f4790, 0x100f4f14, R3, "1358cc3f6b54d192", [](Cpu& c) { c.set(3, finance::club_budget_for_kind(c, c.r(3), std::int8_t(c.r(4)))); })
FINANCE_ROUTINE(0x100f4f80, 0x100f55a8, NONE, "7a4a511c518ccd68", [](Cpu& c) { finance::refresh_account_budget(c, c.r(3)); })
