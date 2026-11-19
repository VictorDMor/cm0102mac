#pragma once
// CUP.CPP, block `draws` (tools/cup_audit.py): the routines that order a round's clubs with the game's sample
// stream. 0x1047a9e0 draws below a bound (the original unrolls its recursion; the deeper levels cannot run, see
// cup_base_draws.cpp). 0x1047fd00, 0x10480620 and 0x104810b0 (`CUP::sort_round_teams`,
// `CUP::check_round_teams_for_nation`, `CUP::randomize_round_teams`) spread a round's clubs over its ties: they
// gather the round's slots (two per tie), order or shuffle them, and place each into a free tie of the round.
// The ties are the 0x36-byte rows of cup::round_matches[round]: halves at +0 and +2 hold the club slots (or -1
// empty, -2 to be filled); +6 .. +0x12 hold the placement.
// Dependencies (other modules, called through the dispatcher): the allocator (new[] 0x10775130, clearing new[]
// 0x10775200, delete[] 0x10775170), qsort 0x10768300 with the comparator 0x10481e70 (block pilot), rand
// 0x1077b770, the assertion report and the localized "Memory Allocation Error" dialog, exit 0x1076f7d0.
#include "guest_abi.hpp"

namespace cm::aot::cup_base::draws {
using guest::Cpu;
// 0x1047a9e0: a draw in 0 .. bound-1 (the sign of the bound flips the draw's sign), 0 for bound 0. The result is
// in r3; r4-r10 are left as the original leaves them.
std::int32_t draw_below(Cpu&, std::int32_t bound);
// 0x1047fd00 `CUP::sort_round_teams` (vt: none; called by 0x1047e980): `round` is the halfword in r4.
void sort_round_teams(Cpu&, std::uint32_t cup, std::uint32_t round);
// 0x10480620 `CUP::check_round_teams_for_nation`.
void check_round_teams_for_nation(Cpu&, std::uint32_t cup, std::uint32_t round);
// 0x104810b0 `CUP::randomize_round_teams`.
void randomize_round_teams(Cpu&, std::uint32_t cup, std::uint32_t round);
}
