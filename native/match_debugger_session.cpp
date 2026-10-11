#include "match_debugger_session.hpp"
#include "match_engine.hpp"
#include <limits>

namespace cm::aot::match_debugger {
namespace {
constexpr std::uint32_t shared_advancement = match_engine::entries[2];

class StepBudget {
    State& state_;
    std::uint64_t previous_;
public:
    StepBudget(State& state, std::uint64_t limit) : state_(state), previous_(state.budget) {
        if (state.executed > std::numeric_limits<std::uint64_t>::max() - limit)
            throw std::runtime_error("Match debugger instruction counter would overflow");
        state.budget = state.executed + limit;
    }
    ~StepBudget() { state_.budget = previous_; }
};
}

Session::Session(Factory factory, std::uint64_t step_instruction_limit)
    : factory_(std::move(factory)), step_instruction_limit_(step_instruction_limit) {
    if (!factory_ || !step_instruction_limit_)
        throw std::invalid_argument("Match debugger requires a session factory and a positive step limit");
    reset();
}

Inspection Session::inspect(const EngineInstance& engine) {
    if (!engine.state || !engine.match)
        throw std::runtime_error("Match debugger requires an initialized independent match");
    const auto& memory = engine.state->memory;
    const auto match = engine.match;
    Inspection result;
    result.elapsed_engine_units = std::int16_t(signed_value<2>(memory.read<2>(match + 0x7948)));
    result.home_score = std::int8_t(signed_value<1>(memory.read<1>(match + 0xe03d)));
    result.away_score = std::int8_t(signed_value<1>(memory.read<1>(match + 0xe073)));
    result.phase = std::int8_t(signed_value<1>(memory.read<1>(match + 0x7929)));
    result.interruption = std::int8_t(signed_value<1>(memory.read<1>(match + 0x7928)));
    result.transition = std::int8_t(signed_value<1>(memory.read<1>(match + 0x792a)));
    result.actor_queue_count = std::int8_t(signed_value<1>(memory.read<1>(match + 0x792e)));
    return result;
}

Inspection Session::inspect() const { return inspect(engine_); }

void Session::reset() {
    // Keep the previous experiment available if creation of its replacement
    // fails. Each successful factory result owns fresh guest and random state.
    auto replacement = factory_();
    (void)inspect(replacement);
    engine_ = std::move(replacement);
    steps_.clear();
    failure_.clear();
    execution_ = ExecutionState::paused;
}

void Session::run() {
    if (execution_ != ExecutionState::paused)
        throw std::logic_error("Match debugger can run only a paused experiment");
    execution_ = ExecutionState::running;
}

void Session::pause() {
    if (execution_ == ExecutionState::running) execution_ = ExecutionState::paused;
}

StepRecord Session::execute_step() {
    auto& state = *engine_.state;
    try {
        StepRecord record;
        record.number = steps_.size() + 1;
        record.before = inspect();
        const auto instructions = state.executed;
        StepBudget budget(state, step_instruction_limit_);
        state.wr<4>(12, engine_.match);
        state.wr<4>(16, 1);
        state.wr<4>(4128, return_sentinel);
        state.pc = shared_advancement;
        run_until(state, return_sentinel);
        if (state.pc != return_sentinel)
            throw std::runtime_error("Match debugger engine step did not return to its boundary");
        record.engine_return = std::uint32_t(state.r<4>(12));
        record.instructions = state.executed - instructions;
        record.after = inspect();
        switch (record.engine_return) {
        case 1: break;
        case 2: execution_ = ExecutionState::completed; break;
        case 4: execution_ = ExecutionState::paused; break;
        case 8: execution_ = ExecutionState::aborted; break;
        default: throw std::runtime_error("Match debugger received an unknown engine advancement result");
        }
        steps_.push_back(record);
        return record;
    } catch (const std::exception& error) {
        // An interrupted guest step may have partial effects. Retain them for
        // diagnosis, block further advancement, and require a fresh reset.
        execution_ = ExecutionState::faulted;
        failure_ = error.what();
        throw;
    } catch (...) {
        execution_ = ExecutionState::faulted;
        failure_ = "Match debugger engine step was interrupted before returning";
        throw;
    }
}

StepRecord Session::step() {
    if (execution_ != ExecutionState::paused)
        throw std::logic_error("Match debugger stepping requires a paused experiment");
    return execute_step();
}

std::optional<StepRecord> Session::advance_if_running() {
    if (execution_ != ExecutionState::running) return std::nullopt;
    return execute_step();
}

}
