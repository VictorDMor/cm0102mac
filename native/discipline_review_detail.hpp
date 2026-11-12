#pragma once
// DISCIPLINE.CPP review block: the draw table, the callees and the person/entry fields it reaches.
#include "discipline_bans_detail.hpp"

namespace cm::aot::discipline {
namespace review {
// The game's draw table (TOC words): the cell holding the base added to each entry, the first entry's
// address, the cell holding the last entry's address and the cell holding the cursor.
namespace draw_toc {
inline constexpr std::int32_t base_cell = -0x7b90, first = -0x7b8c, last_cell = -0x7b88, cursor_cell = -0x7b84;
}
struct DrawTable {
    std::uint32_t base_cell, first, last_cell, cursor_cell;
};
inline DrawTable draw_table(const guest::Cpu& c) {
    return {c.toc(draw_toc::base_cell), c.toc(draw_toc::first), c.toc(draw_toc::last_cell), c.toc(draw_toc::cursor_cell)};
}
inline constexpr std::uint32_t redraw = 0x1077b770;  // the generator: the new base, at each wrap of the table

// The cursor moves two bytes on from `from` (the value the original holds: a fresh read, or the position it reloaded
// after its last draw); past the last entry (unsigned) it goes back to the first and the base is redrawn
// (`back` is the address after the original's `bl`). Returns the cursor as the original reloads it.
inline std::uint32_t step(guest::Cpu& c, const DrawTable& t, std::uint32_t from, std::uint32_t back) {
    const auto last = c.u32(t.last_cell);
    const auto next = from + 2;
    c.put32(t.cursor_cell, next);
    if (next > last) {
        c.put32(t.cursor_cell, t.first);
        c.call(redraw, back);
        c.put32(t.base_cell, c.r(3) & 0xffff);
    }
    return c.u32(t.cursor_cell);
}
// The draw at `position`: its entry plus the base (wrapping 32 bits).
inline std::uint32_t draw_sum(const guest::Cpu& c, const DrawTable& t, std::uint32_t position) {
    const auto base = c.u32(t.base_cell);
    return std::uint32_t(c.u16(position)) + base;
}
// `divw` as the generated code does it: the quotient of the words, truncated (no trap at 0x80000000 / -1).
inline std::int32_t divide(std::int32_t a, std::int32_t b) { return std::int32_t(std::int64_t(a) / b); }
// The signed remainder the small arm of the sampler takes: sum - sum/bound*bound, negated for a bound of 0 or below.
inline std::int32_t oriented_remainder(std::uint32_t sum, std::int32_t bound) {
    const auto quotient = divide(std::int32_t(sum), bound);
    const auto remainder = sum - std::uint32_t(quotient) * std::uint32_t(bound);
    return std::int32_t(bound > 0 ? remainder : 0u - remainder);
}

// The entry the ruling is for (RULING_BODY.CPP's per-player match record).
namespace entry_field {
inline constexpr std::uint32_t person = 0, first_flag = 0x2e, second_flag = 0x2f, cautioned = 0x48;
inline constexpr std::uint8_t ruling_case = 12;
}
// The person record (0x70 bytes): +0x24 an object (null: none) the ruling tests; `human` names it below.
inline constexpr std::uint32_t person_manager = 0x24;
// The object at person+0x24 (its +0 is the id the tables below are indexed by), +0x90 a pointer to a record (bytes +0x63, +0x66, +0x67), +0x1ec a halfword.
namespace human {
inline constexpr std::uint32_t id = 0, conduct = 0x90, reputation = 0x1ec;
inline constexpr std::uint32_t conduct_limit_byte = 0x63, conduct_first = 0x66, conduct_second = 0x67;
inline constexpr std::int16_t reputation_floor = 0x1482;
}
namespace globals {
inline constexpr std::int32_t news = -0x7b94, standing = -0x7984, club_table = -0x79f0, manager = -0x7aa4,
                              doubles = -0x5610;
}
namespace outside {
inline constexpr std::uint32_t date_construct = 0x10715880, key_dates = 0x101929b0, date_gap = 0x10718130,
                               human_flag = 0x1014c2e0, person_standing = 0x100dbbe0, news_ruling = 0x106a15f0,
                               tell_body = 0x100e0700;
}
}
}
