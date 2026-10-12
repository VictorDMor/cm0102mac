#pragma once

#include "aot_runtime.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace cm::aot::match_debugger {

struct RosterMember {
    std::uint32_t staff_id = 0, staff = 0, player = 0;
    bool operator==(const RosterMember&) const = default;
};

// The session factory must construct an independent, initialized real match.
// Database loading and club/lineup selection belong to that factory.
struct EngineInstance {
    std::unique_ptr<State> state;
    std::uint32_t match = 0;
    std::array<std::vector<RosterMember>, 2> rosters{};
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
    std::uint8_t injury_before = 0, injury_after = 0;
    unsigned on_pitch_before = 0, on_pitch_after = 0;
    Inspection before, after;
};

struct GoalRecord {
    std::uint64_t number = 0, at_step = 0, instructions = 0;
    std::uint32_t team = 0, actor = 0, staff_id = 0;
    std::uint8_t scorer_goals_before = 0, scorer_goals_after = 0;
    Inspection before, after;
};

struct LineupRecord {
    enum class Kind { player, position } kind = Kind::player;
    std::uint64_t number = 0, instructions = 0;
    unsigned team = 0, actor = 0, displaced_actor = 20;
    std::uint32_t staff_before = 0, staff_after = 0, displaced_staff = 0;
    std::int8_t position_before = -5, position_after = -5;
    std::int8_t displaced_before = -5, displaced_after = -5;
    unsigned on_pitch_before = 0, on_pitch_after = 0;
};

struct MatchEvent {
    enum class Kind { commentary, kickoff, goal, injury, substitution, completed,
        yellow_card, red_card, yellow_removed, red_removed } kind = Kind::commentary;
    std::uint64_t at_step = 0;
    unsigned seconds = 0;
    int code = -1, team = -1, actor = -1, other_team = -1, other_actor = -1;
    std::uint32_t staff_id = 0, other_staff_id = 0;
    bool injected = false;
};

enum class CardKind { yellow, red };
struct Cards {
    unsigned yellow = 0, red = 0;
    bool operator==(const Cards&) const = default;
};
struct CardRecord {
    CardKind kind = CardKind::yellow;
    bool added = false;
    unsigned team = 0, actor = 0;
    std::uint32_t staff_id = 0;
    std::uint64_t at_step = 0, instructions = 0;
    Cards before, after;
};

struct SubstitutionRecord {
    std::uint64_t number = 0, at_step = 0, instructions = 0;
    unsigned team = 0, outgoing_actor = 0, incoming_actor = 0;
    std::uint32_t outgoing_staff_id = 0, incoming_staff_id = 0;
    std::int8_t position = -1;
    Inspection before, after;
};

// The original clock has eleven engine units per minute. Display only its
// actual granularity; the UI does not add a separate simulated match clock.
inline unsigned clock_seconds(const Inspection& value) {
    return value.elapsed_engine_units > 0 ? unsigned(value.elapsed_engine_units) * 60 / 11 : 0;
}

// One owner drives this session. A UI scheduler invokes advance_if_running()
// between UI events; it must not concurrently access a running guest State.
// The engine call uses the same dispatch as the production game.
class Session {
public:
    using Factory = std::function<EngineInstance()>;
    explicit Session(Factory, std::uint64_t step_instruction_limit = 1000000);
    ~Session();

    ExecutionState execution_state() const { return execution_; }
    const std::string& failure() const { return failure_; }
    Inspection inspect() const;
    const std::vector<StepRecord>& steps() const { return steps_; }
    const std::vector<WithdrawalRecord>& withdrawals() const { return withdrawals_; }
    const std::vector<GoalRecord>& goals() const { return goals_; }
    const std::vector<LineupRecord>& lineup_edits() const { return lineup_edits_; }
    const std::vector<SubstitutionRecord>& substitutions() const { return substitutions_; }
    const std::vector<MatchEvent>& events() const { return events_; }
    const std::vector<CardRecord>& card_edits() const { return cards_; }
    Cards cards(unsigned team, unsigned actor) const;
    const std::vector<RosterMember>& roster(unsigned team) const;
    const State& state() const { return *engine_.state; }
    std::uint32_t match_address() const { return engine_.match; }

    void run();
    void pause();
    StepRecord step();
    std::optional<StepRecord> advance_if_running();
    void reset();
    void back();
    void forward();
    void seek_seconds(unsigned);
    std::size_t timeline_index() const;
    std::size_t timeline_size() const;
    unsigned recorded_seconds() const;
    unsigned on_pitch(unsigned team) const;
    // Process original pending injury flags at a paused boundary after kickoff.
    WithdrawalRecord withdraw_injured_player(unsigned team, unsigned actor);
    CardRecord change_card(unsigned team, unsigned actor, CardKind, bool add);
    // Force an awarded goal through the characterized original bookkeeping
    // regions. It updates the scorer, team, commentary and kickoff.
    GoalRecord add_goal(unsigned team, unsigned actor);
    LineupRecord set_player(unsigned team, unsigned actor, std::uint32_t staff_id);
    LineupRecord set_position(unsigned team, unsigned actor, int position);
    SubstitutionRecord substitute(unsigned team, unsigned outgoing_actor, std::uint32_t incoming_staff_id);

private:
    Factory factory_;
    EngineInstance engine_;
    std::uint64_t step_instruction_limit_;
    ExecutionState execution_ = ExecutionState::paused;
    std::string failure_;
    std::vector<StepRecord> steps_;
    std::vector<WithdrawalRecord> withdrawals_;
    std::vector<GoalRecord> goals_;
    std::vector<LineupRecord> lineup_edits_;
    std::vector<SubstitutionRecord> substitutions_;
    std::vector<MatchEvent> events_;
    std::vector<CardRecord> cards_;
    std::array<std::int8_t, 40> return_positions_{};
    struct History;
    std::unique_ptr<History> history_;

    void capture_boundary();
    void branch_history();
    void restore_boundary(std::size_t);
    bool advance_recorded();
    void observe_event(MatchEvent);
    void remember_positions();

    StepRecord execute_step();
    void require_pre_kickoff_edit() const;
    void validate_kickoff_lineups() const;
    static Inspection inspect(const EngineInstance&);
};

}
