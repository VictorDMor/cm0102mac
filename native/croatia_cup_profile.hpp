#pragma once
#include <array>

namespace cm::aot::croatia_cup {
struct Date {
    unsigned day, month, next_year, weekday;
};
struct RoundRules {
    unsigned label, clubs, new_entrants, first_entrant, match_flags, fixture_rule, legs, leg_gap;
    Date draw, fixture;
};
enum class SecondDivision { regional_groups, national_table };
enum class SecondOrder { database, seed_then_reputation };

struct Profile {
    unsigned prize_pool, seed_range, first_places, second_places, lower_places, lower_pool_limit;
    SecondDivision second_division;
    SecondOrder second_order;
    std::array<RoundRules, 6> rounds;
    constexpr unsigned entrants() const { return first_places + second_places + lower_places; }
};
}
