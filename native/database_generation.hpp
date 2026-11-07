#pragma once
// DATABASE.CPP's generation block (tools/database_audit.py): typed functions, one per
// original routine, as database.hpp describes. These routines draw random numbers
// (rng_core) to generate people's values; the number, order and arguments of the
// draws are exactly the original's on every path.
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// A record of the 0x2c-byte name table drawn for a person's nation (+0x68). With
// a nation row of more than 15 (0x100c42b0) or 10 (0x100c4ba0) entries, the best
// (signed weight byte, strictly greater) of three draws over the row's entries,
// else one draw over a bound of at most 0x7fff. Returns the record's address.
std::uint32_t pick_name_record(Cpu&, std::uint32_t person);          // 0x100c42b0 (row +0x18 count, +0x0 first)
std::uint32_t pick_second_name_record(Cpu&, std::uint32_t person);   // 0x100c4ba0 (row +0x1c count, +0x4 first)

// Adding an id to a three-slot list (generation_lists): nothing when it is in the "kept" list;
// an id in the "opposite" list is removed from it and nothing is added; otherwise the first
// empty slot of the kept list takes it; with all three taken a non-zero `replace` byte
// overwrites the slot a pool sample picks (a draw whenever the pool wraps). Inferred: favoured
// and disliked staff of a club (+0x5c, +0x68), favourite and disliked staff and clubs of a
// person's preferences record (+0x1c, +0x28; +0x4, +0x10).
void club_favour_person(Cpu&, std::uint32_t club, std::uint32_t person, std::int8_t replace);    // 0x100d95e0
void club_dislike_person(Cpu&, std::uint32_t club, std::uint32_t person, std::int8_t replace);   // 0x100d97a0
void person_favour_person(Cpu&, std::uint32_t owner, std::uint32_t other, std::int8_t replace);  // 0x100d9960
// The same, except that nothing is added when `owner` is `other`.
void person_dislike_person(Cpu&, std::uint32_t owner, std::uint32_t other, std::int8_t replace); // 0x100d9b80
// A club for a person's list of clubs, unless the club's list of people (+0x5c, +0x68) holds the person.
void person_favour_club(Cpu&, std::uint32_t person, std::uint32_t club, std::int8_t replace);    // 0x100da110
void person_dislike_club(Cpu&, std::uint32_t person, std::uint32_t club, std::int8_t replace);   // 0x100da320

// Copies between a person's attribute bytes and an array of 4-byte slots whose first two bytes
// are a 16-bit value (the byte sign-extended; going back, its low byte). Inferred meaning: the
// person's attribute ratings in a fixed order, as the generator's working array.
void copy_attributes_to_slots(Cpu&, std::uint32_t person, std::uint32_t array, std::uint32_t count_out);  // 0x100c6150
void copy_attributes_from_slots(Cpu&, std::uint32_t person, std::uint32_t array);                       // 0x100c63c0
// All 54 attributes plus a byte of another record (+0x64); the "to" form clears 63 flag bytes (+2) and sets
// some, and reports how many slots it has written at three points (9, 12, 32) to the optional outputs.
void copy_all_attributes_from_slots(Cpu&, std::uint32_t person, std::uint32_t other, std::uint32_t array);  // 0x100c65d0
void copy_all_attributes_to_slots(Cpu&, std::uint32_t person, std::uint32_t other, std::uint32_t array,
                                  std::uint32_t after_twelve, std::uint32_t after_nine,
                                  std::uint32_t after_thirty_two);                                           // 0x100c5ab0

// How far apart two people's attributes are (inferred): each person's attribute slots go into a static
// pair of arrays (filled with the terminator -0x7f the first time), the total starts at a scaled
// difference of a 16-bit field of the two records (+0x24, or +0x4 for the _all form) and each slot until the
// terminator adds 100 when the first array's value is outside 1-20, and otherwise |difference| weighted by 16
// (by 1000 for the first slot, 25 below the count the copy reports, in the _all form) when both are in range.
// The _all form answers 0 when both first slots are 20. `a_other` and `b_other` are the records at +0x64 of the copies.
std::int32_t attribute_distance(Cpu&, std::uint32_t a, std::uint32_t b, std::uint32_t start);       // 0x100c58c0
std::int32_t attribute_distance_all(Cpu&, std::uint32_t a, std::uint32_t a_other, std::uint32_t b,
                                    std::uint32_t b_other);                                          // 0x100c5630

// The name record for a person, following the person's category byte (+0x119; 33 values pick a
// club-like record of 0x128 bytes through a switch): see database_generation_clubs.cpp.
std::uint32_t pick_name_for_person(Cpu&, std::uint32_t person);    // 0x100c4cf0
// The same over the first-name table words, where each of the twenty draws must also pass 0x100c4b00.
std::uint32_t pick_name_for_person_filtered(Cpu&, std::uint32_t person);    // 0x100c4400

// Balances a person's attribute slots (no draws): the person's record at +0x1c goes into a local slot array;
// per pass, the slots within a limit of 0 (widened by 10 until six fit) give an average that shifts the
// others (clamped to +-0x7d); persons whose bytes +0x60/+0x62 are 10 then keep only part of their slots; a
// second pass redistributes what the positive slots exceed 0x1c by; the array goes back to the record.
void balance_attributes(Cpu&, std::uint32_t person);    // 0x100c3870

// A record of the 0x2c-byte table at TOC -0x7534: the first whose first byte is 0, remembered in a static
// word; with none, one picked from the sample pool (see database_generation_record.cpp).
std::uint32_t pick_unmarked_record(Cpu&);    // 0x100c53e0

// A random number below `bound` from the pool: |(sample + salt) % bound| up to a bound of 65535, otherwise a
// floating-point scaling plus the next level's draw below trunc(int32(bound * 10) / 65535) divided by 10, clamped
// to bound - 1. That next bound is at most 32768 in magnitude, so level 1 is always the plain remainder; levels 2-4
// and the original's self-call cannot run for a 32-bit bound (database_generation_sampler.cpp). 0 for bound 0.
std::uint32_t range_sample(Cpu&, std::int32_t bound);    // 0x100a0d80
// Draws (rng_core and range_sample) that adjust or generate a person's values; see database_generation_draws.cpp.
std::uint32_t adjust_by_draws(Cpu&, std::uint32_t person, std::uint32_t amount, std::uint32_t third);  // amount: its low byte, signed    // 0x100d3570
std::uint32_t generate_value(Cpu&, std::uint32_t person);    // 0x100da5b0
}
