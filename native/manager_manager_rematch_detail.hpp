#pragma once
// MANAGER_MANAGER.CPP rematch block (manager_manager_rematch.hpp): the cells, record offsets and
// small helpers its routines share. Offsets are the original's, named by the use the code shows.
#include "guest_abi.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "manager_manager_detail.hpp"
#include <bit>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>

namespace cm::aot::manager_manager::rematch {
using guest::Cpu;

// TOC cells (`lwz rX,slot(r2)`). A "word" cell holds a pointer to the value; a "record" cell holds the
// record's own address.
namespace cell {
constexpr std::int32_t clubs = -0x7be8, club_count = -0x7b50;        // word: club table base, count
constexpr std::int32_t people = -0x7be0, people_count = -0x7be4;     // word: person table (0x70 each), count
constexpr std::int32_t rows = -0x7b9c;                               // word: per-person rows of 100 bytes
constexpr std::int32_t object = -0x7b7c;                             // word: the module object of the callees
constexpr std::int32_t today = -0x7b80, event_date = -0x7a9c;        // record: a date {day i16, year u16, byte}
constexpr std::int32_t news_desk = -0x7bf0, links = -0x7a6c, deals = -0x7b48, dispatcher = -0x7a54,
                       watch = -0x7ba4, leagues = -0x7b5c, clubs_extra = -0x7b28, tactics = -0x7b40;
constexpr std::int32_t report = -0x7bb8;                             // the text buffer of the assertion's sprintf
constexpr std::int32_t pool = -0x536c;                               // doubles
}
// Pool doubles (cell::pool).
namespace pool {
constexpr std::uint32_t scale_a = 0x10, magic = 0x38, divisor = 0x40, scale_b = 0x70, scale_c = 0x140,
                        bar = 0x1d8, bar_random = 0x1e0, scale_d = 0x1d0, scale_e = 0x1e8, scale_f = 0x1f0,
                        scale_g = 0x1f8;
}
namespace person {  // 0x70 bytes
constexpr std::uint32_t size = 0x70, id = 0, home = 0xc, post_a = 0x14, post_b = 0x18, staff = 0x1c, side_club = 0x20,
                        job_club = 0x24, name_a = 0x28, name_b = 0x2c, name_c = 0x30, since_a = 0x4c, since_b = 0x50,
                        outlook = 0x5c, tenure = 0x5e, side_status = 0x60, status = 0x62, level = 0x65, flag = 0x6b;
}
namespace staff {  // the person's record at +0x1c: three ratings (int16)
constexpr std::uint32_t a = 0x28, b = 0x2a, c = 0x2c;
}
namespace club {  // 0x248 bytes
constexpr std::uint32_t size = 0x248, id = 0, region = 0x2c, league = 0x30, manager = 0x90, first_slot = 0x94,
                        second_slot = 0x160, reputation = 0x1ec, name = 0x1ee, slots = 0x98;
}
namespace row {  // the club's row in the table at the object (0x4c bytes)
constexpr std::uint32_t size = 0x4c;
}
namespace offer {  // the 0x28-byte record of the object's offer list (+8)
constexpr std::uint32_t size = 0x28, flag = 0, flag2 = 1, second = 4, chosen = 8, leaving = 0xc, club = 0x10, score = 0x14,
                        date_a = 0x18, date_b = 0x20;
}

inline std::int32_t people_count(const Cpu& c) { return c.i32(c.toc(cell::people_count)); }
inline std::int32_t club_count(const Cpu& c) { return c.i32(c.toc(cell::club_count)); }

inline std::uint32_t out(Cpu& c, std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> args) {
    unsigned n = 3;
    for (const auto value : args) c.set(n++, value);
    c.call(target, back);
    return c.r(3);
}

// The event date (cell::event_date) against today's, the way the original tests it: the halfword at +2
// unsigned first, then, when they are equal, the one at +0 by the xor/shift sequence (a signed "greater").
inline bool event_after_today(const Cpu& c) {
    const auto other = c.toc(cell::event_date), today = c.toc(cell::today);
    const auto other_year = c.u16(other + 2), today_year = c.u16(today + 2);
    if (other_year > today_year) return true;
    if (other_year < today_year) return false;
    const auto a = std::uint32_t(std::int32_t(c.i16(other))), b = std::uint32_t(std::int32_t(c.i16(today)));
    const auto x = a ^ b;
    const auto r = std::uint32_t(std::int32_t(x) >> 1) - (x & a);
    return (r >> 31) != 0;
}

// An int-to-double through the bias constant (the magic word less the pool's), as the original makes it.
inline double to_double(guest::Fpu& fpu, const Cpu& c, std::uint32_t pool_at, std::int32_t x) {
    const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u);
    return fpu.fsub(std::bit_cast<double>(bits), c.f64(pool_at + pool::magic));
}
// trunc(factor * x), the original's fsub/fmul/fctiwz; the integer is the low word.
inline std::int32_t scaled(guest::Fpu& fpu, const Cpu& c, std::uint32_t pool_at, std::uint32_t factor, std::int32_t x) {
    auto f = to_double(fpu, c, pool_at, x);
    f = fpu.fmul(c.f64(pool_at + factor), f);
    return std::int32_t(fpu.fctiwz(0, f));
}

// The module's draw table, inlined all over these routines: one step, then the halfword it points at
// plus the base. `back` is the address after the `bl` of the redraw at that site.
class Draw {
public:
    explicit Draw(Cpu& c) : c(c), t(draw_table::cells(c)) {}
    std::int32_t next(std::uint32_t back) {
        const auto cursor = c.u32(t.cursor_cell) + 2;
        c.put32(t.cursor_cell, cursor);
        if (cursor > c.u32(t.last_cell)) {
            c.put32(t.cursor_cell, t.first);
            c.call(draw_table::redraw, back);
            c.put32(t.base_cell, c.r(3) & 0xffff);
        }
        return std::int32_t(c.u16(c.u32(t.cursor_cell)) + c.u32(t.base_cell));
    }
    // A draw below n (n != 0): the table's number modulo n, with n's sign, when |n| is under 0x10000
    // (a byte-sized bound at the callers that cannot reach more: `small`); otherwise the original's
    // floating-point split, which draws once more with the module's draw (0x10195e10, `back_draw`).
    std::int32_t below(std::int32_t n, std::uint32_t back_step, std::uint32_t back_draw) {
        const auto drawn = next(back_step);
        if (n <= 0xffff && n >= -0xffff) {
            const auto rest = remainder(drawn, n);
            return n > 0 ? rest : -rest;
        }
        return split(n, drawn, back_draw);
    }
    std::int32_t small(std::int32_t n, std::uint32_t back_step) {
        const auto drawn = next(back_step);
        if (n > 0xffff || n < -0xffff) throw std::logic_error("rematch: a byte-sized draw bound beyond 0xffff");
        const auto rest = remainder(drawn, n);
        return n > 0 ? rest : -rest;
    }
    Cpu& cpu() { return c; }

private:
    std::int32_t split(std::int32_t n, std::int32_t drawn, std::uint32_t back_draw);
    Cpu& c;
    draw_table::Table t;
};
}
