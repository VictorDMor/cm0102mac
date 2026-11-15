FINANCE_ROUTINE(0x100fa6e0, 0x100fcfc4, NONE, "9615b997c1267b61", [](Cpu& c) { finance::daily_pass(c, c.r(3), c.r(4)); })
FINANCE_ROUTINE(0x100fcfd0, 0x100fd194, NONE, "aa84143ae3afb66c", [](Cpu& c) { finance::roll_season_ledgers(c, c.r(3), c.r(4)); })
