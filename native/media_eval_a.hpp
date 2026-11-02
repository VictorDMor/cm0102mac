#pragma once
#include "media.hpp"

namespace cm::aot::media {
// 0x106584d0: the verdict of a club's manager on a press story about a player. `reporter` is the caller's
// context (passed on to the story routines), `manager` the club's manager (a person with a staff record),
// `player` the person the story is about (0: none), `kind` the story's kind. Returns 1, 2 or 3 in r3:
// which way the manager leans (3: not at all), after the draws of his temper against the story's strength.
void manager_verdict(guest::Cpu&, std::uint32_t reporter, std::uint32_t manager, std::uint32_t player, std::uint32_t kind);
}
