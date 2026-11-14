#pragma once
// INJURY.CPP: block "file": building and destroying the injury manager, a new game's first injuries and
// the manager's form in a saved game.
#include "guest_abi.hpp"

namespace cm::aot::injury_unit {
// 0x101840e0: builds the manager in `manager`: finds the game's temp-file folder (asking again until the
// scrambled name resolves), makes the history object, clears the lists, then reads the manager from
// `saved_game` (a file spec, null for a new game) or, for a new game or a failed read, gives everybody
// a status row and the starting injuries. An allocation failure reports and exits. Returns `manager`.
std::uint32_t construct_manager(guest::Cpu& c, std::uint32_t manager, std::uint32_t saved_game);
// 0x10184310: releases the manager's tables, lists and history object and, when `deleting` (a signed
// halfword) is above zero, the manager itself. A null manager is accepted; returns the manager.
std::uint32_t destroy_manager(guest::Cpu& c, std::uint32_t manager, std::uint32_t deleting);
// 0x101843d0: for every club in the first (club_total - 2 * clubs_reserved) of the table, each of its 50
// player slots gets status row word +8 = 10000 - 100 * clamp(days_term - half the player's attribute byte,
// 1, 40), where the days term comes from the club's date (0x101929b0) and today.
void set_fitness_from_club_dates(guest::Cpu& c, std::uint32_t manager);
// 0x10184530: reads the manager from `saved_game` ("injury.dat" in the file spec); false after the
// assertion report of the first failing file call.
bool read_from_save(guest::Cpu& c, std::uint32_t manager, std::uint32_t saved_game);
// 0x10184e60: writes the same form.
void write_to_save(guest::Cpu& c, std::uint32_t manager, std::uint32_t saved_game);
// 0x10188490: may this person start the game injured: 1 for a null person, otherwise 1 only when the
// person's club-side date is not after today, the contract record exists with no pending word, and
// 0x101929b0 reports no obstacle and the days to today are below 40.
std::uint32_t may_start_injured(guest::Cpu& c, std::uint32_t person);
// 0x101885f0: a new game: every person gets a fresh status row; people with an attribute record may start
// injured (type draw, 1 in 10) or with a recurring injury (1 in 100), counted and reported.
void give_starting_injuries(guest::Cpu& c, std::uint32_t manager);
}
