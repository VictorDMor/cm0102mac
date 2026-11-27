#pragma once
#include "competition_overrides.hpp"
#include "season_competition_rules.hpp"
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cm::aot {
inline constexpr unsigned current_rules_revision = 2;
// A date as the game keeps it: the day of the year counted from 0, and the year.
struct HolidayDate {
    std::int16_t day = 0;
    std::uint16_t year = 0;
    friend bool operator==(const HolidayDate&, const HolidayDate&) = default;
};
// A holiday that ends on its own: the date, and the staff id of the manager who went away.
struct HolidayReturn {
    HolidayDate date;
    std::uint32_t staff = 0;
    friend bool operator==(const HolidayReturn&, const HolidayReturn&) = default;
};

// A career's own rules: the third layer, after the global set and the
// edition. They travel in the save as the member `native_rules.tmp`, which the
// original writer bundles and the original loader restores like any other
// temporary file (docs/season-competition-rules-plan.md, Container audit).
// Host-only state, owned by the game worker.
//
//   CM-NATIVE-RULES 1
//   edition cm0001_389
//   revision 1
//   historical_rules off
//   set england.premier_named_reserves 0
//   value league 12 league.points.win 2
//   value league 12 league.points.win 3 from 2004
//   value cup 39 cup.prize.pool 1000000
//   value round 39 2 cup.round.legs 1
//   holiday_return 212 2001 3
//
// A `value` line is one competition override (competition_overrides.hpp):
// `value league|cup <key> <parameter id> <int>`, or for a cup round row
// `value round <cup key> <round index> <parameter id> <int>`, each optionally
// followed by `from <season>`. The member carries the resolved entries, never
// a reference to the custom database they came from. They need no new format
// or revision: `revision` names the edition's checked-in rules, which these
// lines do not change, and a build that does not know `value` keeps the lines
// and ignores them, as for any unknown statement. Such a build does not apply
// the entries: each competition's next setup writes its own values again; the
// ruling body is the constructor's at once (it is not saved); and the Premier
// League's named-reserves rule rewrites that league's bench and its loaded
// fixtures on every load. A later save by this build restores none of it.
//
// A `holiday_return <day> <year> <staff id>` line is the date a human manager's holiday ends by itself
// (holiday_return.hpp): the game's day of the year from 0, its year, and the manager's staff id. There is
// one per manager on a holiday that has a date, and none otherwise; no byte of the original save changes.
class CareerRules {
public:
    inline static constexpr std::string_view member_name = "native_rules.tmp";
    // The format is a few lines of text; a larger member is dropped on read,
    // and never written (the platform keeps the previous one and says so).
    inline static constexpr std::size_t member_limit = 1024 * 1024;

    // A new career, or a load that replaces the running one.
    void reset() { *this = {}; }
    // Takes a member's text. Returns what was not applied, for the status
    // line and the log; empty when everything was. Never fails: text this
    // build cannot interpret is kept and written back unchanged. An empty
    // member is no member.
    std::vector<std::string> read(std::string_view text);
    // The member to save. `current` is the career's edition where no member pinned one.
    std::string text(CompetitionEdition current) const;

    // The edition a member pinned; without one the database names decide.
    std::optional<CompetitionEdition> edition() const { return edition_; }
    unsigned revision() const { return revision_; }
    // Whether the career follows its competitions' historical timelines
    // ("Regras realistas"). Off for careers saved without the switch.
    bool historical_rules() const { return historical_rules_; }
    // False for a member this build keeps without interpreting: changing
    // anything would be lost, since its text is written back as read.
    bool editable() const { return !opaque_; }
    void set_historical_rules(bool on) {
        if (!editable()) return;
        historical_rules_ = on;
        if (on) revision_ = current_rules_revision;
    }
    // The pinned edition's rules with the career's overrides, under each test
    // preset, or null when it has no override.
    const RulePresets* customized() const { return customized_ ? &*customized_ : nullptr; }
    // Applies the overrides to rules resolved elsewhere (a season of a timeline).
    void apply(CompetitionRules&) const;
    // The career's competition overrides (`value` lines this build applies).
    const competition_overrides::OverrideSet& competition_overrides() const { return competition_overrides_; }
    // Replaces them: the `value` lines this build applied give way to one line
    // per entry; lines it ignored stay. False (and nothing changes) for a
    // member kept without interpreting.
    bool set_competition_overrides(const competition_overrides::OverrideSet&);
    // The dates the running holidays end on, one per manager (the `holiday_return` lines). Setting one
    // (or clearing it with nothing) rewrites that manager's line and keeps the others; false, and nothing
    // changes, for a member kept without interpreting.
    const std::vector<HolidayReturn>& holiday_returns() const { return holiday_returns_; }
    std::optional<HolidayReturn> holiday_return(std::uint32_t staff) const;
    bool set_holiday_return(std::uint32_t staff, std::optional<HolidayDate>);
    // The date picked in the holiday dialog before Yes: host state of the open dialog, never saved.
    std::optional<HolidayDate> holiday_pending() const { return holiday_pending_; }
    void set_holiday_pending(std::optional<HolidayDate> date) { holiday_pending_ = date; }

    // A new career's start from a custom database: its entries replace the
    // career's when `edition` (the database in use, none when it is not a
    // recognised release) is the file's base and the member stays within
    // `member_limit`. "" when adopted, else why not (nothing changes then).
    std::string adopt(const competition_overrides::CustomDatabase&, std::optional<CompetitionEdition> edition);

private:
    std::optional<CompetitionEdition> edition_;
    unsigned revision_ = 1;
    bool historical_rules_ = false;
    // Kept verbatim when the header, edition or revision is not this build's.
    std::optional<std::string> opaque_;
    // Every statement after the header ones, as read, applied or not.
    std::vector<std::string> statements_;
    // Index into the overridable fields, and the value.
    std::vector<std::pair<std::size_t, long>> overrides_;
    std::optional<RulePresets> customized_;
    competition_overrides::OverrideSet competition_overrides_;
    std::vector<HolidayReturn> holiday_returns_;
    std::optional<HolidayDate> holiday_pending_;
};
// The `holiday_return` line and its reading (null for a line that is not one).
std::string holiday_return_statement(const HolidayReturn&);
std::optional<HolidayReturn> read_holiday_return_statement(std::string_view line);
// One `value` line and its reading; the reading is null for a line that is
// not a valid entry for this build.
std::string value_statement(const competition_overrides::Override&);
std::optional<competition_overrides::Override> read_value_statement(std::string_view line);

// The revision of an edition's checked-in rules. A member with another
// revision is kept and not applied, so a build that raises this must keep the
// previous revision's rules and resolve members that name it.
// It covers the historical timelines as well: adding or changing an entry is
// a new revision, under the same obligation.
constexpr unsigned rules_revision(CompetitionEdition) { return current_rules_revision; }
// One competition's rules in a season: the edition's (those of
// `edition_season`), the timeline's later changes when `historical`, then the
// career's overrides. The test presets are the caller's.
CompetitionRules resolve_rules(const CompetitionRules& edition, std::span<const RuleChange> timeline, bool historical,
                               const CareerRules* career, RuleCompetition, int season,
                               int edition_season);
std::string_view edition_name(CompetitionEdition);
}
