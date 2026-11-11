#pragma once
// Pieces the assessment block shares: the module's draw table and the int-to-double conversion.
#include "guest_fpu.hpp"
#include "manager_manager_detail.hpp"
#include <bit>

namespace cm::aot::manager_manager::assess {
// TOC slots (the original's) the block reads.
namespace toc_slot {
constexpr std::int32_t pool = -0x5368, constants = -0x536c, message = -0x7ba8, success = -0x7bac,
                       news_desk = -0x7bf0;
}
// Pool constants (TOC -0x536c).
constexpr std::uint32_t pool_scale_a = 0x00, pool_scale_b = 0x10, pool_scale_c = 0x20, pool_magic = 0x38;

inline std::int16_t half(std::uint32_t value) { return std::int16_t(std::uint16_t(value)); }

// The module's draw table, stepped inline as the original does (cursor +2, past the last entry back to the
// first with the base redrawn through 0x1077b770). `back` is the address after that `bl`.
class Draws {
public:
    explicit Draws(guest::Cpu& cpu) : c(cpu), t(draw_table::cells(cpu)) {}
    std::int32_t draw(std::uint32_t back) {
        const auto cursor = c.u32(t.cursor_cell) + 2;
        c.put32(t.cursor_cell, cursor);
        if (cursor > c.u32(t.last_cell)) {
            c.put32(t.cursor_cell, t.first);
            c.call(draw_table::redraw, back);
            c.put32(t.base_cell, c.r(3) & 0xffff);
        }
        return std::int32_t(c.u16(c.u32(t.cursor_cell)) + c.u32(t.base_cell));
    }
    // A draw below the byte n (n != 0): the table's number modulo n, with n's sign. The original's floating-point
    // branch for |n| >= 0x10000 (it calls 0x10195e10) cannot run: n is always the sign-extended byte of a record.
    std::int32_t below(std::int8_t n, std::uint32_t back) {
        const auto rest = remainder(draw(back), n);
        return n > 0 ? rest : -rest;
    }
    // draw % n for a positive constant n (the draw is never negative: a halfword plus a base word).
    std::int32_t modulo(std::int32_t n, std::uint32_t back) { return remainder(draw(back), n); }

private:
    guest::Cpu& c;
    draw_table::Table t;
};

// The assertion the block reports for a null argument (the original's 0x10739890, 0x10768810, 0x1077b720,
// 0x10769b60 and the success flag): `high` and `low` are the frame offsets of the two buffers it passes, `line`
// the source line, `back` the four return addresses of the calls.
void report_assertion(guest::Cpu& c, const guest::Frame& frame, std::uint32_t high, std::uint32_t low,
                      std::uint32_t line, const std::uint32_t (&back)[4]);
}
