#pragma once
// LEAGUE.CPP, block `rollover` (league_base_rollover*.cpp): the daily update, the season's end and the moves of
// clubs between divisions. Declared in namespace cm::aot::league_base (league_base.hpp includes this).
//
// Dependencies still generated or owned by other modules (called through the dispatcher): the database's club
// moves 0x1007fd00/0x1007fe90/0x1007ffb0, the transfer news 0x101aa680/0x101aaa90/0x1014c2e0, the club's
// division flags 0x104ae570, the prize tables 0x10103510/0x100f71f0/0x100f7cf0, COMP.CPP's prize news
// 0x10452010, the news manager (0x102cdbf0, 0x102bdf90, 0x102cb4b0, 0x102cc140), the lists 0x10738c00/0x10738e00/
// 0x10738d10/0x10738c20, the draw lookup 0x1048b4e0, the date routines 0x107156b0/0x10717890/0x10717a10/
// 0x10717b20, 0x104acee0, 0x104646d0, 0x10192130, 0x104b7e30, 0x1000cfc0, 0x10011750, 0x10456290, 0x104573c0,
// 0x10382d70, the sort 0x1049eb30, the allocator and memset. Virtuals go through the object's own vtable.
#include "guest_abi.hpp"

namespace cm::aot::league_base {
// The prize money of a finished league (0x1049f320): the league's money (+0x28) shared out by place over the
// clubs, as the prize news (COMP's 0x10452010) for every share above 1000. Kind 2 at +0xe1: nothing; kind 3: the
// virtual +0x74 of the league's competition object (the assertion when the table of competitions is missing).
void prize_money(Cpu&, std::uint32_t league);                                                         // 0x1049f320
// Moves `club` into `division` (a competition record; 0: none): remembers the old one, tells the database and the
// news (the virtuals 0x104ae570 decide whether the move is a promotion), `flag` asks for the transfer news, then
// the club's league-chain bonus and its flag byte +0x23d (3 becomes 2) when the division's competition exists.
void move_club(Cpu&, std::uint32_t club, std::uint32_t division, std::uint32_t flag);               // 0x1049f780
// Sets `club`'s division (previous kept at +0x34), tells the database, and files the news when the division's
// competition does not exist.
void set_division(Cpu&, std::uint32_t club, std::uint32_t division);                                  // 0x1049f8e0
// News for the clubs in a league that a draw has just paired (recursive over the stages).
void announce_pairings(Cpu&, std::uint32_t league, std::uint32_t seen_list);                          // 0x104a0ab0
// The daily update, the league's virtual +0x1c: returns 0 when nothing was done, else 1.
std::uint32_t daily_update(Cpu&, std::uint32_t league, std::uint32_t date, std::uint32_t active);    // 0x104a1540
// The end of the season, virtual +0x9c: decides every club's result and notifies it (virtual +0x48), then the
// prize/position bookkeeping for the club's row (virtual +0x70).
void end_of_season(Cpu&, std::uint32_t league);                                                       // 0x104a1c30
// Whether the league (virtual +0x18) is finished: the stages' check, the prize money once, the end-of-season
// virtual +0x9c when `finish` is set.
std::uint32_t finished(Cpu&, std::uint32_t league, std::uint32_t check_stages, std::uint32_t finish); // 0x104a8f40
// Virtual +0xb8: for a league without a parent, the chain of competitions after it: each next one exchanges
// clubs with the previous (exchange_clubs).
void chain_next(Cpu&, std::uint32_t league, std::uint32_t argument);                                  // 0x104a9160
// The promotion/relegation exchange between two divisions `from` (below) and `to` (above) (0x104a9210): the clubs
// of `from` with the outcome 3 go up, those of `to` with 0 or 5 go down (paired one to one when `pairwise` is
// nonzero... see the source). Asserts when the counted moves differ from `expect_down`/`expect_up` (not -1).
void exchange_clubs(Cpu&, std::uint32_t league, std::uint32_t from, std::uint32_t to, std::uint32_t transfer_news,
                    std::uint32_t pairwise, std::uint32_t expect_down, std::uint32_t expect_up);      // 0x104a9210
}
