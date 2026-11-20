#pragma once
// A career's own competition values: entries of (competition, parameter, value, first season) over the registry of
// competition_values.hpp, the custom database file that holds them, and the points where they reach the game
// (docs/season-competition-rules-plan.md, "Competition editor, stage 1"). With no entry nothing here writes guest
// memory, changes a register or calls the guest: the game is the original's.
//
// When an entry applies. Every value but the ruling body is written into the competition object at the entry of
// its season setup: LEAGUE.CPP's set_up (0x104a00a0, after configure and collect_clubs) for a league or a league
// stage, CUP.CPP's tree (0x10479100, vt+0x9c, after configure and the club selection) for a cup itself. Those fields are
// saved with the competition and setup never runs on load, so the value persists. The ruling body (+0x4a) is not
// saved (the constructor restores it on every load): `ruling_body` answers it where the discipline module reads it.
// The season is the object's own (+0x40, the calendar year its season starts in), so an entry with a later first
// season takes over at that season's setup. Cup round values apply to the round rows the cup has at its setup (all
// of them: configure builds the whole table); an entry for a round the cup does not have is ignored, and no round
// row is created later in a season. League stages are reached through set_up, under their own key. A cup's stages
// are not editable in this version: entries for a `<id>-s<k>` cup key are refused, and the hook skips CUP_STAGE
// objects (recognised by their vtable, since the CUP_STAGE constructor calls the tree directly on its default-draw
// path, 0x104bc6cc, with +0x44 holding its competition type). The six cup vtables without the base tree are not reached.
//
// Validation (`problem`). Refused: an unknown or read-only parameter, a parameter of the other kind, a value outside
// the registry's range or list, a round index on a non-round parameter (or none on a round one), a first season
// outside 1..32767 (the object's season is a signed halfword), and two coupled cases:
//  * tie-break code 4 (head-to-head): the rows' head-to-head blocks are allocated when the clubs are placed
//    (0x104a60e0, league_base_storage.cpp), before the override applies, so a list that newly reaches a 4 would
//    sort from blocks that do not exist. A league whose own setup already uses 4 keeps it as long as no override
//    replaces that byte.
//  * a ruling body on a stage: the discipline module only asks the competition's own object.
//  * any value of a cup's stage (`cup <id>-s<k>`): not editable in this version.
// Accepted, with their effect:
//  * bench and substitutions are capped at 9 by the range (the match engine clamps both nibbles at 9); more
//    substitutions than named reserves are accepted and the engine allows at most the reserves (0x102065f0).
//  * points for a draw above a win, a tie-break 0 (it ends the list), any prize-sharing mode of the range.
//  * promotion, playoff and relegation counts are not compared with the neighbouring divisions: an exchange that
//    does not balance changes the divisions' sizes at the season's end. Against the league's own club count they
//    are checked where it is known, at set_up (`at_setup`).
//  * cup legs 1 or 2: a round's fixtures follow its legs when they are made, which is after the setup.
// Removing an entry later leaves the value in the record until the competition's next configure rewrites it; a
// cup's bench and substitutions are written only by its constructor, so they keep the last override.
#include "competition_values.hpp"
#include "season_competition_rules.hpp"
#include <compare>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cm::aot {
struct State;
}

namespace cm::aot::competition_overrides {
using competition_values::Kind;
using competition_values::Parameter;

// The competition object an entry is for: its kind, the database competition id (record +0) and, for a stage, its
// index (the object's +0x44, which LEAGUE_STAGE's constructor sets before set_up; -1 for the competition itself).
struct Target {
    Kind kind = Kind::league;
    std::int32_t id = 0;
    std::int32_t stage = -1;
    friend auto operator<=>(const Target&, const Target&) = default;
};
// "12", "12-s0" (competition_values::key).
std::string key(const Target&);
std::optional<Target> parse_target(Kind, std::string_view key);

struct Override {
    Target target;
    const Parameter* parameter = nullptr;
    std::int32_t round = -1;                  // the round row of a cup_round parameter, else -1
    std::int32_t value = 0;
    std::optional<std::int32_t> from_season;  // none: from the career's first setup
    friend bool operator==(const Override&, const Override&) = default;
};
// "league 12 league.points.win = 2 from 2004": names an entry in messages.
std::string describe(const Override&);
// "" when the entry is valid, else why not (see the header comment).
std::string problem(const Override&);

class OverrideSet {
public:
    // Adds a valid entry, replacing one for the same target, parameter, round and first season; else why not.
    std::string add(const Override&);
    // Builds a set in one pass (validate, sort): an invalid entry, or with `refuse` a repeated target, parameter,
    // round and first season, is an error naming its index; with `keep_last` the later of two repeats wins.
    enum class Repeats { refuse, keep_last };
    struct BuildError {
        std::size_t index;
        std::string message;
    };
    static std::expected<OverrideSet, BuildError> from(std::vector<Override>, Repeats);
    bool empty() const { return entries_.empty(); }
    std::size_t size() const { return entries_.size(); }
    // Sorted by target, then registry order, round and first season (none first).
    const std::vector<Override>& entries() const { return entries_; }
    // The value in force in `season`: the entry with the latest first season at or before it, one without a season
    // counting as the earliest. Later entries supersede earlier ones from their season on.
    std::optional<std::int32_t> value(const Target&, const Parameter&, std::int32_t round, std::int32_t season) const;
    friend bool operator==(const OverrideSet&, const OverrideSet&) = default;

private:
    std::vector<Override> entries_;
};

// A custom database: the base edition it was made from and the user's changes (competition_values.hpp has the
// file's syntax). Selected for a new career, its entries are copied into the career's rules; the save carries the
// entries, never the file.
struct CustomDatabase {
    CompetitionEdition base = CompetitionEdition::cm0102;
    std::string name;
    OverrideSet overrides;
};
using competition_values::ParseError;
// Errors carry the line; a refused entry is named.
std::expected<CustomDatabase, ParseError> read_custom_database(std::string_view text);
std::string write_custom_database(const CustomDatabase&);

// The running career's entries, or null when it has none (also with no platform: replays and checks).
const OverrideSet* active(const State&);
// Who an object is, from its own fields: null for an object that is neither a league nor a cup, has no record, or
// is a CUP_STAGE object (its vtable is the class's, TOC -0x613c; its +0x44 is not a stage index).
std::optional<Target> target_of(const State&, std::uint32_t object);

// The apply point: the entry of LEAGUE.CPP set_up 0x104a00a0 (kind league) and of CUP.CPP's tree 0x10479100 (kind
// cup). Returns at once when the career has no entry; skips an object of another kind and a CUP_STAGE object.
// A league's promotion and relegation entries are skipped, with a log line, when with the record's other places one
// of them or their sum would exceed the league's club count.
void at_setup(State&, std::uint32_t object, Kind);
// The ruling body index the discipline module uses for a competition object: `stored` (+0x4a) unless an entry
// overrides it for the object's season.
std::int8_t ruling_body(const State&, std::uint32_t competition, std::int8_t stored);
// Whether an entry sets the bench of this league in `season` (the Premier League's named-reserves rule yields to it).
bool bench_overridden(const State&, std::int32_t competition_id, std::int32_t season);

// A change during a season (the editor's "now"): writes the record of `target`'s live object with every entry in
// force at its season whose effect allows it, without re-running any setup (a setup pays the clubs). Written:
// league promotion, relegation, points, tie-breaks, bench, substitutions, prize pool and sharing; cup bench and
// substitutions. Deferred to the next setup: a cup's prize pool (it is paid out tie by tie and decreases as it
// pays) and cup round values (which rounds already have fixtures is not known here). The ruling body needs no
// write (`ruling_body` reads the entries). Read-only parameters are never entries.
struct ApplyReport {
    bool found = false;                         // the target's object exists and has its kind
    std::vector<const Parameter*> written, deferred, live;
};
ApplyReport apply_now(State&, const Target&);
}
