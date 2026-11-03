#pragma once
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {

// Guest activations for the two goal-line actions. Opposite selects
// 0x1026a1b0; the other orientation selects 0x1026d660.
class GoalLineAction {
    guest::Cpu& c;
    const bool opposite;
    guest::Frame frame;
    guest::Fpu fp;
    Player actor;

    unsigned first_register() const { return opposite ? 18 : 17; }
    unsigned clock_register() const { return opposite ? 24 : 23; }
    unsigned end_register() const { return opposite ? 21 : 20; }
    unsigned begin_register() const { return opposite ? 22 : 21; }
    unsigned floats_register() const { return opposite ? 19 : 18; }
    unsigned strings_register() const { return opposite ? 20 : 19; }
    std::uint32_t resume(std::uint32_t upper, std::uint32_t lower) const { return opposite ? upper : lower; }

    struct KeeperMessages {
        std::uint32_t strong_message, strong_back;
        std::uint32_t first_offer_back, first_message_back;
        std::uint32_t mild_message, mild_back, default_message, default_back;
    };

    void replace_seed(std::uint32_t back);
    std::int32_t divide_pool(std::uint32_t word, unsigned shift);
    void initial_draws();
    void threshold_and_keeper();
    void missing_keeper();
    void sample_keeper();
    bool phase_flag();
    bool phase_random(std::uint32_t back);
    void choose_event();
    void tally_event();
    void announce(std::uint32_t message, std::uint32_t narrator, bool flip_side, std::uint32_t back, bool identify = true, std::uint32_t detail = 0);
    bool keeper_role(std::uint32_t back);
    bool stack_byte(unsigned offset);
    void refill_pool(std::uint32_t back);
    void modify_halfword(std::uint32_t address, int amount, unsigned input_register = 3);
    std::int32_t draw_constant(std::uint32_t back, unsigned bound, std::int32_t magic, unsigned shift);
    bool draw_constant_zero(std::uint32_t back, unsigned bound, std::int32_t magic, unsigned shift);
    bool score_matches(int lead);
    void through_tie_totals();
    void run_after_tie_totals();

    void announce_non_goal(int event);
    void keeper_recovery(std::uint32_t refill_back, std::uint32_t recovery_back, unsigned extra = 0);
    void finish_near_goal();
    void keeper_contact(bool stronger);
    void explain_keeper(const KeeperMessages& sequence);
    void queue_goal_line_restart();
    void finish_close_save();
    void finish_over_line();
    void finish_keeper_possession();
    bool finish_non_goal();
    void prepare_goal_effects();
    bool finish_saved_goal();

    bool draw_keeper_pressure(std::int32_t bound, std::uint32_t back);
    bool draw_keeper_trait(unsigned field, std::uint32_t back);
    void refresh_keeper_after_goal();
    void finish_pending_restart(std::uint32_t back);
    void prepare_awarded_goal();
    bool review_initial_goal();
    bool pending_restart(std::uint32_t back);
    bool review_later_goal();

    bool actor_halfword_is(unsigned field, int value);
    bool milestone_multiple(unsigned field, int minimum, unsigned divisor, std::int32_t magic, unsigned shift);
    bool distinct_counts(unsigned first, unsigned second);
    void announce_milestone(unsigned message, unsigned field, std::uint32_t back);
    void announce_scorer_milestone();
    std::int32_t match_minute(std::int16_t ticks);
    std::int32_t read_match_minute();
    std::int32_t score_margin();
    bool late_match(int normal, int extended);
    void adjust_team(unsigned coefficient, std::uint32_t back);
    bool terminal_minutes(std::int32_t minute);
    void finish_level_score();
    bool trailing_comment();
    bool confirm_goal();
    void apply_confirmed_goal_to_team();

    std::uint32_t live_goal_club();
    bool goal_club_uses_comment();
    bool announce_goal_celebration();
    void announce_late_goal();
    void finish_confirmed_goal();

public:
    GoalLineAction(guest::Cpu& cpu, bool opposite_goal);
    void run();
};
}
