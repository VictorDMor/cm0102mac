#pragma once
// LEAGUE.CPP, block `news`: what its routines share (the news manager's item routines and the registers the
// original holds at its calls).
#include "league_base_detail.hpp"
#include <array>

namespace cm::aot::league_base::news {
namespace item {
// NEWS.CPP: start an item of a kind (item, kind, 0), set field n (item, n, value), file an item under a club
// (manager, item, club), a competition record (manager, item, record), and whether the manager has news on.
inline constexpr std::uint32_t start = 0x102bdf90, set_field = 0x102cb4b0, file_under_club = 0x102cc140,
                               file_under_record = 0x102cc280, file_under_nation = 0x102cc400,
                               file_with_competition = 0x102cc030, enabled = 0x102cdbf0;
}
namespace toc_slot {
inline constexpr std::int32_t club_cash = -0x7ab8,   // cell: the club-cash object 0x10103510 takes (inferred)
                              fixture_rules = -0x7b54, history = -0x7b48, world_table = -0x7b98,
                              nation_cell = -0x7424, club_cell_a = -0x7b04, touch_cell = -0x79f0;
}
// The registers r14-r31 as the original holds them at each of its calls: the news manager, the allocator and
// the date routines can reach imports, where a replay compares the non-volatile registers. `v[n]` is the
// original's rN, written where the original writes it, so a register a path has not reached yet is what the
// original has too (the caller's value, or the stale value of an earlier round).
struct Held {
    Cpu& c;
    std::array<std::uint32_t, 32> v{};
    explicit Held(Cpu& cpu) : c(cpu) { for (unsigned n = 14; n < 32; ++n) v[n] = c.r(n); }
    void publish() const { for (unsigned n = 14; n < 32; ++n) c.set(n, v[n]); }
};
}
