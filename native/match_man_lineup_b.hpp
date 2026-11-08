#pragma once
// MATCH_MAN.CPP lineup_b block: the computer's lineup choice entry and the scaled random value.
#include "guest_abi.hpp"

namespace cm::aot::match_man {
using guest::Cpu;
// 0x102065f0 (the match setup, once per side): fills the lineup record `side` for `club`
// (side + 0x34) in match object `match`, then picks its players (through 0x10206d80 or
// 0x10206ea0) or only counts the eligible ones; returns, in r3, what those returned (the
// number of players found), and stores the bench size at the caller's byte pointer.
// - manager: the choosing person (0 for none), squad_list: the club's squad list,
//   players: the match's player table, slot: 0 home / 1 away, rival: the opposing club;
// - the caller's stack parameters sp+0x38 (the fixture) and sp+0x3c (the byte for the
//   bench size) are read from the frame, as the original does.
// Any null among match, squad_list, players, club and rival reports an assertion and exits.
std::uint32_t choose_lineup(Cpu& c, std::uint32_t side, std::uint32_t match, std::uint32_t manager,
                            std::uint32_t squad_list, std::uint32_t players, std::uint32_t slot,
                            std::uint32_t club, std::uint32_t rival);
// 0x10209d10: a random value in [0, bound) (bound's sign is kept for the small cases) drawn from
// a table of 16-bit values and a 16-bit seed, with the table refilled from the RNG
// (0x1077b770) when the cursor passes its end; large bounds are scaled down by 65535 and
// resolved over up to four levels, then the routine calls itself. Returns it in r3 and
// leaves the last scaling in f1.
std::uint32_t scaled_random(Cpu& c, std::int32_t bound);
}
