#pragma once
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {

// MATCH_PL's verdict for one occupant in the match engine's cell scan.
class CellPressure {
    guest::Cpu& c;
    guest::Frame frame;
    guest::Fpu fp;
    Player actor;

    enum class PressureContinuation {
        unsettled_contact,
        penalty_restart,
        outside_goal_region,
        goal_region
    };

    void refill(std::uint32_t back);
    void refill_after_eligibility_read();
    bool reject();
    bool eligible();
    bool keeper_in_region();
    bool inside_goal_region(bool away, unsigned coordinate, unsigned result);
    bool inside_goal_region(Player subject, bool away, unsigned coordinate, unsigned result);
    std::int32_t shifted_product(std::uint32_t value, std::int32_t magic, unsigned shift);
    void keeper_wide_draw(std::int32_t bound);
    void keeper_threshold();
    void ordinary_draw(std::uint32_t back, unsigned divisor, std::uint32_t magic, unsigned shift);
    void expansive_draw();
    bool invalid_mode();
    bool ordinary_threshold();
    void rescale_threshold();
    bool side_already_in_possession();
    bool restart_chance();
    void restart_with_pressure();
    bool role_is_keeper(std::uint32_t back);
    bool pressure_attribute_draw();
    void load_pressure_attribute(unsigned index);
    void select_pressure_attribute();
    void sample_pressure_attribute();
    void adjust_contact_count(std::uint32_t back, unsigned verdict);
    bool pressure_exceeds_threshold();
    void announce_actor(unsigned message, std::uint32_t back);
    bool has_sanction();
    bool defer_sanction();
    void prepare_sanction_comment();
    void announce_goal_region_contact();
    void announce_other_side(unsigned message, std::uint32_t back);
    void draw_signed_byte_into_r19(std::uint32_t back);
    void apply_goal_region_sanction();
    bool sanction_comment_lottery();
    std::uint32_t opposing_club();
    bool sanction_club_comment();
    void announce_sanction_club();
    bool sanction_updates_lineup();
    bool find_restart_recipient();
    void announce_recipient(unsigned message, std::uint32_t back);
    bool recipient_in_attacking_band(std::uint32_t back);
    void announce_restart_recipient();
    bool sanction_draw_zero(std::uint32_t back, unsigned divisor, std::int32_t magic, unsigned shift);
    void announce_unidentified_actor(unsigned message, std::uint32_t back);
    void announce_sanction_effects();
    bool wins_unsettled_contact();
    void credit_contact(unsigned points);
    bool keeper_defends_goal(std::uint32_t back);
    void scatter_after_contact(std::uint32_t draw_back, std::uint32_t scatter_back, bool confined, bool send_keeper);
    void deflect_unsettled_contact();
    void measure_recovery_rate(unsigned attempts, unsigned successes);
    bool recovery_announced(bool goalward);
    void announce_recovery(bool goalward);
    void win_unsettled_contact();
    bool prepare();
    bool prepare_attribute();
    bool evaluate_pressure();
    PressureContinuation route_sanction();
    bool publish_sanction(PressureContinuation route);
    void announce_published_sanction();
    void finish_published_sanction();
    void finish_unsettled_contact();

public:
    explicit CellPressure(guest::Cpu& cpu);
    void run();
};
}
