// LEAGUE.CPP block `table` (league_base_table*.cpp): entry, end, results, digest of the original body, hook.
// The standings' order (league_base_table_order.cpp).
LEAGUE_BASE_ROUTINE(0x1049eb30, 0x1049f030, R3, "a51811b4130d4028", [](Cpu& c) { c.set(3, league_base::sort_standings(c, c.r(3), c.r(4))); })
LEAGUE_BASE_DRAFT(0x1049f030, 0x1049f318, NONE, "c5d487064e9387a4", [](Cpu& c) { league_base::rank_by_head_to_head(c, c.r(3), c.r(4)); })
LEAGUE_BASE_ROUTINE(0x1049f9a0, 0x1049fc74, R3, "c6794985f8027236", [](Cpu& c) { c.set(3, std::uint32_t(league_base::compare_standings(c, c.r(3), c.r(4)))); })
LEAGUE_BASE_ROUTINE(0x1049fc80, 0x1049fdb4, R3, "c9a688a53f424f32", [](Cpu& c) { c.set(3, std::uint32_t(league_base::compare_standings_by_name(c, c.r(3), c.r(4)))); })
LEAGUE_BASE_ROUTINE(0x1049fdc0, 0x1049ff88, R3, "e2d2740b622b11ca", [](Cpu& c) { c.set(3, std::uint32_t(league_base::compare_head_to_head(c, c.r(3), c.r(4)))); })
LEAGUE_BASE_ROUTINE(0x1049ff90, 0x104a00a0, R3, "732479d41fae0fbb", [](Cpu& c) { c.set(3, std::uint32_t(league_base::compare_tied(c, c.r(3), c.r(4)))); })
// Match results (league_base_table_result.cpp).
LEAGUE_BASE_ROUTINE(0x104a26b0, 0x104a28c4, R3, "2d5c5506a1a5c847", [](Cpu& c) { c.set(3, league_base::result_decides_title(c, c.r(3), c.r(4), c.r(5))); })
LEAGUE_BASE_ROUTINE(0x104a3930, 0x104a3b88, R3, "0ffff4a1f2c39230", [](Cpu& c) { c.set(3, league_base::apply_result(c, c.r(3), c.r(4), c.r(5))); })  // +0xc
LEAGUE_BASE_ROUTINE(0x104a3b90, 0x104a44ec, NONE, "985fdabf82945024", [](Cpu& c) { league_base::apply_to_rows(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
LEAGUE_BASE_ROUTINE(0x104a4870, 0x104a4968, NONE, "e1f98585a45d59fd", [](Cpu& c) { league_base::count_head_to_head(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); })
// Fixture slots and queries (league_base_table_queries.cpp).
LEAGUE_BASE_ROUTINE(0x104a9aa0, 0x104a9bc0, R3, "f0b2e57e62debea7", [](Cpu& c) { c.set(3, league_base::fixture_slot(c, c.r(3), c.r(4), c.r(5), c.r(6))); })  // +0x5c
LEAGUE_BASE_ROUTINE(0x104a9bc0, 0x104aa0bc, R3, "de59487f65e2133a", [](Cpu& c) { c.set(3, std::uint32_t(league_base::evaluate_position(c, c.r(3), c.r(4), c.r(5)))); })
LEAGUE_BASE_ROUTINE(0x104aa240, 0x104aa33c, NONE, "f3d732a31474dc55", [](Cpu& c) { league_base::limit_club_byte(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
LEAGUE_BASE_ROUTINE(0x104aa340, 0x104aa398, NONE, "4562a7f94683bb69", [](Cpu& c) { league_base::mark_clubs_of_competition(c, c.r(3)); })  // +0xac
LEAGUE_BASE_ROUTINE(0x104aa430, 0x104aa55c, R3, "e202ba85bcc8f0ce", [](Cpu& c) { c.set(3, league_base::points_for_result(c, c.r(3), c.r(4), c.r(5))); })  // +0x98
LEAGUE_BASE_ROUTINE(0x104aa560, 0x104aa5b8, R3, "edbea3f83b3ae0e4", [](Cpu& c) { c.set(3, league_base::find_standing(c, c.r(3), c.r(4), c.r(5))); })
LEAGUE_BASE_ROUTINE(0x104aa5c0, 0x104aa620, R3, "ef03bdc8682b4b5e", [](Cpu& c) { c.set(3, league_base::has_standing(c, c.r(3), c.r(4))); })  // +0x88
