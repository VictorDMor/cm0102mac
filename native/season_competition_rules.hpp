#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace cm::aot {
struct State;
// The database release a career was created from. It selects the career's
// rules independently of the calendar year: a career begun in 2000 keeps its
// edition in 2001.
enum class CompetitionEdition { cm0001_389, cm0102, si_3968 };
inline constexpr std::size_t competition_edition_count = 3;

// Recognised from the database names embedded in the career (also in saves).
CompetitionEdition competition_edition(const State&);

namespace english_rules {
// A season owns its rules in the existing serialized competition fields (the
// league serializer reads +0xc0..+0xc3 at 0x104a77b0 and writes them at
// 0x104a8980). The rules below are only read when a season is configured;
// loading a career never changes its promotion places or an already-created playoff.
struct PromotionRules {
    unsigned automatic, playoff_clubs, premier_relegated;
    // Between the Third Division and the Conference: no playoff, the same number each way.
    unsigned conference_promoted, third_relegated;
};
// The game as shipped.
inline constexpr PromotionRules original{2, 4, 3, 1, 1};
// The First Division preset withdrawn on 2026-11-10: three automatic places, an eight-club
// playoff and four clubs down from the Premier League. No new season takes it; a season
// that was configured with it keeps its bytes and ends by it (`expanded_season`); only
// its first two fields are read.
inline constexpr PromotionRules expanded{3, 8, 4, 3, 3};
// The original's rules between the Premier League and the First Division,
// three places each way between the Third Division and the Conference.
inline constexpr PromotionRules current{2, 4, 3, 3, 3};
}

struct ConferenceLeagueRules {
    bool use_tv_kickoff_slots = true;
    bool has_knockout_calendar = true;
    unsigned club_payment = 100000;
};

enum class TrophyQualification { engine_default, english_lower_division };
struct TrophyRules {
    TrophyQualification qualification = TrophyQualification::engine_default;
};

struct CupDate {
    unsigned day, month, next_year, weekday_rule;
};

struct CupRoundRules {
    CupDate main_date, second_date;
    unsigned second_date_mode;
    unsigned label, entrants, ties, direct_entrants, first_direct_entrant;
    unsigned draw_flags, date_rule, first_leg_rules, second_leg_rules, leg_rule, days_between_legs;
    std::uint32_t field_70 = 0xffffffff;
};

struct ConferenceCupRules {
    unsigned entrants;
    std::array<CupRoundRules, 5> rounds;
};

struct EnglandRules {
    english_rules::PromotionRules promotion = english_rules::current;
    // Named reserves in Premier League matches (still three substitutions).
    // Zero leaves the engine's own value.
    unsigned premier_named_reserves = 7;
    ConferenceLeagueRules conference;
    TrophyRules fa_trophy;
    const ConferenceCupRules* conference_cup = nullptr;
};

struct BrazilRules {
    // Série B as one 27-round table, four promoted, no playoff groups. False
    // keeps the original group stages.
    bool serie_b_single_table = true;
};

struct InternationalRules {
    // The Inter-American Cup is not played after this year's edition.
    int inter_american_cup_last_edition = 1998;
};

namespace croatia_cup {
struct Profile;
extern const Profile cm0102, cm0001_389, si_3968;
}
enum class CroatianSecondFormat { regional_groups, national_table };
enum class CroatianFirstFormat { single_table_16, split_table_12 };
enum class CroatianSuperCupFormat { absent, golden_goal, extra_time };

// Every rule the port decides, in one place. `global_rules` is the set every
// edition starts from; an edition inherits it and changes only what its
// database needs (competition_rules.cpp). A field added here is inherited by
// all editions until one overrides it.
struct CompetitionRules {
    EnglandRules england;
    BrazilRules brazil;
    InternationalRules international;
    // League-selection rows of countries this edition's data cannot run.
    std::span<const unsigned> unavailable_countries;
    CroatianFirstFormat croatian_first = CroatianFirstFormat::single_table_16;
    CroatianSecondFormat croatian_second = CroatianSecondFormat::regional_groups;
    const croatia_cup::Profile* croatian_cup = &croatia_cup::cm0102;
    CroatianSuperCupFormat croatian_super_cup = CroatianSuperCupFormat::absent;
};
inline constexpr CompetitionRules global_rules{};

// The equivalence checks compare with the generated original under two
// environment variables, which restore the shipped game's values whatever the
// edition. Each keeps the timing its checks rely on: the English one is read
// on every use (english_bench_checks toggles it), the Brazilian one once.
inline constexpr CompetitionRules with_original_presets(CompetitionRules rules, bool english, bool brazilian) {
    if (english) {
        rules.england.promotion = english_rules::original;
        rules.england.premier_named_reserves = 0;
    }
    if (brazilian) rules.brazil.serie_b_single_table = false;
    return rules;
}
// One rule set under each combination of the two: [english][brazilian].
using RulePresets = std::array<std::array<CompetitionRules, 2>, 2>;
inline constexpr RulePresets original_presets(const CompetitionRules& rules) {
    return {{{with_original_presets(rules, false, false), with_original_presets(rules, false, true)},
             {with_original_presets(rules, true, false), with_original_presets(rules, true, true)}}};
}

// Optional historical rule changes ("Regras realistas"). Each competition has
// its own timeline: zero or more verified changes, each in force from the
// competition season it names (the calendar year that season starts in, 2004
// for 2004/05) until a later entry changes the same rule again. A career
// follows the timelines only with its switch on; its edition supplies the
// starting rules either way.
enum class RuleCompetition {
    english_leagues, // promotion and relegation between the English divisions
    english_premier,
    english_conference,
    fa_trophy,
    conference_cup,
    brazilian_serie_b,
    inter_american_cup,
    croatian_second,
    croatian_cup,
    croatian_super_cup,
};
struct RuleChange {
    RuleCompetition competition;
    int first_season;
    // Changes only the fields this reform touched; the rest carries forward.
    void (*apply)(CompetitionRules&);
    // Where the rule and its first season were verified.
    std::string_view source;
    // Historical additions can begin in an edition whose engine omits them.
    bool include_edition_season = false;
};
// A competition's rules in one of its seasons: `rules`, which are those of
// the season `after`, with every later change of that competition up to
// `season`, oldest first. Only that competition's fields carry their history
// in the result; read another competition's from its own resolution.
constexpr CompetitionRules with_rule_changes(CompetitionRules rules, std::span<const RuleChange> timeline,
                                             RuleCompetition competition, int season, int after) {
    const auto eligible = [&](const RuleChange& change) {
        return change.competition == competition && change.first_season <= season &&
               (change.first_season > after || (change.first_season == after && change.include_edition_season));
    };
    const RuleChange* applied = nullptr;
    for (;;) {
        // The next season with an entry, whatever order the table lists them in.
        const RuleChange* next = nullptr;
        for (const auto& change : timeline)
            if (eligible(change) && (!applied || change.first_season > applied->first_season) &&
                (!next || change.first_season < next->first_season))
                next = &change;
        if (!next) return rules;
        // Entries of one season apply together, in table order.
        applied = next;
        for (const auto& change : timeline)
            if (eligible(change) && change.first_season == applied->first_season) change.apply(rules);
    }
}
// The checked-in timelines of every competition (competition_rules.cpp).
std::span<const RuleChange> rule_timeline();

// The rules of one edition, as checked in.
const CompetitionRules& edition_rules(CompetitionEdition);
// The running career's rules: its edition's, with the test presets that keep
// the original game's behaviour applied last. A career's own overrides
// (career_rules.hpp) come between the two.
const CompetitionRules& competition_rules(const State&);
// The same for one competition in the season being configured, which adds
// that competition's historical changes where the career follows them. Read
// it when a competition's season is set up, before its first draw, with the
// season the competition object itself is entering: a season already under
// way keeps the rules it was configured with.
CompetitionRules competition_rules(const State&, RuleCompetition, int season);
// The season an edition's database starts in. Its rules are that season's:
// only later timeline entries change them.
constexpr int edition_season(CompetitionEdition edition) {
    return edition == CompetitionEdition::cm0001_389 ? 2000 : edition == CompetitionEdition::si_3968 ? 2002 : 2001;
}
inline const EnglandRules& english_season_rules(const State& s) { return competition_rules(s).england; }

void reserve_cm0001_conference_cup_slot(State&);
bool append_cm0001_conference_cup(State&);
bool skip_missing_saved_conference_cup(State&);
void size_cm0001_conference_cup_entrants(State&);
bool apply_cm0001_conference_cup_entrants(State&);
bool apply_cm0001_conference_cup_rounds(State&);
bool apply_cm0001_fa_trophy_qualifiers(State&);
}
