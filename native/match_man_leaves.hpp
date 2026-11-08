#pragma once
// MATCH_MAN.CPP's leaves: routines that call nothing.
#include "guest_abi.hpp"

namespace cm::aot::match_man {
using guest::Cpu;
// 0x102065e0: an empty callback (only a function pointer refers to it).
inline void empty_callback(Cpu&) {}
// 0x102107c0: the player record of a match side whose person is `person`, or 0.
std::uint32_t side_player_for_person(Cpu& c, std::uint32_t side, std::uint32_t person);
}
