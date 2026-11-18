#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

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

// The rules of one edition, as checked in.
const CompetitionRules& edition_rules(CompetitionEdition);
// The running career's rules: its edition's, with the test presets that keep
// the original game's behaviour applied last. A career's own overrides
// (career_rules.hpp) come between the two.
const CompetitionRules& competition_rules(const State&);
inline const EnglandRules& english_season_rules(const State& s) { return competition_rules(s).england; }

void reserve_cm0001_conference_cup_slot(State&);
bool append_cm0001_conference_cup(State&);
bool skip_missing_saved_conference_cup(State&);
void size_cm0001_conference_cup_entrants(State&);
bool apply_cm0001_conference_cup_entrants(State&);
bool apply_cm0001_conference_cup_rounds(State&);
bool apply_cm0001_fa_trophy_qualifiers(State&);
}
