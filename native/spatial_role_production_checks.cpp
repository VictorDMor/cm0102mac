#include "spatial_role_adapter.hpp"
#include "match_debugger_factory.hpp"
#include "semantic_io_checks.hpp"
#include "match_turbo.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include <chrono>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace actor = cm::aot::spatial_role;
enum class Path { direct, generated, cfm };
enum class Mode { modern, global_original, entry_original, profile };
struct Negative final : SpatialRoleBoundaryObserver {
    unsigned index; bool enabled;
    Negative(unsigned n, bool value) : index(n), enabled(value) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& state, std::uint32_t entry, bool original, SpatialRoleOutcome outcome) override {
        if (enabled && !original && entry == actor::entries[index] && outcome == SpatialRoleOutcome::returned)
            state.wr<4>(12, state.r<4>(12) ^ 1);
    }
};
unsigned cases = 0, negatives = 0, budgets = 0;
void arguments(State& state, std::uint32_t owner, unsigned index) {
    const auto player=owner+0x311c+8*0x1c4;
    using E=actor::Entry; const auto entry=E(index);
    state.wr<4>(12,player);state.wr<4>(16,0);state.wr<4>(20,0xffffffff);
    if(entry==E::set_marking_target)state.wr<4>(16,owner+0x311c+7*0x1c4);
    if(entry==E::current_role || entry==E::secondary_role || entry==E::previous_role || entry==E::register_center_cell || entry==E::remove_from_cell){state.wr<4>(12,owner);state.wr<4>(16,player);}
    if(entry==E::clear_goalkeeper || entry==E::resolve_goalkeeper)state.wr<4>(12,owner);
    if(entry==E::set_column)state.wr<4>(16,6);
    if(entry==E::set_row)state.wr<4>(16,8);
    if(entry==E::set_previous_position)state.wr<4>(16,8);
}
std::string execute(State& state, unsigned index, Path path) {
    try {
        if (path == Path::direct) try_modern_spatial_role(state, actor::entries[index]);
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
    bool negative = false, std::uint64_t limit = 50000000) {
    auto original = clone(seed), modern = clone(seed);
    // Platform input is immutable in this interval; each clone owns all guest
    // state. Fresh-world/worker checks separately prove process independence.
    original->platform = modern->platform = seed.platform;
    for (auto* state : {original.get(), modern.get()}) {
        state->memory.map(fixture, 0x1000); arguments(*state,owner,index);
        state->wr<4>(4128, return_sentinel); state->pc = actor::entries[index];
        state->budget = state->executed + limit;
    }
    auto& old = spatial_role_diagnostics(*original); auto& next = spatial_role_diagnostics(*modern); old.original = true;
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
            save_runtime_snapshot(state,std::filesystem::path(argv[3]).parent_path()/"spatial-role-live-profile.snapshot");
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
            auto emergency=clone(state);emergency->platform=state.platform;
            emergency->memory.write<1>(match.match+0x311c+0x19,254);
            emergency->memory.write<1>(match.match+0x311c+0x20,255);
            compare(*emergency,match.match,unsigned(actor::Entry::resolve_goalkeeper),Path::generated,Mode::modern);
        }
        std::cout << "PASS: actual spatial-role production cases=" << cases << " negatives=" << negatives << " budgets=" << budgets
                  << "; direct/generated/CFM, global/entry/profile originals, cell/role/mark/keeper chain, complete guest state/ordered effects/accounting\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
