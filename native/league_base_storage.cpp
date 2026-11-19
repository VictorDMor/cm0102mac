// LEAGUE.CPP, block `storage`: the league's per-club arrays (placing a club in a standings row, copying and freeing
// the rows) and the league's copy for the match screens. The original keeps its loop counters in registers the
// allocator and the replay's imports see: the non-volatile ones are set where the original holds them at each call,
// and the volatile ones the callers read after return (0x104a60e0, 0x104a6440, 0x104a6660 are in
// league_base_volatile_readers.inc, whose r4-r10 the checker compares) are left as the original's last writes leave
// them. Counts are the signed halfwords the original loads with `lha`; arithmetic wraps in 32 bits.
#include "league_base_storage_detail.hpp"

namespace cm::aot::league_base {
using namespace storage;

namespace {
using u32 = std::uint32_t;
using s32 = std::int32_t;
u32 wide(std::int32_t value) { return u32(value); }
// `lha`: a halfword field as the sign-extended 32-bit register.
u32 half(Cpu& c, u32 address) { return u32(s32(c.i16(address))); }
// The signed compare of two registers (`cmpw`).
bool below(u32 a, u32 b) { return s32(a) < s32(b); }
u32 clubs_of(Cpu& c, u32 league_object) { return half(c, league_object + field::clubs); }
// The history size, once: (clubs - 1) * rounds, stored as a halfword (the original's `mullw`, `sth`).
void settle_history(Cpu& c, u32 league_object) {
    c.put16(league_object + field::history_bytes, (clubs_of(c, league_object) - 1) * half(c, league_object + field::rounds));
}
}

std::uint32_t copy_base(Cpu& c, u32 to, u32 from) {
    const Frame frame(c, 0x1049e820, 0x40, {.first_gpr = 31});
    c.set({{31, to}, {6, from}});
    c.put32(to + competition::vtable, c.toc(comp::toc::vtable));
    // Words +4..+0x30, halfwords +0x34..+0x40, bytes +0x42..+0x4c: the same bytes as the original's sign-extended copies.
    for (u32 offset = 4; offset <= 0x30; offset += 4) c.put32(to + offset, c.u32(from + offset));
    for (u32 offset = 0x34; offset <= 0x40; offset += 2) c.put16(to + offset, c.u16(from + offset));
    for (u32 offset = 0x42; offset <= 0x4c; ++offset) c.put8(to + offset, c.u8(from + offset));
    invoke(c, external::memcpy_small, 0x1049e940, {to + field::name, from + field::name, 0x50});
    c.set(3, to);
    return to;
}

void clone(Cpu& c, u32 out, u32 work, u32 source) {
    const Frame frame(c, 0x1049e680, 0x50, {.first_gpr = 29});
    c.set({{29, out}, {30, source}, {31, work}});
    invoke(c, external::memcpy_object, 0x1049e6b4, {work, source, 0xec});
    c.put32(work + field::schedule,
            invoke(c, external::allocate_block, 0x1049e6c0, {half(c, work + field::schedule_count) * league::round_row}));
    invoke(c, external::memcpy_object, 0x1049e6d8,
           {c.u32(work + field::schedule), c.u32(source + field::schedule), half(c, work + field::schedule_count) * league::round_row});
    c.set(3, source);
    c.link(0x1049e6e0);
    c.put32(work + league::standings, copy_standings(c, source));
    c.put32(work + field::word_18, 0);
    c.put32(work + league::preset_fixtures, 0);
    c.put32(work + field::round_rows, 0);
    c.put16(work + field::saved_38, 0);
    c.put16(work + field::used_36, 0);
    c.set({{3, out}, {4, work}});
    c.link(0x1049e70c);
    copy_base(c, out, work);
    c.put32(out + competition::vtable, c.toc(toc::vtable));
    c.put32(out + 0xa0, c.u32(work + 0xa0));
    c.put16(out + 0xa4, c.u16(work + 0xa4));
    c.put16(out + 0xa6, c.u16(work + 0xa6));
    c.put16(out + 0xa8, c.u16(work + 0xa8));
    c.put32(out + 0xac, c.u32(work + 0xac));
    c.put32(out + 0xb0, c.u32(work + 0xb0));
    c.put8(out + 0xb4, c.u8(work + 0xb4));
    c.put16(out + 0xb6, c.u16(work + 0xb6));
    c.put16(out + 0xb8, c.u16(work + 0xb8));
    c.put32(out + 0xbc, c.u32(work + 0xbc));
    c.put16(out + 0xc0, c.u16(work + 0xc0));
    c.put16(out + 0xc2, c.u16(work + 0xc2));
    c.put8(out + 0xc4, c.u8(work + 0xc4));
    c.put8(out + 0xc5, c.u8(work + 0xc5));
    c.put8(out + 0xc6, c.u8(work + 0xc6));
    invoke(c, external::memcpy_small, 0x1049e7a0, {out + field::tie_breaks, work + field::tie_breaks, 4});
    invoke(c, external::copy_date, 0x1049e7b0, {out + league::date_cc, work + league::date_cc});
    invoke(c, external::copy_date, 0x1049e7c0, {out + league::date_d4, work + league::date_d4});
    c.put16(out + 0xdc, c.u16(work + 0xdc));
    c.put16(out + 0xde, c.u16(work + 0xde));
    c.put8(out + 0xe0, c.u8(work + 0xe0));
    c.put8(out + 0xe1, c.u8(work + 0xe1));
    c.put16(out + 0xe2, c.u16(work + 0xe2));
    c.put16(out + 0xe4, c.u16(work + 0xe4));
    c.put8(out + 0xe6, c.u8(work + 0xe6));
    c.put32(out + league::preset_fixtures, c.u32(work + league::preset_fixtures));
}

void place_club(Cpu& c, u32 league_object, u32 slot, u32 club, u32 group_flag, u32 rows) {
    const Frame frame(c, 0x104a60e0, 0xb0, {.first_gpr = 22});
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message), success = c.toc(toc::success);
    c.set({{23, league_object}, {24, slot}, {25, club}, {26, group_flag}, {27, rows}, {29, success}, {30, message}, {31, pool}});
    if (club != 0 && c.u8(club + club_state) == 3) c.put8(club + club_state, 2);
    // The history size is set once, from the club count (at most 0x32).
    if (c.i16(league_object + field::history_bytes) == -1 && s32(clubs_of(c, league_object)) > 0) {
        if (s32(clubs_of(c, league_object)) > 0x32) report(c, frame, {0x104a615c, 0x68, 0x40, 0xfe9});
        settle_history(c, league_object);
    }
    if (rows == 0) rows = c.u32(league_object + league::standings);
    c.set(27, rows);
    const u32 index = u32(s32(std::int8_t(slot)));
    const u32 offset = index * row::size, at = rows + offset;
    c.set({{22, offset}, {28, at}, {4, 0}});
    c.put32(at + row::club, club);
    c.put8(at + row::slot, slot);
    c.put32(at + 0x10, 0);
    // Three runs of counters: bytes +0, +1, +2, +3, halves +4, +6, +8 and, for the first two, a word at +0xc.
    for (const u32 base : {0x14u, 0x24u, 0x34u}) {
        for (u32 n = 0; n < 4; ++n) c.put8(at + base + n, 0);
        c.put16(at + base + 4, 0);
        c.put16(at + base + 6, 0);
        c.put16(at + base + 8, 0);
        if (base != 0x34) c.put32(at + base + 0xc, 0);
    }
    c.put8(at + row::flag, group_flag);
    c.put8(at + row::group, 0xff);
    c.put8(at + row::one, 1);
    c.put8(at + row::clubs, c.u16(league_object + field::clubs));
    c.put32(at + row::opponents, 0);
    if (s32(half(c, league_object + field::rounds)) > 0) {
        c.put32(at + row::history, invoke(c, external::allocate_block, 0x104a6264, {half(c, league_object + field::history_bytes)}));
        if (c.u32(at + row::history) == 0) {
            report(c, frame, {0x104a6288, 0x48, 0x38, 0x100c});
            return;
        }
    } else {
        c.put32(at + row::history, 0);
    }
    c.set(4, 0);
    // The tie-break bytes: a zero ends the search, a 4 asks for the opponent block.
    bool head_to_head = false;
    for (u32 n = 0; n < 4; ++n) {
        const auto byte = c.i8(league_object + field::tie_breaks + n);
        if (byte == 0) break;
        if (byte == 4) { head_to_head = true; break; }
    }
    if (head_to_head) {
        c.put32(at + row::opponents, invoke(c, external::allocate_block, 0x104a6308, {(clubs_of(c, league_object) - 1) << 4}));
        // The original re-reads the block and the count each time; it zeroes a record's bytes +4..+7 and halves +8, +0xa, +0xc.
        u32 done = 0;
        for (u32 i = 0; below(i, clubs_of(c, league_object) - 1); ++i, ++done) {
            const u32 record = i * opponent_size;
            c.put8(c.u32(at + row::opponents) + record + 4, 0);
            c.put8(c.u32(at + row::opponents) + record + 5, 0);
            c.put8(c.u32(at + row::opponents) + record + 4, 0);
            c.put8(c.u32(at + row::opponents) + record + 6, 0);
            c.put8(c.u32(at + row::opponents) + record + 7, 0);
            c.put16(c.u32(at + row::opponents) + record + 8, 0);
            c.put16(c.u32(at + row::opponents) + record + 0xa, 0);
            c.put16(c.u32(at + row::opponents) + record + 0xc, 0);
        }
        // What the last pass leaves in r5-r10 (the block, and 16 * pass + 4 in r9).
        if (done != 0) {
            const u32 opponents = c.u32(at + row::opponents);
            c.set({{5, opponents}, {6, opponents}, {7, opponents}, {8, opponents}, {9, (done - 1) * opponent_size + 4}, {10, 0}});
        }
    }
    const u32 clubs = clubs_of(c, league_object);
    c.set(4, index);
    // Only the last row's placement fills the opponent blocks, and only when it has one: row i gets every other row's club.
    if (index != clubs - 1 || c.u32(at + row::opponents) == 0) return;
    for (u32 i = 0; below(i, clubs_of(c, league_object)); ++i) {
        u32 written = 0;
        for (u32 j = 0; below(j, clubs_of(c, league_object)); ++j) {
            if (i == j) continue;
            c.put32(c.u32(rows + i * row::size + row::opponents) + written, c.u32(rows + j * row::size));
            written += opponent_size;
        }
    }
    if (!below(0, clubs)) {
        c.set({{10, 0}, {8, 0}});
        return;
    }
    const u32 end = clubs * row::size;
    c.set({{10, clubs}, {8, end}, {9, clubs}, {7, end}, {4, rows + end}, {6, (clubs - 1) * row::size + 8}});
    if (clubs >= 2) c.set(5, c.u32(rows + (clubs - 1) * row::size + 8));
}

std::uint32_t copy_standings(Cpu& c, u32 league_object) {
    const Frame frame(c, 0x104a6440, 0xa0, {.first_gpr = 27});
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message), success = c.toc(toc::success);
    c.set({{27, pool}, {28, league_object}, {30, success}, {31, message}});
    if (c.u32(league_object + league::standings) == 0 || clubs_of(c, league_object) == 0) return 0;
    if (c.i16(league_object + field::history_bytes) == -1 && s32(clubs_of(c, league_object)) > 0) {
        if (s32(clubs_of(c, league_object)) > 0x32) report(c, frame, {0x104a64b0, 0x68, 0x40, 0x1059});
        settle_history(c, league_object);
    }
    const u32 copy = invoke(c, external::allocate_block, 0x104a650c, {clubs_of(c, league_object) * row::size});
    c.set(29, copy);
    if (copy == 0) {
        report(c, frame, {0x104a6528, 0x48, 0x38, 0x1061});
        return 0;
    }
    invoke(c, external::memcpy_object, 0x104a657c, {copy, c.u32(league_object + league::standings), clubs_of(c, league_object) * row::size});
    if (c.u32(c.u32(league_object + league::standings) + row::opponents) != 0) {
        std::int16_t i = 0;
        for (u32 offset = 0; below(u32(s32(i)), clubs_of(c, league_object)); ++i, offset += row::size) {
            c.set({{27, offset}, {30, wide(i)}});
            const u32 block = invoke(c, external::allocate_block, 0x104a65a4, {(clubs_of(c, league_object) - 1) << 4});
            c.put32(copy + offset + row::opponents, block);
            const u32 source = c.u32(c.u32(league_object + league::standings) + offset + row::opponents);
            c.set(6, offset + row::opponents);
            invoke(c, external::memcpy_object, 0x104a65c8, {block, source, (clubs_of(c, league_object) - 1) << 4});
        }
        c.set({{27, u32(s32(i)) * row::size}, {30, wide(i)}});
    }
    if (c.u32(c.u32(league_object + league::standings) + row::history) != 0) {
        std::int16_t i = 0;
        for (u32 offset = 0; below(u32(s32(i)), clubs_of(c, league_object)); ++i, offset += row::size) {
            c.set({{27, offset}, {30, wide(i)}});
            const u32 block = invoke(c, external::allocate_block, 0x104a6608, {half(c, league_object + field::history_bytes)});
            c.put32(copy + offset + row::history, block);
            const u32 source = c.u32(c.u32(league_object + league::standings) + offset + row::history);
            invoke(c, external::memcpy_object, 0x104a6624, {block, source, half(c, league_object + field::history_bytes)});
        }
        c.set({{27, u32(s32(i)) * row::size}, {30, wide(i)}});
    }
    return copy;
}

void free_standings(Cpu& c, u32 league_object, u32 rows) {
    const Frame frame(c, 0x104a6660, 0x50, {.first_gpr = 27});
    bool own = false;
    c.set({{28, rows}, {27, league_object}, {30, 0}});
    if (rows == 0) {
        rows = c.u32(league_object + league::standings);
        own = true;
        c.set({{28, rows}, {30, 1}});
    }
    if (s32(clubs_of(c, league_object)) >= 1 && rows != 0) {
        for (const u32 block : {row::opponents, row::history}) {
            if (c.u32(rows + block) == 0) continue;
            u32 i = 0, offset = 0;
            c.set({{29, 0}, {31, 0}});
            for (; below(i, clubs_of(c, league_object)); ++i, offset += row::size) {
                c.set({{29, i}, {31, offset}});
                const u32 pointer = c.u32(rows + offset + block);
                if (pointer != 0) invoke(c, external::free_block, block == row::opponents ? 0x104a66cc : 0x104a6710, {pointer});
            }
            c.set({{29, i}, {31, offset}});
        }
        invoke(c, external::free_block, 0x104a672c, {rows});
        if (own) {
            c.put16(league_object + field::clubs, 0);
            c.put32(league_object + league::standings, 0);
        }
    }
}

std::uint32_t club_list(Cpu& c, u32 league_object, u32 array_cell, u32 count_cell) {
    const Frame frame(c, 0x104a6760, 0x80, {.first_gpr = 27});
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message);
    c.set({{30, message}, {27, league_object}, {31, pool}, {28, array_cell}, {29, count_cell}});
    c.put32(array_cell, invoke(c, external::allocate_block, 0x104a6790, {clubs_of(c, league_object) << 2}));
    if (c.u32(array_cell) == 0) {
        report(c, frame, {0x104a67b4, 0x40, 0x38, 0x10cc});
        c.set(3, c.toc(toc::success));
        c.put8(c.toc(toc::success), 0);
        c.put16(count_cell, 0);
        return c.r(3);
    }
    std::int16_t counter = 0;
    u32 source_offset = 0, target_offset = 0;
    for (; below(u32(s32(counter)), clubs_of(c, league_object)); ++counter, source_offset += row::size, target_offset += 4)
        c.put32(c.u32(array_cell) + target_offset, c.u32(c.u32(league_object + league::standings) + source_offset));
    const u32 clubs = clubs_of(c, league_object);
    c.put16(count_cell, clubs);
    return clubs;
}
}
