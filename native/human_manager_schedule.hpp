#pragma once
// HUMAN_MANAGER.CPP block "schedule": the options a human sets on his record (away, holiday, whether he
// runs his club or a national side), the tactic slot chosen for a club, the 0x50-byte booking row
// at record +0x9a68 (set, compared, cleared when it no longer fits) and the staff a club answers
// to. `humans` is the human-manager table (r3 in the original); `staff` a human's staff record, or
// null for the human whose turn it is. A routine that cannot find the record reports the failed
// check (the game's assertion dialog), clears the shared success flag and returns 0.
#include "guest_abi.hpp"

namespace cm::aot::human_manager {
using guest::Cpu;
// Tactic slot of the record: 4 for a club that 0x100ce250 flags, 3 / 2 for one that 0x100c7d00 flags
// (3 with `flag`), else 1 with `flag`, 0 without. No result.
void choose_tactic_for_club(Cpu&, std::uint32_t humans, std::uint32_t flag, std::uint32_t club, std::uint32_t staff);  // 0x10146830
// The options: store a byte, keep the table's counters and tell the news / the match day.
void set_away(Cpu&, std::uint32_t humans, std::uint32_t flag, std::uint32_t staff);        // 0x10147fd0
void set_runs_club(Cpu&, std::uint32_t humans, std::uint32_t value, std::uint32_t staff);  // 0x10148340
void set_runs_nation(Cpu&, std::uint32_t humans, std::uint32_t value, std::uint32_t staff);  // 0x10148660
void set_holiday(Cpu&, std::uint32_t humans, std::uint32_t flag, std::uint32_t staff);     // 0x101492d0
// Booking rows (0x50 bytes; the human's own is at record +0x9a68).
std::uint32_t booking_matches(Cpu&, std::uint32_t humans, std::uint32_t row, std::uint32_t staff);  // 0x101494b0
std::uint32_t any_booking_matches(Cpu&, std::uint32_t humans, std::uint32_t row);          // 0x10149690
std::uint32_t set_booking(Cpu&, std::uint32_t humans, std::uint32_t row, std::uint32_t staff);  // 0x10149790
std::uint32_t booking_is_free(Cpu&, std::uint32_t humans, std::uint32_t row, std::uint32_t staff);  // 0x101499f0
std::uint32_t clear_taken_bookings(Cpu&, std::uint32_t humans);                            // 0x10149bd0
// A club's staff record: its +0x94, else the one of +0x160.. whose +0x1c record has the highest +0x2a.
std::uint32_t club_head_staff(Cpu&, std::uint32_t club);                                   // 0x1014c5f0
// The current human's 50 chosen person ids of the tactic slot in use: -1 for the ones that do not fit.
void drop_unfit_entries(Cpu&, std::uint32_t humans);                                       // 0x1014c680
}
