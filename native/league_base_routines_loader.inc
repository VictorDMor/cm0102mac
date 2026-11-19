// Block loader (league_base_loader.cpp).
LEAGUE_BASE_ROUTINE(0x104a6850, 0x104a8530, R3, "87af26d1561f124e", [](Cpu& c) { c.set(3, league_base::load_save(c, c.r(3), c.r(4))); })  // +0x20
