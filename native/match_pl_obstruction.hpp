#pragma once
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {

// Guest activation for 0x10272240. R3 carries the obstruction verdict.
class PlayerObstruction {
    guest::Cpu& c;
    guest::Frame frame;
    guest::Fpu fp;
    Player actor;

    bool prepare();
    bool finish_protected_side();
    bool in_goal_region(bool away, unsigned scratch, const Player& candidate);
    bool role_is_keeper(std::uint32_t back);
    void select_attribute();
    void load_attribute(unsigned index);
    std::uint32_t draw_word();
    bool finish_insufficient_threshold();
    bool set_up_contest();
    std::uint32_t fixed_remainder(std::uint32_t word, unsigned bound, std::int32_t magic, unsigned shift);
    void announce_actor(std::uint32_t message, std::uint32_t detail, std::uint32_t back);
    void announce_opposition(std::uint32_t message, std::uint32_t back);
    void choose_restart_message();
    void publish_restart();
    void finish_restart();
    bool draw_zero(std::uint32_t refill_back, unsigned bound, std::int32_t magic, unsigned shift);
    std::uint32_t other_club();
    void announce_restart_reason();
    void announce_neutral(std::uint32_t message, std::uint32_t side, std::uint32_t index, std::uint32_t back);
    void announce_receiver(std::uint32_t message, std::uint32_t back);
    bool receiver_beyond_midfield(const Player& receiver);
    void announce_restart_player();
    void announce_possible_review();
    bool goal_line_center();
    void add_actor_byte(unsigned field, std::uint32_t amount);
    void add_condition(unsigned amount);
    void queue_disruption(std::uint32_t queued_actor, std::uint32_t refill_back, std::uint32_t queue_back);
    void finish_light_disruption();
    void finish_strong_disruption();
    static std::uint32_t narrow_remainder(std::uint32_t word, std::int32_t bound);
    bool reject();
    bool invalid_phase();
    void refill(std::uint32_t back);
    void first_draw(unsigned bound, unsigned shift, std::int32_t magic, std::uint32_t back);
    void scale();

public:
    explicit PlayerObstruction(guest::Cpu& cpu);
    void run();
};
}
