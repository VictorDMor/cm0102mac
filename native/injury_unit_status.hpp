#pragma once
// INJURY.CPP: questions about a person's status row and injury, and the pending daily update.
#include "guest_abi.hpp"

namespace cm::aot::injury_unit {
// 0x10187650: the person's condition (status row +0xc), 0 for an index at or past the people count.
// `index` is the person's index, not the record.
std::int32_t condition_of(guest::Cpu& c, std::uint32_t manager_object, std::int32_t index);
// 0x10187a20: sets the flag byte of the person's status row.
void set_status_flag(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t value);
// 0x1018c180: byte +1 of an injury type's table row.
std::uint32_t type_class(guest::Cpu& c, std::int8_t type);
// 0x1018c340: the person's injury record after the pending daily update (0x10187a40), or null:
// without one, or for a person past the people count. A null person is reported and gives null.
std::uint32_t current_injury(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person);

// 0x10187a40: the pending daily update of a person's status row: the days since the row was last brought up to
// date are shared between rest, light and full training and drain, and fitness, sharpness and (through the
// injury, when one has run its course, 0x1018b9f0) the injury record follow. Nothing for a null person, one
// past the people count, one without a record (+0x14) or a row already dated today.
void update_status(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person);
// 0x10187680: 0 without an injury (or a person past the people count), 2 while the injury has not yet lasted its
// first length, else 1 once the days since it began exceed the second length. A null person is reported.
std::uint32_t injury_stage(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person);
// 0x101877e0: the person's sharpness (status row +0xa) after the update; as a percentage of the unit's float
// divisor when `percent`. Null person: 0; past the people count: 10000 (100 as a percentage).
std::uint32_t sharpness_of(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t percent);
// 0x101878d0: the person's match fitness, fitness times sharpness over 10000 (capped at 9000 while the injury
// stage is 1, at least 100), as a percentage when `percent`.
std::uint32_t match_fitness_of(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t percent);
// 0x10187450: the daily wear of the person's condition (status row +0xc) by index.
void wear_condition(guest::Cpu& c, std::uint32_t manager_object, std::int32_t index);
// 0x10186a20: sets the person's sharpness and moves the condition by `change`.
void set_sharpness(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t sharpness, std::uint32_t change);
// 0x10186ea0: gives the person (by index) a fresh status row, closing their injury and dropping their list entries.
void reset_status(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t index);
// 0x10188930: the injury type to give the person for `kind`; the type of the person's current injury when it is
// not one of the minor ones, else the draw among the kind's types (-1: none).
std::uint32_t pick_injury_type(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t kind, std::uint32_t severity,
                               std::uint32_t person);
// 0x1018b9f0: ends the person's injury: the history entry, the news if asked, and the record is freed.
void close_injury(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t report);
// 0x10186fc0: after a match, each of the two sides' 20 players is rated for the injuries it can bring and the
// manager's list entries of that person are turned into injuries.
void match_injuries(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t options, std::uint32_t match);
// 0x10186cd0: records a (kind, strength) pair for the key in the manager's list (+0x28): raises the strength of
// the same kind (capped at 100), replaces a weaker different one; returns the entry's values through the bytes.
void note_entry(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t key, std::uint32_t kind_cell, std::uint32_t strength_cell);
// 0x10186e10: the entry of the manager's list (+0x28) whose key word matches, or null.
std::uint32_t find_entry(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t key);
// 0x101864a0: a draw in [0, n) (in (n, 0] for negative n) from the sample table; ranges past 0xffff use several
// draws.
std::int32_t random_below(guest::Cpu& c, std::int32_t n);
}
