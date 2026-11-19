#pragma once
// CUP.CPP, block `tree` (tools/cup_audit.py): 0x10479100 `CUP::map_fixture_tree`, the draw tree of a knockout cup.
// A cup's rounds (cup::rounds, 0x78-byte rows) say how many ties each round has; this routine allocates the match
// rows of every round (cup::round_matches[round], 0x36 bytes per tie), blanks them, and then wires the rounds
// together: every tie of round k feeds a tie of round k+1 (or k+2 when no slot is free), through the match rows'
// "feeder" slots (+0xa/+0xc, with the feeding round at +0xe/+0x10) and the feeder's own link (+6 the tie, +8 the
// side, +0x12 the round). The placements are drawn from the game's sample stream, so the order and number of draws
// are part of the result. The last part seeds round entrants (the rounds' +0x1e entrants, from entrant index
// +0x20) into free slots of their round and writes the running first-place byte (+0x68) of every round.
// Dependencies (other modules, called through the dispatcher): the allocator (new[] 0x10775130, delete[]
// 0x10775170), rand 0x1077b770 (through the sample stream), the assertion report (0x10739890, 0x10768810,
// 0x1077b720, 0x10769b60), the localized "Memory Allocation Error" dialog (0x10727890) and exit 0x1076f7d0.
// 0x1047a9e0 (block `draws`) is never called: the original reaches it only when a draw bound exceeds 65535 in
// magnitude, and every bound here is a sign-extended halfword (see draw_below in cup_base_tree.cpp).
#include "guest_abi.hpp"

namespace cm::aot::cup_base::tree {
using guest::Cpu;
// 0x10479100 (vt +0x9c; CUP_STAGE.CPP 0x104bc6cc): the result is 1 when the tree is built, -1 when a check
// failed (the assertion text shown; an allocation failure or an unplaceable tie ends the program through exit).
// r4-r10 are left as the original leaves them.
std::int32_t map_fixture_tree(Cpu&, std::uint32_t cup);
}
