#include "season_competition_rules.hpp"
#include "aot_runtime.hpp"
#include "career_rules.hpp"
#include "platform.hpp"
#include <algorithm>
#include <array>
#include <utility>
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
constexpr auto si_3968 = inherit(cm0102, [](CompetitionRules& rules) {
    rules.croatian_cup = &croatia_cup::si_3968;
});

// 2000/2001 (CM00/01 3.89 data).
constexpr auto cm0001_389 = inherit(global_rules, [](CompetitionRules& rules) {
    rules.england.conference.use_tv_kickoff_slots = false;
    rules.england.conference.has_knockout_calendar = false;
    rules.england.conference.club_payment = 0;
    rules.england.fa_trophy.qualification = TrophyQualification::english_lower_division;
    rules.england.conference_cup = &conference_cup_389;
    rules.unavailable_countries = cm0001_unavailable;
    rules.croatian_first = CroatianFirstFormat::split_table_12;
    rules.croatian_second = CroatianSecondFormat::national_table;
    rules.croatian_cup = &croatia_cup::cm0001_389;
});

// What an edition does not mention, it takes from the global rules.
static_assert(cm0001_389.england.promotion.conference_promoted == global_rules.england.promotion.conference_promoted);
static_assert(cm0001_389.england.premier_named_reserves == global_rules.england.premier_named_reserves);
static_assert(cm0001_389.brazil.serie_b_single_table == global_rules.brazil.serie_b_single_table);
static_assert(si_3968.england.conference.club_payment == global_rules.england.conference.club_payment);
static_assert(!si_3968.england.conference_cup && si_3968.unavailable_countries.empty());
static_assert(cm0001_389.england.conference_cup && cm0001_389.unavailable_countries.size() == 7);

// Historical rule changes, per competition. Each entry needs its source and a
// check of the seasons before, at and after it (database_seasons_checks.cpp).
constexpr std::array timeline{
    RuleChange{RuleCompetition::croatian_super_cup, 2002, [](CompetitionRules& rules) {
        rules.croatian_super_cup = CroatianSuperCupFormat::golden_goal;
    }, "https://hnl.hr/news/11213/superkup-najavio-sjajnu-novu-nogometnu-sezonu/", true},
    RuleChange{RuleCompetition::croatian_super_cup, 2004, [](CompetitionRules& rules) {
        rules.croatian_super_cup = CroatianSuperCupFormat::extra_time;
    }, "https://downloads.theifab.com/downloads/annual-general-meeting-2004-minutes?l=en"},
};
// The competitions whose season setup reads the seasonal rules. An entry for
// any other would be ignored, so wire its reader first (as
// english_rules::new_season does) and list it here.
constexpr std::array wired{RuleCompetition::english_leagues, RuleCompetition::croatian_second,
                           RuleCompetition::croatian_cup, RuleCompetition::croatian_super_cup};
static_assert(std::ranges::all_of(timeline, [](const RuleChange& change) {
    return std::ranges::find(wired, change.competition) != wired.end();
}), "a timeline entry names a competition nothing reads seasonally");

constexpr const CompetitionRules& checked_in(CompetitionEdition edition) {
    switch (edition) {
    case CompetitionEdition::cm0001_389: return cm0001_389;
    case CompetitionEdition::cm0102: return cm0102;
    case CompetitionEdition::si_3968: return si_3968;
    }
    return cm0102;
}

constexpr RulePresets presets(CompetitionEdition edition) { return original_presets(checked_in(edition)); }
constexpr std::array<RulePresets, competition_edition_count> resolved{
    presets(CompetitionEdition(0)), presets(CompetitionEdition(1)), presets(CompetitionEdition(2))};
static_assert(std::size_t(CompetitionEdition::si_3968) + 1 == competition_edition_count);
}

const CompetitionRules& edition_rules(CompetitionEdition edition) { return checked_in(edition); }

std::span<const RuleChange> rule_timeline() { return timeline; }

namespace {
// The equivalence checks' presets: {english, brazilian}. The English one is
// read on every use, the Brazilian one once (season_competition_rules.hpp).
std::pair<bool, bool> test_presets() {
    static const bool brazilian = std::getenv("CM_BRA_NAT_SECOND_RULES_ORIGINAL") != nullptr;
    return {std::getenv("CM_ENGLISH_RULES_ORIGINAL") != nullptr, brazilian};
}
}

CompetitionRules competition_rules(const State& s, RuleCompetition competition, int season) {
    const auto* career = s.platform ? &s.platform->career_rules() : nullptr;
    if (!career || !career->historical_rules() || career->revision() == 1) return competition_rules(s);
    const auto edition = competition_edition(s);
    const auto [english, brazilian] = test_presets();
    return with_original_presets(
        resolve_rules(edition_rules(edition), timeline, true, career, competition, season, edition_season(edition)),
        english, brazilian);
}

const CompetitionRules& competition_rules(const State& s) {
    const auto [english, brazilian] = test_presets();
    const auto edition = competition_edition(s);
    // The career's own overrides come after its edition; the test presets stay last.
    if (s.platform)
        if (const auto* career = s.platform->career_rules().customized()) return (*career)[english][brazilian];
    return resolved[std::size_t(edition)][english][brazilian];
}
}
