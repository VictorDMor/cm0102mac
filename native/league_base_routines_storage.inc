// LEAGUE.CPP, block `storage` (league_base_storage*.cpp): the league's arrays, its copies and the save writer.
LEAGUE_BASE_ROUTINE(0x1049e680, 0x1049e81c, NONE, "28ba9cd3d2d6a31e", [](Cpu& c) { league_base::clone(c, c.r(3), c.r(4), c.r(5)); })
LEAGUE_BASE_ROUTINE(0x1049e820, 0x1049e958, R3, "6703b3c793d24beb", [](Cpu& c) { c.set(3, league_base::copy_base(c, c.r(3), c.r(4))); })
LEAGUE_BASE_ROUTINE(0x104a60e0, 0x104a6440, NONE, "0381a1fa8ae55c70", [](Cpu& c) { league_base::place_club(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
LEAGUE_BASE_ROUTINE(0x104a6440, 0x104a6654, R3, "361ec03623575c1b", [](Cpu& c) { c.set(3, league_base::copy_standings(c, c.r(3))); })
LEAGUE_BASE_ROUTINE(0x104a6660, 0x104a6754, NONE, "3e1dcefc8f713aaf", [](Cpu& c) { league_base::free_standings(c, c.r(3), c.r(4)); })
LEAGUE_BASE_ROUTINE(0x104a6760, 0x104a6850, R3, "20f1931de00dd20f", [](Cpu& c) { c.set(3, league_base::club_list(c, c.r(3), c.r(4), c.r(5))); })  // +0xa8
LEAGUE_BASE_ROUTINE(0x104a8530, 0x104a8f34, NONE, "fb8cb1317e050d8a", [](Cpu& c) { league_base::save(c, c.r(3), c.r(4)); })  // +0x24
