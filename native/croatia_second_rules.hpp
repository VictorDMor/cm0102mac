#pragma once
#include "season_competition_rules.hpp"

namespace cm::aot::croatia_second {
inline constexpr int national_division_slot = -0x6a20;
inline bool uses_single_table(const State& s, int season) {
    return competition_rules(s, RuleCompetition::croatian_second, season).croatian_second ==
           CroatianSecondFormat::national_table;
}
}
