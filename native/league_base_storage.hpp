#pragma once
// LEAGUE.CPP, block `storage`: the league's per-club arrays, its copies and the save writer. Included at the end of
// league_base.hpp. Dependencies still generated (called through the dispatcher): 0x10739ef0/0x10771110 (copies),
// 0x101fe9f0 (date copy), 0x10775130/0x10775170 (new[]/delete[]), 0x10776360 (fwrite), 0x10746280, 0x10739de0,
// 0x107766f0, 0x1077b720, 0x10460230, 0x104bd550, 0x1056f6f0 and the league's virtual +0x2c; the assertion's calls.
#include "guest_abi.hpp"

namespace cm::aot::league_base {
using guest::Cpu;
// The base class's part of a league's copy: the vtable of the base competition (not the league's), the fields up to
// +0x4c and the 0x50 bytes of its name. Returns `to`.
std::uint32_t copy_base(Cpu&, std::uint32_t to, std::uint32_t from);                                  // 0x1049e820
// The league's copy for the match screens: `work` becomes a copy of `source` (its schedule and standings are new
// allocations; the counted blocks +0x14/+0x18/+0xe8 and the counts +0x36/+0x38 are cleared), then `out` is built
// from it (copy_base, then the league's own fields, with the league's vtable).
void clone(Cpu&, std::uint32_t out, std::uint32_t work, std::uint32_t source);                         // 0x1049e680
// Puts `club` in the standings row `slot` (a signed byte; the league's own rows when `rows` is 0), clearing its
// counters and allocating the history block and, for a head-to-head league, the opponent block. A club in state 3
// goes to 2. Fixes the history size when it is still -1. `group_flag` is the byte stored at row +0x41.
void place_club(Cpu&, std::uint32_t league, std::uint32_t slot, std::uint32_t club, std::uint32_t group_flag,
                std::uint32_t rows);                                                                    // 0x104a60e0
// A new copy of the standings (and of their opponent and history blocks): its address, 0 when there are no
// standings or clubs, or after an allocation failure (the assertion).
std::uint32_t copy_standings(Cpu&, std::uint32_t league);                                               // 0x104a6440
// Frees the opponent and history blocks of `rows` (the league's own standings when 0) and the rows; for the
// league's own it also clears the club count and the pointer.
void free_standings(Cpu&, std::uint32_t league, std::uint32_t rows);                                    // 0x104a6660
// A new word array with every club's pointer (the standings' first words) at *`array_cell`, its length at the
// halfword *`count_cell`; after an allocation failure the assertion and a count of 0. Returns the club count
// (or the success flag's address after the failure; the original leaves it in r3).
std::uint32_t club_list(Cpu&, std::uint32_t league, std::uint32_t array_cell, std::uint32_t count_cell);  // 0x104a6760 (vt +0xa8)
// The save writer (vt +0x24): writes the league's record to "CM3_TEMP/<name>_<id>.tmp" ("wb"). `temporary` (signed
// byte) 0 and an inactive league (+0x45) write nothing; a league that is active also names its file by year.
// Club pointers in the standings, the opponent blocks, the history rows and the stage rows are turned into club
// indices for the write and back to pointers afterwards.
void save(Cpu&, std::uint32_t league, std::uint32_t temporary);                                        // 0x104a8530
}
