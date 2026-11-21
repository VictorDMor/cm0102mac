// PLAYER_REGEN.CPP block `club_placement` (player_regen_clubs.cpp): the club choice of a regenerated player, the two club scores, the
// per-nation club lists and the signing. Callers read r3 (and r9, r10 of the three that call out: player_regen_volatile_read.inc).
PLAYER_REGEN_ROUTINE(0x102ea670, 0x102ea870, R3, "74d2983b2f864eac", [](Cpu& c) { c.set(3, std::uint32_t(player_regen::club_rating(c, c.r(4), c.r(5)))); })
PLAYER_REGEN_ROUTINE(0x102e9b90, 0x102ea3b8, R3, "e692be053bd483dc", [](Cpu& c) { c.set(3, std::uint32_t(player_regen::club_score(c, c.r(4), c.r(5), c.r(6)))); })
PLAYER_REGEN_ROUTINE(0x102e99f0, 0x102e9b88, R3, "64733c1c071fc30c", [](Cpu& c) { c.set(3, player_regen::best_destination_club(c, c.r(3), c.r(4))); })
PLAYER_REGEN_ROUTINE(0x102ea3c0, 0x102ea664, R3, "63ea8bfc47d5f062", [](Cpu& c) { c.set(3, player_regen::best_club_of_nation_list(c, c.r(3), c.r(4))); })
PLAYER_REGEN_ROUTINE(0x102e98a0, 0x102e99e4, NONE, "be28e2a4632fb7d8", [](Cpu& c) { player_regen::init_nation_club_lists(c, c.r(3)); })
PLAYER_REGEN_ROUTINE(0x102dc000, 0x102dc404, R3, "bb495027046db1b6", [](Cpu& c) { c.set(3, player_regen::sign_new_player(c, c.r(3), c.r(4), c.r(5))); })
