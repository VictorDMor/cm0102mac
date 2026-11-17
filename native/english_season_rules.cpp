#include "season_competition_rules.hpp"

namespace cm::aot {
namespace {
// CM00/01 3.89, Windows calendar at 0x53f6d0. The two-legged ties use
// the home clubs' grounds. The delay is in days, independently of the
// separate date used by the replay scheduler. Windows leaves the second-leg
// rule word uninitialized. Use zero for that field rather than allocator data.
constexpr ConferenceCupRules conference_cup_389{22, {{
    {{22,  8, 0, 1}, { 6,  9, 0, 1}, 2,  10, 12, 6, 12,  0, 3, 4, 0, 0, 1,  0, 0},
    {{30,  9, 0, 4}, { 2, 11, 0, 1}, 2,  20, 16, 8, 10, 12, 3, 4, 0, 0, 0,  0},
    {{ 4, 11, 0, 2}, {16, 11, 0, 2}, 2,  30,  8, 4,  0,  0, 3, 4, 0, 0, 0,  0},
    {{17, 11, 0, 3}, { 2,  1, 1, 1}, 2, 130,  4, 2,  0,  0, 0, 6, 3, 0, 2, 49},
    {{24,  2, 1, 2}, {12,  3, 1, 5}, 1, 150,  2, 1,  0,  0, 0, 6, 3, 0, 2, 14},
}}};

constexpr EnglishSeasonRules defaults{};

constexpr auto cm0001_389 = [] {
    auto rules = defaults;
    rules.conference.use_tv_kickoff_slots = false;
    rules.conference.has_knockout_calendar = false;
    rules.conference.club_payment = 0;
    rules.fa_trophy.qualification = TrophyQualification::english_lower_division;
    rules.conference_cup = &conference_cup_389;
    return rules;
}();
}

const EnglishSeasonRules& english_season_rules(const State& s) {
    switch (competition_edition(s)) {
    case CompetitionEdition::cm0001_389_v1: return cm0001_389;
    case CompetitionEdition::cm0102: return defaults;
    }
    return defaults;
}
}
