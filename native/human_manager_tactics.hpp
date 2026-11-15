#pragma once
// HUMAN_MANAGER.CPP block "tactics": a human's squad lists (the 50 staff ids of a tactic slot), the
// names of the files he used last, and the flags that pick a team's squad.
// `humans` is the human-manager table (r3 in the original); `staff` a human's staff record, or null
// for the human whose turn it is. A routine that cannot find the record (or gets an index outside
// 0..49) reports the failed check, clears the shared success flag and returns 0 (-1 where noted).
// The squad routines work on the slot the record's current_tactic byte names, except that slot 1
// reads as slot 0 (the original's quirk).
#include "guest_abi.hpp"

namespace cm::aot::human_manager {
// The person record in squad entry `which`, or 0 when the entry is empty (-1).                0x1014c890
std::uint32_t squad_player(guest::Cpu&, std::uint32_t humans, std::int8_t which, std::uint32_t staff);
// Entry `which` := the id in the staff record `player` points at.                              0x1014caa0
void set_squad_player(guest::Cpu&, std::uint32_t humans, std::int8_t which, std::uint32_t player, std::uint32_t staff);
// The first entry holding `player`'s id becomes empty.                                         0x1014ccf0
void remove_squad_player(guest::Cpu&, std::uint32_t humans, std::uint32_t player, std::uint32_t staff);
// Entry `which` becomes empty.                                                                  0x1014cf00
void clear_squad_slot(guest::Cpu&, std::uint32_t humans, std::int8_t which, std::uint32_t staff);
// Entries `first` and `second` trade places.                                                    0x1014d0f0
void swap_squad_slots(guest::Cpu&, std::uint32_t humans, std::int8_t first, std::int8_t second, std::uint32_t staff);
// The entry holding `player`'s id, or -1 (also when the record cannot be found).               0x1014d300
std::int32_t squad_slot_of(guest::Cpu&, std::uint32_t humans, std::uint32_t player, std::uint32_t staff);

// The names of recently used files, four of 0x100 bytes per kind (1 tactics in the current slot, 3
// training, otherwise search/print; kind 4 is refused).
// The name at `which` (0..3) of `kind` copied into `buffer` (at least 0x100 bytes), and, unless
// `bare` (a byte), turned into the file's path under the kind's folder with the kind's extension. 1 when done.
//                                                                                                0x10147310
std::uint32_t recent_file_path(guest::Cpu&, std::uint32_t humans, std::int8_t which, std::uint32_t buffer,
                               std::int32_t size, std::uint32_t bare, std::uint32_t staff, std::uint32_t kind);
// Puts `name` (its file name part) first in the kind's list: where it already is it moves up, else
// the last one drops out. 1 when done.                                                          0x101476e0
std::uint32_t remember_file(guest::Cpu&, std::uint32_t humans, std::uint32_t name, std::uint32_t staff,
                            std::uint32_t kind);

// Sets flag `which` of the record; the flag 3, when set and the staff has a team, loads the squad
// list of that team (16 pairs of staff id and position code, -1 for none) into the record.      0x10149f50
void set_flag(guest::Cpu&, std::uint32_t humans, std::int8_t which, std::uint32_t value, std::uint32_t staff);
}
