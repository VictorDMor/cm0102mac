#pragma once
#include "guest_values.hpp"
#include <array>
#include <cstdlib>

namespace cm::aot::english_rules {
// A season owns its rules in the existing serialized competition fields.
// This preset only selects rules when a new season is configured; loading a
// career never changes its promotion places or an already-created playoff.
struct PromotionRules {
    unsigned automatic, playoff_clubs, premier_relegated;
};
inline constexpr PromotionRules original{2, 4, 3};
inline constexpr PromotionRules expanded{3, 8, 4};
inline PromotionRules new_season() {
    return std::getenv("CM_ENGLISH_RULES_ORIGINAL") ? original : expanded;
}
inline bool expanded_season(const guest::Cpu& c, std::uint32_t league) {
    return c.u8(league + 0xc0) == expanded.automatic &&
           c.u8(league + 0xc1) == expanded.playoff_clubs;
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
