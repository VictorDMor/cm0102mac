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
    withdrawals_.clear();
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

unsigned Session::on_pitch(unsigned team) const {
    if (team > 1) throw std::invalid_argument("Match side must be home or away");
    unsigned count = 0;
    for (unsigned actor = 0; actor < 20; ++actor) {
        const auto address = engine_.match + 0x311c + (team * 20 + actor) * 0x1c4;
        const auto position = signed_value<1>(engine_.state->memory.read<1>(address + 0x19));
        count += position >= 0 && position < 11;
    }
    return count;
}

WithdrawalRecord Session::withdraw_injured_player(unsigned team, unsigned actor) {
    if (execution_ != ExecutionState::paused || steps_.empty())
        throw std::logic_error("Injury withdrawal requires a paused match after kickoff");
    if (team > 1 || actor >= 20)
        throw std::invalid_argument("Invalid match side or actor index");
    auto& state = *engine_.state;
    const auto address = engine_.match + 0x311c + (team * 20 + actor) * 0x1c4;
    const auto staff = std::uint32_t(state.memory.read<4>(address + 0x6c));
    const auto position = signed_value<1>(state.memory.read<1>(address + 0x19));
    if (!staff || position < 0 || position >= 11 || state.memory.read<1>(address + 0x27) != team ||
        state.memory.read<4>(address + 0x1a4) != engine_.match)
        throw std::invalid_argument("Select an actual player currently on the pitch");

    WithdrawalRecord record;
    record.number = withdrawals_.size() + 1;
    record.at_step = steps_.size();
    record.team = team; record.actor = actor;
    record.staff_id = std::uint32_t(state.memory.read<4>(staff));
    record.position_before = std::int8_t(position);
    record.withdrawals_before = std::uint8_t(state.memory.read<1>(address + 0x23));
    record.on_pitch_before = on_pitch(team);
    record.before = inspect();
    const auto instructions = state.executed;
    try {
        StepBudget budget(state, step_instruction_limit_);
        state.wr<4>(12, engine_.match);
        state.wr<4>(16, address);
        state.wr<4>(20, 0); // Original injury count/time and tactical update path.
        state.wr<4>(4128, return_sentinel);
        state.pc = 0x101f1530;
        run_until(state, return_sentinel);
        if (state.pc != return_sentinel)
            throw std::runtime_error("Original injury withdrawal did not return to its boundary");
        record.instructions = state.executed - instructions;
        record.position_after = std::int8_t(signed_value<1>(state.memory.read<1>(address + 0x19)));
        record.withdrawals_after = std::uint8_t(state.memory.read<1>(address + 0x23));
        record.on_pitch_after = on_pitch(team);
        record.after = inspect();
        if (record.position_after != -3)
            throw std::runtime_error("Original injury withdrawal did not mark the selected actor unavailable");
        withdrawals_.push_back(record);
        return record;
    } catch (const std::exception& error) {
        execution_ = ExecutionState::faulted;
        failure_ = "Engine intervention fault at " + hex_address(state.pc) + ": " + error.what();
        throw;
    } catch (...) {
        execution_ = ExecutionState::faulted;
        failure_ = "Engine intervention was interrupted before returning";
        throw;
    }
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
        failure_ = "Engine step fault at " + hex_address(state.pc) + ": " + error.what();
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
