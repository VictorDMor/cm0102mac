#pragma once
// MATCH_MAN.CPP squad_a block: the squad's per-player instructions for a side.
#include "guest_abi.hpp"

namespace cm::aot::match_man {
using guest::Cpu;
// 0x1020a290 (called by 0x10206ea0 and 0x1020d7d0; no result is read): `side`
// is a match side record (club at +0x34, the rival's club at +0x38, orders
// object at +0x30, staff data at +0x24). It flags the orders object from the
// two clubs' ratings and the manager's traits, then walks the side's 20
// players, summing a fatigue-like score (first loop), and for each player
// on the pitch applies instructions through 0x10111310, 0x10113b60 and
// 0x10113e30 (second loop) from the player's attributes scaled by a noise
// value of the match's clock.
// Calls DATABASE 0x100d1cb0, 0x100d2a80, 0x100d2f50, FORMATION 0x10111310,
// 0x10113b60, 0x10113e10, 0x10113e30, 0x10114b50, HUMAN_MANAGER 0x10147e70,
// 0x10149df0, MATCH_ENG 0x101fd100, MATCH_PL 0x10289560, 0x102895e0,
// PLAYER_STATS 0x10303560, TACTICS 0x103b14d0, the RNG 0x1077b770 and
// 0x10780fc0, all through the dispatcher in the original's order.
void squad_a(Cpu& c, std::uint32_t side);
}
