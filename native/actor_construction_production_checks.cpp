#include "actor_construction_adapter.hpp"
#include "routine_check.hpp"
#include <fstream>
#include "match_debugger_factory.hpp"
#include "semantic_io_checks.hpp"
#include "match_turbo.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include <chrono>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace actor = cm::aot::actor_construction;
enum class Path { direct, generated, cfm };
enum class Mode { modern, global_original, entry_original, profile };
struct Negative final : ActorConstructionBoundaryObserver {
    unsigned index; bool enabled;
    Negative(unsigned n, bool value) : index(n), enabled(value) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& state, std::uint32_t entry, bool original, ActorConstructionOutcome outcome) override {
        if (enabled && !original && entry == actor::entries[index] && outcome == ActorConstructionOutcome::returned)
            state.wr<4>(12, state.r<4>(12) ^ 1);
    }
};
unsigned cases = 0, negatives = 0, budgets = 0;
std::string execute(State& state, unsigned index, Path path) {
    try {
        if (path == Path::direct) try_modern_actor_construction(state, actor::entries[index]);
        else {
            if (path == Path::cfm) {
                state.memory.write<4>(fixture + 0x400, state.pc); state.memory.write<4>(fixture + 0x404, state.r<4>(8));
                state.wr<4>(48, fixture + 0x400); state.pc = 0x10781320;
            }
            run_until(state, return_sentinel);
        }
    } catch (const std::exception& error) { return std::string(error.what()); }
    return {};
}
unsigned selected_index = 0;
Path selected_path = Path::direct;
bool selected_negative = false;
void returned_route(State& s, bool original) {
    const auto sp = std::uint32_t(s.r<4>(4));
    auto& diagnostics = actor_construction_diagnostics(s);
    diagnostics.original = original;
    Negative mutation(selected_index, selected_negative && !original);
    diagnostics.observer = &mutation;
    const auto error = execute(s, selected_index, selected_path);
    diagnostics.observer = nullptr;
    require(error.empty() && s.pc == return_sentinel && s.r<4>(4) == sp,
        "Legacy constructor delegation did not return: " + error);
    require(!diagnostics.original_body && !diagnostics.dispatch_escapes, "Legacy scope leaked");
    if (!original) require(diagnostics.calls[selected_index] == 1 && diagnostics.returns[selected_index] == 1,
        "Legacy selected root did not return through its modern delegate");
}
void returned_original(State& s, std::uint32_t) { returned_route(s, true); }
void returned_modern(State& s, std::uint32_t) { returned_route(s, false); }
void compare(const State& seed, std::uint32_t owner, unsigned index, Path path, Mode mode,
    bool negative = false, std::uint64_t limit = 50000000) {
    auto original = clone(seed), modern = clone(seed);
    // Platform input is immutable in this interval; each clone owns all guest
    // state. Fresh-world/worker checks separately prove process independence.
    original->platform = modern->platform = seed.platform;
    const auto player = owner + 0x311c + 8 * 0x1c4;
    for (auto* state : {original.get(), modern.get()}) {
        state->memory.map(fixture, 0x1000);
        const auto staff = std::uint32_t(state->memory.read<4>(player + 0x6c));
        unsigned reg = 3;
        if (index == 0) for (auto value : {owner + 0x7aac, 8u, staff, 8u}) state->wr<4>(reg++ * 4, value);
        else for (auto value : {player, owner, staff, 0u, 8u, 8u}) state->wr<4>(reg++ * 4, value);
        state->wr<4>(4128, return_sentinel); state->pc = actor::entries[index];
        state->budget = state->executed + limit;
    }
    if (mode == Mode::modern && limit == 50000000) {
        // MATCH_PL owns the constructor now. Use the game's observable return
        // contract; exact translated temporary registers/read transcripts are
        // retained below only for original controls and pre-child budget stops.
        selected_index = index; selected_path = path; selected_negative = negative;
        const auto differences = compare_routine_call(*original, actor::entries[index],
            returned_original, returned_modern, RoutineChecker::result_r3 | RoutineChecker::result_f1);
        if (!negative) for (const auto& difference : differences) std::cerr << difference << '\n';
        require(negative ? !differences.empty() : differences.empty(), "Legacy returned comparison/corruption control failed");
        ++cases; negatives += negative; return;
    }
    auto& old = actor_construction_diagnostics(*original); auto& next = actor_construction_diagnostics(*modern); old.original = true;
    if (mode == Mode::global_original) next.original = true;
    if (mode == Mode::entry_original) next.entry_original[index] = true;
    Negative mutation(index, negative); next.observer = &mutation;
    Effects x, y; x.state = original.get(); y.state = modern.get();
    original->memory.date_observer = &x; modern->memory.date_observer = &y;
    const auto expected = execute(*original, index, path), actual = execute(*modern, index, path);
    original->memory.date_observer = modern->memory.date_observer = nullptr; next.observer = nullptr;
    const auto label = hex_address(actor::entries[index]) + " actual path=" + std::to_string(unsigned(path)) + " mode=" + std::to_string(unsigned(mode));
    require(expected == actual, label + " original=" + expected + " modern=" + actual);
    require(limit == 50000000 ? expected.empty() : expected == "Native instruction budget exhausted", label + " expected normal/budget boundary");
    if (negative) {
        bool rejected = false; try { Image(*original).compare(*modern, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Actual entry negative did not change observable state"); ++negatives;
    } else { Image(*original).compare(*modern, label); require(x.bytes == y.bytes, label + " complete ordered effects"); }
    require(!next.original_body && !next.dispatch_escapes, label + " original scope residue or escape");
    if (mode == Mode::modern) {
        require(next.calls[index] == 1 && next.returns[index] == unsigned(limit == 50000000) &&
            next.budget_stops[index] == unsigned(limit != 50000000) && !next.original_calls[index], label + " dedicated C++23 root coverage");
        // The parent's child now belongs to MATCH_PL. Its generated route,
        // original bypass and complete parent replay are checked there.
    } else {
        require(!next.calls[index] && !next.returns[index], label + " original control executed modern root");
        require(mode == Mode::profile ? next.profile_original_calls[index] == 1 : next.original_calls[index] == 1,
            label + " selected original/profile root coverage");
    }
    ++cases; budgets += limit != 50000000;
}
}
int main(int argc, char** argv) {
    try {
        const bool captured = argc == 6 && std::string_view(argv[4]) == "--capture";
        require(argc == 4 || argc == 5 || captured, "Expected original program, game, private root and optional capture");
        const bool profile = captured ? match_pc_profile_enabled() : argc == 5;
        require(profile == match_pc_profile_enabled(), "Actual profile flag differs from requested mode");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        cm::aot::match_debugger::EngineInstance match;
        if (captured) {
            match.state = std::make_unique<State>();
            const auto input = std::filesystem::path(argv[5]);
            load_match_snapshot(*match.state, input / "initial.snapshot");
            std::ifstream(input / "match.txt") >> match.match;
            require(match.match != 0, "Existing capture lacks match identity");
        } else if (profile) {
            match.state = std::make_unique<State>(); load_program(*match.state, argv[1]);
            match.state->platform = std::make_shared<NativePlatform>(argv[2], root, AudioMode::offline);
            load_match_snapshot(*match.state, argv[4]);
            match.match = match.state->memory.read<4>(match.state->r<4>(12) + 0x14);
            require(match.state->pc == actor::entries[0], "Actual freshly created profile prerequisite");
        } else {
            match = cm::aot::match_debugger::create_match({argv[1], argv[2], root}, {677, 5677, 12345, 2001, 194});
            auto& state = *match.state;
            state.wr<4>(12, match.match + 0x7aac); state.pc = actor::entries[0];
            save_runtime_snapshot(state, std::filesystem::path(argv[3]).parent_path() / "actor-construction-live-profile.snapshot");
        }
        auto& state = *match.state;
        for (unsigned index = 0; index < 2; ++index) {
            for (auto path : {Path::direct}) {
                // Explicit legacy compatibility only. Current generated routes
                // are verified by MATCH_PL's production controls and replay.
                if (profile) compare(state, match.match, index, path, Mode::profile);
                else for (auto mode : {Mode::modern, Mode::global_original, Mode::entry_original}) compare(state, match.match, index, path, mode);
            }
            if (!profile) {
                compare(state, match.match, index, Path::direct, Mode::modern, true);
                // MATCH_PL's zero-budget route controls distinguish generated
                // instructions from typed execution; partial translated-register
                // images are not a contract of this compatibility adapter.
            }
        }
        std::cout << "PASS: actual actor-construction production cases=" << cases << " negatives=" << negatives << " budgets=" << budgets
                  << "; direct legacy adapters, global/entry/profile originals, legacy constructor delegation; observable typed returns, exact original-control state/effects\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
