#pragma once
#include "media.hpp"

namespace cm::aot::media {
// 0x10649fb0: what the press does with a player-and-club event of `kind` (story 0xbd0): it may turn
// into another kind by draws against the player's state, posts an item with the club's persons and
// logs the story. `other` and `third` are optional persons (the 0x70-byte records).
void post_item_bd0(guest::Cpu&, std::uint32_t self, std::uint32_t player, std::uint32_t club, std::uint32_t kind,
                   std::uint32_t other, std::uint32_t third);
// 0x10655d60: story 0xbd9 for a request {player, club, flag, second player, kind} (0x18 bytes on the
// caller's stack), with an optional person `involved`.
void post_item_bd9(guest::Cpu&, std::uint32_t self, std::uint32_t request, std::uint32_t involved);
}
