#pragma once
// DATABASE.CPP's records block (tools/database_audit.py): typed functions, one per
// original routine, as database.hpp describes. Out-of-range arguments report
// through the game's assert macro and answer 0 (-1 for date_difference).
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// Two dates' signed difference as a byte; the first has 16-bit fields at
// +0x34/+0x36, the second at +0/+2 (inferred).
std::int32_t date_difference(Cpu&, std::uint32_t a, std::uint32_t b);                // 0x100c6a10
// The club at record[+0x68] plus the range words' difference (doubled low
// word for the first), in the club table.
std::uint32_t club_at_doubled_bias(Cpu&, std::uint32_t record);                      // 0x100c76c0
std::uint32_t club_at_bias(Cpu&, std::uint32_t record);                              // 0x100c7860
// Two packed 16-bit values from a record's linked records into the outputs.
bool record_kind_values(Cpu&, std::uint32_t record, std::uint32_t kind,
                        std::uint32_t first_out, std::uint32_t second_out);          // 0x100c7a50
bool record_pair_values(Cpu&, std::uint32_t record, std::uint32_t first_out,
                        std::uint32_t second_out);                                   // 0x100c7bc0
// A record's first word against the range words (doubled low word for the first).
std::int32_t range_position_doubled(Cpu&, std::uint32_t record);                     // 0x100c7d00
std::int32_t range_position(Cpu&, std::uint32_t record);                             // 0x100ce250
// The flags' high nibble less 10 of a person's club contract, 0 if none.
std::int32_t contract_rating(Cpu&, std::uint32_t person, std::uint32_t club_index);  // 0x100ca010
// Comparators of two records' doubles through their links (function pointers).
std::int32_t compare_rating_first(Cpu&, std::uint32_t a, std::uint32_t b);           // 0x100cd060
std::int32_t compare_rating_second(Cpu&, std::uint32_t a, std::uint32_t b);          // 0x100cd180

// Whether a record's three linked records carry the three ids and, given a date, its date.
bool record_matches(Cpu&, std::uint32_t record, std::uint32_t first_id, std::uint32_t second_id,
                    std::uint32_t third_id, std::uint32_t date);                     // 0x100cbde0
// Text of the gap between two dates in days, into a buffer of at least 5 bytes.
bool format_date_gap(Cpu&, std::uint32_t a, std::uint32_t b, std::uint32_t buffer,
                     std::int32_t capacity);                                         // 0x100cd7c0
// The class (1-10) of an entry's value among the words the TOC slots point at.
std::int32_t classify_by_slots(Cpu&, std::uint32_t entry);                           // 0x100cdae0
// Attribute number 1-12 of a person (raw bytes or ratings), -1 after an assert.
std::int32_t attribute_by_number(Cpu&, std::uint32_t first, std::uint32_t second,
                                 std::int8_t number);                                // 0x100d2b40
// Attribute number 1-31 of a person (raw bytes or ratings), -1 after an assert.
std::int32_t attribute_by_number_31(Cpu&, std::uint32_t first, std::uint32_t second,
                                    std::int8_t number);                             // 0x100d2330
// Whether a record's id word or list for `kind` is empty or holds the id.
bool record_accepts_id(Cpu&, std::uint32_t record, std::int8_t kind, std::uint32_t id); // 0x100c7dd0
// Records an id record in the slot a kind names of a record, with date and flags.
bool add_to_record(Cpu&, std::uint32_t record, std::uint32_t id_record, std::uint32_t kind,
                   std::uint32_t date, std::uint32_t flag_a, std::uint32_t flag_b);  // 0x100c82a0
// Allocates and fills the per-slot summaries of the three member tables.
void build_slot_summaries(Cpu&);                                                    // 0x100c3cb0
// The club a record's code names (0 for none); `found` receives 1 or 0 by the code's set.
std::uint32_t club_of_code(Cpu&, std::uint32_t record, std::uint32_t found,
                           std::int8_t require_flag);                               // 0x100ce320

// How many of a record's id list (five at +0x160, three at +0x190) and its club's point at a record below the limit.
std::uint32_t count_valid_five(Cpu&, std::uint32_t record);                          // 0x100c9440
std::uint32_t count_valid_three(Cpu&, std::uint32_t record);                         // 0x100c9590
std::uint32_t id_word_94_or_club(Cpu&, std::uint32_t record);                        // 0x100c9bb0
std::int32_t person_attribute_31(Cpu&, std::uint32_t person, std::uint32_t number);  // 0x100d1cb0
std::int32_t person_attribute_12(Cpu&, std::uint32_t person, std::uint32_t number);  // 0x100d2a80
bool in_second_set(Cpu&, std::uint32_t record);                                      // 0x100d3e60
bool classes_compatible(Cpu&, std::uint32_t a, std::uint32_t b);                     // 0x100cd980
std::uint32_t copy_ids_five(Cpu&, std::uint32_t record, std::uint32_t out);          // 0x100c9c90
std::uint32_t copy_ids_seven(Cpu&, std::uint32_t record, std::uint32_t out);         // 0x100c9e50
std::uint32_t copy_member_ids(Cpu&, std::uint32_t record, std::uint32_t out, std::uint32_t flag);  // 0x100d06f0
std::uint32_t collect_ids(Cpu&, std::uint32_t record, std::uint32_t out, std::uint32_t limit);     // 0x100d4370
std::uint32_t id_word_90_chain(Cpu&, std::uint32_t record);                          // 0x100c96e0
bool club_less_candidate(Cpu&, std::uint32_t record);                                // 0x100d12d0
bool eligible_record(Cpu&, std::uint32_t record);                                    // 0x100d3c40
std::uint32_t count_members_kept(Cpu&, std::uint32_t record, std::uint32_t kind, std::uint32_t flag);  // 0x100c9070
void trim_club_members(Cpu&);                                                        // 0x100d0540
}
