#include "marking_pressure_adapter.hpp"
#include "match_debugger_factory.hpp"
#include "semantic_io_checks.hpp"
#include "match_turbo.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include <chrono>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace actor = cm::aot::marking_pressure;
enum class Path { direct, generated, cfm };
enum class Mode { modern, global_original, entry_original, profile };
struct Negative final : MarkingPressureBoundaryObserver {
    unsigned index; bool enabled;
    Negative(unsigned n, bool value) : index(n), enabled(value) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& state, std::uint32_t entry, bool original, MarkingPressureOutcome outcome) override {
        if (enabled && !original && entry == actor::entries[index] && outcome == MarkingPressureOutcome::returned)
            state.wr<4>(12, state.r<4>(12) ^ 1);
    }
};
unsigned cases = 0, negatives = 0, budgets = 0;
void arguments(State& state,std::uint32_t owner,unsigned index){
    const auto player=owner+0x311c+8*0x1c4,opposite=owner+0x546c+8*0x1c4;
    using E=actor::Entry;const auto entry=E(index);state.wr<4>(12,player);state.wr<4>(16,opposite);
    if(entry==E::set_pressure_adjustment)state.wr<4>(16,123);
    if(entry==E::assess_cell){state.wr<4>(12,owner);state.wr<4>(16,player);state.wr<4>(20,fixture+0x800);state.wr<4>(24,fixture+0x600);}
    if(entry==E::eligible_marker){state.wr<4>(12,owner);state.wr<4>(16,player);state.wr<4>(20,7);state.wr<4>(24,6);}
}
std::string execute(State& state, unsigned index, Path path) {
    try {
        if (path == Path::direct) try_modern_marking_pressure(state, actor::entries[index]);
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
void compare(const State& seed, std::uint32_t owner, unsigned index, Path path, Mode mode,
    bool negative = false, std::uint64_t limit = 50000000, bool expect_two_candidates = false) {
    auto original = clone(seed), modern = clone(seed);
    // Platform input is immutable in this interval; each clone owns all guest
    // state. Fresh-world/worker checks separately prove process independence.
    original->platform = modern->platform = seed.platform;
    for (auto* state : {original.get(), modern.get()}) {
        state->memory.map(fixture, 0x1000); arguments(*state,owner,index);
        state->wr<4>(4128, return_sentinel); state->pc = actor::entries[index];
        state->budget = state->executed + limit;
    }
    auto& old = marking_pressure_diagnostics(*original); auto& next = marking_pressure_diagnostics(*modern); old.original = true;
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
    if(expect_two_candidates)require(original->memory.read<2>(fixture+0x600)==2 && modern->memory.read<2>(fixture+0x600)==2,
        "Actual pressure fixture did not append both selected actor records through the original collection operation");
    if (negative) {
        bool rejected = false; try { Image(*original).compare(*modern, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Actual entry negative did not change observable state"); ++negatives;
    } else { Image(*original).compare(*modern, label); require(x.bytes == y.bytes && original->executed == modern->executed, label + " complete ordered effects and accounting"); }
    require(!next.original_body && !next.dispatch_escapes, label + " original scope residue or escape");
    if (mode == Mode::modern) {
        require(next.calls[index] == 1 && next.returns[index] == unsigned(limit == 50000000) &&
            next.budget_stops[index] == unsigned(limit != 50000000) && !next.original_calls[index], label + " dedicated C++23 root coverage");

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
        require(argc == 4 || argc == 5, "Expected original program, game, private root and optional live profile fixture");
        const bool profile = argc == 5;
        require(profile == match_pc_profile_enabled(), "Actual profile flag differs from requested mode");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        cm::aot::match_debugger::EngineInstance match;
        if (profile) {
            match.state = std::make_unique<State>(); load_program(*match.state, argv[1]);
            match.state->platform = std::make_shared<NativePlatform>(argv[2], root, AudioMode::offline);
            load_match_snapshot(*match.state, argv[4]);
            match.match = match.state->memory.read<4>(match.state->r<4>(12) + 0x1a4);
            require(match.state->pc == actor::entries[0], "Actual freshly created profile prerequisite");
        } else {
            match = cm::aot::match_debugger::create_match({argv[1], argv[2], root}, {677, 5677, 12345, 2001, 194});
            auto& state=*match.state;state.wr<4>(12,match.match);state.wr<4>(16,1);state.wr<4>(4128,return_sentinel);
            state.pc=0x101d3f60;state.budget=state.executed+50000000;run_until(state,return_sentinel);
            state.wr<4>(12,match.match+0x311c+8*0x1c4);state.pc=actor::entries[0];
            save_runtime_snapshot(state,std::filesystem::path(argv[3]).parent_path()/"marking-pressure-live-profile.snapshot");
        }
        auto& state = *match.state;
        for (unsigned index = 0; index < actor::entries.size(); ++index) {
            for (auto path : {Path::direct, Path::generated, Path::cfm}) {
                if (profile) compare(state, match.match, index, path, Mode::profile);
                else for (auto mode : {Mode::modern, Mode::global_original, Mode::entry_original}) compare(state, match.match, index, path, mode);
            }
            if (!profile) {
                compare(state, match.match, index, Path::generated, Mode::modern, true);
                for (auto limit : {0u, 1u}) compare(state, match.match, index, Path::generated, Mode::modern, false, limit);
            }
        }
        if(!profile){
            auto ranked=clone(state);ranked->platform=state.platform;
            const auto subject=match.match+0x311c+8*0x1c4,opponent=match.match+0x546c+8*0x1c4;
            // The controlled fixture is a real guest world: opposing actors
            // share the actual subject cell and original attributes are changed
            // only in this private clone to exercise record-copy collection output.
            const auto col=signed_value<1>(ranked->memory.read<1>(subject+0x106)),row=signed_value<1>(ranked->memory.read<1>(subject+0x105));
            const auto cell=match.match+0xa08+std::uint32_t(row)*0x450+std::uint32_t(col)*0x5c;
            ranked->memory.write<4>(cell+0x2c,opponent);ranked->memory.write<4>(cell+0x30,opponent+0x1c4);ranked->memory.write<1>(cell+0x59,2);
            ranked->memory.write<1>(subject+0x10a,0);ranked->memory.write<2>(subject+0x1a0,0xff38);ranked->memory.write<1>(subject+0x104,2);
            ranked->memory.write<4>(subject+0x1b4,opponent);ranked->memory.write<4>(subject+0x1b8,opponent+0x1c4);
            for(auto p:{opponent,opponent+0x1c4}){ranked->memory.write<1>(p+0x19,8);ranked->memory.write<1>(p+0x10b,20);ranked->memory.write<1>(p+0x10d,20);ranked->memory.write<1>(p+0x2c,0);}
            compare(*ranked,match.match,unsigned(actor::Entry::assess_cell),Path::generated,Mode::modern,false,50000000,true);
        }
        std::cout << "PASS: actual marking-pressure production cases=" << cases << " negatives=" << negatives << " budgets=" << budgets
                  << "; direct/generated/CFM, global/entry/profile originals, mark-list/eligibility/local-pressure chain, complete guest state/ordered effects/accounting\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
