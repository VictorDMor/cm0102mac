// cm-competition-overrides-checks [SNAPSHOT]: the competition overrides (competition_overrides.hpp) from the custom
// database file and the career's save member to the two setup routines and the ruling body.
//
// Without a snapshot: the file's differences mode (parse, errors with their lines, round trip), validation,
// supersession by season, the member's `value` lines (round trip, unknown lines kept, replacement), and the
// platform's choice of a custom database for a new career (wrong base refused, environment variable).
// With the season-day snapshot: the key every league and cup object derives from its own fields; a real league
// set_up (0x104a00a0) and a real cup tree (0x10479100) run in forked children with no platform, with an empty set
// and with entries: the empty set is byte-identical to no platform in every mapped page and every register, and
// the entries change exactly their bytes of the object and its round rows (a negative control computes the
// expectation with a wrong offset and must fail); the ruling body seen by governing_body; the Premier League's
// named-reserves rule yielding to a bench entry; apply_now.
#include "aot_runtime.hpp"
#include "career_rules.hpp"
#include "competition_defaults.hpp"
#include "competition_editor_document.hpp"
#include "competition_editor_text.hpp"
#include "competition_overrides.hpp"
#include "competition_values.hpp"
#include "comp_detail.hpp"
#include "cup_base_detail.hpp"
#include "discipline_table_detail.hpp"
#include "english_competition_rules.hpp"
#include "guest_abi.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "routine_check.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sys/wait.h>
#include <unistd.h>

using namespace cm::aot;
namespace ov = cm::aot::competition_overrides;
namespace cv = cm::aot::competition_values;
namespace fs = std::filesystem;

namespace {
int failures = 0, passed = 0;
void expect(bool ok, const std::string& what) {
    static const bool verbose = std::getenv("CM_CHECKS_VERBOSE") != nullptr;
    if (ok) ++passed;
    else ++failures;
    if (!ok || verbose) std::fprintf(stderr, "%s: %s\n", ok ? "ok" : "FAIL", what.c_str());
}
const cv::Parameter& P(std::string_view id) { return *cv::find_parameter(id); }
ov::Override entry(cv::Kind kind, std::int32_t id, std::int32_t stage, std::string_view parameter, std::int32_t value,
                   std::optional<std::int32_t> from = std::nullopt, std::int32_t round = -1) {
    return {.target = {.kind = kind, .id = id, .stage = stage}, .parameter = &P(parameter), .round = round, .value = value, .from_season = from};
}
constexpr auto L = cv::Kind::league;
constexpr auto C = cv::Kind::cup;

// ---- the file -------------------------------------------------------------------------------------------------
void check_file() {
    const std::string text = R"(# a custom database
format = 1
base = "cm0102"
name = "Two points # not a comment"

[league.12]
points.win = 2   # comment after a value
relegation.places = 4

[league.12.from.2004]
points.win = 3

[league.5-s1]
tiebreak.2 = 5

[cup.39]
prize.pool = 1000000
ruling_body = 7

[[cup.39.round]]

[[cup.39.round]]
legs = 1
prize.winner = -1

[[cup.39.from.2003.round]]
replay_ties = 2
)";
    const auto read = ov::read_custom_database(text);
    expect(read.has_value(), read ? "the sample file reads" : "the sample file reads (line " + std::to_string(read.error().line) + ": " + read.error().message + ")");
    if (!read) return;
    expect(read->base == CompetitionEdition::cm0102 && read->name == "Two points # not a comment", "header base and name");
    const auto& set = read->overrides;
    expect(set.size() == 9, "nine entries (" + std::to_string(set.size()) + ")");
    expect(set.value({L, 12, -1}, P("league.points.win"), -1, 2001) == 2 && set.value({L, 12, -1}, P("league.points.win"), -1, 2004) == 3,
           "points.win 2 until 2003, 3 from 2004");
    expect(set.value({C, 39, -1}, P("cup.round.legs"), 1, 2001) == 1 && !set.value({C, 39, -1}, P("cup.round.legs"), 0, 2001),
           "round 1 legs 1, the empty round 0 changes nothing");
    expect(!set.value({C, 39, -1}, P("cup.round.replay_ties"), 0, 2002) && set.value({C, 39, -1}, P("cup.round.replay_ties"), 0, 2003) == 2,
           "a from-season round table");
    expect(set.value({L, 5, 1}, P("league.tiebreak.2"), -1, 2001) == 5, "a stage key");
    // Writer: what it writes reads back to the same set, and writing that again gives the same text.
    const auto written = ov::write_custom_database(*read);
    const auto again = ov::read_custom_database(written);
    expect(again && again->overrides == set && again->name == read->name && ov::write_custom_database(*again) == written,
           "write(read(file)) reads back the same entries and writes the same text");

    // Errors and their lines.
    const std::string head = "format = 1\nbase = \"cm0102\"\nname = \"x\"\n";
    const struct { std::string body; unsigned line; std::string what; } bad[] = {
        {"[league.1]\nno_such_key = 1\n", 5, "an unknown key"},
        {"[league.1]\npoints.win = 200\n", 5, "a value outside its range"},
        {"[league.1]\nbench = 10\n", 5, "a bench above the engine's 9"},
        {"[league.1.structure]\nclubs = 18\n", 4, "a read-only table"},
        {"[[cup.1.round]]\nties = 3\n", 5, "a read-only round value"},
        {"[league.1]\nprize.pool = x\n", 5, "a non-integer"},
        {"[league.1]\ntiebreak.1 = 4\n", 5, "tie-break 4 (head-to-head)"},
        {"[league.1-s0]\nruling_body = 3\n", 5, "a stage's ruling body"},
        {"[league.1]\nlegs = 1\n", 5, "a cup round value in a league"},
        {"[[league.1.round]]\n", 4, "round tables of a league"},
        {"[league.1]\n[league.1]\n", 5, "a duplicate table"},
        {"[league.1]\nid = 2\n", 5, "an id that disagrees with the key"},
        {"[league.1.from.x]\n", 4, "a from table without a season"},
        {"[league.1.from.40000]\npoints.win = 2\n", 5, "a first season outside 1..32767"},
        {"[league.1]\npoints.win = 2\npoints.win = 3\n", 6, "a duplicate key"},
        {"[cup.39-s0]\nbench = 3\n", 4, "a cup's stage"},
        {"[[cup.39-s1.round]]\nlegs = 1\n", 4, "a cup stage's round"},
    };
    for (const auto& b : bad) {
        const auto r = ov::read_custom_database(head + b.body);
        expect(!r && r.error().line == b.line, b.what + " is refused at line " + std::to_string(b.line) +
               (r ? " (accepted)" : " (got line " + std::to_string(r.error().line) + ": " + r.error().message + ")"));
    }
    const struct { std::string text; unsigned line; std::string what; } bad_header[] = {
        {"format = 2\nbase = \"cm0102\"\nname = \"x\"\n", 1, "another format"},
        {"format = 1\nbase = \"cm9999\"\nname = \"x\"\n", 2, "an unknown base"},
        {"format = 1\nname = \"x\"\n", 1, "a missing base"},
        {"format = 1\nbase = \"cm0102\"\nname = \"x\"\nformat = 1\n", 4, "a repeated header key"},
    };
    for (const auto& b : bad_header) {
        const auto r = ov::read_custom_database(b.text);
        expect(!r && r.error().line == b.line, b.what + " is refused at line " + std::to_string(b.line) +
               (r ? " (accepted)" : " (got line " + std::to_string(r.error().line) + ")"));
    }
    // UTF-8: a byte-order mark, raw UTF-8 and the standard escapes are read; the writer escapes only what TOML needs.
    {
        const auto utf8 = ov::read_custom_database("\xEF\xBB\xBF" "format = 1\nbase = \"cm0102\"\n"
                                                   "name = \"Divis\xC3\xA3o \\u00e3 \\U0001F600 \\\"q\\\" \\\\ \\t\"\n");
        expect(utf8 && utf8->name == "Divis\xC3\xA3o \xC3\xA3 \xF0\x9F\x98\x80 \"q\" \\ \t", "UTF-8 name with a BOM, raw UTF-8 and escapes");
        if (utf8) {
            const auto out = ov::write_custom_database(*utf8);
            expect(out.find("name = \"Divis\xC3\xA3o \xC3\xA3 \xF0\x9F\x98\x80 \\\"q\\\" \\\\ \\t\"\n") != std::string::npos,
                   "the writer keeps UTF-8 raw and escapes only quote, backslash and controls");
            const auto again = ov::read_custom_database(out);
            expect(again && again->name == utf8->name, "the UTF-8 name reads back");
        }
        for (const auto* bad_string : {"\"a\xE3" "b\"", "\"\\uD800\"", "\"a\x01" "b\"", "\"\\x41\"", "\"\xC0\xAF\""}) {
            const auto r = ov::read_custom_database(std::string("format = 1\nbase = \"cm0102\"\nname = ") + bad_string + "\n");
            expect(!r && r.error().line == 3, std::string("an invalid string is refused: ") + bad_string);
        }
    }
    // The full dump stays strict: the same text with a key removed is refused.
    const auto parsed = cv::parse("[league.1]\nid = 1\n");
    expect(!parsed, "the full-dump parser still requires every key");
}

// ---- validation and supersession -------------------------------------------------------------------------------
void check_set() {
    ov::OverrideSet set;
    expect(set.add(entry(L, 3, -1, "league.points.win", 2)).empty(), "an entry without a season");
    expect(set.add(entry(L, 3, -1, "league.points.win", 4, 2006)).empty() && set.add(entry(L, 3, -1, "league.points.win", 3, 2004)).empty(),
           "two later entries, added out of order");
    const auto win = [&](std::int32_t season) { return set.value({L, 3, -1}, P("league.points.win"), -1, season); };
    expect(win(1990) == 2 && win(2003) == 2 && win(2004) == 3 && win(2005) == 3 && win(2006) == 4 && win(2030) == 4,
           "each entry supersedes the earlier ones from its season on");
    expect(set.add(entry(L, 3, -1, "league.points.win", 1, 2004)).empty() && set.size() == 3 && win(2004) == 1,
           "an entry for the same season replaces the value");
    expect(!set.value({L, 3, 0}, P("league.points.win"), -1, 2004) && !set.value({C, 3, -1}, P("cup.bench"), -1, 2004),
           "another stage or kind has its own entries");
    ov::OverrideSet later;
    expect(later.add(entry(L, 3, -1, "league.points.draw", 0, 2005)).empty() && !later.value({L, 3, -1}, P("league.points.draw"), -1, 2004),
           "an entry with only a later season leaves the earlier seasons alone");
    const ov::Override invalid[] = {
        {.target = {L, 1, -1}, .parameter = nullptr},
        entry(L, 1, -1, "league.structure.clubs", 18),
        entry(C, 1, -1, "league.points.win", 2),
        entry(L, 1, -1, "cup.bench", 2),
        entry(C, 1, -1, "cup.round.legs", 1),            // no round index
        entry(C, 1, -1, "cup.bench", 2, std::nullopt, 0), // a round index on a non-round value
        entry(C, 1, -1, "cup.round.legs", 3, std::nullopt, 0),
        entry(L, 1, -1, "league.tiebreak.3", 4),
        entry(L, 1, -1, "league.tiebreak.1", 7),
        entry(L, 1, 2, "league.ruling_body", 3),
        entry(L, 1, -1, "league.substitutions", 10),
        entry(L, 1, -1, "league.points.win", 2, 0),
        entry(L, -1, -1, "league.points.win", 2),
    };
    for (const auto& o : invalid) expect(!ov::problem(o).empty() && !set.add(o).empty(), "refused: " + ov::describe(o));
    expect(set.size() == 3, "refused entries are not added");
    expect(!ov::problem(entry(C, 39, 0, "cup.bench", 3)).empty(), "a cup stage's value is refused");
    // Bulk building: repeats refused, or the later one kept.
    const std::vector<ov::Override> repeated{entry(L, 3, -1, "league.points.win", 2), entry(L, 2, -1, "league.points.win", 1),
                                             entry(L, 3, -1, "league.points.win", 5)};
    const auto refused = ov::OverrideSet::from(repeated, ov::OverrideSet::Repeats::refuse);
    expect(!refused && refused.error().index == 2, "a repeated entry is refused at its index");
    const auto kept = ov::OverrideSet::from(repeated, ov::OverrideSet::Repeats::keep_last);
    expect(kept && kept->size() == 2 && kept->value({L, 3, -1}, P("league.points.win"), -1, 2001) == 5 &&
           kept->entries().front().target.id == 2, "the later repeat wins, entries sorted");
    const auto invalid_index = ov::OverrideSet::from({entry(L, 3, -1, "league.points.win", 2), entry(L, 3, -1, "league.tiebreak.1", 4)},
                                                     ov::OverrideSet::Repeats::refuse);
    expect(!invalid_index && invalid_index.error().index == 1, "an invalid entry is named by its index");
    // Accepted with their effect (competition_overrides.hpp): substitutions above the bench, a draw above a win.
    expect(ov::problem(entry(L, 1, -1, "league.substitutions", 9)).empty() && ov::problem(entry(L, 1, -1, "league.points.draw", 5)).empty(),
           "coupled values documented as accepted are accepted");
}

// ---- the save member --------------------------------------------------------------------------------------------
void check_member() {
    const std::string member = "CM-NATIVE-RULES 1\nedition cm0102\nrevision 1\nhistorical_rules off\n"
        "set england.premier_named_reserves 0\n"
        "value league 12 league.points.win 2\n"
        "value league 12 league.points.win 3 from 2004\n"
        "value round 39 2 cup.round.legs 1 from 2003\n"
        "value cup 39 cup.prize.pool 1000000\n"
        "value league 12 league.points.win 300\n"          // out of range: kept, ignored
        "value cup 39 cup.round.legs 1\n"                   // a round value on a cup line: kept, ignored
        "value future 1 x.y 1\n"                            // a later build's: kept, ignored
        "value cup 39-s0 cup.bench 3\n"                     // a cup's stage: kept, ignored
        "later statement\n";
    CareerRules career;
    const auto ignored = career.read(member);
    expect(ignored.size() == 5, "five statements not applied (" + std::to_string(ignored.size()) + ")");
    const auto& set = career.competition_overrides();
    expect(set.size() == 4 && set.value({L, 12, -1}, P("league.points.win"), -1, 2003) == 2 &&
           set.value({L, 12, -1}, P("league.points.win"), -1, 2004) == 3 && set.value({C, 39, -1}, P("cup.round.legs"), 2, 2003) == 1 &&
           !set.value({C, 39, -1}, P("cup.round.legs"), 2, 2002), "value lines read with their rounds and seasons");
    expect(career.customized() != nullptr, "the set line still applies");
    expect(career.text(CompetitionEdition::cm0102) == member, "text -> read -> text is identical, unknown lines included");
    for (const auto& o : set.entries()) {
        const auto again = read_value_statement(value_statement(o));
        expect(again && *again == o, "statement round trip: " + value_statement(o));
    }
    // Replacing the entries: the applied value lines give way, the ignored ones stay.
    ov::OverrideSet replacement;
    replacement.add(entry(L, 7, -1, "league.relegation.places", 4, 2005));
    expect(career.set_competition_overrides(replacement) && career.competition_overrides() == replacement, "entries replaced");
    const auto text = career.text(CompetitionEdition::cm0102);
    expect(text.find("value league 12") == std::string::npos || text.find("value league 12 league.points.win 300") != std::string::npos,
           "applied lines removed");
    expect(text.find("value league 7 league.relegation.places 4 from 2005\n") != std::string::npos && text.find("later statement\n") != std::string::npos &&
           text.find("value future 1 x.y 1\n") != std::string::npos && text.find("league.points.win 2\n") == std::string::npos,
           "the new line written, the ignored ones kept");
    CareerRules reread;
    reread.read(text);
    expect(reread.competition_overrides() == replacement && reread.text(CompetitionEdition::cm0102) == text, "the replaced member reads back");
    // A member this build keeps opaque applies none and refuses a replacement.
    CareerRules opaque;
    const std::string unknown = "CM-NATIVE-RULES 1\nedition cm0102\nrevision 3\nvalue league 12 league.points.win 2\n";
    opaque.read(unknown);
    expect(opaque.competition_overrides().empty() && !opaque.set_competition_overrides(replacement) &&
           opaque.text(CompetitionEdition::cm0102) == unknown, "an unknown revision applies no value and stays verbatim");
    career.reset();
    expect(career.competition_overrides().empty(), "reset forgets the entries");
}

// ---- a custom database for a new career ------------------------------------------------------------------------
ov::OverrideSet many(std::int32_t leagues) {
    std::vector<ov::Override> entries;
    for (std::int32_t id = 0; id < leagues; ++id)
        for (const auto* p : {"league.points.win", "league.points.draw", "league.promotion.automatic", "league.relegation.places",
                              "league.tiebreak.1", "league.tiebreak.2", "league.bench", "league.substitutions", "league.prize.pool",
                              "league.prize.sharing"})
            entries.push_back(entry(L, id, -1, p, 1, 2001));
    return *ov::OverrideSet::from(std::move(entries), ov::OverrideSet::Repeats::refuse);
}
void check_adopt() {
    ov::CustomDatabase database{.base = CompetitionEdition::cm0102, .name = "x"};
    database.overrides.add(entry(L, 3, -1, "league.points.win", 2));
    CareerRules career;
    expect(!career.adopt(database, std::nullopt).empty() && career.competition_overrides().empty(),
           "a database that is not a recognised release refuses a custom database");
    expect(!career.adopt(database, CompetitionEdition::cm0001_389).empty() && career.competition_overrides().empty(),
           "a custom database made from another edition is refused");
    expect(career.adopt(database, CompetitionEdition::cm0102).empty() && career.competition_overrides() == database.overrides,
           "a custom database made from the database in use is adopted");
    // A file within its own 4 MiB can still give a member above the 1 MiB the reader takes.
    ov::CustomDatabase large{.base = CompetitionEdition::cm0102, .name = "large", .overrides = many(3000)};
    const auto file_size = ov::write_custom_database(large).size();
    CareerRules probe;
    probe.set_competition_overrides(large.overrides);
    const auto member_size = probe.text(CompetitionEdition::cm0102).size();
    const auto why = career.adopt(large, CompetitionEdition::cm0102);
    expect(file_size < 4u * 1024 * 1024 && member_size > CareerRules::member_limit && why.find(std::to_string(member_size)) != std::string::npos &&
           career.competition_overrides() == database.overrides,
           "a custom database whose member would pass the limit is refused with both sizes (file " + std::to_string(file_size) +
           ", member " + std::to_string(member_size) + ": " + why + ")");
}

void check_platform(const fs::path& root) {
    const auto game = root / "game", writable = root / "writable";
    fs::create_directories(game);
    const auto good = root / "good.toml", broken = root / "broken.toml";
    std::ofstream(good) << "format = 1\nbase = \"cm0102\"\nname = \"good\"\n\n[league.3]\npoints.win = 2\n";
    std::ofstream(broken) << "format = 1\nbase = \"cm0102\"\nname = \"broken\"\n\n[league.3]\npoints.win = 999\n";
    NativePlatform platform(game, writable, AudioMode::offline, EventMode::isolated);
    auto& rules = platform.career_rules();
    expect(platform.select_custom_database(good).empty(), "a valid custom database is selected");
    expect(rules.competition_overrides().empty(), "selecting changes nothing in the running career");
    platform.career_load_reset();
    expect(rules.competition_overrides().empty(), "no database indexed for this New Game: the custom database is not used");
    // This fixture's names are no recognised release (database_seasons_checks adopts one with the real 2001 names).
    platform.inspect_database_season_names();
    platform.career_load_reset();
    expect(rules.competition_overrides().empty(), "an unrecognised database refuses the custom database");
    const auto why = platform.select_custom_database(broken);
    expect(why.find("line 6") != std::string::npos, "a broken file is refused with its line (" + why + ")");
    expect(platform.select_custom_database(std::nullopt).empty(), "none clears the choice");
    // The member writer never writes above the reader's limit: the previous member stays.
    platform.note_temporary_directory("");
    auto state = std::make_unique<State>();
    state->wr<4>(guest::reg::gpr(4), 0x16);
    rules.set_competition_overrides({});
    platform.quick_save_write_begin(*state);
    std::optional<fs::path> member;
    for (const auto& item : fs::recursive_directory_iterator(writable))
        if (item.path().filename() == CareerRules::member_name) member = item.path();
    expect(member.has_value(), "the career writer leaves the member");
    if (member) {
        const auto before = fs::file_size(*member);
        rules.set_competition_overrides(many(3000));
        platform.quick_save_write_begin(*state);
        expect(fs::file_size(*member) == before, "a member above the limit is not written; the previous one stays");
    }
    // The environment variable, read once when the platform starts.
    ::setenv("CM_CUSTOM_DATABASE", good.c_str(), 1);
    NativePlatform from_environment(game, root / "writable2", AudioMode::offline, EventMode::isolated);
    ::unsetenv("CM_CUSTOM_DATABASE");
    expect(from_environment.custom_database_name() == "good", "CM_CUSTOM_DATABASE selects at start-up");
}

// ---- the snapshot ---------------------------------------------------------------------------------------------
constexpr std::uint32_t toc_base = 0x108bba90, page = 0x1000;
std::uint32_t u32(const State& s, std::uint32_t at) { return std::uint32_t(s.memory.read<4>(at)); }
std::uint32_t global(const State& s, std::int32_t slot) { return u32(s, u32(s, toc_base + std::uint32_t(slot))); }

struct Object {
    std::uint32_t address;
    ov::Target expected;
};
std::vector<Object> objects(const State& s) {
    std::vector<Object> out;
    const auto table = global(s, comp::toc::competitions);
    const auto count = std::int32_t(global(s, comp::toc::competition_count));
    for (std::int32_t id = 0; table && id < count && id < 4096; ++id) {
        const auto object = u32(s, table + std::uint32_t(id) * 4);
        if (!object) continue;
        const auto record = u32(s, object + comp::competition::record);
        const auto record_id = record ? std::int32_t(u32(s, record)) : -1;
        const auto kind_of = [&](std::uint32_t at) -> std::optional<cv::Kind> {
            const auto k = s.memory.read<1>(at + comp::competition::kind);
            if (k == 2) return L;
            if (k == 1) return C;
            return std::nullopt;
        };
        if (const auto k = kind_of(object)) out.push_back({object, {*k, record_id, -1}});
        const auto last = std::int32_t(u32(s, object + comp::competition::last_stage));
        const auto stages = u32(s, object + comp::competition::stages);
        for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) {
            const auto stage = u32(s, stages + std::uint32_t(n) * 4);
            if (const auto k = stage ? kind_of(stage) : std::nullopt) out.push_back({stage, {*k, record_id, n}});
        }
    }
    return out;
}

// What a run leaves: every register, a hash of every mapped page, and the bytes of the pages under watch.
struct Run {
    std::string error;
    std::vector<std::uint8_t> registers;
    std::uint32_t pc = 0;
    std::map<std::uint32_t, std::uint64_t> hashes;
    std::map<std::uint32_t, std::vector<std::uint8_t>> watched;  // page -> bytes
};
std::uint64_t fnv(const std::uint8_t* data, std::size_t size) {
    std::uint64_t h = 1469598103934665603ull;
    for (std::size_t i = 0; i < size; ++i) h = (h ^ data[i]) * 1099511628211ull;
    return h;
}
void put_bytes(std::string& out, const void* data, std::size_t size) { out.append(static_cast<const char*>(data), size); }
template <class T> void put_value(std::string& out, T v) { put_bytes(out, &v, sizeof v); }

// The fixture manager's routines set_up files its fixtures with are answered, as in the league checker's
// season-setup cases (league_base_checks_season.cpp manager_answers): they reach the file system otherwise. The
// answers are the same in every run.
bool answer_fixture_manager(State& s) {
    static const std::map<std::uint32_t, std::uint32_t> answers = {
        {0x1048a2a0, 1}, {0x10738c20, 0}, {0x10738e60, 0}, {0x104aff20, 0}, {0x104888f0, 0}, {0x1048a140, 0},
        {0x102bdf90, 0}, {0x102cb4b0, 0}, {0x102cc140, 0}, {0x1048b180, 0}};
    const auto it = answers.find(s.pc);
    if (it == answers.end()) return false;
    s.wr<4>(guest::reg::gpr(3), it->second);
    s.pc = std::uint32_t(s.r<4>(guest::reg::lr));
    return true;
}

enum class Platform { none, empty, entries };
// Runs `entry` with r3 = object in a child; `watch` lists the pages whose bytes come back whole.
Run run(State& s, std::uint32_t entry, std::uint32_t object, Platform platform, const ov::OverrideSet& set,
        const fs::path& root, const std::set<std::uint32_t>& watch) {
    int fds[2];
    if (::pipe(fds) != 0) throw std::runtime_error("pipe");
    const auto pid = ::fork();
    if (pid == 0) {
        ::close(fds[0]);
        std::string out;
        std::string error;
        try {
            if (platform != Platform::none) {
                s.platform = std::make_shared<NativePlatform>(root / "game", root / ("child-" + std::to_string(::getpid())), AudioMode::offline, EventMode::isolated);
                if (platform == Platform::entries) s.platform->career_rules().set_competition_overrides(set);
            }
            guest::Cpu c(s);
            c.set(3, object);
            c.set_lr(return_sentinel);
            s.pc = entry;
            s.budget = s.executed + 20000000000ull;
            run_until(s, return_sentinel);
            if (s.pc != return_sentinel) error = "did not return";
            s.platform.reset();
        } catch (const std::exception& e) { error = e.what(); }
        put_value(out, std::uint32_t(error.size())); put_bytes(out, error.data(), error.size());
        put_bytes(out, s.registers.data(), s.registers.size());
        put_value(out, s.pc);
        std::vector<std::pair<std::uint32_t, std::uint64_t>> hashes;
        std::vector<std::uint8_t> buffer;
        for (const auto& [low, length] : s.memory.mapped_ranges()) {
            for (std::uint64_t at = low; at < std::uint64_t(low) + length; at += page) {
                const auto size = std::size_t(std::min<std::uint64_t>(page, std::uint64_t(low) + length - at));
                buffer = s.memory.copy_out(std::uint32_t(at), size);
                hashes.emplace_back(std::uint32_t(at), fnv(buffer.data(), buffer.size()));
                if (watch.contains(std::uint32_t(at))) {
                    put_value(out, std::uint32_t(1)); put_value(out, std::uint32_t(at)); put_value(out, std::uint32_t(size));
                    put_bytes(out, buffer.data(), buffer.size());
                }
            }
        }
        put_value(out, std::uint32_t(0));
        put_value(out, std::uint32_t(hashes.size()));
        put_bytes(out, hashes.data(), hashes.size() * sizeof hashes[0]);
        for (std::size_t done = 0; done < out.size();) {
            const auto n = ::write(fds[1], out.data() + done, out.size() - done);
            if (n <= 0) _exit(3);
            done += std::size_t(n);
        }
        _exit(0);
    }
    ::close(fds[1]);
    std::string in;
    char chunk[1 << 16];
    for (ssize_t n; (n = ::read(fds[0], chunk, sizeof chunk)) > 0;) in.append(chunk, std::size_t(n));
    ::close(fds[0]);
    int status = 0;
    ::waitpid(pid, &status, 0);
    Run r;
    std::size_t at = 0;
    const auto take = [&](void* data, std::size_t size) {
        if (at + size > in.size()) throw std::runtime_error("short child output");
        std::memcpy(data, in.data() + at, size);
        at += size;
    };
    const auto take32 = [&] { std::uint32_t v; take(&v, sizeof v); return v; };
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) { r.error = "child failed"; return r; }
    r.error.resize(take32()); take(r.error.data(), r.error.size());
    r.registers.resize(s.registers.size()); take(r.registers.data(), r.registers.size());
    r.pc = take32();
    while (take32() == 1) {
        const auto address = take32();
        std::vector<std::uint8_t> bytes(take32());
        take(bytes.data(), bytes.size());
        r.watched[address] = std::move(bytes);
    }
    const auto count = take32();
    std::vector<std::pair<std::uint32_t, std::uint64_t>> hashes(count);
    take(hashes.data(), count * sizeof hashes[0]);
    r.hashes.insert(hashes.begin(), hashes.end());
    return r;
}

// The bytes a set of entries must leave, from the registry's offsets and widths: address -> byte.
using Expected = std::map<std::uint32_t, std::uint8_t>;
void expect_value(Expected& e, std::uint32_t at, std::uint8_t width, std::int32_t value) {
    for (std::uint8_t i = 0; i < width; ++i) e[at + i] = std::uint8_t(std::uint32_t(value) >> (8 * (width - 1 - i)));
}

// Problems between a run without entries (`base`) and one with (`changed`): registers, every page outside the
// watched ones, and every byte of the watched pages against `expected`.
std::vector<std::string> differences(const Run& base, const Run& changed, const Expected& expected) {
    std::vector<std::string> problems;
    if (!base.error.empty() || !changed.error.empty()) problems.push_back("run error: " + base.error + " / " + changed.error);
    if (base.registers != changed.registers) problems.push_back("registers differ");
    if (base.pc != changed.pc) problems.push_back("pc differs");
    if (base.hashes.size() != changed.hashes.size()) problems.push_back("mapped pages differ");
    for (const auto& [address, hash] : base.hashes) {
        if (base.watched.contains(address)) continue;
        const auto other = changed.hashes.find(address);
        if (other == changed.hashes.end() || other->second != hash) {
            char text[64];
            std::snprintf(text, sizeof text, "page %08x differs", address);
            problems.push_back(text);
        }
    }
    for (const auto& [address, bytes] : base.watched) {
        const auto other = changed.watched.find(address);
        if (other == changed.watched.end() || other->second.size() != bytes.size()) { problems.push_back("watched page missing"); continue; }
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            const auto at = address + std::uint32_t(i);
            const auto it = expected.find(at);
            const auto want = it == expected.end() ? bytes[i] : it->second;
            if (other->second[i] != want) {
                char text[96];
                std::snprintf(text, sizeof text, "byte %08x: %02x, expected %02x (without entries %02x)", at, other->second[i], want, bytes[i]);
                problems.push_back(text);
            }
        }
    }
    return problems;
}
std::string first(const std::vector<std::string>& problems) {
    return problems.empty() ? "" : " [" + problems.front() + (problems.size() > 1 ? ", +" + std::to_string(problems.size() - 1) : "") + "]";
}
std::set<std::uint32_t> pages(std::uint32_t from, std::uint32_t size) {
    std::set<std::uint32_t> out;
    for (auto at = from & ~(page - 1); at < from + size; at += page) out.insert(at);
    return out;
}

// One routine on one object: no platform, empty set, entries; the comparisons; and the negative control.
void check_routine(State& s, const fs::path& root, const std::string& what, std::uint32_t entry, std::uint32_t object,
                   const ov::OverrideSet& set, const Expected& expected, const Expected& wrong, std::set<std::uint32_t> watch) {
    const auto none = run(s, entry, object, Platform::none, {}, root, watch);
    if (!none.error.empty()) { expect(false, what + ": the run without entries failed: " + none.error); return; }
    if (entry == 0x104a00a0)
        expect(read_be<4>(none.registers.data() + guest::reg::gpr(3)) == 1, what + ": set_up ran to its success return (r3 = 1)");
    const auto empty = run(s, entry, object, Platform::empty, {}, root, watch);
    const auto with = run(s, entry, object, Platform::entries, set, root, watch);
    const auto same = differences(none, empty, {});
    expect(same.empty() && none.hashes.size() > 0, what + ": an empty set is byte-identical to no platform, registers included (" +
           std::to_string(none.hashes.size()) + " pages, " + std::to_string(none.watched.size()) + " watched)" + first(same));
    const auto changed = differences(none, with, expected);
    expect(changed.empty(), what + ": the entries change exactly their " + std::to_string(expected.size()) + " expected bytes" + first(changed));
    std::size_t moved = 0;
    for (const auto& [address, byte] : expected) {
        const auto pageof = address & ~(page - 1);
        if (none.watched.contains(pageof) && none.watched.at(pageof)[address - pageof] != byte) ++moved;
    }
    expect(moved > 0, what + ": the entries change some byte (" + std::to_string(moved) + ")");
    expect(!differences(none, with, wrong).empty(), what + ": negative control: an expectation with a wrong offset is caught");
}

const cv::Parameter* find(std::string_view id) { return cv::find_parameter(id); }

void check_snapshot(const fs::path& snapshot, const fs::path& root) {
    auto state = std::make_unique<State>();
    auto& s = *state;
    load_match_snapshot(s, snapshot);
    dispatch_probe = answer_fixture_manager;
    const auto all = objects(s);
    // Keys: every league and cup object's own fields (record id, +0x44) give the key its place in the world gives;
    // a CUP_STAGE object (vtable at TOC -0x613c) has no key.
    const auto cup_stage_vtable = u32(s, toc_base + std::uint32_t(-0x613c));  // the TOC word is the vtable's address
    std::size_t matched = 0, leagues = 0, cups = 0, stages = 0, cup_stages = 0, cup_stage_minus_one = 0, cup_stage_top = 0;
    std::uint32_t a_cup_stage = 0;
    std::int32_t a_cup_stage_parent = -1;
    std::string mismatch;
    for (const auto& o : all) {
        const auto t = ov::target_of(s, o.address);
        const bool cup_stage = u32(s, o.address) == cup_stage_vtable;
        if (cup_stage) {
            ++cup_stages;
            cup_stage_minus_one += std::int8_t(s.memory.read<1>(o.address + 0x44)) == -1;
            cup_stage_top += o.expected.stage < 0;
            if (!a_cup_stage) { a_cup_stage = o.address; a_cup_stage_parent = o.expected.id; }
        }
        if (cup_stage ? !t : t && *t == o.expected) ++matched;
        else if (mismatch.empty()) mismatch = " (first: " + ov::key(o.expected) + ")";
        (o.expected.kind == L ? leagues : cups) += 1;
        stages += o.expected.stage >= 0;
    }
    expect(matched == all.size() && !all.empty() && cup_stage_top == 0, "every object's key from its own fields, CUP_STAGE objects none: " +
           std::to_string(matched) + " of " + std::to_string(all.size()) + " (" + std::to_string(leagues) + " leagues, " +
           std::to_string(cups) + " cups, " + std::to_string(stages) + " stages, " + std::to_string(cup_stages) + " CUP_STAGE objects, " +
           std::to_string(cup_stage_minus_one) + " of them with +0x44 = -1)" + mismatch);

    // A league (the English Premier Division: nation ING, 20 clubs) and a league stage.
    const auto league_entries = [&](const ov::Target& t, std::int32_t season, Expected& expected, Expected& wrong, std::uint32_t object) {
        ov::OverrideSet set;
        const auto add = [&](std::string_view id, std::int32_t value, std::optional<std::int32_t> from, bool applies) {
            const auto o = ov::Override{.target = t, .parameter = find(id), .value = value, .from_season = from};
            expect(set.add(o).empty(), "entry " + ov::describe(o));
            if (applies) expect_value(expected, object + o.parameter->offset, o.parameter->width, value);
        };
        add("league.points.win", 2, std::nullopt, true);
        add("league.points.win", 5, season + 1, false);    // a later season: not this one
        add("league.points.draw", 0, season, true);         // this season
        add("league.promotion.automatic", 5, std::nullopt, true);
        add("league.relegation.places", 6, std::nullopt, true);
        add("league.tiebreak.2", 5, std::nullopt, true);
        add("league.prize.pool", 123456789, std::nullopt, true);
        add("league.prize.sharing", 2, std::nullopt, true);
        if (t.stage < 0) add("league.ruling_body", 3, std::nullopt, false);   // read by governing_body, never written
        wrong = expected;
        const auto& win = *find("league.points.win");
        wrong.erase(object + win.offset);
        expect_value(wrong, object + win.offset + 1, 1, 2);
        return set;
    };
    const auto season_of = [&](std::uint32_t object) { return std::int32_t(std::int16_t(s.memory.read<2>(object + comp::competition::year))); };
    std::uint32_t premier = 0, stage = 0;
    ov::Target premier_target, stage_target;
    {
        const auto values = cv::read_competitions(s);
        for (const auto& o : all) {
            if (o.expected.kind != L) continue;
            const auto clubs = std::int16_t(s.memory.read<2>(o.address + 0x3e));
            for (const auto& e : values)
                if (e.kind == L && e.identity.id == o.expected.id && e.identity.stage == o.expected.stage && o.expected.stage < 0 &&
                    e.identity.nation == "ING" && clubs == 20 && !premier) { premier = o.address; premier_target = o.expected; }
            if (o.expected.stage >= 0 && !stage && std::int16_t(s.memory.read<2>(o.address + 0x3e)) >= 4) { stage = o.address; stage_target = o.expected; }
        }
    }
    expect(premier && stage, "the Premier Division and a league stage were found");
    for (const auto& [object, target, name] : {std::tuple(premier, premier_target, "league set_up (Premier, " + ov::key(premier_target) + ")"),
                                                std::tuple(stage, stage_target, "league set_up (stage " + ov::key(stage_target) + ")")}) {
        if (!object) continue;
        Expected expected, wrong;
        const auto set = league_entries(target, season_of(object), expected, wrong, object);
        check_routine(s, root, name, 0x104a00a0, object, set, expected, wrong, pages(object, 0x100));
    }

    // A cup whose vt+0x9c is CUP.CPP's tree, with round rows.
    std::uint32_t cup = 0;
    ov::Target cup_target;
    for (const auto& o : all) {
        if (o.expected.kind != C || o.expected.stage >= 0) continue;
        const auto vtable = u32(s, o.address);
        const auto rounds = std::int16_t(s.memory.read<2>(o.address + comp::competition::rounds));
        if (u32(s, u32(s, vtable + 0x9c)) == 0x10479100 && rounds >= 3 && u32(s, o.address + cup_base::cup::rounds)) {
            cup = o.address; cup_target = o.expected; break;
        }
    }
    expect(cup != 0, "a cup with CUP.CPP's tree and three rounds was found");
    if (cup) {
        const auto season = season_of(cup);
        const auto rounds = std::int16_t(s.memory.read<2>(cup + comp::competition::rounds));
        const auto table = u32(s, cup + cup_base::cup::rounds);
        ov::OverrideSet set;
        Expected expected;
        const auto add = [&](std::string_view id, std::int32_t value, std::int32_t round, std::optional<std::int32_t> from, bool applies) {
            const auto o = ov::Override{.target = cup_target, .parameter = find(id), .round = round, .value = value, .from_season = from};
            expect(set.add(o).empty(), "entry " + ov::describe(o));
            const auto base = round < 0 ? cup : table + std::uint32_t(round) * cup_base::cup::round_row;
            if (applies) expect_value(expected, base + o.parameter->offset, o.parameter->width, value);
        };
        add("cup.prize.pool", 7654321, -1, std::nullopt, true);
        add("cup.bench", 4, -1, std::nullopt, true);
        add("cup.substitutions", 2, -1, season, true);
        add("cup.ruling_body", 5, -1, std::nullopt, false);
        add("cup.round.legs", 2, 0, std::nullopt, true);
        add("cup.round.legs", 1, 0, season + 1, false);
        add("cup.round.prize.match", 1111, 1, std::nullopt, true);
        add("cup.round.prize.winner", -1, rounds - 1, std::nullopt, true);
        add("cup.round.prize.loser", 2222, rounds - 1, std::nullopt, true);
        add("cup.round.replay_ties", 3, rounds, std::nullopt, false);   // a round the cup does not have: ignored
        auto wrong = expected;
        const auto& legs = *find("cup.round.legs");
        wrong.erase(table + legs.offset);
        expect_value(wrong, table + legs.offset - 1, 1, 2);
        auto watch = pages(cup, 0x100);
        for (const auto p : pages(table, std::uint32_t(rounds + 1) * cup_base::cup::round_row)) watch.insert(p);
        check_routine(s, root, "cup tree (" + ov::key(cup_target) + ")", 0x10479100, cup, set, expected, wrong, watch);
    }

    // The rest runs in this process, with a platform.
    s.platform = std::make_shared<NativePlatform>(root / "game", root / "snapshot", AudioMode::offline, EventMode::isolated);
    auto& career = s.platform->career_rules();
    guest::Cpu c(s);
    // The record of defaults: nothing without a known edition; then one entry per competition, with the object's
    // values, and none for a competition the career has entries for. The guest is only read.
    if (premier && cup) {
        const auto directory = s.platform->competition_defaults_directory();
        const auto file = CompetitionDefaults::file(directory, CompetitionEdition::cm0102);
        const auto before = s.memory.copy_out(premier, 0xec);
        ov::at_setup(s, premier, L);
        career.read("CM-NATIVE-RULES 1\nedition cm0102\nrevision 1\n");
        ov::at_setup(s, premier, L);
        expect(!std::filesystem::exists(file), "no defaults are recorded for an unrecognised database or a loaded career");
        career.reset();
        CompetitionDefaults defaults(directory);
        defaults.record(s, premier, premier_target, CompetitionEdition::cm0102);
        defaults.record(s, premier, premier_target, CompetitionEdition::cm0102);
        const auto recorded = read_competition_defaults(file);
        expect(recorded.size() == 1 && recorded.front().kind == L && recorded.front().identity.id == premier_target.id &&
               recorded.front().identity.stage == -1 &&
               recorded.front().values.at("league.points.win") == std::int8_t(s.memory.read<1>(premier + 0xc4)) &&
               recorded.front().values.at("league.structure.clubs") == 20, "defaults: the league once, with its values");
        expect(s.memory.copy_out(premier, 0xec) == before, "recording defaults leaves the league's object alone");
        // A torn record is skipped and the others kept; a later session records only what is missing.
        { std::ofstream(file, std::ios::app) << "# Competition values: torn\n[league.77]\nid = 77\n"; }
        CompetitionDefaults again(directory);
        again.record(s, premier, premier_target, CompetitionEdition::cm0102);
        again.record(s, cup, cup_target, CompetitionEdition::cm0102);
        unsigned skipped = 0;
        const auto reread = read_competition_defaults(file, &skipped);
        expect(reread.size() == 2 && skipped == 1 && reread.back().kind == C, "defaults: a torn record is skipped, a later session adds only the new cup");
    }
    // The ruling body as governing_body sees it.
    const auto manager = global(s, -0x7aa4);
    if (manager && premier) {
        const auto stored = std::int8_t(s.memory.read<1>(premier + 0x4a));
        const auto body = [&] { return discipline::governing_body(c, manager, premier); };
        const auto stored_body = stored < 0 ? 0u : discipline::body_object(c, manager, std::uint32_t(std::int32_t(stored)));
        expect(body() == stored_body, "governing_body without entries uses +0x4a");
        const std::int8_t other = std::int8_t(stored == 3 ? 4 : 3);
        ov::OverrideSet set;
        set.add({.target = premier_target, .parameter = find("league.ruling_body"), .value = other});
        set.add({.target = premier_target, .parameter = find("league.ruling_body"), .value = -1, .from_season = season_of(premier) + 1});
        career.set_competition_overrides(set);
        expect(body() == discipline::body_object(c, manager, std::uint32_t(other)) && s.memory.read<1>(premier + 0x4a) == std::uint8_t(stored),
               "governing_body uses the override for the season, +0x4a unchanged");
        s.memory.write<2>(premier + 0x40, std::uint16_t(season_of(premier) + 1));
        expect(body() == 0, "a later season's override (no ruling body) takes over in that season");
        s.memory.write<2>(premier + 0x40, std::uint16_t(season_of(premier) - 1));
        career.set_competition_overrides({});
        expect(body() == stored_body, "governing_body without entries again");
    } else expect(false, "the discipline manager and the Premier Division");

    // The Premier League's named-reserves rule yields to a bench entry, in the record after a load and in fixtures.
    if (premier && u32(s, premier) == c.toc(-0x6020)) {
        const auto season = season_of(premier);
        s.memory.write<1>(premier + 0x46, 4);
        english_rules::premier_bench(c, premier);
        const auto without = s.memory.read<1>(premier + 0x46);
        s.memory.write<1>(premier + 0x46, 4);
        ov::OverrideSet set;
        set.add({.target = premier_target, .parameter = find("league.bench"), .value = 4});
        career.set_competition_overrides(set);
        english_rules::premier_bench(c, premier);
        // CM_ENGLISH_RULES_ORIGINAL (the shipped game's rules) has no named-reserves rule: the byte stays.
        const bool original = std::getenv("CM_ENGLISH_RULES_ORIGINAL") != nullptr;
        expect(without == (original ? 4 : 7) && s.memory.read<1>(premier + 0x46) == 4, "premier_bench keeps a bench entry (without one it applies its rule)");
        const auto fixture = c.sp() - 0x2000;
        const auto make = [&] {
            s.memory.write<4>(fixture, u32(s, c.toc(-0x67a0)));
            s.memory.write<1>(fixture + 0x43, 0xff);
            s.memory.write<2>(fixture + 0x2c, std::uint16_t(season));
            s.memory.write<1>(fixture + 0x4e, 0x43);
        };
        make();
        english_rules::loaded_premier_fixture(c, fixture);
        const auto kept = s.memory.read<1>(fixture + 0x4e);
        career.set_competition_overrides({});
        make();
        english_rules::loaded_premier_fixture(c, fixture);
        expect(kept == 0x43 && s.memory.read<1>(fixture + 0x4e) == (original ? 0x43 : 0x73) && u32(s, c.toc(-0x67a0)) == std::uint32_t(premier_target.id),
               "a loaded Premier fixture keeps its bench under an entry (without one it gets 7)");
    } else expect(false, "the Premier Division's vtable");

    // The places at the ends of the table are checked against the club count at the apply point.
    if (premier) {
        const auto before = s.memory.copy_out(premier, 0xec);
        const auto clubs = std::int16_t(s.memory.read<2>(premier + 0x3e));
        const auto places = [&](std::int32_t automatic, std::int32_t playoff, std::int32_t relegation) {
            s.memory.copy_in(premier, before);
            ov::OverrideSet set;
            set.add({.target = premier_target, .parameter = find("league.points.win"), .value = 2});
            if (automatic >= 0) set.add({.target = premier_target, .parameter = find("league.promotion.automatic"), .value = automatic});
            if (playoff >= 0) set.add({.target = premier_target, .parameter = find("league.promotion.playoff"), .value = playoff});
            if (relegation >= 0) set.add({.target = premier_target, .parameter = find("league.relegation.places"), .value = relegation});
            career.set_competition_overrides(set);
            ov::at_setup(s, premier, L);
            return std::tuple(s.memory.read<1>(premier + 0xc0), s.memory.read<1>(premier + 0xc1), s.memory.read<1>(premier + 0xc3),
                              s.memory.read<1>(premier + 0xc4));
        };
        const auto original = std::tuple(before[0xc0], before[0xc1], before[0xc3]);
        const auto [a1, p1, r1, w1] = places(15, -1, 10);
        expect(std::tuple(a1, p1, r1) == original && w1 == 2, "automatic 15 + relegation 10 over " + std::to_string(clubs) + " clubs: places skipped, points written");
        const auto [a2, p2, r2, w2] = places(-1, std::int32_t(clubs) + 1, -1);
        expect(std::tuple(a2, p2, r2) == original && w2 == 2, "a playoff count above the club count: skipped");
        const auto [a3, p3, r3, w3] = places(4, 4, 4);
        expect(a3 == 4 && p3 == 4 && r3 == 4 && w3 == 2, "4 + 4 + 4 places over " + std::to_string(clubs) + " clubs: written");
        s.memory.copy_in(premier, before);
    }
    // A CUP_STAGE object is skipped by the hook, whatever entries its parent has.
    if (a_cup_stage) {
        const auto before = s.memory.copy_out(a_cup_stage, 0xb4);
        ov::OverrideSet set;
        set.add({.target = {C, a_cup_stage_parent, -1}, .parameter = find("cup.bench"), .value = 2});
        set.add({.target = {C, a_cup_stage_parent, -1}, .parameter = find("cup.prize.pool"), .value = 99});
        career.set_competition_overrides(set);
        ov::at_setup(s, a_cup_stage, C);
        expect(s.memory.copy_out(a_cup_stage, 0xb4) == before, "the cup hook leaves a CUP_STAGE object alone");
    } else expect(false, "a CUP_STAGE object in the snapshot");
    career.set_competition_overrides({});

    // apply_now: the live record, without a setup.
    if (premier && cup) {
        ov::OverrideSet set;
        set.add({.target = premier_target, .parameter = find("league.points.win"), .value = 2});
        set.add({.target = premier_target, .parameter = find("league.points.draw"), .value = 0, .from_season = season_of(premier) + 1});
        set.add({.target = premier_target, .parameter = find("league.ruling_body"), .value = 2});
        set.add({.target = cup_target, .parameter = find("cup.bench"), .value = 6});
        set.add({.target = cup_target, .parameter = find("cup.prize.pool"), .value = 1});
        set.add({.target = cup_target, .parameter = find("cup.round.legs"), .round = 0, .value = 1});
        career.set_competition_overrides(set);
        const auto draw = s.memory.read<1>(premier + 0xc5);
        const auto pool = u32(s, cup + 0x28);
        const auto legs = s.memory.read<1>(u32(s, cup + cup_base::cup::rounds) + 0x23);
        const auto league = ov::apply_now(s, premier_target);
        expect(league.found && league.written.size() == 1 && league.live.size() == 1 && s.memory.read<1>(premier + 0xc4) == 2 &&
               s.memory.read<1>(premier + 0xc5) == draw, "apply_now writes a league's value in force, not a later season's");
        const auto knockout = ov::apply_now(s, cup_target);
        expect(knockout.found && knockout.written.size() == 1 && knockout.deferred.size() == 2 && s.memory.read<1>(cup + 0x46) == 6 &&
               u32(s, cup + 0x28) == pool && s.memory.read<1>(u32(s, cup + cup_base::cup::rounds) + 0x23) == legs,
               "apply_now writes a cup's bench and defers its pool and rounds");
        expect(!ov::apply_now(s, {L, 99999, -1}).found && !ov::apply_now(s, {C, premier_target.id, -1}).found, "apply_now finds no object for a wrong key or kind");
        // Places beyond the club count are no more written here than at set_up.
        {
            const auto before = s.memory.copy_out(premier, 0xe8);
            ov::OverrideSet many;
            many.add({.target = premier_target, .parameter = find("league.relegation.places"),
                      .value = std::int16_t(s.memory.read<2>(premier + 0x3e)) + 1});
            career.set_competition_overrides(many);
            const auto report = ov::apply_now(s, premier_target);
            expect(report.found && report.written.empty() && s.memory.copy_out(premier, 0xe8) == before,
                   "apply_now writes no relegation places beyond the club count");
            s.memory.copy_in(premier, before);
        }
        career.set_competition_overrides({});
    }
    s.platform.reset();
}
}  // namespace

// The editor's document (competition_editor_document.hpp), without a window.
void check_editor_document() {
    using competition_values::Entry;
    const auto param = [](std::string_view id) -> const ov::Parameter& { return *competition_values::find_parameter(id); };
    const auto entry = [](competition_values::Kind kind, std::int32_t id, std::int32_t stage, const char* nation) {
        Entry e;
        e.kind = kind;
        e.identity.id = id;
        e.identity.stage = stage;
        e.identity.nation = nation;
        for (const auto& p : competition_values::parameters())
            if (p.scope == (kind == competition_values::Kind::league ? competition_values::Scope::league : competition_values::Scope::cup))
                e.values[std::string(p.id)] = p.read_only ? 20 : p.min + 1;
        return e;
    };
    auto cup = entry(competition_values::Kind::cup, 7, -1, "ING");
    cup.rounds.resize(2);
    for (auto& row : cup.rounds)
        for (const auto& p : competition_values::parameters())
            if (p.scope == competition_values::Scope::cup_round) row[std::string(p.id)] = 1;
    CompetitionEditorDocument doc(CompetitionEdition::cm0102,
                                  {cup, entry(competition_values::Kind::cup, 7, 0, "ING"), entry(competition_values::Kind::league, 3, -1, "ING"),
                                   entry(competition_values::Kind::league, 1, -1, "BEL")});
    const auto& list = doc.competitions();
    expect(list.size() == 3 && list[0].identity.id == 1 && list[1].identity.id == 3 && list[2].kind == competition_values::Kind::cup,
           "editor: by nation, leagues first, cup stages left out");
    const auto& win = param("league.points.win");
    expect(doc.value(1, win, -1) == 1 && !doc.edited(1, win, -1) && !doc.dirty(), "editor: a new document shows the originals");
    expect(doc.set(1, win, -1, 2).empty() && doc.value(1, win, -1) == 2 && doc.original(1, win, -1) == 1 && doc.edited(1, win, -1) &&
           doc.modified(1) && !doc.modified(0) && doc.dirty(), "editor: an edit");
    expect(!doc.set(1, win, -1, 500).empty() && doc.value(1, win, -1) == 2, "editor: a value out of range is refused and nothing changes");
    expect(!doc.set(1, param("league.relegation.places"), -1, 21).empty() && !doc.set(1, param("league.promotion.playoff"), -1, 19).empty() &&
           doc.set(1, param("league.relegation.places"), -1, 4).empty(), "editor: places beyond the 20 clubs are refused");
    expect(!doc.set(1, param("league.tiebreak.1"), -1, 4).empty(), "editor: head-to-head is refused");
    expect(!doc.warnings(1).empty() && doc.warnings(0).empty(), "editor: changed places carry the neighbouring-division warning");
    expect(doc.set(2, param("cup.round.legs"), 1, 2).empty() && !doc.set(2, param("cup.round.legs"), 2, 2).empty() &&
           doc.value(2, param("cup.round.legs"), 1) == 2 && doc.value(2, param("cup.round.legs"), 0) == 1, "editor: a cup round, and none beyond the cup's");
    expect(doc.set(1, win, -1, 1).empty() && !doc.edited(1, win, -1), "editor: the original value removes the edit");
    doc.rename("Teste \xC3\xA7");
    // Through the file and back, with a scheduled entry the window does not edit and one for an unknown competition.
    auto database = doc.database();
    database.overrides.add({.target = {competition_values::Kind::league, 3, -1}, .parameter = &win, .value = 2, .from_season = 2004});
    database.overrides.add({.target = {competition_values::Kind::league, 99, -1}, .parameter = &win, .value = 5});
    const auto read = ov::read_custom_database(ov::write_custom_database(database));
    CompetitionEditorDocument again(CompetitionEdition::cm0102, {cup, entry(competition_values::Kind::league, 3, -1, "ING"), entry(competition_values::Kind::league, 1, -1, "BEL")});
    expect(read && again.load(*read).empty() && !again.dirty() && again.name == doc.name && again.value(1, param("league.relegation.places"), -1) == 4 &&
           again.value(1, win, -1) == 1 && again.scheduled(1) == 1 && again.database().overrides == database.overrides,
           "editor: a saved database opens with its edits, keeps a scheduled entry and an unknown competition's");
    again.revert(1);
    expect(again.scheduled(1) == 1 && !again.edited(1, param("league.relegation.places"), -1) && again.dirty(), "editor: revert keeps scheduled entries");
    // Every text and every registry label exists in every language of the game, with its placeholders.
    bool complete = true;
    const auto holes = [](std::string_view text) { return std::count(text.begin(), text.end(), '{'); };
    for (unsigned language = 0; language < editor_language_count; ++language) {
        for (std::size_t i = 0; i < std::size_t(EditorText::count); ++i) {
            const auto text = editor_text(language, EditorText(i));
            complete &= !text.empty() && holes(text) == holes(editor_text(0, EditorText(i)));
        }
        for (const auto& p : competition_values::parameters())
            complete &= language < 2 || editor_label(language, p) != std::string(p.label) || p.id == "league.prize.pool" || p.id == "cup.prize.pool";
    }
    expect(complete, "editor: every text and label is translated in the 11 game languages");
    expect(editor_text(9, EditorText::save) == "Salvar" && editor_text(200, EditorText::save) == "Save" &&
           editor_label(9, param("league.tiebreak.2")) == "Critério de desempate 2" &&
           filled(editor_text(9, EditorText::round_of), {"1", "3"}) == "Rodada 1 de 3", "editor: Portuguese texts, and English for an unknown language");
    CompetitionEditorDocument other(CompetitionEdition::si_3968, {});
    expect(read && !other.load(*read).empty(), "editor: a database of another base is refused");
}

int main(int argc, char** argv) {
    const auto root = fs::temp_directory_path() / ("cm-competition-overrides-checks-" + std::to_string(::getpid()));
    try {
        fs::create_directories(root / "game");
        check_file();
        check_set();
        check_member();
        check_adopt();
        check_editor_document();
        check_platform(root);
        if (argc > 1) check_snapshot(argv[1], root);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        ++failures;
    }
    std::error_code error;
    fs::remove_all(root, error);
    std::printf("competition overrides: %d passed, %d failed%s\n", passed, failures, argc > 1 ? "" : " (no snapshot)");
    return failures ? 1 : 0;
}
