FINANCE_ROUTINE(0x100f5770, 0x100f7050, R3 | R4, "df0ab9ff3c5de9c8", [](Cpu& c) { const auto bid = finance::bid_limit(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); c.set({{3, std::uint32_t(std::uint64_t(bid) >> 32)}, {4, std::uint32_t(bid)}}); })
FINANCE_ROUTINE(0x100f9f40, 0x100fa1e8, NONE, "4e10db2305eb4cb3", [](Cpu& c) { finance::upgrade_facilities(c, c.r(3), c.r(4)); })
FINANCE_ROUTINE(0x100fa1f0, 0x100fa6d8, NONE, "f3e974bd0427deb3", [](Cpu& c) { finance::book_sampled_income(c, c.r(3), c.r(4)); })
