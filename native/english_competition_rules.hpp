#pragma once
#include "guest_values.hpp"
#include <array>
#include <cstdlib>

namespace cm::aot::english_rules {
// A season owns its rules in the existing serialized competition fields (the
// league serializer reads +0xc0..+0xc3 at 0x104a77b0 and writes them at
// 0x104a8980). `new_season()` only selects rules when a season is configured;
// loading a career never changes its promotion places or an already-created playoff.
struct PromotionRules {
    unsigned automatic, playoff_clubs, premier_relegated;
    // Between the Third Division and the Conference: no playoff, the same number each way.
    unsigned conference_promoted, third_relegated;
};
inline constexpr PromotionRules original{2, 4, 3, 1, 1};
// The First Division preset withdrawn on 2026-11-10: three automatic places, an eight-club
// playoff and four clubs down from the Premier League. No new season takes it; a season
// that was configured with it keeps its bytes and ends by it (`expanded_season`); only
// its first two fields are read.
inline constexpr PromotionRules expanded{3, 8, 4, 3, 3};
// New seasons: the original's rules between the Premier League and the First Division,
// three places each way between the Third Division and the Conference.
inline constexpr PromotionRules current{2, 4, 3, 3, 3};
// CM_ENGLISH_RULES_ORIGINAL keeps the original game's rules and code paths
// (the equivalence checks run with it).
inline bool original_forced() { return std::getenv("CM_ENGLISH_RULES_ORIGINAL") != nullptr; }
inline PromotionRules new_season() { return original_forced() ? original : current; }
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
