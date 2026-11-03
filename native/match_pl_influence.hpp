#pragma once
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
// Draft activation: completed phases are checked at external service boundaries
// while the attribute suffix is reconstructed. No production entry is registered.
class PlayerInfluence {
    guest::Cpu& c;
    guest::Frame frame;
    guest::Fpu fp;
    Player actor;
    const std::uint32_t floats, doubles;
    std::uint32_t at(unsigned offset) const { return frame.at(offset); }
    void select_captain(unsigned side_register, unsigned match_register, std::uint32_t back);
    void captain_effects();
    void contract_and_morale();
    void fixture_pressure();
    void initial_ability();
    void ability_penalty();
    void experience_penalty();
    void limit_ability();
    void publish_ability();
    void coaching_effects();
    void technical_attributes();
    void positional_attribute();
    void leadership_attribute();
    void late_ratings();
    void profile_ratings();
    std::int8_t fixture_rating(std::int8_t rating, std::int8_t kind, bool secondary);
    void publish_fixture_rating(std::int8_t rating, unsigned offset, std::uint32_t back);
    void base_profile_ratings();
    void age_and_fixture_ratings();
    void aged_profile_rating(unsigned profile_offset, unsigned result_register);
    void weighted_coaching(unsigned player_offset, unsigned coefficient_register, unsigned base_offset, unsigned fallback_offset,
        std::uint32_t coefficient_site, std::uint32_t rating_back, std::uint32_t adjustment_site);
    void derived_attribute(unsigned profile_offset, unsigned player_offset, std::uint32_t multiply_site, unsigned penalty_weight = 0, bool subtract_penalty = true);
    void floor_attribute(unsigned player_offset);
    void role_attribute_scale(unsigned player_offset, std::uint32_t first_back, std::uint32_t second_back);
    void soften_attribute(unsigned player_offset, unsigned threshold, unsigned exponent, std::uint32_t power_back, bool club_gate = false);
public:
    PlayerInfluence(guest::Cpu&, std::uint32_t player);
    void run();
};
}
