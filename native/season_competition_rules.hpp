#pragma once
#include <array>
#include <cstdint>

namespace cm::aot {
struct State;
enum class CompetitionEdition { cm0001_389_v1, cm0102 };

// Selected from the database embedded in the career, independently of its year.
CompetitionEdition competition_edition(const State&);

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

struct EnglishSeasonRules {
    ConferenceLeagueRules conference;
    TrophyRules fa_trophy;
    const ConferenceCupRules* conference_cup = nullptr;
};

const EnglishSeasonRules& english_season_rules(const State&);

void reserve_cm0001_conference_cup_slot(State&);
bool append_cm0001_conference_cup(State&);
bool skip_missing_saved_conference_cup(State&);
void size_cm0001_conference_cup_entrants(State&);
bool apply_cm0001_conference_cup_entrants(State&);
bool apply_cm0001_conference_cup_rounds(State&);
bool apply_cm0001_fa_trophy_qualifiers(State&);
}
