#pragma once
#include "media.hpp"

namespace cm::aot::media {
// 0x106620b0: the press's reaction to a story about a player and his club (`reporter` is the caller's
// context, `flags` the kinds of reaction asked for: 0x1 .. 0x20 and 0x200 nudge each person's morale,
// 0x40, 0x80 and 0x100 are the verdict stages), moving the club's third rating column by `amount`, the
// verdict's weight (`factor`), and then letting each of the club's 50 persons react to the player.
// `side` is the story's side (1..3) as the callers pass it.
void react_to_story(guest::Cpu&, std::uint32_t reporter, std::uint32_t club, std::uint32_t kind, std::uint32_t flags, double factor,
                    std::uint32_t side, std::uint32_t amount, std::uint32_t player);
}
