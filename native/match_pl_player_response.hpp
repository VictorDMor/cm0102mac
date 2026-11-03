#pragma once
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {

// Guest activation for 0x1022ba10, including the final attribute refresh.
class PlayerResponse {
    guest::Cpu& c;
    guest::Frame frame;
    guest::Fpu fp;
    Player actor;
    const std::uint32_t doubles;

    std::uint32_t at(unsigned offset) const { return frame.at(offset); }
    bool role_has(std::uint32_t back, unsigned mask, bool secondary = false);
    void reputation();
    void primary_position();
    void primary_specialty();
    void secondary_position();
    void secondary_specialty();
    void publish_initial_ratings();
    void publish_remaining_bytes();
    void publish_reputation_composite();
    void publish_physical_composites();
    void publish_composite_attributes();
    void normalize_attributes();
    void publish_simple_rating(unsigned field, unsigned previous_field, unsigned output_slot,
        unsigned compare_slot, unsigned recalculate_slot, unsigned coefficient = 0x84);
    void subtract_scaled(unsigned field, unsigned multiplier, bool count_side = false);
    void subtract_scaled_gap();
    void subtract_adjacent(unsigned multiplier);
    void subtract_wide(std::uint32_t site, unsigned field);
    void subtract_specialty(unsigned field);
    void subtract_hybrid_specialty(unsigned half_field);
    void increment_side_counter(unsigned amount);

public:
    PlayerResponse(guest::Cpu&, std::uint32_t player, std::uint32_t count_side);
    void run();
};
}
