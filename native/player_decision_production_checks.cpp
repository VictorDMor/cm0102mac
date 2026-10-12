#include "player_decision_adapter.hpp"
#include "match_debugger_factory.hpp"
#include "semantic_io_checks.hpp"
#include "match_turbo.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "code_fragment_adapter.hpp"
#include "process_bridge.hpp"
#include <chrono>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace actor = cm::aot::player_decision;
enum class Path { direct, generated, cfm };
enum class Mode { modern, global_original, entry_original, profile };
struct Negative final : PlayerDecisionBoundaryObserver {
    unsigned index; bool enabled;
    Negative(unsigned n, bool value) : index(n), enabled(value) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& state, std::uint32_t entry, bool original, PlayerDecisionOutcome outcome) override {
        if (enabled && !original && entry == actor::entry && outcome == PlayerDecisionOutcome::returned)
            state.wr<4>(12, state.r<4>(12) ^ 1);
    }
};
struct ClockInput final : CodeFragmentBoundaryObserver {
    std::uint64_t initial;
    unsigned calls = 0;
    struct Event {
        bool leave;
        std::string name;
        decltype(State::registers) registers;
        std::uint32_t pc;
        bool operator==(const Event&) const = default;
    };
    std::vector<Event> events;
    explicit ClockInput(std::uint64_t value) : initial(value) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& state, const char* name) override { events.push_back({false, name, state.registers, state.pc}); }
    void import_leave(const State& state, const char* name) override { events.push_back({true, name, state.registers, state.pc}); }
    bool import_fixture(State& state, const char* name) override {
        // Headless characterization ends at the native event boundary. It does
        // not supply a fictitious click or treat a pending UI path as a return.
        if (std::string_view(name) == "CarbonLib::WaitNextEvent" || std::string_view(name) == "CarbonLib::GetNextEvent")
            throw std::runtime_error("Original player decision awaits native event input");
        // Presentation consumes host time. Both executions receive the same
        // monotonic external input; football, RNG, rendering and other services
        // still execute their real production bodies.
        if (std::string_view(name) != "CarbonLib::Microseconds") return false;
        state.memory.write<8>(state.r<4>(12), initial + UINT64_C(1000000) * ++calls);
        state.pc = std::uint32_t(state.r<4>(4128));
        return true;
    }
};
unsigned cases = 0, negatives = 0, budgets = 0, normal_cases = 0, input_boundaries = 0;
void arguments(State& state,std::uint32_t owner,unsigned variant){
    const auto side=variant/3,slot=std::array<unsigned,3>{0,2,8}[variant%3];
    state.wr<4>(12,owner+0x311c+side*0x2350+slot*0x1c4);
}
std::string execute(State& state, unsigned, Path path) {
    try {
        if (path == Path::direct) try_modern_player_decision(state, actor::entry);
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
void compare(const State& seed, std::uint32_t owner, unsigned variant, Path path, Mode mode,
    bool negative = false, std::uint64_t limit = 50000000) {
    auto original = clone(seed), modern = clone(seed);
    // Platform input is immutable in this interval; each clone owns all guest
    // state. Fresh-world/worker checks separately prove process independence.
    original->platform = modern->platform = seed.platform;
    for (auto* state : {original.get(), modern.get()}) {
        state->memory.map(fixture, 0x1000); arguments(*state,owner,variant);
        state->wr<4>(4128, return_sentinel); state->pc = actor::entry;
        state->budget = state->executed + limit;
    }
    auto& old = player_decision_diagnostics(*original); auto& next = player_decision_diagnostics(*modern); old.original = true;
    if (mode == Mode::global_original) next.original = true;
    if (mode == Mode::entry_original) next.entry_original[0] = true;
    Negative mutation(0, negative); next.observer = &mutation;
    const auto initial_clock=native_microseconds();ClockInput clock_old(initial_clock),clock_new(initial_clock);
    original->code_fragment_observer=&clock_old;modern->code_fragment_observer=&clock_new;
    Effects x, y; x.state = original.get(); y.state = modern.get();
    original->memory.date_observer = &x; modern->memory.date_observer = &y;
    const auto expected = execute(*original, variant, path), actual = execute(*modern, variant, path);
    original->memory.date_observer = modern->memory.date_observer = nullptr; next.observer = nullptr;original->code_fragment_observer=modern->code_fragment_observer=nullptr;
    const auto label = hex_address(actor::entry) + " actual path=" + std::to_string(unsigned(path)) + " mode=" + std::to_string(unsigned(mode));
    require(expected == actual, label + " original=" + expected + " modern=" + actual);
    const bool awaiting=expected=="Original player decision awaits native event input";
    require(limit == 50000000 ? (expected.empty() || awaiting) : expected == "Native instruction budget exhausted", label + " expected normal/budget/native-input boundary");
    require(clock_old.calls==clock_new.calls && clock_old.events==clock_new.events,label+" native input/import boundaries");
    if (negative) {
        bool rejected = false; try { Image(*original).compare(*modern, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Actual entry negative did not change observable state"); ++negatives;
    } else { Image(*original).compare(*modern, label); require(x.bytes == y.bytes && original->executed == modern->executed, label + " complete ordered effects and accounting"); }
    require(!next.original_body && !next.dispatch_escapes, label + " original scope residue or escape");
    if (mode == Mode::modern) {
        require(next.calls[0] >= 1 && next.returns[0] == (expected.empty()?next.calls[0]:0) &&
            next.budget_stops[0] >= unsigned(limit != 50000000) && !next.original_calls[0], label + " dedicated C++23 root coverage");

    } else {
        require(!next.calls[0] && !next.returns[0], label + " original control executed modern root");
        require(mode == Mode::profile ? next.profile_original_calls[0] == 1 : next.original_calls[0] == 1,
            label + " selected original/profile root coverage");
    }
    ++cases; budgets += limit != 50000000;normal_cases+=expected.empty();input_boundaries+=awaiting;
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
            auto& state=*match.state;for(unsigned advance=0;advance<3;++advance){state.wr<4>(12,match.match);state.wr<4>(16,1);state.wr<4>(4128,return_sentinel);
            state.pc=0x101d3f60;state.budget=state.executed+50000000;run_until(state,return_sentinel);}
            state.wr<4>(12,match.match+0x311c+8*0x1c4);state.pc=actor::entries[0];
            save_runtime_snapshot(state,std::filesystem::path(argv[3]).parent_path()/"player-decision-live-profile.snapshot");
        }
        auto& state = *match.state;
        for(unsigned variant=0;variant<6;++variant)for(auto path:{Path::direct,Path::generated,Path::cfm}){
            if(profile)compare(state,match.match,variant,path,Mode::profile);
            else for(auto mode:{Mode::modern,Mode::global_original,Mode::entry_original})compare(state,match.match,variant,path,mode);
        }
        if(!profile){compare(state,match.match,1,Path::generated,Mode::modern,true);for(auto limit:{0u,1u})compare(state,match.match,1,Path::generated,Mode::modern,false,limit);}
        std::cout << "PASS: actual player-decision production cases=" << cases << " negatives=" << negatives << " budgets=" << budgets
                  << " normal="<<normal_cases<<" native-input-boundaries="<<input_boundaries<<"; direct/generated/CFM and original/profile controls, complete guest state/ordered effects/accounting\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
