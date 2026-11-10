#pragma once
// MANAGER_MANAGER.CPP people text block: the text chooser 0x101980a0 and its three helpers.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x10197e00: the club's band 0..6 from the halfword at +6 of its row in the module's table (the row is
// object[0] + club[0] * 0x4c), cut at 0xbb8, 0xdac, 0x10b3, 0x1964, 0x2134 and 0x251c. A leaf: it leaves
// r3 = the band, cr0 = the last compare and, on the top two bands, r4 = 0x251c.
std::uint32_t club_first_band(guest::Cpu& c, std::uint32_t object, std::uint32_t club);
// 0x10197e80: the same from the halfword at +0x12, cut at -0x7d0, -0x3e8, -0x1f4, 0x1f4, 0x3e8 and 0x7d0
// (r4 = 0x7d0 on the top two bands).
std::uint32_t club_second_band(guest::Cpu& c, std::uint32_t object, std::uint32_t club);
// 0x10197f00: looks at the four entries of the record the club's league leads to (club +0x30 -> a table
// at TOC -0x7b5c -> its record -> 0x10459b50), the first with a count above 1 whose date is under ten
// days away (0x10718130 against the date at TOC -0x7b80) decides: *kind_at = 1, 3, 4 or 2 and
// *count_at = its count; none (or no such record): *kind_at = 5, *count_at = 0. Leaves r3 as the original.
void club_upcoming_entry(guest::Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t kind_at,
                         std::uint32_t count_at);
// 0x101980a0: one sentence about a club into `out` (r6), by the days (r5: under 30, under 60, more), the
// relation of 0x100f55b0 on the club's employer record, the two bands and the helper's bytes at
// sp+0x39 / sp+0x38; clubs 0x100c7d00 flags get a text by the first band with their nation's name instead.
// r7 (0x1388 or 0x7d0 at the callers) is never read. Writes only `out` and its own frame.
void club_people_text(guest::Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t days,
                      std::uint32_t out);
}
