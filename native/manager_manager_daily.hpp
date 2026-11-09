#pragma once
// MANAGER_MANAGER.CPP daily update block: the module's day, the job offers it settles and the
// post bookkeeping they use.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x10196390: the module's once-a-day update (called with the module object, *TOC[-0x79f0]). When
// the month's number changes it runs the clubs' monthly work (0x101b7830 per club, then 0x101acf00);
// the human managers' rows lose a flag whose date has passed; every seventh day it runs 0x101a3350;
// then it walks the object's list of job offers (+8): an offer past its first date is settled (the
// person is hired by the club, or the offer is refused and dropped), one past its second date is
// handed to 0x101b1720 once. It ends with 0x101c0fc0. It keeps the original's frame, and reads the
// slot at sp+0xb4 that an earlier callee of its caller left (never written as a word).
void daily(guest::Cpu& c, std::uint32_t object);
// 0x10197650: files `person` in the first free of the club's 50 applicant slots (+0x98); 1 when
// there was one, else 0 (r3). Also leaves r6 = the index (50 when none) and, when a slot
// was free, r5 = club + 4 * index (the slot itself is at +0x98 from there); else r5 stays the club.
std::uint32_t add_applicant(guest::Cpu& c, std::uint32_t object, std::uint32_t person, std::uint32_t club);
// 0x101976b0: ends `person`'s post with `club` (as club job at +0x24, then as side job at +0x20):
// the status byte moves on (12, 13, 15 become 5, 6, 8 and the post stays; others become 11 and the
// post is cleared), news and the club's applicant slot are cleared, and the person's deal record
// (0x10082f20) is flagged 4.
void end_post(guest::Cpu& c, std::uint32_t object, std::uint32_t person, std::uint32_t club);
// 0x10197960: releases `person`'s hold on `club`'s posts by his status (the club's slots at +0x80..+0x190
// are cleared), clears the link when the status says so, tells 0x10365590 and, for a human manager,
// 0x1030f950 and 0x103d2860. Reads r31 as the caller left it when the status sets none.
void clear_post(guest::Cpu& c, std::uint32_t object, std::uint32_t person, std::uint32_t club);
// 0x101b5090: the news of `person`'s move to `club` (from `employer`, which may be 0), of the given
// kind (a byte; 5 sends nothing). Built in a message record and sent unless the club is out of
// scope or the date is past; a human manager's goes through 0x102cb630 and 0x102cc8e0.
void announce(guest::Cpu& c, std::uint32_t object, std::uint32_t person, std::uint32_t employer,
              std::uint32_t club, std::uint32_t kind);
}
