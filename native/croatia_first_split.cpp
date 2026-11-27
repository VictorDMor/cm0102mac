#include "croatia_first_split.hpp"
#include "league_base_table_detail.hpp"

namespace cm::aot::croatia_first {
namespace {
using guest::Cpu;
using guest::Frame;
using comp::invoke;
namespace competition = comp::competition;
namespace league = league_base::league;
namespace row = league_base::table::row;
using u32 = std::uint32_t;

constexpr unsigned group_size = 6, opening_matches = 22;
constexpr int split_order_descriptor = -0x5fb0;

// The 3.89 groups use the same guest ABI and temporary LEAGUE_STAGE objects as the Scottish split
// in the Mac engine. Stage -1 places their fixtures on the root competition. Only the group code,
// the calendar and the number of meetings distinguish the Croatian fixtures.
void make_group(Cpu& c, const Frame& frame, u32 root, u32 entrants, u32 group) {
    const auto rows = c.u32(root + league::standings);
    for (unsigned i = 0; i < group_size; ++i) {
        const auto seed = group == 0 ? i : 2 * group_size - 1 - i;
        c.put32(entrants + 4 * i, c.u32(rows + seed * league::standing_row + row::club));
    }

    const auto count = frame.at(0x7a), code = frame.at(0x78);
    const auto dates = comp::virtual_call(c, 0x44, group == 0 ? 0x1057e4dc : 0x1057e66c,
                                         {root, group, count, code, 0});
    const auto stage = invoke(c, comp::callee::allocate_object, group == 0 ? 0x1057e4f0 : 0x1057e67c, {league::size});
    if (stage != 0) {
        c.put32(frame.at(0x38), comp::sext8(c.u8(root + league::win_points)));
        c.put32(frame.at(0x3c), comp::sext8(c.u8(root + league::draw_points)));
        c.put32(frame.at(0x40), c.u8(root + league::byte_c6));
        c.put32(frame.at(0x44), root + league::tie_breaks);
        c.put32(frame.at(0x48), root + league::automatic_promotion);
        c.put32(frame.at(0x4c), comp::sext16(c.u16(root + competition::year)));
        c.put32(frame.at(0x50), ~0u);
        c.put32(frame.at(0x54), comp::sext16(c.u16(code)));
        c.put32(frame.at(0x58), 20);
        c.put32(frame.at(0x5c), 1);
        c.put32(frame.at(0x60), 0);
        c.put32(frame.at(0x64), 3);
        c.put32(frame.at(0x68), ~0u);
        c.put32(frame.at(0x6c), 0);
        c.put32(frame.at(0x70), 2);
        invoke(c, 0x1056dc60, group == 0 ? 0x1057e590 : 0x1057e71c,
               {stage, root, group_size, entrants, 2, c.u32(root + competition::record), dates,
                comp::sext16(c.u16(count))});
        comp::virtual_call(c, 0x08, group == 0 ? 0x1057e5b0 : 0x1057e73c, {stage, 1});
    }
    invoke(c, comp::callee::free_block, group == 0 ? 0x1057e5bc : 0x1057e750, {dates});
}
}

void maybe_split(Cpu& c, u32 root, u32 context) {
    if (!context || c.i16(root + competition::rounds) != 2 ||
        c.i16(root + competition::club_count) != 2 * group_size) return;
    const auto rows = c.u32(root + league::standings);
    if (!rows) return;
    for (unsigned i = 0; i < 2 * group_size; ++i) {
        if (c.i8(rows + league::standing_row * i + row::totals + row::played) < int(opening_matches)) return;
    }

    Frame frame(c, 0x1057e3f0, 0xd0, {.first_gpr = 24});
    const auto entrants = invoke(c, comp::callee::allocate_block, 0x1057e414, {group_size * 4});
    if (!entrants) {
        c.put8(c.toc(comp::toc::success), 0);
        return;
    }
    make_group(c, frame, root, entrants, 0);
    make_group(c, frame, root, entrants, 1);
    invoke(c, comp::callee::free_block, 0x1057e748, {entrants});
    c.put16(root + competition::rounds, 3);
    restore_split_order(c, root);
}

void restore_split_order(Cpu& c, u32 root) {
    if (c.i16(root + competition::rounds) == 3)
        c.put32(root + league_base::table::field::compare, c.toc(split_order_descriptor));
}

std::int32_t compare_split(Cpu& c, u32 left_row, u32 right_row) {
    const auto bottom_group = [&](u32 standing) {
        return c.i8(c.u32(standing + row::history) + opening_matches - 1) >= int(group_size);
    };
    return int(bottom_group(left_row)) - int(bottom_group(right_row));
}
}
