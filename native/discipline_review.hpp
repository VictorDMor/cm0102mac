#pragma once
// DISCIPLINE.CPP review block: the ruling of one player after a match, and the game's range sampler
// that block keeps its own copy of. The ruling body objects (RULING_BODY.CPP), the news and the
// player records are reached through the dispatcher; bans are registered and cleared by the bans block.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::discipline {
// 0x100e8780: after a match, RULING_BODY.CPP hands each player's entry (`entry`, the match record of the
// player: person id at +0, bytes at +0x2e/+0x2f/+0x48; `argument` is a pointer or null passed on to 0x100e0700)
// to the manager. An entry with 12 in byte +0x2e or +0x2f is a ruling case (else the result is 0):
//   - every ban the person holds in the first four slots of its row is cleared (0x100e0090);
//   - a ban of 90 days (180 when 0x10718130 gives a gap below 0x50) is built in the frame, registered for body 0
//     only (0x100dfb30) and 0x100e0700 told of it;
//   - then, if the object at person+0x24 passes its tests (reputation halfword above 0x1482, a record at +0x90,
//     0x1014c2e0 false, 0x100dbbe0 true, a byte of the table at TOC -0x7984 below -3, a club field below 0x109a), up
//     to three draws decide news items of 0x106a15f0: kind 1 (draw one in twenty, with the ban's days), else kind 3
//     when a draw bounded by two conduct bytes is 0, and kind 2 (draw one in twenty) when both conduct bytes pass.
// Returns 0 when the entry is not a ruling case, else 1.
std::uint32_t review_player(guest::Cpu& c, std::uint32_t manager, std::uint32_t entry, std::uint32_t argument);
// 0x100e8d10: this module's copy of the game's inline range sampler: a value in [0, bound) from the
// draw table (bound 0: 0, without a draw; |bound| <= 65535 the signed remainder of the next draw;
// wider bounds a scaled draw plus a tenth of a second draw's correction, capped at bound-1).
std::int32_t sample_range(guest::Cpu& c, std::int32_t bound);
}
