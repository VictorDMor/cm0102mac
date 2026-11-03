#pragma once
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
// Draft activation. It is exercised only by the prefix checker until the
// complete constructor is reconstructed; production retains its current owner.
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
public:
    PlayerConstruction(guest::Cpu&, std::uint32_t player, std::uint32_t match,
                       std::uint32_t staff, std::uint32_t side,
                       std::uint32_t initial, std::uint32_t slot);
    void prepare_record();
};
}
