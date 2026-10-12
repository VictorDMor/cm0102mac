#pragma once

#include "aot_runtime.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace cm::aot::match_debugger {

// The session factory must construct an independent, initialized real match.
// Database loading and club/lineup selection belong to that factory.
struct EngineInstance {
    std::unique_ptr<State> state;
    std::uint32_t match = 0;
};

enum class ExecutionState { paused, running, completed, aborted, faulted };

struct Inspection {
    std::int16_t elapsed_engine_units = 0;
    std::int8_t home_score = 0, away_score = 0;
    std::int8_t phase = 0, interruption = 0, transition = 0;
    // This is the live actor work queue, which can contain repeated actors.
    // Its count is not a count of players on the pitch.
    std::int8_t actor_queue_count = 0;
    bool operator==(const Inspection&) const = default;
};

struct StepRecord {
    std::uint64_t number = 0, instructions = 0;
    std::uint32_t engine_return = 0;
    Inspection before, after;
};

struct WithdrawalRecord {
    std::uint64_t number = 0, at_step = 0, instructions = 0;
    std::uint32_t team = 0, actor = 0, staff_id = 0;
    std::int8_t position_before = 0, position_after = 0;
    std::uint8_t withdrawals_before = 0, withdrawals_after = 0;
    unsigned on_pitch_before = 0, on_pitch_after = 0;
    Inspection before, after;
};

struct GoalRecord {
    std::uint64_t number = 0, at_step = 0, instructions = 0;
    std::uint32_t team = 0, actor = 0, staff_id = 0;
    std::uint8_t scorer_goals_before = 0, scorer_goals_after = 0;
    Inspection before, after;
};

// One owner drives this session. A UI scheduler invokes advance_if_running()
// between UI events; it must not concurrently access a running guest State.
// The engine call uses the same dispatch as the production game.
class Session {
public:
    using Factory = std::function<EngineInstance()>;
    explicit Session(Factory, std::uint64_t step_instruction_limit = 1000000);

    ExecutionState execution_state() const { return execution_; }
    const std::string& failure() const { return failure_; }
    Inspection inspect() const;
    const std::vector<StepRecord>& steps() const { return steps_; }
    const std::vector<WithdrawalRecord>& withdrawals() const { return withdrawals_; }
    const std::vector<GoalRecord>& goals() const { return goals_; }
    const State& state() const { return *engine_.state; }
    std::uint32_t match_address() const { return engine_.match; }

    void run();
    void pause();
    StepRecord step();
    std::optional<StepRecord> advance_if_running();
    void reset();
    unsigned on_pitch(unsigned team) const;
    // Original injury withdrawal, applied only after kickoff at a paused
    // boundary. This is distinct from lineup editing or a red-card operation.
    WithdrawalRecord withdraw_injured_player(unsigned team, unsigned actor);
    // Force an awarded goal through the characterized original bookkeeping
    // regions. It updates the scorer, team, commentary and kickoff.
    GoalRecord add_goal(unsigned team, unsigned actor);

private:
    Factory factory_;
    EngineInstance engine_;
    std::uint64_t step_instruction_limit_;
    ExecutionState execution_ = ExecutionState::paused;
    std::string failure_;
    std::vector<StepRecord> steps_;
    std::vector<WithdrawalRecord> withdrawals_;
    std::vector<GoalRecord> goals_;

    StepRecord execute_step();
    static Inspection inspect(const EngineInstance&);
};

}
