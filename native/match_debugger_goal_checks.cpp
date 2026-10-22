#include "match_debugger_factory.hpp"
#include "match_engine_adapter.hpp"
#include "match_actor_adapter.hpp"
#include "match_debugging.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace {
using namespace cm::aot;
using namespace cm::aot::match_debugger;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Effect {
    unsigned kind = 0;
    std::uint32_t address = 0;
    std::uint64_t length = 0;
    std::vector<std::uint8_t> bytes;
    bool operator==(const Effect&) const = default;
};
struct Trace final : MemoryObserver {
    std::vector<Effect> effects;
    void mapped(std::uint32_t address, std::uint64_t length) override { effects.push_back({0, address, length, {}}); }
    void cleared(std::uint32_t address, std::uint64_t length) override { effects.push_back({1, address, length, {}}); }
    void written(std::uint32_t address, const std::uint8_t* bytes, std::size_t length) override {
        effects.push_back({2, address, length, {bytes, bytes + length}});
    }
};
struct Observe {
    Memory& memory; MemoryObserver* previous;
    Observe(State& state, Trace& trace) : memory(state.memory), previous(memory.observer) { memory.observer = &trace; }
    ~Observe() { memory.observer = previous; }
};
std::uint64_t invoke(State& state, std::uint32_t entry, std::initializer_list<std::uint32_t> values) {
    const auto before = state.executed, budget = state.budget;
    unsigned reg = 3;
    for (auto value : values) state.wr<4>(reg++ * 4, value);
    state.wr<4>(4128, return_sentinel); state.pc = entry; state.budget = before + fresh_match_step_limit;
    run_until(state, return_sentinel); state.budget = budget;
    return state.executed - before;
}
struct Baseline {
    std::uint64_t first_instructions, second_instructions, first_calls, second_calls, first_budget, second_budget;
};
void same_match(const Session& session, const EngineInstance& control, const Baseline& baseline) {
    const auto& first = session.state(); const auto& second = *control.state;
    require(session.match_address() == control.match, "Independent seeded match allocation differs");
    require(first.memory.copy_out(control.match, 0xe0bc) == second.memory.copy_out(control.match, 0xe0bc),
        "Complete actual match differs from direct original execution");
    require(first.registers == second.registers, "Original intervention registers differ");
    require(first.unique == second.unique, "Original intervention unique bytes differ");
    require(first.pc == second.pc, "Original intervention return PC differs");
    // Independent world startup can have a different path/time/import cost.
    // Compare all execution from the common initialized-match boundary onward,
    // while checking each world's own restored caller budget separately.
    require(first.executed - baseline.first_instructions == second.executed - baseline.second_instructions,
        "Original intervention instruction accounting differs");
    require(first.native_calls - baseline.first_calls == second.native_calls - baseline.second_calls,
        "Original intervention native call accounting differs");
    require(first.budget == baseline.first_budget && second.budget == baseline.second_budget, "Original intervention caller budget differs");
    require(first.allocated == second.allocated && first.available == second.available && first.heap_end == second.heap_end,
        "Original intervention allocations differ");
}
template<class Action> void reject_without_effects(Session& session, Action action) {
    const auto inspection = session.inspect();
    const auto memory = session.state().memory.copy_out(session.match_address(), 0xe0bc);
    const auto registers = session.state().registers;
    const auto unique = session.state().unique;
    const auto instructions = session.state().executed;
    const auto goals = session.goals().size();
    bool rejected = false;
    try { action(); } catch (const std::logic_error&) { rejected = true; }
    require(rejected && session.inspect() == inspection && session.state().memory.copy_out(session.match_address(), 0xe0bc) == memory &&
        session.state().registers == registers && session.state().unique == unique && session.state().executed == instructions &&
        session.goals().size() == goals, "Rejected goal changed live state or audit history");
}

struct OriginalRegionBoundary { std::uint32_t pc; };
struct OriginalRegions final : MemoryObserver {
    State& state; MemoryObserver* transcript; unsigned stage = 0;
    OriginalRegions(State& value, MemoryObserver* previous) : state(value), transcript(previous) {}
    void mapped(std::uint32_t address, std::uint64_t size) override { if (transcript) transcript->mapped(address, size); }
    void cleared(std::uint32_t address, std::uint64_t size) override { if (transcript) transcript->cleared(address, size); }
    void written(std::uint32_t address, const std::uint8_t* bytes, std::size_t size) override {
        if (transcript) transcript->written(address, bytes, size);
        if ((stage == 0 && state.pc == 0x102578d8) || (stage == 1 && state.pc == 0x10263614))
            throw OriginalRegionBoundary{state.pc};
    }
};
std::uint64_t direct_shot(State& state, std::uint32_t match, std::uint32_t actor, unsigned team) {
    const auto instructions = state.executed, previous_budget = state.budget;
    const auto parent = std::uint32_t(state.r<4>(4)), caller = parent - 0x80, context = caller + 0x50;
    state.wr<4>(4, caller);
    for (unsigned offset = 0; offset < 16; offset += 4) state.memory.write<4>(context + offset, 0);
    unsigned reg = 3;
    for (auto value : {actor, 0x16u, 1u, 0x1f86u, 1u, 0u, context}) state.wr<4>(reg++ * 4, value);
    state.wr<4>(4128, return_sentinel); state.pc = 0x102576e0; state.budget = state.executed + fresh_match_step_limit;
    auto* previous = state.memory.observer;
    OriginalRegions cuts(state, previous); state.memory.observer = &cuts;
    try { run_until(state, return_sentinel); require(false, "Original setup boundary missing"); }
    catch (const OriginalRegionBoundary& boundary) { require(boundary.pc == 0x102578d8, "Original setup boundary wrong"); }
    (void)invoke(state, 0x101f1e10, {match, 1 - team});
    state.wr<4>(96, state.r<4>(12));
    cuts.stage = 1; state.pc = 0x102626bc;
    try { run_until(state, return_sentinel); require(false, "Original statistics boundary missing"); }
    catch (const OriginalRegionBoundary& boundary) { require(boundary.pc == 0x10263614, "Original statistics boundary wrong"); }
    cuts.stage = 2;
    state.pc = 0x102636d8;
    run_until(state, 0x10263700);
    state.pc = 0x10267154;
    run_until(state, return_sentinel);
    state.memory.observer = previous; state.budget = previous_budget;
    require(state.r<4>(4) == caller, "Original goal failed to unwind its own frame");
    state.wr<4>(4, parent);
    return state.executed - instructions;
}
struct CutAfterScore final : MemoryObserver {
    State& state; std::uint32_t score; bool reached = false;
    CutAfterScore(State& value, std::uint32_t address) : state(value), score(address) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t address, const std::uint8_t*, std::size_t size) override {
        if (address == score && size == 1 && state.pc == 0x102671c0) {
            reached = true; state.budget = state.executed + 1;
        }
    }
};
}

int main(int argc, char** argv) {
    const char* stage = "initial world";
    try {
        if (argc != 4) throw std::runtime_error("Expected original program, read-only game and private check root");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        unsigned generation = 0;
        const MatchSetup setup{677, 5677, 12345, 2001, 194};
        auto factory = [&] { return create_match({argv[1], argv[2], root / ("session-" + std::to_string(generation++))}, setup); };
        Session session(factory, fresh_match_step_limit);
        reject_without_effects(session, [&] { session.add_goal(0, 8); });
        auto control = create_match({argv[1], argv[2], root / "control"}, setup);
        const Baseline baseline{session.state().executed, control.state->executed, session.state().native_calls,
            control.state->native_calls, session.state().budget, control.state->budget};
        const auto kickoff = session.step();
        match_engine_diagnostics(*control.state).entry_original[2] = true;
        require(invoke(*control.state, 0x101d3f60, {control.match, 1}) == kickoff.instructions, "Original kickoff accounting");
        same_match(session, control, baseline);
        for (auto bad : {std::pair{2u, 8u}, std::pair{0u, 20u}, std::pair{0u, 19u}})
            reject_without_effects(session, [&] { session.add_goal(bad.first, bad.second); });
        session.run(); reject_without_effects(session, [&] { session.add_goal(0, 8); }); session.pause();
        unsigned goals = 0;
        for (auto selection : {std::pair{0u, 8u}, std::pair{1u, 8u}, std::pair{0u, 0u},
                               std::pair{1u, 0u}, std::pair{0u, 8u}, std::pair{1u, 1u}}) {
            const auto [team, actor] = selection;
            const auto before = session.state().memory.copy_out(session.match_address(), 0xe0bc);
            Trace observed, original; GoalRecord record;
            { Observe observe(const_cast<State&>(session.state()), observed); record = session.add_goal(team, actor); }
            const auto address = control.match + 0x311c + (team * 20 + actor) * 0x1c4;
            std::uint64_t instructions = 0;
            { Observe observe(*control.state, original); instructions = direct_shot(*control.state, control.match, address, team); }
            require(!observed.effects.empty() && observed.effects == original.effects, "Complete ordered original goal effects differ");
            same_match(session, control, baseline);
            require(record.number == ++goals && record.instructions == instructions && record.at_step == session.steps().size() &&
                record.scorer_goals_after == unsigned(record.scorer_goals_before) + 1 &&
                record.after.home_score == record.before.home_score + int(team == 0) &&
                record.after.away_score == record.before.away_score + int(team == 1) &&
                record.after.elapsed_engine_units == record.before.elapsed_engine_units && record.after.interruption == 8 &&
                session.on_pitch(0) == 11 && session.on_pitch(1) == 11, "Original scorer/score/kickoff bookkeeping is incomplete");
            auto scoreboard_only = before;
            ++scoreboard_only[team ? 0xe073 : 0xe03d];
            require(scoreboard_only != session.state().memory.copy_out(session.match_address(), 0xe0bc),
                "Negative control cannot distinguish a full goal from a score-only edit");
            for (unsigned tick = 0; tick < 5; ++tick) {
                const auto next = session.step();
                require(invoke(*control.state, 0x101d3f60, {control.match, 1}) == next.instructions, "Original post-goal advancement accounting");
                same_match(session, control, baseline);
            }
            std::cout << "goal team=" << team << " actor=" << actor << " instructions=" << record.instructions
                << " ordered-effects=" << observed.effects.size() << " following-original-steps=5\n";
        }
        require(session.goals().size() == 6 && session.steps().size() == 31 &&
            match_engine_diagnostics(session.state()).calls[2] == 31 && match_engine_diagnostics(*control.state).original_calls[2] == 31,
            "Goal interventions were confused with accepted shared engine steps");
        auto& state = const_cast<State&>(session.state());
        const auto saved_score = state.memory.read<1>(session.match_address() + 0xe03d);
        state.memory.write<1>(session.match_address() + 0xe03d, 127);
        reject_without_effects(session, [&] { session.add_goal(0, 8); });
        state.memory.write<1>(session.match_address() + 0xe03d, saved_score);
        const auto actor = session.match_address() + 0x311c + 8 * 0x1c4;
        const auto saved_goals = state.memory.read<1>(actor + 0xc);
        state.memory.write<1>(actor + 0xc, 255);
        reject_without_effects(session, [&] { session.add_goal(0, 8); });
        state.memory.write<1>(actor + 0xc, saved_goals);
        const auto retained = state.memory.copy_out(session.match_address(), 0xe0bc);
        stage = "budget world/kickoff";
        Session limited(factory, fresh_match_step_limit); limited.step();
        auto& interrupted = const_cast<State&>(limited.state());
        const auto original_budget = interrupted.budget;
        CutAfterScore cut(interrupted, limited.match_address() + 0xe03d);
        interrupted.memory.observer = &cut;
        bool faulted = false;
        stage = "partial original budget fault";
        try { limited.add_goal(0, 8); } catch (const std::runtime_error&) { faulted = true; }
        interrupted.memory.observer = nullptr;
        require(cut.reached && faulted && limited.execution_state() == ExecutionState::faulted && limited.goals().empty() &&
            limited.steps().size() == 1 && interrupted.budget == original_budget && limited.inspect().home_score == 1 &&
            !limited.failure().empty(), "A partial original goal was accepted or lost its faulted diagnostic state");
        reject_without_effects(limited, [&] { limited.add_goal(0, 0); });
        reject_without_effects(limited, [&] { limited.step(); });
        require(state.memory.copy_out(session.match_address(), 0xe0bc) == retained, "Another goal experiment changed the accepted match");
        stage = "fresh reset after fault";
        session.reset();
        require(session.goals().empty() && session.steps().empty() && session.inspect().home_score == 0 &&
            session.inspect().away_score == 0, "Seeded reset retained injected goals");
        const MatchSetup alternate{5346, 1953, 67890, 2001, 194};
        stage = "alternate world/goals";
        Session other([&]{return create_match({argv[1], argv[2], root / "other"}, alternate);}, fresh_match_step_limit);
        other.step();
        for (auto selection : {std::pair{0u, 0u}, std::pair{1u, 8u}}) {
            other.add_goal(selection.first, selection.second);
            for (unsigned tick = 0; tick < 5; ++tick) {
                try { other.step(); } catch (...) { std::cerr << other.failure() << '\n'; throw; }
            }
        }
        require(other.inspect().home_score == 1 && other.inspect().away_score == 1 && other.goals().size() == 2 &&
            other.execution_state() == ExecutionState::paused, "Alternate actual clubs/seed rejected original goals or failed to resume");
        stage = "in-game goal command";
        auto live = create_match({argv[1], argv[2], root / "live-ui"}, setup);
        auto reference = create_match({argv[1], argv[2], root / "live-reference"}, setup);
        initialize_native_debugging();
        require(!native_debugging_enabled() && !eligible_debug_goal(*live.state, live.match, 8),
            "Debugging must default off and reject pre-kickoff goals");
        invoke(*live.state, 0x101d3f60, {live.match, 1});
        invoke(*reference.state, 0x101d3f60, {reference.match, 1});
        // Nonzero timestamps expose a missing minute rather than accepting a
        // zero-filled report field. Both original worlds retain the same clock.
        live.state->memory.write<2>(live.match + 0x7948, 137);
        reference.state->memory.write<2>(reference.match + 0x7948, 137);
        const auto unchanged = live.state->memory.copy_out(live.match, 0xe0bc);
        bool disabled = false;
        try { award_debug_goal(*live.state, live.match, 8); } catch (const std::invalid_argument&) { disabled = true; }
        require(disabled && live.state->memory.copy_out(live.match, 0xe0bc) == unchanged, "Disabled goal changed the match");
        invoke(*live.state, 0x10773520, {0xdfc});
        const auto report = std::uint32_t(live.state->r<4>(12));
        require(report != 0, "Cannot allocate original live match report");
        live.state->memory.copy_in(report, std::vector<std::uint8_t>(0xdfc));
        struct ExpectedScorer { unsigned side, slot, minute; std::uint32_t staff; };
        std::vector<ExpectedScorer> scorers;
        select_native_debugging(true);
        for (unsigned player : {8u, 8u, 28u, 0u}) {
            auto& actual = *live.state;
            const auto registers = actual.registers;
            const auto scratch = actual.unique;
            const auto pc = actual.pc;
            award_debug_goal(actual, live.match, player);
            require(actual.registers == registers && actual.unique == scratch && actual.pc == pc,
                "In-game goal corrupted its interrupted UI caller");
            auto& expected = *reference.state;
            const auto expected_registers = expected.registers;
            const auto expected_scratch = expected.unique;
            const auto expected_pc = expected.pc;
            expected.wr<4>(4, expected.r<4>(4) - 0x400);
            direct_shot(expected, reference.match, reference.match + 0x311c + player * 0x1c4, player / 20);
            expected.registers = expected_registers; expected.unique = expected_scratch; expected.pc = expected_pc;
            require(actual.memory.copy_out(live.match, 0xe0bc) == expected.memory.copy_out(reference.match, 0xe0bc),
                "In-game goal differs from original goal bookkeeping");
            const auto actor = live.match + 0x311c + player * 0x1c4;
            scorers.push_back({player / 20, unsigned(actual.memory.read<1>(actor + 0x2f)),
                unsigned(actual.memory.read<2>(live.match + 0x7948) / 11), std::uint32_t(actual.memory.read<4>(actor + 0x6c))});
            invoke(actual, 0x1029f920, {0, live.match, report, 1});
            unsigned rows = 0;
            for (unsigned row = 0; row < 50; ++row) {
                const auto event = report + 0x55 + row * 4;
                if (actual.memory.read<1>(event) == 255) break;
                if (actual.memory.read<1>(event + 2) != 1) continue;
                require(rows < scorers.size(), "Duplicate goal in the original scorer list");
                const auto& scorer = scorers[rows++];
                require(actual.memory.read<1>(event) == scorer.side && actual.memory.read<1>(event + 1) == scorer.slot &&
                    actual.memory.read<1>(event + 3) == scorer.minute, "Original scorer list lost the side, player or minute");
                const auto listed = report + 0x194 + scorer.side * 0x664 + scorer.slot * 0x4c;
                require(actual.memory.read<4>(listed) == actual.memory.read<4>(scorer.staff),
                    "Original scorer list does not identify the selected player");
            }
            require(rows == scorers.size(), "Awarded goal is missing from the original scorer list");
            // The first two selections model consecutive repeat-button clicks
            // without a clock advance between them.
            const unsigned following_steps = scorers.size() == 1 ? 0 : scorers.size() == 2 ? 10 : 5;
            for (unsigned tick = 0; tick < following_steps; ++tick) {
                invoke(actual, 0x101d3f60, {live.match, 1});
                invoke(expected, 0x101d3f60, {reference.match, 1});
                require(actual.memory.copy_out(live.match, 0xe0bc) == expected.memory.copy_out(reference.match, 0xe0bc),
                    "In-game goal failed to resume the original match");
            }
        }
        require(!eligible_debug_goal(*live.state, live.match, 40) && !eligible_debug_goal(*live.state, live.match, 19),
            "In-game goals accept an invalid or off-pitch player");
        live.state->memory.write<1>(live.match, 1);
        require(!eligible_debug_goal(*live.state, live.match, 8), "Completed match accepts goals");
        live.state->memory.write<1>(live.match, 0);
        live.state->memory.write<1>(live.match + 0xe03d, 127);
        require(!eligible_debug_goal(*live.state, live.match, 8), "Saturated score accepts goals");
        select_native_debugging(false);
        reset_match_debugging();
        std::cout << "PASS: eight debugger and four in-game original goal interventions; complete bookkeeping, UI caller preservation, repeated/home/away/keeper scorers, 50 following steps, rejection, counter limits and reset\n";
    } catch (const std::exception& error) { std::cerr << stage << ": " << error.what() << '\n'; return 1; }
}
