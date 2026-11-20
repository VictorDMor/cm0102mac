#pragma once
#include "competition_overrides.hpp"
#include "guest_values.hpp"
#include "season_competition_rules.hpp"
#include <array>

namespace cm::aot::english_rules {
// Match squads: seven named reserves, still three substitutions. These are
// existing bytes, not new save fields. The league type excludes English cups
// and reserve/youth competitions even when their clubs play in the Premier League.
// A career's own bench for the league (competition_overrides.hpp) wins: the
// record and the fixtures already hold it, from the setup that made them.
inline void premier_bench(guest::Cpu& c, std::uint32_t league) {
    if (c.u32(league) != c.toc(-0x6020)) return;
    if (competition_overrides::active(c.s) &&
        competition_overrides::bench_overridden(c.s, c.i32(c.u32(league + 4)), c.i16(league + 0x40))) return;
    if (const auto reserves = competition_rules(c.s).england.premier_named_reserves) c.put8(league + 0x46, reserves);
}
inline void loaded_premier_fixture(guest::Cpu& c, std::uint32_t fixture) {
    // Runs for every fixture a load reads: the cheap tests come before the rules.
    const auto id = c.u32(c.toc(-0x67a0));
    if (id == 0xffffffffu || c.u32(fixture) != id || c.i8(fixture + 0x43) != -1) return;
    // +0x2c: the league's season year the setup copied into the fixture.
    if (competition_overrides::active(c.s) &&
        competition_overrides::bench_overridden(c.s, std::int32_t(id), c.i16(fixture + 0x2c))) return;
    if (const auto reserves = competition_rules(c.s).england.premier_named_reserves)
        c.put8(fixture + 0x4e, (reserves << 4) | (c.u8(fixture + 0x4e) & 0xf));
}
// Read where a league season is configured. `league` has its season year
// already (+0x40: the constructor and the rollover set it before configuring).
inline PromotionRules new_season(const guest::Cpu& c, std::uint32_t league) {
    return competition_rules(c.s, RuleCompetition::english_leagues, c.i16(league + 0x40)).england.promotion;
}
inline bool expanded_season(const guest::Cpu& c, std::uint32_t league) {
    return c.u8(league + 0xc0) == expanded.automatic &&
           c.u8(league + 0xc1) == expanded.playoff_clubs;
}
// The season in these two leagues was configured with more than one place between the Third
// Division and the Conference (`conference` is 0 when that league is not in the game).
inline bool several_places(const guest::Cpu& c, std::uint32_t third, std::uint32_t conference) {
    return c.u8(third + 0xc3) > 1 || (conference && c.u8(conference + 0xc0) > 1);
}
// Adjacent matches feed each semifinal: 4/11 vs 7/8, 5/10 vs 6/9.
inline constexpr std::array<unsigned, 8> playoff_places{4, 11, 7, 8, 5, 10, 6, 9};
inline constexpr unsigned playoff_rounds = 3;
inline constexpr unsigned round_bytes = 0x78;
inline constexpr unsigned fixed_draw = 0x200;
inline constexpr unsigned extra_time_and_penalties = 3;
inline constexpr unsigned higher_seed_home = 8;
inline constexpr unsigned home_venue = 0, neutral_venue = 4;
inline constexpr unsigned seed_for_place(unsigned place) { return 12 - place; }
}
