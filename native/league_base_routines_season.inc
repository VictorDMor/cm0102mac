// LEAGUE.CPP, block season (league_base_season*.cpp): the season setup, the fixture list and the schedule rounds.
// Drafts: the coordinator promotes them. 0x104a28d0 leaves the stack bytes read by GAME.CPP 0x101275fc/0x10129b94,
// COMP 0x1045314c, CUP.CPP 0x1047f55c/0x104830e8 and the BlockMoveData bridge (its prologue image: the Frame).
LEAGUE_BASE_ROUTINE(0x104a00a0, 0x104a0aa8, R3, "668c3b7d6c8bbc4b", [](Cpu& c) { c.set(3, league_base::season::set_up(c, c.r(3)) ? 1u : 0u); })
LEAGUE_BASE_ROUTINE(0x104a10d0, 0x104a1348, NONE, "f144db8965cce601", [](Cpu& c) { league_base::season::pair_rounds(c, c.r(4), std::int32_t(c.r(5))); })
LEAGUE_BASE_ROUTINE(0x104a28d0, 0x104a3344, NONE, "4afd3f7ff08fa52f", [](Cpu& c) { league_base::season::schedule_fixtures(c, c.r(3), c.r(4)); })
LEAGUE_BASE_ROUTINE(0x104a3350, 0x104a38c8, R3, "77201ba98a841c4f", [](Cpu& c) { c.set(3, std::uint32_t(league_base::season::random_below(c, std::int32_t(c.r(3))))); })
LEAGUE_BASE_ROUTINE(0x104a38d0, 0x104a3924, R3, "701ca413bc57ef9c", [](Cpu& c) { c.set(3, std::uint32_t(league_base::season::compare_rounds(c, c.r(3), c.r(4)))); })
LEAGUE_BASE_ROUTINE(0x104a9600, 0x104a9808, R3, "4b6948cf13bcee1f", [](Cpu& c) { c.set(3, std::uint32_t(league_base::season::next_round(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)))); })
LEAGUE_BASE_ROUTINE(0x104a9810, 0x104a988c, NONE, "16a83ac5504bda6b", [](Cpu& c) {
    const auto tail = c.u32(c.sp() + 0x38);  // the ninth argument, in the caller's parameter area
    league_base::season::set_round_date(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10), tail);
})
LEAGUE_BASE_ROUTINE(0x104a9890, 0x104a9a94, NONE, "43969f816f120a87", [](Cpu& c) { league_base::season::set_match_slot(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
LEAGUE_BASE_ROUTINE(0x104aab30, 0x104aac14, NONE, "8716f25d0dacfc3a", [](Cpu& c) { league_base::season::preset_fixture(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
