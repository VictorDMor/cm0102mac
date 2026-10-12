#include "match_debugger_factory.hpp"
#include "match_engine_adapter.hpp"
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
    const auto withdrawals = session.withdrawals().size();
    bool rejected = false;
    try { action(); } catch (const std::logic_error&) { rejected = true; }
    require(rejected && session.inspect() == inspection && session.state().memory.copy_out(session.match_address(), 0xe0bc) == memory &&
        session.state().registers == registers && session.state().unique == unique && session.state().executed == instructions &&
        session.withdrawals().size() == withdrawals, "Rejected withdrawal changed live state or audit history");
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("Expected original program, read-only game and private check root");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        unsigned generation = 0;
        const MatchSetup setup{677, 5677, 12345, 2001, 194};
        auto factory = [&] { return create_match({argv[1], argv[2], root / ("session-" + std::to_string(generation++))}, setup); };
        Session session(factory, fresh_match_step_limit);
        require(session.on_pitch(0) == 11 && session.on_pitch(1) == 11, "Fresh actual starting elevens");
        reject_without_effects(session, [&] { session.withdraw_injured_player(0, 8); });
        auto control = create_match({argv[1], argv[2], root / "control-0"}, setup);
        const Baseline baseline{session.state().executed, control.state->executed, session.state().native_calls,
            control.state->native_calls, session.state().budget, control.state->budget};
        const auto kickoff = session.step();
        match_engine_diagnostics(*control.state).entry_original[2] = true;
        require(invoke(*control.state, 0x101d3f60, {control.match, 1}) == kickoff.instructions, "Original kickoff accounting");
        same_match(session, control, baseline);
        reject_without_effects(session, [&] { session.withdraw_injured_player(2, 8); });
        reject_without_effects(session, [&] { session.withdraw_injured_player(0, 20); });
        reject_without_effects(session, [&] { session.withdraw_injured_player(0, 19); });
        session.run();
        reject_without_effects(session, [&] { session.withdraw_injured_player(0, 8); });
        session.pause();
        std::array<unsigned, 2> expected{11, 11};
        for (auto selection : {std::pair{0u, 8u}, std::pair{0u, 0u}, std::pair{1u, 8u}}) {
            const auto [team, actor] = selection;
            Trace observed, original;
            const auto before = session.state().memory.copy_out(session.match_address(), 0xe0bc);
            WithdrawalRecord record;
            {
                // Inspection is const; this test attaches a write observer to
                // the same exclusively owned state without guest mutation.
                Observe observe(const_cast<State&>(session.state()), observed);
                record = session.withdraw_injured_player(team, actor);
            }
            const auto address = control.match + 0x311c + (team * 20 + actor) * 0x1c4;
            std::uint64_t instructions = 0;
            { Observe observe(*control.state, original); instructions = invoke(*control.state, 0x101f1530, {control.match, address, 0}); }
            require(observed.effects == original.effects && !observed.effects.empty(), "Complete ordered original intervention effects");
            same_match(session, control, baseline);
            --expected[team];
            require(record.instructions == instructions && record.position_after == -3 &&
                record.withdrawals_after == std::uint8_t(record.withdrawals_before + 1) &&
                record.on_pitch_before == expected[team] + 1 && record.on_pitch_after == expected[team] &&
                session.on_pitch(1 - team) == expected[1 - team] &&
                record.before.elapsed_engine_units == record.after.elapsed_engine_units,
                "Actual injury/tactical bookkeeping was not applied");
            auto position_only = before;
            position_only[0x311c + (team * 20 + actor) * 0x1c4 + 0x19] = 0xfd;
            require(position_only != session.state().memory.copy_out(session.match_address(), 0xe0bc),
                "Negative control cannot distinguish original withdrawal from a position-only edit");
            reject_without_effects(session, [&] { session.withdraw_injured_player(team, actor); });
            for (unsigned tick = 0; tick < 10; ++tick) {
                const auto next = session.step();
                require(invoke(*control.state, 0x101d3f60, {control.match, 1}) == next.instructions, "Post-withdrawal original step accounting");
                same_match(session, control, baseline);
                require(session.on_pitch(team) == expected[team] && session.state().memory.read<1>(address + 0x19) == 0xfd,
                    "Original engine brought the withdrawn actor back on pitch");
            }
            std::cout << "withdrawn team=" << team << " actor=" << actor << " instructions=" << record.instructions << " ordered-effects=" << observed.effects.size()
                      << " on-pitch=" << record.on_pitch_after << " original-following-steps=10\n";
        }
        require(session.withdrawals().size() == 3 && session.steps().size() == 31, "Interventions were confused with completed engine steps");
        require(match_engine_diagnostics(session.state()).calls[2] == 31 && match_engine_diagnostics(*control.state).original_calls[2] == 31,
            "Subsequent modern/original engine controls were bypassed");
        const auto retained = session.state().memory.copy_out(session.match_address(), 0xe0bc);
        Session limited(factory, 400000);
        (void)limited.step();
        const auto original_budget = limited.state().budget;
        const auto before_fault = limited.state().memory.copy_out(limited.match_address(), 0xe0bc);
        bool faulted = false;
        try { limited.withdraw_injured_player(0, 8); } catch (const std::runtime_error&) { faulted = true; }
        require(faulted && limited.execution_state() == ExecutionState::faulted && limited.withdrawals().empty() &&
            limited.steps().size() == 1 && limited.state().budget == original_budget &&
            limited.state().memory.copy_out(limited.match_address(), 0xe0bc) != before_fault && !limited.failure().empty(),
            "Interrupted withdrawal was reported as accepted or discarded its partial diagnostic state");
        reject_without_effects(limited, [&] { limited.withdraw_injured_player(0, 0); });
        require(session.state().memory.copy_out(session.match_address(), 0xe0bc) == retained, "Another experiment changed the accepted match");
        session.reset();
        require(session.withdrawals().empty() && session.steps().empty() && session.on_pitch(0) == 11 && session.on_pitch(1) == 11,
            "Fresh reset retained intervention state");
        std::cout << "PASS: real home/away/outfield/goalkeeper withdrawals, complete original ABI/match/accounting/ordered-effects agreement, 30 following original-agreeing steps, position-only negatives, rejected edits, partial budget fault, independence and fresh reset\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
