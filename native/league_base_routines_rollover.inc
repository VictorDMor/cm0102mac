// LEAGUE.CPP block `rollover` (league_base_rollover*.cpp): entry, end, results, digest of the original body, hook.
LEAGUE_BASE_ROUTINE(0x1049f320, 0x1049f53c, NONE, "4ad67bc0f29180a5", [](Cpu& c) { league_base::prize_money(c, c.r(3)); })
LEAGUE_BASE_ROUTINE(0x1049f780, 0x1049f8d8, NONE, "e33505ebe5f587fe", [](Cpu& c) { league_base::move_club(c, c.r(4), c.r(5), c.r(6)); })
LEAGUE_BASE_ROUTINE(0x1049f8e0, 0x1049f998, NONE, "007b8ce62fdbba1a", [](Cpu& c) { league_base::set_division(c, c.r(4), c.r(5)); })
LEAGUE_BASE_DRAFT(0x104a0ab0, 0x104a0e34, NONE, "8f2e000e491dc475", [](Cpu& c) { league_base::announce_pairings(c, c.r(3), c.r(4)); })
LEAGUE_BASE_ROUTINE(0x104a1540, 0x104a1c30, R3, "19102a0314f2265f", [](Cpu& c) { c.set(3, league_base::daily_update(c, c.r(3), c.r(4), c.r(5))); })  // +0x1c
LEAGUE_BASE_ROUTINE(0x104a1c30, 0x104a26a8, NONE, "565f9b8e241261c3", [](Cpu& c) { league_base::end_of_season(c, c.r(3)); })  // +0x9c
LEAGUE_BASE_ROUTINE(0x104a8f40, 0x104a9160, R3, "f9e8d8f7d900ed12", [](Cpu& c) { c.set(3, league_base::finished(c, c.r(3), c.r(4), c.r(5))); })  // +0x18
LEAGUE_BASE_ROUTINE(0x104a9160, 0x104a9208, NONE, "1de81e820b666e0e", [](Cpu& c) { league_base::chain_next(c, c.r(3), c.r(4)); })  // +0xb8
LEAGUE_BASE_ROUTINE(0x104a9210, 0x104a95f8, NONE, "e8e17e2b77bf383e", [](Cpu& c) { league_base::exchange_clubs(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
