#pragma once
// DISCIPLINE.CPP table block: the person table's slots, the pending list and history file it keeps per person,
// and the queries other modules make of the ruling bodies through the manager.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::discipline {
// 0x100e7040: the value in slot `slot` of person `person`'s row when the slot holds ruling body
// `body`, else -1. Called by RULING_BODY.CPP for every ban lookup (the module's hottest leaf).
std::uint32_t slot_value_for(const guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::int8_t slot,
                             std::int8_t body);
// 0x100e7080: stores ruling body `body` and `value` in slot `slot` of person `person`'s row.
void set_slot(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::int8_t slot, std::uint8_t body,
              std::uint32_t value);

// 0x100e70b0: the first pending-list item, from the list's start, that belongs to `person` and `slot` (0 when the
// list ends or the person's row counts no pending item first). Leaves the list cursor after the item.
std::uint32_t next_pending_item(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::int8_t slot);
// 0x100df950: removes every pending item of the person whose id is the word at `person_word`, decrementing the
// row's count (a byte) for each, while the row still counts one.
void remove_pending_items(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person_word);
// 0x100e7160: hands the person's history position (two words of its row) to the history file with `argument`.
void pass_history_position(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t argument);
// 0x100e71a0: the history file's entry for the person's position, 0 when the person's id is not below the count.
std::uint32_t read_history_entry(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person);
// 0x100e8660: appends the history entry (id at +0, a nonzero word at +0x14) at the person's position; a null or
// incomplete entry reports an assertion and exits.
void append_history(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t entry);
// 0x100e9290: the order of the pending list's sort (0x100e05c0 passes it as a function pointer): items of 8 bytes
// (word +0 person, word +4 the key), by key and then person, with -1 as a wildcard person.
std::uint32_t compare_pending_items(const guest::Cpu& c, std::uint32_t first, std::uint32_t second);
// 0x100e7bd0: allocates the person table (0x2c bytes per person) filled with 0xff and zeroes each row's pending
// count; a failed allocation reports an assertion.
void allocate_person_table(guest::Cpu& c, std::uint32_t manager_object);

// 0x100df260 / 0x100df2d0: the ruling body governing the competition whose id is the word at
// `competition_id_at` runs its virtual method at vtable offset 8; every ruling body runs the one at 0xc with
// `argument`.
void notify_competition_body(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t competition_id_at);
void notify_all_bodies(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t argument);
// 0x100e7920: whether the person is banned for a fixture of the competition (0 or 1). `rule_argument_a/b` are
// the caller's r6/r7, passed on to the ruling bodies. May clear the person's entry in its human manager's list.
std::uint32_t is_suspended(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person,
                           std::uint32_t competition_id_at, std::uint32_t rule_argument_a, std::uint32_t rule_argument_b);
// 0x100e7a90 / 0x100e7b40: the governing body's answers for a person (0 where there is none).
std::uint32_t body_query_for_competition(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person,
                                         std::uint32_t competition_id_at);
std::uint32_t body_query_for_slot(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::int8_t slot,
                                  std::uint32_t argument_a, std::uint32_t argument_b);
// 0x100e6e90: the match-day hook: a fixture goes to the ruling body of its competition, else (for a fixture the
// filter accepts or the friendly competition) to the bodies of its two sides' leagues. `argument` is r5, passed on.
void match_day_fixture(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t fixture, std::uint32_t argument);
}
