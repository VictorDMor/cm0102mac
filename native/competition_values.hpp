#pragma once
// The plain values a competition keeps in its own record (docs/season-competition-rules-plan.md, "Competition
// editor, stage 1: the editable values"), as a table-driven registry: one declaration per parameter in
// competition_values.cpp (`parameters()`), and reading a competition, writing TOML and parsing TOML all iterate it.
// Adding a parameter is one row there; the tool, the game and the editor pick it up. No command-line concerns.
// Field meanings: comp_detail.hpp, league_base_detail.hpp, cup_base_detail.hpp and the CONFIRMED rows of
// .local/rewrites/editor/fields.md.
#include <cstdint>
#include <expected>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace cm::aot {
struct State;
}

namespace cm::aot::competition_values {
enum class Kind { league, cup };

// The record a parameter lives in: a league object, a cup object, or one 0x78-byte round row of a cup.
enum class Scope { league, cup, cup_round };

// When an edit takes effect (the plan's table).
enum class Effect { at_once, results_after_edit, fixtures_after_edit, season_end, rounds_not_drawn };

struct Parameter {
    std::string_view id;           // stable: "league.points.win"; the TOML key is the id without its table prefix
    Scope scope;
    std::uint16_t offset;          // in the object (league, cup) or the round row
    std::uint8_t width;            // bytes: 1, 2 or 4 (big-endian in guest memory)
    bool is_signed;
    bool read_only;                // context, not editable
    std::int32_t min, max;         // valid range (inclusive); `allowed`, when not empty, narrows it to a list
    std::span<const std::int32_t> allowed;
    std::string_view label;        // short English label
    Effect effect;                 // meaningless for read-only parameters (at_once)
};

// The registry, in the order the TOML lists the keys.
std::span<const Parameter> parameters();
const Parameter* find_parameter(std::string_view id);
// The key a parameter has inside its TOML table: "points.win" for league.points.win; read-only league parameters
// ("league.structure.clubs") sit in the `.structure` sub-table as "clubs"; cup round parameters as "legs".
std::string toml_key(const Parameter& parameter);
std::string_view effect_name(Effect effect);
// "" when the value is valid for the parameter, else why not.
std::string range_problem(const Parameter& parameter, std::int32_t value);

// Who the object is, independent of memory addresses: the database competition id (record +0), and for a stage
// object the stage index inside its parent (-1 for the competition itself; then parent is -1 too).
struct Identity {
    std::int32_t id = 0;
    std::int32_t stage = -1;
    std::int32_t parent = -1;       // a stage's owning competition id
    std::string name;               // record +0x1a, the database's long name
    std::string short_name;         // record +0x4d
    std::string continent;          // the continent record (record +0x04)'s code, e.g. "EUR" (inferred layout)
    std::string nation;             // the nation record (record +0x08)'s three-letter code, e.g. "BEL" (inferred layout)
    std::int32_t nation_number = -1;  // the nation record's number (+0x68, inferred); -1: none
};

using Values = std::map<std::string, std::int32_t, std::less<>>;  // parameter id -> value

struct Entry {
    Kind kind = Kind::league;
    Identity identity;
    Values values;                  // every parameter of scope league or cup
    std::vector<Values> rounds;     // cups: every parameter of scope cup_round, per round row
};

// Every league and cup object of the world (the competitions table and the stages of each), sorted by (id, stage).
// Objects of another kind are counted in `skipped` when given.
std::vector<Entry> read_competitions(const State& state, unsigned* skipped = nullptr);

// The stable text key of an entry: "12" or "12-s0" (competition id, stage).
std::string key(const Identity& identity);

std::string write(const std::vector<Entry>& entries);

struct ParseError {
    unsigned line = 0;
    std::string message;
};
std::expected<std::vector<Entry>, ParseError> parse(std::string_view text);
}
