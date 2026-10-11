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

    void run();
    void pause();
    StepRecord step();
    std::optional<StepRecord> advance_if_running();
    void reset();

private:
    Factory factory_;
    EngineInstance engine_;
    std::uint64_t step_instruction_limit_;
    ExecutionState execution_ = ExecutionState::paused;
    std::string failure_;
    std::vector<StepRecord> steps_;

    StepRecord execute_step();
    static Inspection inspect(const EngineInstance&);
};

}
