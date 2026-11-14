INJURY_UNIT_ROUTINE(0x10185410, 0x101864a0, NONE, "e48f4463e5529c57", [](Cpu& c) { injury_unit::daily_injury_pass(c, c.r(3)); })
INJURY_UNIT_ROUTINE(0x10188cc0, 0x1018aec4, R3, "8152718bc92a4124", [](Cpu& c) {
    c.set(3, std::uint32_t(injury_unit::register_injury(c, {c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10), c.u8(c.sp() + 0x3b)})));
})
INJURY_UNIT_ROUTINE(0x1018aed0, 0x1018b2b0, NONE, "cf5b7223158f6151", [](Cpu& c) { injury_unit::post_injury_news(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
INJURY_UNIT_ROUTINE(0x1018b6e0, 0x1018b9ec, NONE, "c4ea96e0ec4a8c48", [](Cpu& c) { injury_unit::settle_pending_injury(c, c.r(3), c.r(4), c.r(5)); })
