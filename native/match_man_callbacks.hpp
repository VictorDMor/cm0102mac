#pragma once
// MATCH_MAN.CPP callbacks block: the match side's release and squad preparation, the
// actor preparation, and the two squad-ordering comparators.
#include "guest_abi.hpp"

namespace cm::aot::match_man {
using guest::Cpu;
// 0x10206cb0: the side's release (reached through a transition vector). Releases its
// squad (0x103b1c00) unless the squad is held by someone else, then frees the side when
// the halfword `destroy` is positive. Returns the side.
std::uint32_t release_side(Cpu& c, std::uint32_t side, std::uint32_t destroy);
// 0x10206d80: prepares an actor (0x1020e8f0) for each squad member that is not suspended
// and not already seated; returns how many were prepared.
std::uint32_t prepare_squad_actors(Cpu& c, std::uint32_t side);
// 0x1020e8f0: prepares the actor `index` (a signed byte, 0..19) of the side for `staff`
// (0: nothing to prepare, returns 0). An index outside 0..19 reports the game's
// assertion and exits. Returns 1 after preparing.
std::uint32_t prepare_actor(Cpu& c, std::uint32_t side, std::uint32_t index, std::uint32_t staff, std::uint32_t tag);
// 0x1020ea00 and 0x1020ee00: qsort comparators over 4-byte slots holding player record
// pointers, each answering (negative: `first` goes first) by the players' scores (a
// halfword plus a float). `first` and `second` point at the slots. A missing or
// staffless player orders last, one at position -2 first, and so does a player of role 1
// whose staff rating (+0xf) is below 20.
std::int32_t compare_by_scaled_score(Cpu& c, std::uint32_t first, std::uint32_t second);
std::int32_t compare_by_score(Cpu& c, std::uint32_t first, std::uint32_t second);
}
