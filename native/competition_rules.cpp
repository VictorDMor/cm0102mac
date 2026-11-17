#include "season_competition_rules.hpp"
#include <array>
#include <cstdlib>

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

// League-selection rows whose CM01/02 constructors need competitions, awards
// or fixed club identities the audited CM00/01 data does not have. Their
// football records stay in the database; no league is synthesized.
constexpr std::array cm0001_unavailable{8u, 11u, 15u, 20u, 21u, 23u, 31u};

// An edition is the rule set it inherits with its own changes applied.
template <class Customize>
constexpr CompetitionRules inherit(CompetitionRules rules, Customize&& customize) {
    customize(rules);
    return rules;
}

// 2001/2002, the release this port is built from: the global rules as they are.
constexpr auto cm0102 = global_rules;

// 2002/2003 (SI 3.9.68 data): the same engine and competitions. Nothing
// differs yet; this is where a 2002/03 rule goes.
constexpr auto si_3968 = inherit(cm0102, [](CompetitionRules&) {});

// 2000/2001 (CM00/01 3.89 data).
constexpr auto cm0001_389 = inherit(global_rules, [](CompetitionRules& rules) {
    rules.england.conference.use_tv_kickoff_slots = false;
    rules.england.conference.has_knockout_calendar = false;
    rules.england.conference.club_payment = 0;
    rules.england.fa_trophy.qualification = TrophyQualification::english_lower_division;
    rules.england.conference_cup = &conference_cup_389;
    rules.unavailable_countries = cm0001_unavailable;
});

// What an edition does not mention, it takes from the global rules.
static_assert(cm0001_389.england.promotion.conference_promoted == global_rules.england.promotion.conference_promoted);
static_assert(cm0001_389.england.premier_named_reserves == global_rules.england.premier_named_reserves);
static_assert(cm0001_389.brazil.serie_b_single_table == global_rules.brazil.serie_b_single_table);
static_assert(si_3968.england.conference.club_payment == global_rules.england.conference.club_payment);
static_assert(!si_3968.england.conference_cup && si_3968.unavailable_countries.empty());
static_assert(cm0001_389.england.conference_cup && cm0001_389.unavailable_countries.size() == 7);

constexpr const CompetitionRules& checked_in(CompetitionEdition edition) {
    switch (edition) {
    case CompetitionEdition::cm0001_389: return cm0001_389;
    case CompetitionEdition::cm0102: return cm0102;
    case CompetitionEdition::si_3968: return si_3968;
    }
    return cm0102;
}

// The equivalence checks compare with the generated original under two
// environment variables, which restore the shipped game's values whatever the
// edition. Each keeps the timing its checks rely on: the English one is read
// on every use (english_bench_checks toggles it), the Brazilian one once.
constexpr CompetitionRules with_original_presets(CompetitionRules rules, bool english, bool brazilian) {
    if (english) {
        rules.england.promotion = english_rules::original;
        rules.england.premier_named_reserves = 0;
    }
    if (brazilian) rules.brazil.serie_b_single_table = false;
    return rules;
}
using Presets = std::array<std::array<CompetitionRules, 2>, 2>; // [english][brazilian]
constexpr Presets presets(CompetitionEdition edition) {
    const auto& rules = checked_in(edition);
    return {{{with_original_presets(rules, false, false), with_original_presets(rules, false, true)},
             {with_original_presets(rules, true, false), with_original_presets(rules, true, true)}}};
}
constexpr std::array<Presets, competition_edition_count> resolved{
    presets(CompetitionEdition(0)), presets(CompetitionEdition(1)), presets(CompetitionEdition(2))};
static_assert(std::size_t(CompetitionEdition::si_3968) + 1 == competition_edition_count);
}

const CompetitionRules& edition_rules(CompetitionEdition edition) { return checked_in(edition); }

const CompetitionRules& competition_rules(const State& s) {
    static const bool brazilian = std::getenv("CM_BRA_NAT_SECOND_RULES_ORIGINAL") != nullptr;
    const bool english = std::getenv("CM_ENGLISH_RULES_ORIGINAL") != nullptr;
    return resolved[std::size_t(competition_edition(s))][english][brazilian];
}
}
