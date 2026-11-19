// LEAGUE.CPP, block `news`: the remaining queries on a league's standings (vtable +0x38, +0x64 and the per-club
// call 0x104aa3a0). Their callees are other modules' routines, called through the dispatcher with the original's
// non-volatile registers set.
#include "league_base_news_detail.hpp"

namespace cm::aot::league_base {
using namespace news;

void settle_places(Cpu& c, std::uint32_t league) {
    const Frame frame(c, 0x104a1350, 0x60, {.first_gpr = 26});
    Held h(c);
    const auto world = c.toc(toc_slot::world_table);               // r31: cell of the 0x128-byte table (inferred)
    const auto limit_cell = c.toc(toc_slot::club_cell_a);          // r6
    const auto count_cell = c.toc(toc::club_count);                // r5
    const auto competitions = c.toc(toc::competitions);
    const auto record = c.u32(league + competition::record);
    h.v[26] = league; h.v[31] = world; h.v[28] = 0; h.v[27] = 0;
    const auto rows = c.u32(league + league::standings);
    const bool third = c.i16(league + competition::club_count) > 2;
    h.v[30] = c.u32(rows);
    h.v[29] = c.u32(rows + league::standing_row);
    if (third) h.v[28] = c.u32(rows + 2 * league::standing_row);
    // The registers the virtual +0x90 receives besides the competition (the glue reads r3-r10).
    c.set({{5, count_cell}, {6, limit_cell}});
    if (c.u32(h.v[30] + 0x30) == record) {
        std::uint32_t row = c.u32(c.toc(toc::clubs));
        std::int32_t n = 0;
        while (n < std::int32_t(c.u32(count_cell) - c.u32(limit_cell))) {
            if (c.u32(row + 0x34) == record) c.put32(row + 0x34, 0);
            ++n;
            row += comp::club::size;
        }
        c.set({{7, std::uint32_t(n)}, {8, row}});
    }
    c.set(4, competitions);
    h.publish();
    const auto table = c.u32(competitions);
    if (std::int8_t(virtual_call(c, 0x90, 0x104a140c, {c.u32(table + c.u32(record) * 4)})) != 0) {
        h.publish();
        const auto status = std::int8_t(invoke(c, 0x1049d1c0, 0x104a1440,
            {c.u32(c.toc(toc_slot::nation_cell)), c.u32(record), c.u16(league + competition::year), frame.at(0x38), frame.at(0x3c), 0}));
        const auto first = c.u32(frame.at(0x38)), second = c.u32(frame.at(0x3c));
        const auto base = c.u32(world);
        std::uint32_t target = 0, back = 0x104a14b0;
        bool present = true;
        if (status == 2) {
            // The one with fewer of the half at +0xac (signed), the first on a tie.
            const auto a = c.i16(base + 0xac + first * 0x128), b = c.i16(base + 0xac + second * 0x128);
            target = base + (a < b ? second : first) * 0x128;
            back = a < b ? 0x104a148c : 0x104a147c;
        } else if (status != 0) {
            target = base + first * 0x128;
        } else present = false;
        if (present) {
            h.publish();
            h.v[27] = invoke(c, 0x100c76c0, back, {target});
        }
    }
    h.publish();
    invoke(c, 0x10463990, 0x104a14d4, {league, h.v[30], h.v[29], h.v[28], h.v[27]});
}

void record_positions(Cpu& c, std::uint32_t league) {
    const Frame frame(c, 0x104aa0c0, 0xa0, {.first_gpr = 27});
    Held h(c);
    const auto flag = c.toc(toc::success), message = c.toc(toc::message), pool = c.toc(toc::pool);
    h.v[28] = flag; h.v[31] = league; h.v[29] = message; h.v[30] = pool;
    const auto clubs = [&] { return std::int32_t(c.i16(league + competition::club_count)); };
    if (c.u32(league + competition::stats) == 0) {
        h.publish();
        report(c, frame, {0x104aa100, 0x68, 0x40, 0x1848});
        return;
    }
    h.publish();
    const auto array = invoke(c, comp::callee::allocate_block, 0x104aa14c, {std::uint32_t(clubs()) * 4});
    h.v[27] = array;
    if (array == 0) {
        h.publish();
        report(c, frame, {0x104aa168, 0x48, 0x38, 0x1850});
        return;
    }
    for (std::int32_t i = 0; i < clubs(); ++i)
        c.put32(array + 4 * std::uint32_t(i), c.u32(c.u32(league + league::standings) + std::uint32_t(i) * league::standing_row));
    // The loop's last test leaves the club count in r4: the sort's second argument.
    c.set(4, std::uint32_t(clubs()));
    h.publish();
    invoke(c, 0x10477b40, 0x104aa1e0, {array});
    h.v[30] = array; h.v[28] = 0;
    for (; std::int32_t(h.v[28]) < clubs(); ++h.v[28], h.v[30] += 4) {
        const auto club = c.u32(h.v[30]);
        if (club == 0) continue;
        h.publish();
        invoke(c, 0x10457490, 0x104aa20c, {c.u32(league + competition::stats), club, (h.v[28] + 1) & 0xff});
    }
    h.publish();
    invoke(c, comp::callee::free_block, 0x104aa228, {array});
}

void touch_clubs(Cpu& c, std::uint32_t league) {
    const Frame frame(c, 0x104aa3a0, 0x50, {.first_gpr = 28});
    Held h(c);
    const auto cell = c.toc(toc_slot::touch_cell);
    h.v[28] = league; h.v[31] = cell;
    if (c.u32(cell) == 0) return;
    h.v[29] = 0; h.v[30] = 0;
    for (; std::int16_t(h.v[29]) < c.i16(league + competition::club_count); ++h.v[29], h.v[30] += league::standing_row) {
        h.publish();
        invoke(c, 0x101b5b20, 0x104aa3f4, {c.u32(cell), c.u32(c.u32(league + league::standings) + h.v[30]), 1});
    }
}
}
