#pragma once
// MANAGER_MANAGER.CPP manager review block.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x10195e10: the module's random number below `bound` (0 answers 0), read from the halfword
// table the game keeps (cursor, wrap and redraw of the base through 0x1077b770). Bounds beyond
// +-0xffff split into digits through floating point and call themselves for the rest.
std::int32_t draw(guest::Cpu& c, std::int32_t bound);
// 0x101c1690: the club's pending departure in the object's list at +0x10. A match past 100 days
// of tenure posts a record {1, 2 or 7, club id} to the list at +4 and flags the club's row (bit
// 0x40). Answers whether it posted (r3 0/1).
bool departure(guest::Cpu& c, std::uint32_t object, std::uint32_t club);
// 0x101a8ad0: the board's review of a club's manager. `flag` is a byte: set, it runs
// the departure check first.
void review(guest::Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t flag);
}
