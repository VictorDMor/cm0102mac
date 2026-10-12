#include "match_debugger_session.hpp"
#include "match_engine.hpp"
#include "match_debugger_goal.hpp"
#include <limits>
#include <algorithm>

namespace cm::aot::match_debugger {
namespace {
constexpr std::uint32_t shared_advancement = match_engine::entries[2];

std::uint32_t invoke(State& state, std::uint32_t entry, std::initializer_list<std::uint32_t> values) {
    unsigned reg = 3;
    for (auto value : values) state.wr<4>(reg++ * 4, value);
    state.wr<4>(4128, return_sentinel); state.pc = entry;
    run_until(state, return_sentinel);
    if (state.pc != return_sentinel) throw std::runtime_error("Original lineup operation did not return to its boundary");
    return std::uint32_t(state.r<4>(12));
}

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
    goals_.clear();
    lineup_edits_.clear();
    failure_.clear();
    execution_ = ExecutionState::paused;
}

void Session::run() {
    if (execution_ != ExecutionState::paused)
        throw std::logic_error("Match debugger can run only a paused experiment");
    validate_kickoff_lineups();
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
        count += position >= 0 && position < 11 && engine_.state->memory.read<4>(address + 0x6c) != 0;
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

const std::vector<RosterMember>& Session::roster(unsigned team) const {
    if (team > 1) throw std::invalid_argument("Match side must be home or away");
    return engine_.rosters[team];
}

void Session::require_pre_kickoff_edit() const {
    if (execution_ != ExecutionState::paused || !steps_.empty() || inspect().elapsed_engine_units != 0)
        throw std::logic_error("Lineup editing requires a paused experiment before kickoff");
}

void Session::validate_kickoff_lineups() const {
    if (!steps_.empty() || inspect().elapsed_engine_units != 0) return;
    for (unsigned team = 0; team < 2; ++team) {
        std::array<bool, 11> assigned{};
        for (unsigned actor = 0; actor < 20; ++actor) {
            const auto address = engine_.match + 0x311c + (team * 20 + actor) * 0x1c4;
            const auto& memory = engine_.state->memory;
            const auto position = signed_value<1>(memory.read<1>(address + 0x19));
            if (position < 0) continue;
            if (position >= 11 || assigned[unsigned(position)] || !memory.read<4>(address + 0x6c) ||
                memory.read<1>(address + 0x27) != team || memory.read<4>(address + 0x1a4) != engine_.match)
                throw std::logic_error("Starting lineup has an invalid or duplicate position");
            assigned[unsigned(position)] = true;
        }
        if (!assigned[0]) throw std::logic_error("Assign an actual player to goalkeeper slot 0 before kickoff");
    }
}

LineupRecord Session::set_player(unsigned team, unsigned actor, std::uint32_t staff_id) {
    require_pre_kickoff_edit();
    if (team > 1 || actor >= 20) throw std::invalid_argument("Invalid match side or actor index");
    const auto& members = roster(team);
    const auto member = std::find_if(members.begin(), members.end(), [&](const auto& value) { return value.staff_id == staff_id; });
    auto& state = *engine_.state;
    if (member == members.end() || !member->staff || !member->player ||
        state.memory.read<4>(member->staff) != staff_id || state.memory.read<4>(member->staff + 0x14) != member->player)
        throw std::invalid_argument("Select an actual player from this club's roster");
    for (unsigned side = 0; side < 2; ++side) for (unsigned index = 0; index < 20; ++index)
        if (state.memory.read<4>(engine_.match + 0x311c + (side * 20 + index) * 0x1c4 + 0x6c) == member->staff)
            throw std::invalid_argument("This player already occupies a match actor");
    const auto address = engine_.match + 0x311c + (team * 20 + actor) * 0x1c4;
    const auto previous = std::uint32_t(state.memory.read<4>(address + 0x6c));
    LineupRecord record;
    record.number = lineup_edits_.size() + 1; record.team = team; record.actor = actor;
    record.staff_before = previous ? std::uint32_t(state.memory.read<4>(previous)) : 0;
    record.staff_after = staff_id; record.position_before = std::int8_t(signed_value<1>(state.memory.read<1>(address + 0x19)));
    record.on_pitch_before = on_pitch(team);
    const auto before = state.executed;
    try {
        StepBudget budget(state, step_instruction_limit_);
        const auto controller = engine_.match + 0x7aac + team * 0xd8;
        // The complete original preparation root invokes the full original
        // actor initializer with this live staff/player and original actor ID.
        if (invoke(state, 0x1020e8f0, {controller, actor, member->staff, actor}) != 1)
            throw std::runtime_error("Original player initialization did not complete");
        const auto initialized = signed_value<1>(state.memory.read<1>(address + 0x19));
        if (previous && initialized >= -1) {
            const auto wanted = record.position_before >= -1 ? record.position_before : -1;
            if (initialized != wanted) (void)invoke(state, 0x101f1870, {address, std::uint32_t(wanted)});
        }
        if (state.memory.read<4>(address + 0x6c) != member->staff || state.memory.read<4>(address + 0x1a4) != engine_.match ||
            state.memory.read<1>(address + 0x27) != team || state.memory.read<1>(address + 0x2f) != actor)
            throw std::runtime_error("Original player initialization did not retain its live identity");
        record.position_after = std::int8_t(signed_value<1>(state.memory.read<1>(address + 0x19)));
        record.on_pitch_after = on_pitch(team); record.instructions = state.executed - before;
        lineup_edits_.push_back(record); return record;
    } catch (const std::exception& error) {
        execution_ = ExecutionState::faulted;
        failure_ = "Lineup player intervention fault at " + hex_address(state.pc) + ": " + error.what(); throw;
    } catch (...) {
        execution_ = ExecutionState::faulted; failure_ = "Lineup player intervention was interrupted"; throw;
    }
}

LineupRecord Session::set_position(unsigned team, unsigned actor, int position) {
    require_pre_kickoff_edit();
    if (team > 1 || actor >= 20 || position < -1 || position > 10)
        throw std::invalid_argument("Invalid match side, actor or lineup position");
    auto& state = *engine_.state;
    const auto address = engine_.match + 0x311c + (team * 20 + actor) * 0x1c4;
    const auto staff = std::uint32_t(state.memory.read<4>(address + 0x6c));
    const auto old_position = signed_value<1>(state.memory.read<1>(address + 0x19));
    if (!staff || old_position < -1 || old_position > 10 || state.memory.read<1>(address + 0x27) != team ||
        state.memory.read<4>(address + 0x1a4) != engine_.match)
        throw std::invalid_argument("Select an actual available player in this match");
    if (old_position == position) throw std::invalid_argument("The player already has this lineup position");
    LineupRecord record;
    record.kind = LineupRecord::Kind::position; record.number = lineup_edits_.size() + 1;
    record.team = team; record.actor = actor; record.staff_before = record.staff_after = std::uint32_t(state.memory.read<4>(staff));
    record.position_before = std::int8_t(old_position); record.on_pitch_before = on_pitch(team);
    if (position >= 0) for (unsigned other = 0; other < 20; ++other) {
        const auto candidate = engine_.match + 0x311c + (team * 20 + other) * 0x1c4;
        if (other == actor || signed_value<1>(state.memory.read<1>(candidate + 0x19)) != position) continue;
        const auto other_staff = state.memory.read<4>(candidate + 0x6c);
        if (!other_staff || state.memory.read<1>(candidate + 0x27) != team || state.memory.read<4>(candidate + 0x1a4) != engine_.match ||
            record.displaced_actor != 20) throw std::invalid_argument("Starting lineup has an invalid or duplicate position");
        record.displaced_actor = other; record.displaced_staff = std::uint32_t(state.memory.read<4>(other_staff));
        record.displaced_before = std::int8_t(position);
    }
    const auto before = state.executed;
    try {
        StepBudget budget(state, step_instruction_limit_);
        if (record.displaced_actor != 20) {
            const auto other = engine_.match + 0x311c + (team * 20 + record.displaced_actor) * 0x1c4;
            (void)invoke(state, 0x101f1870, {other, std::uint32_t(old_position)});
            record.displaced_after = std::int8_t(signed_value<1>(state.memory.read<1>(other + 0x19)));
        }
        (void)invoke(state, 0x101f1870, {address, std::uint32_t(position)});
        record.position_after = std::int8_t(signed_value<1>(state.memory.read<1>(address + 0x19)));
        record.on_pitch_after = on_pitch(team); record.instructions = state.executed - before;
        lineup_edits_.push_back(record); return record;
    } catch (const std::exception& error) {
        execution_ = ExecutionState::faulted; failure_ = "Lineup position intervention fault at " + hex_address(state.pc) + ": " + error.what(); throw;
    } catch (...) {
        execution_ = ExecutionState::faulted; failure_ = "Lineup position intervention was interrupted"; throw;
    }
}

GoalRecord Session::add_goal(unsigned team, unsigned actor) {
    if (execution_ != ExecutionState::paused || steps_.empty())
        throw std::logic_error("Adding a goal requires a paused match after kickoff");
    if (team > 1 || actor >= 20)
        throw std::invalid_argument("Invalid match side or actor index");
    auto& state = *engine_.state;
    const auto address = engine_.match + 0x311c + (team * 20 + actor) * 0x1c4;
    const auto staff = std::uint32_t(state.memory.read<4>(address + 0x6c));
    const auto position = signed_value<1>(state.memory.read<1>(address + 0x19));
    if (!staff || position < 0 || position >= 11 || state.memory.read<1>(address + 0x27) != team ||
        state.memory.read<4>(address + 0x1a4) != engine_.match)
        throw std::invalid_argument("Select an actual player currently on the pitch");
    GoalRecord record;
    record.before = inspect();
    const auto score = team ? record.before.away_score : record.before.home_score;
    record.scorer_goals_before = std::uint8_t(state.memory.read<1>(address + 0xc));
    if (score < 0 || score == 127 || record.scorer_goals_before == 255)
        throw std::invalid_argument("Original goal counters have reached their supported limit");
    record.number = goals_.size() + 1;
    record.at_step = steps_.size();
    record.team = team; record.actor = actor;
    record.staff_id = std::uint32_t(state.memory.read<4>(staff));
    const auto instructions = state.executed;
    try {
        StepBudget budget(state, step_instruction_limit_);
        apply_original_awarded_goal(state, engine_.match, address, team);
        record.instructions = state.executed - instructions;
        record.scorer_goals_after = std::uint8_t(state.memory.read<1>(address + 0xc));
        record.after = inspect();
        if (record.after.home_score != record.before.home_score + int(team == 0) ||
            record.after.away_score != record.before.away_score + int(team == 1) ||
            record.scorer_goals_after != unsigned(record.scorer_goals_before) + 1 ||
            record.after.elapsed_engine_units != record.before.elapsed_engine_units)
            throw std::runtime_error("Original awarded goal did not complete its bookkeeping: score " +
                std::to_string(record.before.home_score) + "-" + std::to_string(record.before.away_score) + " -> " +
                std::to_string(record.after.home_score) + "-" + std::to_string(record.after.away_score) + "; scorer " +
                std::to_string(record.scorer_goals_before) + " -> " + std::to_string(record.scorer_goals_after) + "; clock " +
                std::to_string(record.before.elapsed_engine_units) + " -> " + std::to_string(record.after.elapsed_engine_units));
        goals_.push_back(record);
        return record;
    } catch (const std::exception& error) {
        execution_ = ExecutionState::faulted;
        failure_ = "Engine goal intervention fault at " + hex_address(state.pc) + ": " + error.what();
        throw;
    } catch (...) {
        execution_ = ExecutionState::faulted;
        failure_ = "Engine goal intervention was interrupted before returning";
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
    validate_kickoff_lineups();
    return execute_step();
}

std::optional<StepRecord> Session::advance_if_running() {
    if (execution_ != ExecutionState::running) return std::nullopt;
    return execute_step();
}

}
