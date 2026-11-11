#pragma once
// MANAGER_MANAGER.CPP's shared record views and constants. Offsets are the
// original's; meanings come from the callers and are named as far as the code
// shows them.
#include "guest_abi.hpp"
#include "guest_values.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// TOC slots of the unit (tools/manager_manager_audit.py): -0x5368 is its
// source-name pool ("MANAGER_MANAGER.CPP", the assert texts after it).
namespace slots {
constexpr std::int32_t pool = -0x5368;
// The game's shared assert buffer and the success flag the assert clears.
constexpr std::int32_t message = -0x7ba8, success = -0x7bac;
}
// The game's linked list (the container classes of the runtime library): a
// 16-bit count, the first node and a cursor that list_next (0x10738e00) moves.
namespace list {
constexpr std::uint32_t count = 0x0, head = 0x4, cursor = 0xc;  // count: int16
}
namespace list_node {
constexpr std::uint32_t data = 0x0, next = 0x8;
}
// The module's draw table: TOC slots of the cell with its base (a value redrawn at each wrap),
// its first entry, the cell with its last entry and the cell with the cursor (a pointer to the
// next halfword). The draw 0x10195e10 and its inlined copies in the review and the post-match
// update step it the same way.
namespace draw_table {
constexpr std::int32_t base = -0x7b90, first = -0x7b8c, last = -0x7b88, cursor = -0x7b84;
constexpr std::uint32_t redraw = 0x1077b770;  // the new base, at each wrap
struct Table {
    std::uint32_t base_cell, first, last_cell, cursor_cell;
};
inline Table cells(const guest::Cpu& c) { return {c.toc(base), c.toc(first), c.toc(last), c.toc(cursor)}; }
// The cursor (the one the caller holds) advances by two halfwords, left in r3; past the last
// entry it returns to the first and the base is redrawn. `back` is the address after the `bl`.
inline void step_from(guest::Cpu& c, const Table& t, std::uint32_t cursor, std::uint32_t back) {
    const auto end = c.u32(t.last_cell);
    cursor += 2;
    c.set(3, cursor);
    c.put32(t.cursor_cell, cursor);
    guest::compare_logical(c, cursor, end);
    if (cursor > end) {
        c.put32(t.cursor_cell, t.first);
        c.call(redraw, back);
        c.put32(t.base_cell, c.r(3) & 0xffff);
    }
}
inline void step(guest::Cpu& c, const Table& t, std::uint32_t back) { step_from(c, t, c.u32(t.cursor_cell), back); }
}
// `divw` as the generated code does it: the quotient of the sign-extended words, truncated (the
// callers never divide by 0 or 0x80000000 by -1), and the remainder of a divw, mullw, subf.
inline std::int32_t divide(std::int32_t a, std::int32_t b) { return std::int32_t(std::int64_t(a) / b); }
inline std::int32_t remainder(std::int32_t a, std::int32_t b) {
    return std::int32_t(std::uint32_t(a) - std::uint32_t(divide(a, b)) * std::uint32_t(b));
}
// An epilogue's `lmw rN,-k(r1)` leaves Ghidra's effective-address temporary
// (register byte 2432, compared at imports) one word past r31's slot: the
// caller's stack pointer for the usual save area right below it.
// extsb: a byte sign-extended to a word.
inline std::uint32_t sext8(std::uint32_t byte) { return std::uint32_t(std::int32_t(std::int8_t(byte))); }
// The news message record: 0x102bdf90 starts one of `kind` in `buffer`, 0x102cb4b0 sets its field `index`; `back` is
// the address after the original's `bl`.
inline void start_message(guest::Cpu& c, std::uint32_t buffer, std::uint32_t kind, std::uint32_t back) {
    c.set({{3, buffer}, {4, kind}, {5, 0}});
    c.call(0x102bdf90, back);
}
inline void set_field(guest::Cpu& c, std::uint32_t buffer, std::uint32_t index, std::uint32_t value, std::uint32_t back) {
    c.set({{3, buffer}, {4, index}, {5, value}});
    c.call(0x102cb4b0, back);
}
inline void lmw_leaves(guest::Cpu& c, std::uint32_t caller_sp) { c.s.wr<4>(2432, caller_sp); }
}
