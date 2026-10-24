// cm-tactic-lab: plays complete detailed matches between two original clubs
// with a chosen tactic (.tct) for one side and reports the scores. A research
// tool: it measures tactics; it changes no game file and no production path.
//
//   cm-tactic-lab PROGRAM GAME SCRATCH --home ID --away ID [--side 0|1]
//       [--seeds N] [--first-seed S] [--watch] TACTIC...
//   cm-tactic-lab GAME --list-clubs
//
// TACTIC is a .tct file, `respot:FILE` to place the file's players at its
// position codes as the tactics screen does (its own spots are dropped),
// `load:FILE` to read the file with the game's own loader as the tactics
// screen does (it reports the codes the loader kept and the assertion reports
// it raised in a "loaded" line), or `ai` for the side's own AI choice. Each tactic
// plays N matches, seeds S..S+N-1; the same seeds for every tactic (common
// random numbers). Output: one line per match ("match TACTIC SEED FOR AGAINST
// ASSERTIONS SHOTS ON-TARGET SHOTS-AGAINST ON-TARGET-AGAINST"; assertion
// reports are those the game would have shown as dialogs), then a summary per
// tactic.
// One process plays its matches in turn; run several over different seed
// ranges for more cores (the platform's frameworks do not survive fork).
//
// How a tactic gets in: the AI team setup (0x10206ea0) chooses a formation
// (0x1020d7d0), picks the players for its positions (0x10210c50) and then
// adjusts the formation to its manager (0x1020a290). A dispatch probe copies
// the tactic over the side's formation record in the match after both calls,
// so the original selector fields the best players for the tactic's
// positions and the manager's adjustments are undone, as for a human
// manager's formation. During the match a memory observer undoes every write
// to the tactic's fields at once: the AI's in-match changes of mentality,
// counter attack, forward runs and so on (--watch lists them instead). The
// side keeps the AI's substitutions, injury changes and set-piece takers.
#include "match_debugger_checkpoint.hpp"
#include "match_debugger_database.hpp"
#include "match_debugger_factory.hpp"
#include "routine_check.hpp"
#include <unistd.h>
#include <array>
#include <charconv>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace cm::aot;
using namespace cm::aot::match_debugger;

constexpr std::uint32_t formation_size = 0x18e0, formations = 0x7c6c, teams = 0x7aac, team_size = 0xd8;
constexpr std::uint32_t home_score = 0xe03d, away_score = 0xe073;
// Return addresses in 0x10206ea0: after the formation choice and after the
// manager's adjustments; r24 holds the team record.
constexpr std::uint32_t after_choice = 0x10208cc4, after_adjustment = 0x1020975c;

// The tactic fields of a formation record (formation.hpp, formation_zones.hpp).
struct Range { std::uint32_t offset, size; };
constexpr std::array<Range, 7> tactic_fields{{
    {0x12e, 0x38},    // positions, first holders, positions with the ball
    {0x166, 0x420},   // spots
    {0x588, 4},       // team options
    {0x5b8, 11},      // roles
    {0x5f0, 0x1290},  // zone grids and their mirror images
    {0x1880, 4},      // shape
    {0x1888, 0x58}}}; // player instructions

struct Tactic {
    std::string name;
    bool ai = false, respot = false;  // respot: spots from the position codes, as the UI sets them
    std::vector<std::uint8_t> file;
    std::string guest_path;  // load: the file's copy in the user data folder, for the original loader
};

std::uint32_t read_word(const std::uint8_t* p, bool little) {
    return little ? std::uint32_t(p[0] | p[1] << 8 | p[2] << 16 | std::uint32_t(p[3]) << 24)
                  : std::uint32_t(std::uint32_t(p[0]) << 24 | p[1] << 16 | p[2] << 8 | p[3]);
}

std::uint32_t invoke(State& state, std::uint32_t entry, std::initializer_list<std::uint32_t> arguments) {
    unsigned reg = 3;
    for (auto value : arguments) state.wr<4>(reg++ * 4, value);
    state.wr<4>(4128, return_sentinel);
    state.pc = entry;
    run_until(state, return_sentinel);
    return std::uint32_t(state.r<4>(12));
}

std::uint32_t build_formation_record(State& state, const Tactic& tactic);

// Builds a formation record in guest memory from a .tct file (current
// format), as the original loader 0x10115000 leaves it: the fields as read,
// the team option fix-up, each player's first spot set again by hand, the
// zone grids rebuilt.
std::uint32_t build_formation(State& state, const Tactic& tactic) {
    // The original reports a failed assertion in a modal dialog, which waits
    // for a click here; while this byte is set it writes the report to its
    // log instead (0x10769b60). A hand-edited spot out of range asserts in
    // the loader's round trip below, as in the game, and the load goes on.
    constexpr std::uint32_t log_assertions = 0x108b8540;
    const auto logging = state.memory.read<1>(log_assertions);
    state.memory.write<1>(log_assertions, 1);
    const auto formation = build_formation_record(state, tactic);
    state.memory.write<1>(log_assertions, logging);
    return formation;
}

std::uint32_t build_formation_record(State& state, const Tactic& tactic) {
    if (!tactic.guest_path.empty()) {
        // As the tactics screen loads a file (0x103c7ad0): quiet, not a preset.
        const auto formation = invoke(state, 0x10773520, {formation_size});
        const auto path = invoke(state, 0x10773520, {std::uint32_t(tactic.guest_path.size() + 1)});
        if (!formation || !path) throw std::runtime_error("Could not allocate a formation record");
        (void)invoke(state, 0x10110c80, {formation});
        for (std::uint32_t i = 0; i <= tactic.guest_path.size(); ++i)
            state.memory.write<1>(path + i, i < tactic.guest_path.size() ? std::uint8_t(tactic.guest_path[i]) : 0);
        if (!(invoke(state, 0x10115000, {formation, path, 1, 0}) & 0xff))
            throw std::runtime_error(tactic.name + ": the game's loader refused the file");
        return formation;
    }
    const auto& f = tactic.file;
    if (f.size() < 1472) throw std::runtime_error(tactic.name + ": not a formation file");
    const bool little = read_word(f.data(), false) < 0x98ec54 || read_word(f.data(), false) > 0x98ec5e;
    if (read_word(f.data(), little) != 0x98ec5e) throw std::runtime_error(tactic.name + ": only the current .tct format is read");
    const auto formation = invoke(state, 0x10773520, {formation_size});
    if (!formation) throw std::runtime_error("Could not allocate a formation record");
    (void)invoke(state, 0x10110c80, {formation});
    auto& memory = state.memory;
    std::size_t at = 4 + 0x32 + 0xc8;
    const auto copy = [&](std::uint32_t offset, std::size_t count, std::size_t width) {
        for (std::size_t i = 0; i < count; ++i, at += width) {
            std::uint64_t value = 0;
            for (std::size_t b = 0; b < width; ++b)
                value |= std::uint64_t(f[at + b]) << (little ? 8 * b : 8 * (width - 1 - b));
            const auto address = formation + offset + std::uint32_t(i * width);
            if (width == 1) memory.write<1>(address, value);
            else if (width == 2) memory.write<2>(address, value);
            else if (width == 4) memory.write<4>(address, value);
            else memory.write<8>(address, value);
        }
    };
    copy(0x1880, 1, 4);   // shape
    copy(0x12e, 11, 2);   // positions
    copy(0x144, 11, 1);   // display order
    copy(0x150, 11, 2);   // positions with the ball
    copy(0x166, 0x210, 2);
    if (tactic.respot) {
        // The tactics screen's way: each player put at his two positions
        // (0x10111310), starting from the default 4-4-2's.
        std::array<std::uint16_t, 22> codes;
        for (std::uint32_t i = 0; i < 22; ++i)
            codes[i] = std::uint16_t(memory.read<2>(formation + (i < 11 ? 0x12e + i * 2 : 0x150 + (i - 11) * 2)));
        (void)invoke(state, 0x10110c80, {formation});
        for (std::uint32_t player = 0; player < 11; ++player) {
            (void)invoke(state, 0x10111310, {formation, player, codes[player], 0});
            (void)invoke(state, 0x10111310, {formation, player, codes[11 + player], 1});
        }
    }
    copy(0x588, 1, 4);    // team options
    copy(0x1888, 11, 8);  // player instructions
    copy(0x5b8, 11, 1);   // roles
    if (!(memory.read<4>(formation + 0x588) & 0x4000))
        memory.write<4>(formation + 0x588, memory.read<4>(formation + 0x588) | 0x2000);
    (void)invoke(state, 0x10114c00, {formation});  // shape from the positions
    // The loader's per-player spot round trip (cell 0 without the ball).
    const auto spot = invoke(state, 0x10773520, {8});
    if (!spot) throw std::runtime_error("Could not allocate a spot");
    for (std::uint32_t player = 0; player < 11; ++player) {
        (void)invoke(state, 0x101124c0, {formation, player, 0, 0, spot});
        (void)invoke(state, 0x10111e60, {formation, player, 0, 0, std::uint32_t(memory.read<4>(spot))});
    }
    (void)invoke(state, 0x10116560, {formation});
    return formation;
}

// The probe's context (one match at a time per process).
struct Injection {
    std::uint32_t match = 0, side = 0, source = 0;
    unsigned applied = 0;
} injection;

// Copies the tactic's fields over the side's record, or only those within
// [begin, end) of the record.
void apply(State& state, std::uint32_t begin = 0, std::uint32_t end = formation_size) {
    const auto target = injection.match + formations + injection.side * formation_size;
    for (const auto& range : tactic_fields)
        for (auto offset = std::max(range.offset, begin); offset < std::min(range.offset + range.size, end); ++offset)
            state.memory.write<1>(target + offset, state.memory.read<1>(injection.source + offset));
}
unsigned assertions = 0;
std::map<std::uint32_t, unsigned> assertion_sites;  // caller of the report -> count
bool probe(State& state) {
    if (state.pc == 0x10769b60) {
        ++assertions;
        ++assertion_sites[std::uint32_t(state.r<4>(4128))];
    }
    if ((state.pc == after_choice || state.pc == after_adjustment) && injection.source &&
        state.r<4>(24 * 4) == injection.match + teams + injection.side * team_size) {
        apply(state);
        ++injection.applied;
    }
    return false;
}

// Writes into the side's formation record during the match: counted by
// (pc, offset), and with `guard` the tactic's bytes are put back at once.
class FormationWatch final : public MemoryObserver {
    State& state_;
    std::uint32_t begin_;
    bool guard_, restoring_ = false;
public:
    std::map<std::pair<std::uint32_t, std::uint32_t>, unsigned> writes;  // (pc, first offset) -> count
    FormationWatch(State& state, std::uint32_t begin, bool guard) : state_(state), begin_(begin), guard_(guard) {
        restrict_write_notifications(begin, begin + formation_size);
    }
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t address, std::uint64_t size) override { changed(address, size); }
    void written(std::uint32_t address, const std::uint8_t*, std::size_t size) override { changed(address, size); }
private:
    void changed(std::uint64_t address, std::uint64_t size) {
        if (restoring_ || address >= begin_ + formation_size || address + size <= begin_) return;
        const auto first = std::uint32_t(std::max<std::uint64_t>(address, begin_) - begin_);
        const auto last = std::uint32_t(std::min<std::uint64_t>(address + size, begin_ + formation_size) - begin_);
        ++writes[{state_.pc, first}];
        if (!guard_) return;
        restoring_ = true;
        apply(state_, first, last);
        restoring_ = false;
    }
};

std::uint32_t advance(State& state, std::uint32_t match) {
    state.budget = state.executed + fresh_match_step_limit;
    state.wr<4>(12, match); state.wr<4>(16, 1); state.wr<4>(4128, return_sentinel);
    state.pc = 0x101d3f60;
    run_until(state, return_sentinel);
    return std::uint32_t(state.r<4>(12));
}

struct Options {
    std::filesystem::path program, game, scratch;
    std::uint32_t home = 0, away = 0, side = 0, seeds = 10, first_seed = 1;
    bool watch = false;
    std::vector<Tactic> tactics;
};

struct Result {
    int goals_for = 0, goals_against = 0;
    unsigned assertions = 0;
    int shots_for = 0, on_target_for = 0, shots_against = 0, on_target_against = 0;
};

Result play(const Checkpoint& prepared, const MatchPreparation& shape, const Options& options, std::size_t tactic_index,
            std::uint32_t seed, std::ostream* watch_out) {
    auto state = prepared.restore();
    state->budget = state->executed + 4000000000ULL;
    const auto& tactic = options.tactics[tactic_index];
    assertions = 0;
    assertion_sites.clear();
    injection = {shape.match, options.side, 0, 0};
    struct Hooks {  // the probe and the observer end with this match, however it ends
        State* state = nullptr;
        ~Hooks() {
            dispatch_probe = nullptr;
            if (state) state->memory.observer = nullptr;
        }
    } hooks;
    dispatch_probe = &probe;  // counts the reports of the load too
    injection.source = tactic.ai ? 0u : build_formation(*state, tactic);
    const auto at_load = assertions;
    if (!tactic.guest_path.empty() && seed == options.first_seed) {
        std::ostringstream line;
        line << "loaded " << tactic_index << ' ' << at_load << std::hex;
        for (std::uint32_t at : {0x12eu, 0x150u})
            for (std::uint32_t player = 0; player < 11; ++player)
                line << ' ' << state->memory.read<2>(injection.source + at + player * 2);
        std::cout << line.str() << std::endl;
    }
    (void)invoke(*state, 0x107474a0, {seed});
    // Assertion reports go to the log instead of a dialog nobody can close;
    // their count is part of the result (the game would show each one).
    state->memory.write<1>(0x108b8540, 1);
    auto engine = start_match(std::move(state), shape.match, shape.fixture, shape.rosters);
    if (injection.source && injection.applied != 2)
        throw std::runtime_error(tactic.name + ": the tactic was applied " + std::to_string(injection.applied) + " times");
    auto& s = *engine.state;
    const auto record = shape.match + formations + options.side * formation_size;
    FormationWatch watch(s, record, injection.source && !watch_out);
    hooks.state = &s;
    if (watch_out || injection.source) s.memory.observer = &watch;
    for (unsigned tick = 0;; ++tick) {
        if (tick > 3000) throw std::runtime_error("The match did not finish");
        const auto result = advance(s, shape.match);
        if (result == 2) break;
        if (result != 1 && result != 4) throw std::runtime_error("Unexpected engine result " + std::to_string(result));
    }
    if (watch_out)
        for (const auto& [key, count] : watch.writes)
            *watch_out << "watch pc=" << std::hex << key.first << " offset=" << key.second << std::dec << " x" << count << '\n';
    if (assertions && watch_out) {
        *watch_out << "assertions " << assertions;
        for (const auto& [site, count] : assertion_sites) *watch_out << ' ' << std::hex << site << std::dec << ':' << count;
        *watch_out << '\n';
    }
    // Each side's statistics start with goals, shots, shots on target.
    const auto stat = [&](std::uint32_t side, std::uint32_t at) {
        return int(std::int8_t(s.memory.read<1>(shape.match + (side ? away_score : home_score) + at)));
    };
    const auto own = options.side, other = 1 - options.side;
    return {stat(own, 0), stat(other, 0), assertions - at_load, stat(own, 1), stat(own, 2), stat(other, 1), stat(other, 2)};
}

std::uint32_t number(const char* text) {
    std::uint32_t value = 0;
    const auto end = text + std::strlen(text);
    if (std::from_chars(text, end, value).ptr != end) throw std::invalid_argument(std::string("Not a number: ") + text);
    return value;
}

Options parse(int argc, char** argv) {
    if (argc < 5) throw std::invalid_argument("Usage: cm-tactic-lab PROGRAM GAME SCRATCH --home ID --away ID [--side 0|1] [--seeds N] [--first-seed S] [--watch] TACTIC...");
    Options o;
    o.program = argv[1]; o.game = argv[2]; o.scratch = argv[3];
    for (int i = 4; i < argc; ++i) {
        const std::string arg = argv[i];
        const auto value = [&] { if (++i >= argc) throw std::invalid_argument(arg + " needs a value"); return number(argv[i]); };
        if (arg == "--home") o.home = value();
        else if (arg == "--away") o.away = value();
        else if (arg == "--side") o.side = value();
        else if (arg == "--seeds") o.seeds = value();
        else if (arg == "--first-seed") o.first_seed = value();
        else if (arg == "--watch") o.watch = true;
        else if (arg == "ai") o.tactics.push_back({"ai", true, false, {}, {}});
        else {
            const bool respot = arg.starts_with("respot:"), load = arg.starts_with("load:");
            const auto path = respot ? arg.substr(7) : load ? arg.substr(5) : arg;
            std::ifstream in(path, std::ios::binary);
            if (!in) throw std::invalid_argument("Cannot read " + path);
            o.tactics.push_back({arg, false, respot, {std::istreambuf_iterator<char>(in), {}}, {}});
            if (load) o.tactics.back().guest_path = "load";  // named once the folder exists
        }
    }
    if (o.tactics.empty() || o.side > 1 || !o.first_seed || o.seeds > 0xffffffffu - o.first_seed)
        throw std::invalid_argument("Bad arguments");
    return o;
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string_view(argv[2]) == "--list-clubs") {
            for (const auto& club : cm::aot::match_debugger::DatabaseCatalog::load(argv[1]).clubs())
                if (club.kind == cm::aot::match_debugger::ClubKind::club) std::cout << club.id << '\t' << club.name << '\n';
            return 0;
        }
        auto options = parse(argc, argv);
        const auto root = options.scratch / std::to_string(::getpid());
        auto prepared = prepare_match({options.program, options.game, root}, {options.home, options.away, 12345, 2001, 194});
        // Files for the original loader go into the user data folder (root).
        for (std::size_t t = 0; t < options.tactics.size(); ++t) {
            auto& tactic = options.tactics[t];
            if (tactic.guest_path.empty()) continue;
            const auto name = "load" + std::to_string(t) + ".tct";
            std::ofstream out(root / name, std::ios::binary);
            out.write(reinterpret_cast<const char*>(tactic.file.data()), std::streamsize(tactic.file.size()));
            if (!out) throw std::runtime_error("Cannot write " + (root / name).string());
            tactic.guest_path = "CM User Data:" + name;
        }
        const Checkpoint checkpoint(*prepared.state);
        struct Totals { int played = 0, won = 0, drawn = 0, lost = 0, scored = 0, conceded = 0, errors = 0; };
        std::vector<Totals> totals(options.tactics.size());
        for (std::uint32_t seed = options.first_seed; seed < options.first_seed + options.seeds; ++seed)
            for (std::size_t t = 0; t < options.tactics.size(); ++t) {
                try {
                    std::ostringstream watched;
                    const auto r = play(checkpoint, prepared, options, t, seed, options.watch ? &watched : nullptr);
                    std::cout << watched.str() << "match " << t << ' ' << seed << ' ' << r.goals_for << ' '
                              << r.goals_against << ' ' << r.assertions << ' ' << r.shots_for << ' ' << r.on_target_for
                              << ' ' << r.shots_against << ' ' << r.on_target_against << std::endl;
                    auto& total = totals[t];
                    ++total.played; total.scored += r.goals_for; total.conceded += r.goals_against;
                    if (r.goals_for > r.goals_against) ++total.won;
                    else if (r.goals_for == r.goals_against) ++total.drawn;
                    else ++total.lost;
                } catch (const std::exception& error) {
                    ++totals[t].errors;
                    std::cout << "error " << t << ' ' << seed << ' ' << error.what() << std::endl;
                }
            }
        for (std::size_t t = 0; t < totals.size(); ++t) {
            const auto& x = totals[t];
            const double n = x.played ? x.played : 1;
            std::printf("tactic %zu %s: P%d W%d D%d L%d GF%d GA%d pts/game %.3f gd/game %+.3f errors %d\n", t,
                        options.tactics[t].name.c_str(), x.played, x.won, x.drawn, x.lost, x.scored, x.conceded,
                        (3.0 * x.won + x.drawn) / n, (x.scored - x.conceded) / n, x.errors);
        }
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
