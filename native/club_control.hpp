#pragma once
#include <cstdint>
#include <optional>

namespace cm::aot {
struct State;
}

namespace cm::aot::club_control {
// 0x1014c2e0(club, r4, r5, r6, r7): whether a club is in the hands of a human
// manager, as the original decides it. The club's manager (r6, or the club's
// own when r6 is null) must be one of the human managers, who hold the last
// 16 staff ids. Unless r4's low byte is set, two of that manager's options can
// veto it. With r5 or r7 set that is enough; otherwise a third option decides
// (one for national teams, one for clubs). Failing that, a club among the
// last nation-count clubs of the list defers to its nation's team, with r5
// cleared. Reads guest memory in the original's order and writes nothing.
// Returns nullopt where the original would report an internal error (a null
// record, a missing table, a human manager index out of range); callers then
// run the original, which reports it.
std::optional<bool> human_controlled(const State&, std::uint32_t club, std::uint32_t r4, std::uint32_t r5,
                                     std::uint32_t r6, std::uint32_t r7);
// The path 0x1014c2e0 takes for most clubs with r6 null: the club (non-null)
// has no manager or one who is not human, and it is not among the last
// nation-count clubs, so the routine calls 0x100ce250 once and returns 0
// for any r4, r5 and r7.
bool plainly_uncontrolled(const State&, std::uint32_t club);
// What 0x1014c2e0 holds in r31 on that path: the TOC slot of the human
// manager table.
std::uint32_t human_table_slot(const State&);
}
