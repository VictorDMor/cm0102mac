#pragma once
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {

enum class MovementPhase { returned, carrier, noncarrier };

class MovementFrame {
public:
    explicit MovementFrame(Cpu& cpu)
        : c(cpu), caller(c.sp()), scope(c.s, 0x1022e7e0) {
        c.set(0, c.lr());
        c.put64(caller - 8, c.s.r<8>(guest::reg::fpr(31)));
        c.put64(caller - 0x10, c.s.r<8>(guest::reg::fpr(30)));
        for (unsigned n = 13; n < 32; ++n) c.put32(saved(n), c.r(n));
        c.s.wr<4>(2432, saved(32));
    }

    void enter() {
        c.put32(caller + 8, c.r(0));
        c.set(0, 0);
        c.put32(at(0), caller);
        c.set(1, at(0));
        scope.frame(at(0));
    }

    ~MovementFrame() {
        if (std::uncaught_exceptions() != exceptions) return;
        c.set(0, c.u32(caller + 8));
        c.set(1, caller);
        c.s.wr<8>(guest::reg::fpr(31), c.u64(caller - 8));
        c.s.wr<8>(guest::reg::fpr(30), c.u64(caller - 0x10));
        c.set_lr(c.r(0));
        for (unsigned n = 13; n < 32; ++n) c.set(n, c.u32(saved(n)));
        c.s.wr<4>(2432, saved(32));
    }

    std::uint32_t at(std::int32_t offset) const { return caller - 0x180 + std::uint32_t(offset); }

private:
    std::uint32_t saved(unsigned n) const { return caller - 0x5c + (n - 13) * 4; }
    Cpu& c;
    const std::uint32_t caller;
    GuestNativeScope scope;
    const int exceptions = std::uncaught_exceptions();
};

// The typed activation keeps the incoming frame alive across movement phases.
class PlayerMovement {
    Cpu& c;
    MovementFrame frame;
    guest::Fpu fp;
    Player actor;

    void initialize_cell();
    bool stationary_request();
    void refill_pool(std::uint32_t back, unsigned first, unsigned second, unsigned second_field);
    std::uint32_t movement_draw(std::int32_t magic, unsigned shift, unsigned divisor, bool mixed,
                                unsigned first, unsigned second);
    bool next_neighbour(unsigned index);
    void react_neighbours(std::uint32_t query_back, std::uint32_t refill_back, std::uint32_t response_back,
                          unsigned index = 13, unsigned first = 14, unsigned second = 16);
    bool carries_ball(std::uint32_t back);
    bool turns_in_place();
    void approach_axis(unsigned coordinate, unsigned target);
    void approach_row();
    bool changed_cell();
    std::uint32_t signed_side() const;
    std::uint32_t cell_index() const;
    void remove_cell_occupant();
    void insert_cell_occupant();
    void migrate_cell();
    bool update_defensive_extent(unsigned field, int empty_row, bool first_side);
    void publish_defensive_cell();
    void select_surface_pace();
    void apply_pace_coefficient(unsigned divisor, unsigned coefficient);
    void measure_move_pace();
    void query_pace_tactics(std::uint32_t back);
    bool pace_needs_tactics();
    void describe_capture(unsigned message, std::uint32_t back);
    void acquire_and_reward(unsigned points, std::uint32_t back);
    bool exceptional_goal_capture();
    bool loose_ball_in_cell();
    void capture_unowned_ball();
    int read_ball_coordinate(unsigned field);
    void synchronize_ball();
    static std::int32_t focus_high(std::uint32_t value);
    void face_ball(bool far_goal, std::uint32_t back);
    bool same_ball_side_or_target();
    void face_movement_context();
    static std::uint32_t high_product(std::uint32_t value, std::uint32_t multiplier);
    std::uint32_t shift_signed(std::uint32_t word, unsigned shift);
    static std::uint32_t toward_zero(std::uint32_t shifted);
    std::uint32_t condition_deficit(std::uint32_t word);
    void refill_carrier_pool(std::uint32_t back);
    void store_control_pair(unsigned first, unsigned second);
    std::uint32_t control_interval(unsigned first, unsigned second, unsigned first_register, unsigned multiplier);
    void sample_narrow_control();
    void sample_carrier_control();
    void accumulate_movement_bias();
    void sample_wide_action(std::int32_t bound, std::uint32_t back, unsigned destination = 21);
    void sample_action_control(unsigned field, std::uint32_t reseed, std::uint32_t wide_back);
    void apply_carrier_surface();
    std::uint32_t carrier_ball_bound() const;
    void sample_wide_ball_state(std::uint32_t bound);
    void sample_narrow_ball_state(std::uint32_t bound);
    void sample_carrier_ball_state();
    void sample_carrier_surface_condition();
    void query_carrier_neighbour();
    bool carrier_commentary_allowed();
    void commentary_strength(std::uint32_t back, unsigned spill, unsigned destination);
    void narrate_carrier_position(unsigned message, std::uint32_t back);
    bool carrier_flank(bool left);
    bool carrier_forward();
    bool carrier_backward();
    void narrate_carrier_centre();
    void narrate_carrier_zones();
    bool contact_draw_is_zero(std::uint32_t back, std::uint32_t magic, unsigned shift, unsigned divisor);
    void describe_contact();
    bool evaluate_carrier_contact();
    bool marked_contact_goal_area(std::uint32_t player, std::uint32_t back);
    bool contact_requires_warning();
    void describe_carrier_warning();
    bool contact_frame_byte(unsigned slot);
    bool resolve_contact_directly();
    void publish_contact_outcome(bool failed);
    void queue_contact_decision();
    void narrow_possession_draw(std::int32_t bound, unsigned destination);
    void sample_possession_failure();
    bool should_keep_possession();
    void release_ball();
    void react_to_possession_release();
    void adjust_play_condition();
    void count_team_play();
    void clamp_play_bias(std::uint32_t player);
    void play_bias_factor(unsigned spill, std::int32_t difference, std::uint32_t site);
    void apply_play_bias();
    void record_play_totals(unsigned amount);
    void credit_player_contact(unsigned condition);
    void react_to_play_success();
    void credit_carrier_play();
    std::uint32_t contact_direction_target(std::int16_t table_slot, std::uint32_t default_target);
    bool narrate_contact_direction(std::uint32_t target, std::uint32_t first_target);
    void select_contact_result();
    void narrate_random_contact();
    void set_match_contact_state(unsigned status, unsigned message);
    void apply_major_contact();
    bool report_staff_eligible(std::uint32_t player);
    bool report_contact_eligible();
    bool report_contact_attribute(unsigned field, std::uint32_t back);
    bool report_contact_severity();
    bool report_contact_rating();
    void append_contact_report();
    void consider_contact_report();
    void mark_followup_direction();
    void narrate_followup_contact(std::uint32_t message, std::uint32_t back, bool for_actor, bool shirt = false);
    std::uint32_t opponent_contact_club();
    void narrate_contact_interest();
    void narrate_restart(std::uint32_t message, std::uint32_t back, unsigned player_register, bool opponent);
    bool restart_qualified(std::uint32_t back);
    void choose_restart_recipient();
    bool restart_comment_expected();
    bool even_restart_sample(std::uint32_t back);
    bool zero_restart_sample(std::uint32_t back, unsigned divisor, std::uint32_t multiplier);
    void narrate_flagged_restart();
    void narrate_unflagged_restart();
    bool play_rule(std::uint32_t target, std::uint32_t back);
    void narrate_opposed_play(std::uint32_t message, std::uint32_t back);
    bool opposed_skill_is_distinct();
    void sample_opposed_skill(std::uint32_t delta);
    void describe_carrier_opposition(std::uint32_t delta);
    bool opposed_space_is_open(std::uint32_t back);
    bool opponent_space_flag();
    bool crosses_zone_boundary(bool target, std::uint32_t back);
    bool play_column_allowed(std::uint32_t back);
    bool crosses_play_row(bool goal);
    bool moved_play_axis(bool column, bool left);
    bool attached_player_is_stationary();
    bool narrate_stationary_attachment(std::uint32_t delta);
    void narrate_clear_fallback(std::uint32_t delta);
    void complete_clear_play(std::uint32_t delta);
    bool repeated_contact_warns();
    void narrate_repeated_pair(std::uint32_t message, std::uint32_t back);
    std::uint32_t narrate_repeat_warning(std::uint32_t target);
    std::uint32_t repeated_contact_fallback();
    void penalize_transfer_contact(unsigned amount);
    void react_to_transfer_contact();
    bool transfer_statistics_required();
    void choose_distance_samples();
    void distance_sample_bound();
    void sample_contact_distance();
    void position_after_transfer();
    void compute_transfer_ratio(bool shot);
    bool transfer_commentary_due(bool shot);
    void narrate_transfer_statistics(bool shot);
    bool transfer_role(std::uint32_t bit, std::uint32_t back);
    void choose_transfer_rating();
    bool credit_transfer_rating();
    void reward_transfer_rating();
    void finish_transfer_rating();
    void report_movement_diagnostic(unsigned first, unsigned second, unsigned code, std::uint32_t back);

public:
    explicit PlayerMovement(Cpu& cpu);
    MovementPhase prepare();
    void move_without_ball();
    void consume_move_condition();
    void finish_without_ball();
    void sample_carrier();
    void sample_carrier_environment();
    void prepare_carrier_interaction();
    bool decide_carrier_contact();
    void resume_carrier_contact_outcome(bool failed);
    bool finish_carrier_possession();
    bool prepare_carrier_play();
    std::uint32_t choose_carrier_play();
    std::uint32_t choose_contact_action(std::uint32_t target = 0x102303fc);
    void follow_up_carrier_contact();
    void apply_carrier_restart();
    void continue_simple_play(std::uint32_t phase);
    bool continue_kind17_contact();
    void continue_kind17_clear();
    void continue_kind18();
    std::uint32_t continue_repeated_contact(std::uint32_t phase = 0x102338f8);
    std::uint32_t continue_carrier_transfer();
    void continue_carrier_distance(std::uint32_t phase);
    void continue_transfer_statistics();
    void continue_transfer_rating();
    void continue_null_contact();
};
}
