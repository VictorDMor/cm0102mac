#pragma once
// TRANSFER_MANAGER.CPP native-first block "queries": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x103f4c00: whether the person has an offer of a kind that counts as open (kinds 0, 1, 5, 6).
bool person_has_open_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t person);
// 0x103f4d10: whether the contract fee blocks `offer` (true, with the reason 0x1b / 0x1c stored in `reason_out` when non-null).
bool offer_fee_refused(guest::Cpu& c, std::uint32_t offer, std::uint32_t competing, std::uint32_t reason_out);
// 0x103f5a50: reason bits of a staff offer from 0x101a0f50; the 0x2000 case scales the person's three profile values.
bool staff_offer_refused(guest::Cpu& c, std::uint32_t offer, std::uint32_t club_a, std::uint32_t person, std::uint32_t club_b,
                         std::uint32_t reason_out);
// 0x103f5c00: whether one of the person's offers was bought by the club whose index is stored at `club_id_at`.
bool person_has_offer_from_club(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club_id_at);
// 0x103f5d20: the person's offer from the club whose index is stored at `club_id_at` (0 when none); a nonzero low byte of `state_flag` skips the row test.
std::uint32_t person_offer_from_club(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club_id_at, std::uint32_t state_flag);
// 0x103f80f0: marks the person's status row active and ages its shift register by the days since the stored date.
void touch_status_row(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t date);
// 0x103f83c0: whether `person` may be offered to `other`; the refusal reason (0x27, 0x21, 7, 1, 0x12, 0x20, 0x19, 0x28, 0x1a, 0) goes to `reason_out` when non-null.
bool offer_allowed(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t other, std::uint32_t reason_out,
                   std::uint32_t skip_pending_checks);
// 0x103f8cc0: the first date the offer can be made, written to `out` (3 date fields); returns the refusing reason or 0.
std::int32_t earliest_offer_date(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t out);
// 0x103fa290: whether the offer rules of both nations let `person` join `club` now, in some month of the coming window;
// explains a refusal in the text sink `text` when non-null.
bool transfer_possible(guest::Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t person, std::uint32_t text);
// 0x103fa8e0: narrows the (month, year) window the nation rule gave, down to the month before the person's contract ends.
void narrow_window_to_contract(guest::Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t club_b, std::uint32_t person,
                               std::uint32_t month_a, std::uint32_t year_a, std::uint32_t month_b, std::uint32_t year_b);
// 0x103fb130: the current season's transfer window as two dates (start in `out`, end in `out_end` when non-null).
void season_window(guest::Cpu& c, std::uint32_t out, std::uint32_t out_end);
}
