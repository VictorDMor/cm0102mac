#pragma once
// DATABASE.CPP's lookups block (tools/database_audit.py): typed functions, one per
// original routine, as database.hpp describes. Record meanings are inferred from the
// routines' use of the fields (database_lookups_detail.hpp); TOC "nation sets" are
// words that hold ids other modules compare with the same fields.
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// Small value tests on a byte argument.
bool value_is_not_11(Cpu&, std::uint32_t value);                    // 0x100c7a00
bool value_is_11_to_13_or_15(Cpu&, std::uint32_t value);            // 0x100c7a20
std::uint32_t read_flag_7ab0(Cpu&);                                 // 0x100b8b70: the byte the TOC -0x7ab0 word points at
// Byte +0x6a holds two 2-bit fields (bits 0-1, bits 2-3). With value & 3 nonzero the
// byte keeps bits 2-3 (mask 0xc), otherwise bits 0-1 (mask 0x3); either way bits 4-7
// are cleared, then `value` is ORed in.
void set_pair_field(Cpu&, std::uint32_t record, std::uint32_t value);  // 0x100ce200

// The linked record at +0x2c (inferred: the holder's club).
bool linked_status_is_2(Cpu&, std::uint32_t holder);                // 0x100ca7d0
bool linked_nation_differs_7508(Cpu&, std::uint32_t holder);        // 0x100dbb20
bool linked_nation_differs_7508_again(Cpu&, std::uint32_t holder);  // 0x100dbbe0 (the same instructions)
bool linked_nation_in_set_6(Cpu&, std::uint32_t holder);            // 0x100de260
bool linked_nation_outside_set_3(Cpu&, std::uint32_t holder);       // 0x100dbb60
bool club_nation_in_set_6(Cpu&, std::uint32_t club);                // 0x100de320
bool listed_id_with_linked_nation_7a18(Cpu&, std::uint32_t holder); // 0x100ca800
bool club_nation_in_set_a(Cpu&, std::uint32_t club);                // 0x100caa10
bool club_nation_in_set_b(Cpu&, std::uint32_t club);                // 0x100caa90

// Staff and club lists (preferences are at +0x18 of a person).
std::uint32_t forward_if_set(Cpu&, std::uint32_t record);           // 0x100d9440: 0x101929b0(record, 0, 0)
void club_remove_favoured_staff(Cpu&, std::uint32_t club, std::uint32_t id);        // 0x100d9db0
void person_remove_favourite_staff(Cpu&, std::uint32_t person, std::uint32_t id);   // 0x100d9e10
void person_remove_disliked_staff(Cpu&, std::uint32_t person, std::uint32_t id);    // 0x100d9e90
void person_remove_favourite_club(Cpu&, std::uint32_t person, std::uint32_t id);    // 0x100da530
std::uint32_t count_disliked_staff_keyed(Cpu&, std::uint32_t person, std::uint32_t key);   // 0x100d9f10
std::uint32_t count_favourite_staff_keyed(Cpu&, std::uint32_t person, std::uint32_t key);  // 0x100d9fb0
void collect_favourite_staff(Cpu&, std::uint32_t person, std::uint32_t out, std::uint32_t count);  // 0x100dafa0
void collect_disliked_staff(Cpu&, std::uint32_t person, std::uint32_t out, std::uint32_t count);   // 0x100db060
std::uint32_t count_with_position_mask(Cpu&, std::uint32_t holder, std::uint32_t mask);    // 0x100db120

// A club's two links (record index, key) in the 0x64-byte row its first word indexes.
void clear_club_links(Cpu&, std::uint32_t club);                                    // 0x100ddf30
void add_club_link(Cpu&, std::uint32_t club, std::uint32_t other, std::uint32_t key);   // 0x100ddfc0
void remove_club_link(Cpu&, std::uint32_t club, std::uint32_t key);                 // 0x100de090
std::uint32_t find_club_link_record(Cpu&, std::uint32_t club, std::uint32_t key);   // 0x100de160

// Rules over a club's nation (r3 the club, TOC nation words).
bool club_nation_rule_a(Cpu&, std::uint32_t club);                  // 0x100d33d0
bool club_nation_rule_b(Cpu&, std::uint32_t club);                  // 0x100d34c0
bool person_club_status_rule(Cpu&, std::uint32_t person, std::uint32_t club);  // 0x100d3040

// Sort comparators (qsort4x calls them with pointers to the array elements).
std::int32_t compare_club_pointers_by_rating(Cpu&, std::uint32_t a, std::uint32_t b);  // 0x100cd330
std::int32_t compare_club_indexes_by_rating(Cpu&, std::uint32_t a, std::uint32_t b);   // 0x100cd390
std::int32_t compare_holders_by_player_rating(Cpu&, std::uint32_t a, std::uint32_t b); // 0x100cd740
std::int32_t compare_by_key_then_value(Cpu&, std::uint32_t a, std::uint32_t b);        // 0x100cde70
std::int32_t compare_by_player_values(Cpu&, std::uint32_t a, std::uint32_t b);         // 0x100ddea0

// The person/club gate and classes over the contract manager and the date.
bool person_exempt_from_club_rule(Cpu&, std::uint32_t person, std::uint32_t club);   // 0x100ca6a0
bool person_club_gate(Cpu&, std::uint32_t person, std::uint32_t club, std::uint32_t flag);  // 0x100cab10
std::uint32_t person_club_class(Cpu&, std::uint32_t person, std::uint32_t other);    // 0x100ce0a0
std::int32_t person_club_class_value(Cpu&, std::uint32_t person, std::uint32_t other, std::uint32_t flag);  // 0x100cdef0
}
