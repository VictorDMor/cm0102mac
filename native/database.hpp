#pragma once
// DATABASE.CPP (tools/database_audit.py): the game database's records,
// lookups and queries. Each function here is one original routine. It
// returns its result as a C++ value and, because generated code still calls
// it, also leaves what the original leaves where the game can see it: r3,
// the volatile registers r4-r10 its callers' next varargs call would spill,
// cr0, the FPSCR and the stack words it stores below r1. Only generated
// code calls them so far, so the routine and its guest ABI edge are one
// function; a C++ caller with no use for those leftovers gets a pure variant
// when the first one appears (native-first, TASKS.md).
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// Preference lists of three entries each. The meanings are inferred: the
// score at 0x1001c070 adds after a hit in a club's first list and in a
// person's favourite clubs and subtracts after a hit in a club's second
// list, which fits CM 01/02's favoured and disliked staff of a club and the
// favourite and disliked clubs and staff of a person's preferences (the
// record a person keeps at +0x18). A null person, club or preferences
// pointer answers false.
bool club_favours(Cpu&, std::uint32_t person, std::uint32_t club);        // 0x100d9480
bool club_dislikes(Cpu&, std::uint32_t person, std::uint32_t club);       // 0x100d94d0
bool favours_person(Cpu&, std::uint32_t other, std::uint32_t person);     // 0x100d9520
bool dislikes_person(Cpu&, std::uint32_t other, std::uint32_t person);    // 0x100d9580
bool dislikes_club(Cpu&, std::uint32_t person, std::uint32_t club);       // 0x100da050
bool favours_club(Cpu&, std::uint32_t person, std::uint32_t club);       // 0x100da0b0

// An attribute stored as a signed byte, shown as 1 to 20. The record's
// 16-bit value at +0x24 (0x100d2f50) or +0x4 (0x100d2e60) is added to twice
// the byte; 0x100d2d70 adds a tenth of the +0x4 value instead. Then
// r = single(10 + x / 20), s = single((r + r * r / 10) / 3) and the result
// is 0.5 + s truncated, at least 1 and at most 20.
std::int32_t attribute_rating(Cpu&, std::uint32_t record, std::int8_t value);       // 0x100d2f50
std::int32_t attribute_rating_04(Cpu&, std::uint32_t record, std::int8_t value);    // 0x100d2e60
std::int32_t attribute_rating_tenth(Cpu&, std::uint32_t record, std::int8_t value); // 0x100d2d70

// Whether a person's contract with a club has flag bits set in byte +0x5c
// (0x100d3150 tests bit 0 then bit 3, so any of 0x9). With no club, the club
// of the person's primary contract; that lookup's answer, the primary
// contract's address or null, is stored in the shared "last primary
// contract" word (TOC -0x7b78) first. A null person, a null primary
// contract, a club address that comes out 0 or no club contract answers
// false.
bool contract_flag_09(Cpu&, std::uint32_t person, std::uint32_t club);   // 0x100d3150
bool contract_flag_02(Cpu&, std::uint32_t person, std::uint32_t club);   // 0x100d3230
bool contract_flag_08(Cpu&, std::uint32_t person, std::uint32_t club);   // 0x100d3300

// The positions a person's player plays naturally (rating 20), as a mask (a
// natural goalkeeper is 0x1 alone). When none is natural, only the first
// outfield position rated 15 or more, from the sweeper to the attacker, and
// 0x10 when none is. 0 for a null person or a person without a player
// record. Bit meanings are not established beyond the ratings they come
// from (database_detail.hpp, player).
std::uint32_t position_mask(Cpu&, std::uint32_t person);                  // 0x100db1b0
}
