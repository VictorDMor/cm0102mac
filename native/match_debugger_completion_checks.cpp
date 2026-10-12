#include "match_debugger_factory.hpp"
#include "match_debugger_checkpoint.hpp"
#include "match_engine_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <chrono>

namespace {
using namespace cm::aot::match_debugger;
using namespace cm::semantic_io_checks;
struct OwnedBacking {
    std::filesystem::path root;
    ~OwnedBacking() {
        if (!std::filesystem::exists(root) || std::filesystem::is_symlink(root)) return;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) if (entry.is_symlink()) return;
        std::filesystem::remove_all(root);
    }
};
std::uint32_t advance(State& state, std::uint32_t match) {
    const auto budget = state.budget;
    state.budget = state.executed + fresh_match_step_limit;
    state.wr<4>(12, match); state.wr<4>(16, 1); state.wr<4>(4128, return_sentinel);
    state.pc = 0x101d3f60;
    try { run_until(state, return_sentinel); }
    catch (...) { state.budget = budget; throw; }
    state.budget = budget;
    return std::uint32_t(state.r<4>(12));
}
unsigned finish(State& state, std::uint32_t match) {
    for (unsigned tick = 1; tick <= 100; ++tick) {
        const auto result = advance(state, match);
        if (result == 2) return tick;
        require(result == 1 || result == 4, "Unexpected actual completion result");
    }
    throw std::runtime_error("The original match did not finish within its bounded final interval");
}
struct RefereeReadReady {};
struct RefereeRead final : MemoryObserver {
    State& state; std::uint32_t official; bool observed = false;
    RefereeRead(State& value, std::uint32_t pointer) : state(value), official(pointer) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void read(std::uint32_t address, const std::uint8_t*, unsigned size) override {
        if (state.pc == 0x10223378) {
            require(address == official + 0x24 && size == 1, "Original foul query used an incorrect referee");
            observed = true; throw RefereeReadReady{};
        }
    }
};
void referee_query(const State& input, std::uint32_t match) {
    const auto official = std::uint32_t(input.memory.read<4>(match + 0xe034));
    require(official && input.memory.read<4>(official) == 0 && input.memory.read<4>(match + 0xe030) == match,
        "Original referee context is absent or has an incorrect identity");
    for (bool missing : {false, true}) {
        auto state = clone(input); state->platform = input.platform;
        state->memory.map(fixture, 0x80); state->memory.write<1>(fixture + 0x14, 0); state->memory.write<1>(fixture + 0x24, 1);
        if (missing) state->memory.write<4>(match + 0xe034, 0);
        RefereeRead query(*state, official); state->memory.date_observer = &query;
        state->wr<4>(12, match + 0xe030); state->wr<4>(16, fixture); state->wr<4>(20, 0);
        state->wr<4>(24, 1); state->wr<4>(28, 0); state->wr<4>(4128, return_sentinel);
        state->pc = 0x10223100; state->budget = state->executed + 1000;
        bool rejected = false;
        try { run_until(*state, return_sentinel); }
        catch (const RefereeReadReady&) {}
        catch (const std::runtime_error& error) {
            rejected = missing && state->pc == 0x10223378 && std::string_view(error.what()).starts_with("Unmapped data access");
        }
        state->memory.date_observer = nullptr;
        require(missing ? rejected && !query.observed : query.observed && !rejected,
            "Original referee query or missing-context negative did not discriminate");
    }
}
}

int main(int argc, char** argv) {
    try {
        require(argc == 4, "Expected original program, game and private completion root");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        OwnedBacking cleanup{root};
        {
            auto arsenal = create_match({argv[1], argv[2], root / "arsenal"}, {677, 5677, 12345, 2001, 194});
            referee_query(*arsenal.state, arsenal.match);
            for (unsigned tick = 0; tick < 100; ++tick) require(advance(*arsenal.state, arsenal.match) == 1,
                "Arsenal's original referee handling failed within the reported initial interval");
        }
        auto engine = create_match({argv[1], argv[2], root / "cruzeiro"}, {2262, 5677, 12345, 2001, 194});
        auto& state = *engine.state;
        const auto match = engine.match, fixture = std::uint32_t(state.memory.read<4>(match + 0x3118));
        require(state.memory.read<4>(fixture) < 536 && state.memory.read<4>(fixture + 0x14) &&
            state.memory.read<1>(fixture + 0x4b) == 0xff && state.memory.read<1>(fixture + 0x4c) == 0xff,
            "Detailed friendly fixture is incomplete");
        require(state.memory.read<4>(state.memory.read<4>(fixture + 0x14)) == state.memory.read<4>(fixture),
            "Detailed fixture has an incorrect competition record");
        unsigned prefix = 0;
        while (state.memory.read<2>(match + 0x7948) < 980) {
            require(++prefix <= 1200, "Actual match did not reach its final interval");
            const auto result = advance(state, match);
            require(result == 1 || result == 4, "Actual match terminated before its final interval");
        }
        const Checkpoint before_final(state);
        auto original = before_final.restore();
        match_engine_diagnostics(*original).entry_original[2] = true;
        const auto final_ticks = finish(state, match);
        require(finish(*original, match) == final_ticks, "Original and C++23 completion tick counts differ");
        const Image completed(state);
        completed.compare(*original, "Original/C++23 actual completion");
        require(state.executed == original->executed, "Original/C++23 completion instruction accounting differs");
        for (bool invalid_competition : {true, false}) {
            auto broken = before_final.restore();
            if (invalid_competition) broken->memory.write<4>(fixture, 0xfffffffe);
            else broken->memory.write<4>(fixture + 0x14, 0);
            bool rejected = false;
            try { (void)finish(*broken, match); }
            catch (const std::runtime_error& error) {
                rejected = std::string_view(error.what()).starts_with("Unmapped data access") &&
                    broken->pc == (invalid_competition ? 0x10464744u : 0x101f958cu);
            }
            require(rejected, "Removing a required fixture binding did not expose the original fault");
        }
        Session session([&] { return EngineInstance{before_final.restore(), match, engine.rosters}; }, fresh_match_step_limit);
        for (unsigned tick = 0; tick < final_ticks; ++tick) (void)session.step();
        require(session.execution_state() == ExecutionState::completed && session.failure().empty() &&
            !session.events().empty() && session.events().back().kind == MatchEvent::Kind::completed &&
            session.events().back().seconds == clock_seconds(session.inspect()), "Full time or its readable event is missing");
        completed.compare(session.state(), "Session actual completion");
        bool stopped = false;
        try { (void)session.step(); } catch (const std::logic_error&) { stopped = true; }
        require(stopped, "A completed match accepted another live step");
        const auto final_inspection = session.inspect();
        const auto event_count = session.events().size();
        session.back();
        require(session.execution_state() == ExecutionState::paused && session.events().size() < event_count,
            "Back retained the completed state or full-time event");
        session.forward();
        require(session.execution_state() == ExecutionState::completed && session.inspect() == final_inspection &&
            session.events().size() == event_count, "Redo lost full time or the final football state");
        completed.compare(session.state(), "Redo actual completion");
        std::cout << "PASS: 100 Arsenal/Manchester United actual ticks, original referee query and missing-context negative; Cruzeiro/Manchester United seed 12345, " << prefix + final_ticks << " actual ticks, full time "
            << clock_seconds(final_inspection) << "s, original/C++23 final state and instruction agreement, 2 original-fault negatives, full-time event, undo/redo and completed-step rejection\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
