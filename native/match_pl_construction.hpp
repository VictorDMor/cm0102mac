#pragma once
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
// One activation preserves the constructor frame across its external services.
class PlayerConstruction {
    guest::Cpu& c;
    guest::Frame frame;
    guest::Fpu fp;
    Player actor;
    const std::uint32_t staff, side_argument, slot;
    const std::uint32_t doubles, floats, seed, begin, end, cursor, staff_count;
    std::uint32_t at(unsigned offset) const { return frame.at(offset); }
    void advance(std::uint32_t back);
    std::uint32_t fallback_index(std::uint32_t count);
    void choose_record();
    void copy_attributes_and_clubs();
    void initialize_fitness();
    void initialize_affiliation();
    bool opponent_lists_club();
    bool eligibility_requires_history();
    bool competition_history_available();
    void unknown_eligibility();
    void special_club_eligibility();
    void ordinary_eligibility();
    void initialize_eligibility();
    void copy_preferences_and_names();
    void inspect_special_club_context();
    void initialize_random_preferences();
    std::uint32_t narrow_preference(std::int32_t bound, std::uint32_t refill_back);
    bool prepare_personality_factors();
    void affiliation_factor();
    void fixture_reputation();
    void repair_competition_descriptor();
    std::uint32_t personality_bound();
    std::uint32_t variable_draw(std::uint32_t bound, std::uint32_t refill_back, std::uint32_t rounding_back);
    std::uint32_t constant_draw(std::uint32_t refill_back, std::int32_t multiplier,
                                unsigned shift_bits, std::int32_t divisor, bool negate = false);
    void probability_threshold(std::uint32_t refill_back, std::uint32_t rounding_back,
                               unsigned coefficient, unsigned rounding_bound, int limit);
    void choose_personality_band();
    void apply_personality_attributes();
    bool apply_coach_influence();
    void apply_fallback_influence();
    bool own_competition_has_history();
    void apply_standings_pressure();
    void apply_club_history_pressure();
    void apply_goalkeeper_context();
    void apply_staff_context();
    void finish_attributes();
public:
    PlayerConstruction(guest::Cpu&, std::uint32_t player, std::uint32_t match,
                       std::uint32_t staff, std::uint32_t side,
                       std::uint32_t initial, std::uint32_t slot);
    void run();
};
}
