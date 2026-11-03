#pragma once
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
// Draft activation: the captain phase is checked before the remaining team
// influence phases are implemented. No production entry is registered yet.
class PlayerInfluence {
    guest::Cpu& c;
    guest::Frame frame;
    guest::Fpu fp;
    Player actor;
    const std::uint32_t floats, doubles;
    std::uint32_t at(unsigned offset) const { return frame.at(offset); }
    void select_captain(unsigned side_register, unsigned match_register, std::uint32_t back);
    void captain_effects();
public:
    PlayerInfluence(guest::Cpu&, std::uint32_t player);
    void run();
};
}
