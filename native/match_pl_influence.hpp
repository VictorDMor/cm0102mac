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
    void derived_attribute(unsigned profile_offset, unsigned player_offset, std::uint32_t multiply_site, unsigned penalty_weight = 0);
    void floor_attribute(unsigned player_offset);
    void role_attribute_scale(unsigned player_offset, std::uint32_t first_back, std::uint32_t second_back);
    void soften_attribute(unsigned player_offset, unsigned threshold, unsigned exponent, std::uint32_t power_back, bool club_gate = false);
public:
    PlayerInfluence(guest::Cpu&, std::uint32_t player);
    void run();
};
}
