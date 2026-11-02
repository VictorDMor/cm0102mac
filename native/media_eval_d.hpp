#pragma once
#include "media.hpp"

namespace cm::aot::media {
// 0x10677d60: a press story of `kind` about a player (0x70-byte record) and his club, as seen from
// `side` (1..3): what it does to the club's ratings and the player's traits, the reactions of the
// club's other persons, and finally the item it posts (0x106620b0). `reporter` is the caller's
// context, passed on to the posting routines.
void evaluate_story(guest::Cpu&, std::uint32_t reporter, std::uint32_t player, std::uint32_t club, std::uint32_t kind, std::uint32_t side);
}
