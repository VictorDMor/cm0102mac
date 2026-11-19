#pragma once
// LEAGUE.CPP, block `news` (league_base_news*.cpp): the prizes, the news items a league files and the
// remaining queries on its standings. Included at the end of league_base.hpp.
//
// Dependencies still generated (called through the dispatcher): the news manager's item routines (NEWS.CPP
// 0x102bdf90 start, 0x102cb4b0 set field, 0x102cc030/0x102cc140/0x102cc280/0x102cc400 file, 0x102cdbf0 enabled),
// 0x10103510/0x100f5730 (club cash), 0x10494d20, 0x1049d1c0, 0x100c76c0, 0x10463990, 0x10477b40, 0x10457490,
// 0x101b5b20, 0x10381b70/0x103822e0/0x10382e50, the competitions' virtuals +0x4c/+0x70/+0x90, the allocator and
// qsort, 0x107755c0 (an assertion). COMP.CPP's 0x104527c0 goes through the dispatcher (a direct call of comp::news_competition_year faulted in the checker, see the report); DATE.CPP is called directly.
#include "guest_abi.hpp"

namespace cm::aot::league_base {
using guest::Cpu;
// The sum over the league's clubs of (place + 1) * `amount`, places counted from 0, wrapping (the prize pool of a
// league paying `amount` per place); asserts (line 0x214) when the league has no clubs.
std::uint32_t prize_total(Cpu&, std::uint32_t league, std::uint32_t amount);                       // 0x1049f540
// When the club-cash object exists: for every club of the league (those whose +0x34 equals `only`, or all when
// `only` is 0) tells the club's cash (0x10103510/0x100f5730) `value` and files COMP's item 0x1f for it.
void file_prize_news(Cpu&, std::uint32_t league, std::uint32_t value, std::uint32_t only);        // 0x1049f6d0
// News item 0x15 (inferred: the league's first round): for a league of at most eight clubs, files under each
// club whose news is on an item with the record id, the code, the earliest date of the first round's weekday
// slots, the club count - 1 and the other clubs.
void file_first_round_news(Cpu&, std::uint32_t league);                                            // 0x104a0e40
// (vt +0x38) Clears the clubs' +0x34 that equals the league's record when the first club's is; asks the
// competition's virtual +0x90, finds a club row of the world's 0x128-byte table (inferred) through 0x1049d1c0 and
// hands the first three standings rows and it to 0x10463990.
void settle_places(Cpu&, std::uint32_t league);                                                    // 0x104a1350
// (vt +0x48) News item 0xc for a result (`club`, `kind`, `stage`): `kind` 0..3, 5, 6, 7, -2 pick the filing.
// Returns 0 when the fixture rules (0x10494d20) refuse or the virtual +0x4c answers -1, else 1.
std::uint32_t file_result_news(Cpu&, std::uint32_t league, std::uint32_t club, std::uint32_t kind, std::uint32_t stage);  // 0x104a44f0
// (vt +0x4c) Records a place with the club-history object (kind 5, 0 or 3); returns 0.
std::uint32_t record_place(Cpu&, std::uint32_t league, std::uint32_t club, std::uint32_t kind);   // 0x104a47f0
// (vt +0x64) Gives every club of the league, sorted by 0x10477b40, its position (1-based, low byte) in the
// statistics object (+8): 0x10457490; asserts (0x1848 no statistics, 0x1850 allocation).
void record_positions(Cpu&, std::uint32_t league);                                                 // 0x104aa0c0
// Calls 0x101b5b20(club, 1) for every club when the global at TOC -0x79f0 is set.
void touch_clubs(Cpu&, std::uint32_t league);                                                      // 0x104aa3a0
// (vt +0x8c) The "team_points" listing for `club`: projects every club's points, sorts four times and passes the
// positions to the virtual +0x70. Returns 0 when the club has no row, else 1.
std::uint32_t team_points(Cpu&, std::uint32_t league, std::uint32_t club);                         // 0x104aa620
}
