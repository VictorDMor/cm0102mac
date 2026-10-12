#include "match_debugger_session.hpp"
#include "match_engine.hpp"
#include "match_debugger_goal.hpp"
#include "match_debugger_cards.hpp"
#include "match_debugger_checkpoint.hpp"
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

class MatchEventsCapture final : public MemoryObserver {
    State& state_;
    std::uint32_t match_;
    bool injected_;
    MemoryObserver* previous_;
    std::function<void(MatchEvent)> event_;
    std::uint32_t word(std::uint32_t address) const {
        const auto bytes = state_.memory.copy_out(address, 4); return std::uint32_t(read_be<4>(bytes.data()));
    }
public:
    MatchEventsCapture(State& state, std::uint32_t match, bool injected, std::function<void(MatchEvent)> event)
        : state_(state), match_(match), injected_(injected), previous_(state.memory.date_observer), event_(std::move(event)) {
        state_.memory.date_observer = this;
    }
    ~MatchEventsCapture() { state_.memory.date_observer = previous_; }
    void mapped(std::uint32_t address, std::uint64_t size) override { if (previous_) previous_->mapped(address, size); }
    void cleared(std::uint32_t address, std::uint64_t size) override { if (previous_) previous_->cleared(address, size); }
    void read(std::uint32_t address, const std::uint8_t* bytes, unsigned size) override { if (previous_) previous_->read(address, bytes, size); }
    void written(std::uint32_t address, const std::uint8_t* bytes, std::size_t size) override {
        if (previous_) previous_->written(address, bytes, size);
        // Original commentary commits its twenty-byte ring record here, after
        // writing the event code, both player/side pairs and context pointer.
        // Observe the committed event; attempted/rejected commentary is absent.
        if (state_.pc != 0x10201704 || size != 1 || state_.r<4>(120) != match_) return;
        MatchEvent value;
        value.code = int(signed_value<2>(state_.r<4>(96))) - 8000;
        if (value.code < 0 || value.code > 676) throw std::runtime_error("Original commentary committed an unknown event");
        value.actor = int(signed_value<1>(state_.r<4>(100))); value.team = int(signed_value<1>(state_.r<4>(104)));
        value.other_actor = int(signed_value<1>(state_.r<4>(108))); value.other_team = int(signed_value<1>(state_.r<4>(112)));
        const auto staff_id = [&](int team, int actor) {
            if (team < 0 || team > 1 || actor < 0 || actor >= 20) return std::uint32_t(0);
            const auto staff = word(match_ + 0x311c + (unsigned(team) * 20 + unsigned(actor)) * 0x1c4 + 0x6c);
            return staff ? word(staff) : 0;
        };
        value.staff_id = staff_id(value.team, value.actor); value.other_staff_id = staff_id(value.other_team, value.other_actor);
        const auto time = state_.memory.copy_out(match_ + 0x7948, 2);
        Inspection clock; clock.elapsed_engine_units = std::int16_t(signed_value<2>(read_be<2>(time.data())));
        value.seconds = clock_seconds(clock); value.injected = injected_;
        event_(value);
    }
};
}

struct Session::History {
    struct Frame {
        std::unique_ptr<Checkpoint> guest;
        std::size_t steps, withdrawals, goals, lineups, substitutions, events, cards;
        std::array<std::int8_t, 40> return_positions;
        unsigned seconds;
        ExecutionState execution;
    };
    std::vector<Frame> frames;
    std::size_t cursor = 0;
    std::vector<StepRecord> steps;
    std::vector<WithdrawalRecord> withdrawals;
    std::vector<GoalRecord> goals;
    std::vector<LineupRecord> lineups;
    std::vector<SubstitutionRecord> substitutions;
    std::vector<MatchEvent> events;
    std::vector<CardRecord> cards;
};

Session::~Session() = default;

void Session::capture_boundary() {
    if (!history_) history_ = std::make_unique<History>();
    auto guest = std::make_unique<Checkpoint>(*engine_.state,
        history_->frames.empty() ? nullptr : history_->frames.back().guest.get());
    if (!history_->frames.empty()) guest->caller_budget(history_->frames[history_->cursor].guest->budget());
    history_->frames.push_back({std::move(guest), steps_.size(), withdrawals_.size(), goals_.size(), lineup_edits_.size(),
        substitutions_.size(), events_.size(), cards_.size(), return_positions_, clock_seconds(inspect()),
        execution_ == ExecutionState::running ? ExecutionState::paused : execution_});
    history_->cursor = history_->frames.size() - 1;
    history_->steps = steps_; history_->withdrawals = withdrawals_; history_->goals = goals_;
    history_->lineups = lineup_edits_; history_->substitutions = substitutions_; history_->events = events_; history_->cards = cards_;
}

void Session::branch_history() {
    if (!history_ || history_->cursor + 1 == history_->frames.size()) return;
    history_->frames.resize(history_->cursor + 1);
    history_->steps = steps_; history_->withdrawals = withdrawals_; history_->goals = goals_;
    history_->lineups = lineup_edits_; history_->substitutions = substitutions_; history_->events = events_; history_->cards = cards_;
}

void Session::restore_boundary(std::size_t index) {
    if (!history_ || index >= history_->frames.size()) throw std::invalid_argument("Choose a recorded moment");
    const auto& frame = history_->frames[index];
    auto restored = frame.guest->restore();
    engine_.state = std::move(restored);
    steps_.assign(history_->steps.begin(), history_->steps.begin() + frame.steps);
    withdrawals_.assign(history_->withdrawals.begin(), history_->withdrawals.begin() + frame.withdrawals);
    goals_.assign(history_->goals.begin(), history_->goals.begin() + frame.goals);
    lineup_edits_.assign(history_->lineups.begin(), history_->lineups.begin() + frame.lineups);
    substitutions_.assign(history_->substitutions.begin(), history_->substitutions.begin() + frame.substitutions);
    events_.assign(history_->events.begin(), history_->events.begin() + frame.events);
    cards_.assign(history_->cards.begin(), history_->cards.begin() + frame.cards);
    return_positions_ = frame.return_positions;
    history_->cursor = index; failure_.clear(); execution_ = frame.execution;
}

std::size_t Session::timeline_index() const { return history_ ? history_->cursor : 0; }
std::size_t Session::timeline_size() const { return history_ ? history_->frames.size() : 0; }
unsigned Session::recorded_seconds() const { return history_ ? history_->frames.back().seconds : 0; }
void Session::back() {
    if (execution_ == ExecutionState::running) throw std::logic_error("Pause before going back");
    if (!timeline_index()) throw std::logic_error("Already at the first recorded moment");
    restore_boundary(timeline_index() - 1);
}
bool Session::advance_recorded() {
    if (history_ && history_->cursor + 1 < history_->frames.size()) {
        restore_boundary(history_->cursor + 1); return true;
    }
    return false;
}
void Session::forward() {
    if (execution_ == ExecutionState::running) throw std::logic_error("Pause before going forward");
    if (!advance_recorded()) (void)step();
}
void Session::seek_seconds(unsigned target) {
    if (execution_ == ExecutionState::running) throw std::logic_error("Pause before moving through time");
    if (target > 180 * 60) throw std::invalid_argument("Choose a match time between 00:00 and 180:00");
    std::size_t closest = 0;
    for (std::size_t index = 0; index < history_->frames.size(); ++index)
        if (history_->frames[index].seconds <= target) closest = index;
    restore_boundary(closest);
    if (target <= recorded_seconds()) {
        if (clock_seconds(inspect()) < target && closest + 1 < timeline_size()) restore_boundary(closest + 1);
        return;
    }
    for (unsigned calls = 0; clock_seconds(inspect()) < target; ++calls) {
        if (execution_ != ExecutionState::paused) break;
        if (calls == 6000) throw std::runtime_error("The match did not reach the selected time within this advance");
        const auto record = step();
        if (record.engine_return != 1) break;
    }
}
void Session::observe_event(MatchEvent value) {
    value.at_step = steps_.size(); events_.push_back(value);
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
    substitutions_.clear(); events_.clear(); cards_.clear(); return_positions_.fill(-1); history_.reset();
    remember_positions();
    failure_.clear();
    execution_ = ExecutionState::paused;
    capture_boundary();
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

Cards Session::cards(unsigned team, unsigned actor) const {
    if (team > 1 || actor >= 20) throw std::invalid_argument("Choose an actual match player");
    const auto address = engine_.match + 0x311c + (team * 20 + actor) * 0x1c4;
    return {unsigned(engine_.state->memory.read<1>(address + 0x24)),
            unsigned(engine_.state->memory.read<1>(address + 0x23))};
}

void Session::remember_positions() {
    for (unsigned index = 0; index < 40; ++index) {
        const auto address = engine_.match + 0x311c + index * 0x1c4;
        const auto position = signed_value<1>(engine_.state->memory.read<1>(address + 0x19));
        if (position >= 0 && position < 11 && engine_.state->memory.read<4>(address + 0x6c))
            return_positions_[index] = std::int8_t(position);
    }
}

CardRecord Session::change_card(unsigned team, unsigned actor, CardKind kind, bool add) {
    if (execution_ != ExecutionState::paused || steps_.empty())
        throw std::logic_error("Pause the match after kickoff to change cards");
    if (team > 1 || actor >= 20 || (kind != CardKind::yellow && kind != CardKind::red))
        throw std::invalid_argument("Choose a player and a yellow or red card");
    auto& state = *engine_.state;
    const auto address = engine_.match + 0x311c + (team * 20 + actor) * 0x1c4;
    const auto staff = std::uint32_t(state.memory.read<4>(address + 0x6c));
    const auto position = signed_value<1>(state.memory.read<1>(address + 0x19));
    const auto before = cards(team, actor);
    if (!staff || state.memory.read<1>(address + 0x27) != team || state.memory.read<4>(address + 0x1a4) != engine_.match)
        throw std::invalid_argument("Choose an actual player in this match");
    if (add && (before.red || position < 0 || position > 10 || inspect().phase != 0))
        throw std::invalid_argument("Choose an on-pitch player during active play to add a card");
    if (!add && !(kind == CardKind::yellow ? before.yellow : before.red))
        throw std::invalid_argument("The selected player does not have that card");
    const bool restore = !add && before.red && (kind == CardKind::red || before.yellow == 2);
    const auto previous_position = return_positions_[team * 20 + actor];
    if (restore && (position != -3 || previous_position < 0 || previous_position > 10 || before.red != 1))
        throw std::invalid_argument("This dismissal cannot be reversed at the current match boundary");
    unsigned displaced = 20, vacancy = 11;
    if (restore) {
        std::array<bool, 11> occupied{};
        for (unsigned other = 0; other < 20; ++other) {
            const auto candidate = engine_.match + 0x311c + (team * 20 + other) * 0x1c4;
            const auto role = signed_value<1>(state.memory.read<1>(candidate + 0x19));
            if (role >= 0 && role < 11) {
                occupied[unsigned(role)] = true;
                if (role == previous_position) displaced = other;
            }
        }
        const auto empty = std::find(occupied.begin(), occupied.end(), false);
        if (empty == occupied.end()) throw std::invalid_argument("There is no vacant position for the returning player");
        vacancy = unsigned(empty - occupied.begin());
    }
    CardRecord record{kind, add, team, actor, std::uint32_t(state.memory.read<4>(staff)), steps_.size(), 0, before, {}};
    const auto instructions = state.executed;
    branch_history();
    try {
        StepBudget budget(state, step_instruction_limit_);
        MatchEventsCapture events(state, engine_.match, true, [&](MatchEvent value) { observe_event(value); });
        const auto first_event = events_.size();
        remember_positions();
        if (add) {
            // An actual opposing actor supplies the original foul-received field.
            const auto opponent = engine_.match + 0x311c + ((1 - team) * 20 + 8) * 0x1c4;
            apply_original_card(state, engine_.match, address, opponent, kind == CardKind::yellow);
        } else {
            // Removal is an explicit debugger edit, not a football rule or a
            // retroactive removal of the underlying foul/rating effects.
            if (kind == CardKind::yellow || (kind == CardKind::red && before.yellow == 2))
                state.memory.write<1>(address + 0x24, before.yellow - 1);
            if (restore) {
                state.memory.write<1>(address + 0x23, 0);
                state.memory.write<1>(address + 0x21, 0);
                if (displaced != 20)
                    (void)invoke(state, 0x101f1870, {engine_.match + 0x311c + (team * 20 + displaced) * 0x1c4, vacancy});
                return_dismissed_player(state, engine_.match, address, unsigned(previous_position));
            }
            if (before.yellow == 2 || (before.red && before.yellow == 1 && kind == CardKind::red))
                state.memory.write<1>(address + 0x26, 0);
            else if (before.red && before.yellow == 1 && kind == CardKind::yellow) {
                state.memory.write<1>(address + 0x25, state.memory.read<1>(address + 0x26));
                state.memory.write<1>(address + 0x26, 0);
            } else state.memory.write<1>(address + 0x25, 0);
        }
        record.instructions = state.executed - instructions; record.after = cards(team, actor);
        cards_.push_back(record);
        MatchEvent event;
        event.kind = add ? (kind == CardKind::yellow ? MatchEvent::Kind::yellow_card : MatchEvent::Kind::red_card) :
            (kind == CardKind::yellow ? MatchEvent::Kind::yellow_removed : MatchEvent::Kind::red_removed);
        event.code = add && record.after.yellow == 2 ? 183 : -1;
        event.seconds = clock_seconds(inspect()); event.team = int(team); event.actor = int(actor);
        event.staff_id = record.staff_id; event.injected = true;
        const auto original_card = add && std::any_of(events_.begin() + first_event, events_.end(), [&](const MatchEvent& original) {
            return original.staff_id == record.staff_id && (original.code == 183 || original.code == 184 || original.code == 185);
        });
        if (!original_card) observe_event(event);
        capture_boundary();
        return record;
    } catch (const std::exception& error) {
        execution_ = ExecutionState::faulted;
        failure_ = "Engine card intervention fault at " + hex_address(state.pc) + ": " + error.what();
        throw;
    } catch (...) {
        execution_ = ExecutionState::faulted; failure_ = "Engine card intervention was interrupted"; throw;
    }
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
    record.injury_before = std::uint8_t(state.memory.read<1>(address + 0x1c));
    record.on_pitch_before = on_pitch(team);
    record.before = inspect();
    const auto instructions = state.executed;
    branch_history();
    try {
        StepBudget budget(state, step_instruction_limit_);
        MatchEventsCapture events(state, engine_.match, true, [&](MatchEvent value) { observe_event(value); });
        remember_positions();
        apply_original_injury(state, engine_.match, address);
        record.instructions = state.executed - instructions;
        record.position_after = std::int8_t(signed_value<1>(state.memory.read<1>(address + 0x19)));
        record.injury_after = std::uint8_t(state.memory.read<1>(address + 0x1c));
        record.on_pitch_after = on_pitch(team);
        record.after = inspect();
        if (record.position_after != -2 && record.position_after != -1)
            throw std::runtime_error("Original injury withdrawal did not mark the selected actor unavailable");
        withdrawals_.push_back(record);
        MatchEvent event; event.kind = MatchEvent::Kind::injury; event.seconds = clock_seconds(record.after);
        event.team = int(team); event.actor = int(actor); event.staff_id = record.staff_id; event.injected = true; observe_event(event);
        capture_boundary();
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
    branch_history();
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
        lineup_edits_.push_back(record); capture_boundary(); return record;
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
    branch_history();
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
        lineup_edits_.push_back(record); capture_boundary(); return record;
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
    branch_history();
    try {
        StepBudget budget(state, step_instruction_limit_);
        MatchEventsCapture events(state, engine_.match, true, [&](MatchEvent value) { observe_event(value); });
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
        MatchEvent event; event.kind = MatchEvent::Kind::goal; event.seconds = clock_seconds(record.after);
        event.team = int(team); event.actor = int(actor); event.staff_id = record.staff_id; event.injected = true; observe_event(event);
        capture_boundary();
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

SubstitutionRecord Session::substitute(unsigned team, unsigned outgoing_actor, std::uint32_t incoming_staff_id) {
    if (execution_ != ExecutionState::paused || steps_.empty()) throw std::logic_error("Pause the match after kickoff to make a substitution");
    if (team > 1 || outgoing_actor >= 20) throw std::invalid_argument("Choose a player to replace");
    auto& state = *engine_.state;
    const auto outgoing = engine_.match + 0x311c + (team * 20 + outgoing_actor) * 0x1c4;
    const auto staff = std::uint32_t(state.memory.read<4>(outgoing + 0x6c));
    const auto current_position = signed_value<1>(state.memory.read<1>(outgoing + 0x19));
    const bool injured = (current_position == -2 || current_position == -1) && state.memory.read<1>(outgoing + 0x1c) != 0;
    const auto position = injured ? return_positions_[team * 20 + outgoing_actor] : current_position;
    if (!staff || position < 0 || position > 10 || (current_position < 0 && !injured))
        throw std::invalid_argument("Choose an on-pitch or injured player to replace");
    const auto& members = roster(team);
    const auto member = std::find_if(members.begin(), members.end(), [&](const auto& value) { return value.staff_id == incoming_staff_id; });
    if (member == members.end()) throw std::invalid_argument("Choose a replacement from this club");
    if (member->staff == staff) throw std::invalid_argument("Choose a different replacement player");
    unsigned incoming_actor = 20, available = 20;
    for (unsigned actor = 0; actor < 20; ++actor) {
        const auto address = engine_.match + 0x311c + (team * 20 + actor) * 0x1c4;
        const auto active_position = signed_value<1>(state.memory.read<1>(address + 0x19));
        if (actor != outgoing_actor && active_position < 0 && available == 20) available = actor;
        if (state.memory.read<4>(address + 0x6c) == member->staff) {
            if (active_position >= 0) throw std::invalid_argument("This replacement is already on the pitch");
            incoming_actor = actor;
        }
    }
    const bool initialize = incoming_actor == 20;
    if (initialize) incoming_actor = available;
    if (incoming_actor == 20) throw std::invalid_argument("No substitute slot is available");
    SubstitutionRecord record;
    record.number = substitutions_.size() + 1; record.at_step = steps_.size(); record.team = team;
    record.outgoing_actor = outgoing_actor; record.incoming_actor = incoming_actor;
    record.outgoing_staff_id = std::uint32_t(state.memory.read<4>(staff)); record.incoming_staff_id = incoming_staff_id;
    record.position = std::int8_t(position); record.before = inspect();
    const auto before = state.executed;
    const auto pitch_before = on_pitch(team);
    branch_history();
    try {
        StepBudget budget(state, step_instruction_limit_);
        MatchEventsCapture events(state, engine_.match, true, [&](MatchEvent value) { observe_event(value); });
        if (initialize && invoke(state, 0x1020e8f0, {engine_.match + 0x7aac + team * 0xd8, incoming_actor, member->staff, incoming_actor}) != 1)
            throw std::runtime_error("The replacement player could not be prepared");
        const auto incoming = engine_.match + 0x311c + (team * 20 + incoming_actor) * 0x1c4;
        if (initialize && signed_value<1>(state.memory.read<1>(incoming + 0x19)) >= 0)
            (void)invoke(state, 0x101f1870, {incoming, std::uint32_t(-1)});
        if (injured) {
            std::array<bool, 11> occupied{};
            unsigned reassigned = 20;
            for (unsigned actor = 0; actor < 20; ++actor) {
                const auto address = engine_.match + 0x311c + (team * 20 + actor) * 0x1c4;
                const auto current = signed_value<1>(state.memory.read<1>(address + 0x19));
                if (current >= 0 && current < 11) {
                    occupied[unsigned(current)] = true;
                    if (current == position) reassigned = actor;
                }
            }
            if (reassigned != 20) {
                const auto vacant = std::find(occupied.begin(), occupied.end(), false);
                if (vacant == occupied.end()) throw std::runtime_error("No vacant position remains for this injury replacement");
                (void)invoke(state, 0x101f1870, {engine_.match + 0x311c + (team * 20 + reassigned) * 0x1c4,
                    std::uint32_t(vacant - occupied.begin())});
            }
            // Restore only the outgoing role for the original substitution's
            // boundary; its injury data remains intact and it finishes off pitch.
            (void)invoke(state, 0x101f1870, {outgoing, std::uint32_t(position)});
        }
        // This complete original in-match substitution method only reads the
        // owning-match pointer and side from its presentation caller. Supply
        // those two fields in a reserved guest stack context. The original
        // body performs pitch/marking changes, times, counters and commentary.
        const auto parent = std::uint32_t(state.r<4>(4));
        if (parent < 0x1100) throw std::runtime_error("Invalid substitution caller stack");
        const auto context = parent - 0x1000;
        state.memory.write<4>(context + 4, engine_.match); state.memory.write<1>(context + 0xe68, team);
        // Unlimited debugger substitutions still require a valid original
        // remaining-substitution counter. The UI normally blocks at zero;
        // blindly bypassing that UI would underflow it and confuse automatic
        // injury/tactical handling. Grant one allowance only when exhausted.
        const auto allowance = engine_.match + 0x7aac + team * 0xd8 + 0x19;
        if (signed_value<1>(state.memory.read<1>(allowance)) <= 0) state.memory.write<1>(allowance, 1);
        if (invoke(state, 0x103cb760, {context, incoming_actor, outgoing_actor}) != 1)
            throw std::runtime_error("The original substitution did not complete");
        if (signed_value<1>(state.memory.read<1>(incoming + 0x19)) != position ||
            signed_value<1>(state.memory.read<1>(outgoing + 0x19)) != -4 || on_pitch(team) != pitch_before + unsigned(injured))
            throw std::runtime_error("The original substitution did not retain the selected lineup");
        (void)invoke(state, 0x101f1de0, {engine_.match, team});
        (void)invoke(state, 0x101f1e10, {engine_.match, team});
        record.instructions = state.executed - before; record.after = inspect();
        substitutions_.push_back(record);
        MatchEvent event; event.kind = MatchEvent::Kind::substitution; event.seconds = clock_seconds(record.after);
        event.team = event.other_team = int(team); event.actor = int(incoming_actor); event.other_actor = int(outgoing_actor);
        event.staff_id = incoming_staff_id; event.other_staff_id = record.outgoing_staff_id; event.injected = true; observe_event(event);
        capture_boundary(); return record;
    } catch (const std::exception& error) {
        execution_ = ExecutionState::faulted; failure_ = "Substitution could not complete: " + std::string(error.what()); throw;
    }
}

StepRecord Session::execute_step() {
    auto& state = *engine_.state;
    branch_history();
    try {
        StepRecord record;
        record.number = steps_.size() + 1;
        record.before = inspect();
        remember_positions();
        const auto first_event = events_.size();
        const auto instructions = state.executed;
        StepBudget budget(state, step_instruction_limit_);
        MatchEventsCapture events(state, engine_.match, false, [&](MatchEvent value) { observe_event(value); });
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
        if (steps_.size() == 1 || record.engine_return == 2) {
            MatchEvent event; event.kind = record.engine_return == 2 ? MatchEvent::Kind::completed : MatchEvent::Kind::kickoff;
            event.seconds = clock_seconds(record.engine_return == 2 ? record.after : record.before);
            if (event.kind == MatchEvent::Kind::kickoff) events_.insert(events_.begin() + first_event, event);
            else observe_event(event);
        }
        capture_boundary();
        return record;
    } catch (const std::exception& error) {
        // An interrupted guest step may have partial effects. Retain them for
        // diagnosis, block further advancement until a recorded boundary is restored.
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
