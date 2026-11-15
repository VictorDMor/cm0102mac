#pragma once
// fog_of_war.cpp (tools/fog_of_war_audit.py): what each human manager knows of every person.
//
// The game keeps a table of sixteen slots, one per human manager (the last sixteen people of the
// people table; a manager's slot is people - 1 - their id). A slot holds a knowledge object or
// null: the object's +0 points at one 12-byte entry per person (fog_of_war_detail.hpp) and +4 at
// the manager. An entry's level runs from 0 (unknown) to 3 (fully known); the daily pass lowers
// levels not refreshed for 540 days and raises each to the floor the manager's position gives.
// The table is saved after the human managers in "fog.dat".
#include "guest_abi.hpp"

namespace cm::aot::fog_of_war {
using guest::Cpu;
// The table of sixteen slots.
std::uint32_t construct_table(Cpu&, std::uint32_t table, std::uint32_t file);                    // 0x106fd120
// `deleting` is the r4 word: its low halfword, signed, above 0 frees the object; the original keeps the word in a register.
std::uint32_t destroy_table(Cpu&, std::uint32_t table, std::uint32_t deleting);                  // 0x106fd230
bool read_table(Cpu&, std::uint32_t table, std::uint32_t file);                                  // 0x106fd2c0
void write_table(Cpu&, std::uint32_t table, std::uint32_t file);                                 // 0x106fd6d0
void age_table(Cpu&, std::uint32_t table);                                                       // 0x106fd990
void forget_in_table(Cpu&, std::uint32_t table, std::uint32_t person);                           // 0x106fd9f0
void add_manager(Cpu&, std::uint32_t table, std::uint32_t manager);                              // 0x106fda80
std::uint32_t knowledge_of_manager(Cpu&, std::uint32_t table, std::uint32_t manager);            // 0x106fdb00
void record_match(Cpu&, std::uint32_t match, std::uint32_t sheets);                              // 0x106fdb30
// One manager's knowledge object.
std::uint32_t construct_knowledge(Cpu&, std::uint32_t knowledge, std::uint32_t manager);         // 0x106fdc90
std::uint32_t destroy_knowledge(Cpu&, std::uint32_t knowledge, std::uint32_t deleting);          // 0x106fdde0
bool read_knowledge(Cpu&, std::uint32_t knowledge, std::uint32_t file);                          // 0x106fde50
void write_knowledge(Cpu&, std::uint32_t knowledge, std::uint32_t file);                         // 0x106fe000
void age_knowledge(Cpu&, std::uint32_t knowledge);                                               // 0x106fe130
void forget(Cpu&, std::uint32_t knowledge, std::uint32_t person);                                // 0x106fe220
std::uint8_t level_of(Cpu&, std::uint32_t knowledge, std::uint32_t person);                      // 0x106fe280
// The rules.
std::int32_t initial_level(Cpu&, std::uint32_t knowledge, std::uint32_t manager, std::uint32_t person);  // 0x106fe3f0
bool famous(Cpu&, std::uint32_t person);                                                         // 0x106fe700
bool well_known(Cpu&, std::uint32_t person);                                                     // 0x106fe7d0
std::int32_t level_floor(Cpu&, std::uint32_t manager, std::uint32_t person);                     // 0x106fe820
void sight(Cpu&, std::uint32_t knowledge, std::uint32_t person, std::uint32_t observer);          // 0x106fec70
std::int32_t random_below(Cpu&, std::int32_t limit);                                             // 0x106fef10
bool at_club_within_a_season(Cpu&, std::uint32_t person, std::uint32_t club);                    // 0x106ff490
bool at_club_within_two_seasons(Cpu&, std::uint32_t person, std::uint32_t club);                 // 0x106ff4f0
}
