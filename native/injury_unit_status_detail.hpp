#pragma once
// INJURY.CPP status block: what its files share (names for the words the routines read, the sample draw
// with the registers held at its redraw call, the original's int-to-double).
#include "guest_fpu.hpp"
#include "injury_unit_detail.hpp"
#include <bit>

namespace cm::aot::injury_unit::status_detail {
using guest::Cpu;
// Words of the table of contents the block reads besides injury_unit_detail.hpp's.
namespace cell {
inline constexpr std::int32_t person_stats = -0x7b9c,   // the cell holding the per-person 100-byte rows
                              window_a = -0x5450,       // two dates the daily update measures the days from and to
                              window_b = -0x544c,
                              checked_date = -0x5448,   // the last day the club check was made (a static of 0x10187a40)
                              club_hooks = -0x7a64,     // the cell holding the object 0x102ae9c0 / 0x102af650 are asked of
                              match_date = -0x7a9c,     // the date 0x10186fc0 compares today with
                              float_one = -0x5444;      // a single-precision constant (the divisor of the rounding)
}
// Bytes of a person record (0x70) the block reads.
namespace person {
inline constexpr std::uint32_t index = 0x00, profile = 0x14, club_entry = 0x20, club = 0x24, form = 0x5c,
                               temper = 0x67, flags = 0x6a;
}
// Per-person 100-byte row.
namespace stats {
inline constexpr std::uint32_t size = 100, counter = 10, flags = 0xd;
}
// Bytes of a status row (injury_unit_detail.hpp has +0xc, +0x10 and +0x22) and the rest of the injury record.
namespace row {
inline constexpr std::uint32_t date = 0x00, fitness = 0x08, sharpness = 0x0a, condition = 0x0c;
}
namespace record {
inline constexpr std::uint32_t kind = 0x00;
}
// The date object (DATE.CPP): a signed day halfword, an unsigned year halfword (the major part) and a byte.
namespace date {
inline constexpr std::uint32_t day_part = 0, year_part = 2, byte_part = 4;
}
// The statics of the daily update (0x10187a40), r2-relative bytes and words.
namespace statics {
inline constexpr std::int32_t cached_club = -0x3944, flag_cached = -0x394b, init_cache = -0x394c,
                              init_a = -0x3947, init_b = -0x3948, init_c = -0x3949, init_d = -0x394a;
}
// The non-volatile registers r13-r31 as the original holds them at a call. They start as the caller's values
// (what the original leaves alone until it sets them); `sync()` makes them the guest's before a call, because the
// callees save them into their frames.
class Registers {
public:
    explicit Registers(Cpu& cpu) : c(cpu) {
        for (unsigned n = 13; n < 32; ++n) r[n] = c.r(n);
    }
    std::uint32_t& operator[](unsigned n) { return r[n]; }
    void sync() {
        for (unsigned n = 13; n < 32; ++n) c.set(n, r[n]);
    }

private:
    Cpu& c;
    std::uint32_t r[32] = {};
};

// The 0x10717dd0 result: days (signed halfword) and years (unsigned halfword) apart.
inline std::int32_t days_of(Cpu& c, std::uint32_t at) { return c.i16(at) + std::int32_t(c.u16(at + 2)) * 365; }

// The word the original keeps on the stack for an int-to-double: the magic high word and the biased integer.
inline double magic(std::int32_t x) {
    return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u));
}
// `magic` as the original stores it, and the double the constant `pool_magic` leaves.
inline double to_double(guest::Fpu& fpu, Cpu& c, std::uint32_t pool, std::int32_t x) {
    return fpu.fsub(magic(x), c.f64(pool + 8));
}
inline double to_single_double(guest::Fpu& fpu, Cpu& c, std::uint32_t pool, std::int32_t x) {
    return fpu.fsubs(magic(x), c.f64(pool + 8));
}

// The sample draw: as injury_unit::draw, except that `hold()` sets the non-volatile registers just before the
// redraw call (rare), so a draw costs nothing more when it does not redraw.
template <class Hold>
inline std::uint32_t draw_sample(Cpu& c, std::uint32_t back, Hold&& hold) {
    const auto cursor_cell = c.toc(sample_table::cursor), base_cell = c.toc(sample_table::base);
    const auto next = c.u32(cursor_cell) + 2;
    c.put32(cursor_cell, next);
    if (next > c.u32(c.toc(sample_table::last))) {
        c.put32(cursor_cell, c.toc(sample_table::first));
        hold();
        c.call(sample_table::redraw, back);
        c.put32(base_cell, c.r(3) & 0xffff);
    }
    return std::uint32_t(c.u16(c.u32(cursor_cell))) + c.u32(base_cell);
}
// `sample` taken as a remainder of the range `n` (a signed value within +-0xffff, not 0): the sign of the
// range sets the sign of the answer.
inline std::int32_t remainder_in(std::uint32_t sample, std::int32_t n) {
    // divw, mullw, subf as the generated code does them (the sample is the cell's base plus a halfword, so it may
    // be any word: the signed quotient is truncated in 64 bits and wraps, as for 0x80000000 / -1).
    const auto quotient = std::int32_t(std::int64_t(std::int32_t(sample)) / n);
    const auto rest = std::int32_t(sample - std::uint32_t(quotient) * std::uint32_t(n));
    return n > 0 ? rest : std::int32_t(0u - std::uint32_t(rest));
}
inline bool fits_one_draw(std::int32_t n) { return n >= -0xffff && n <= 0xffff; }

// The signed compare the original compiles to for two halfword values (the year parts of two dates, a day count
// against a limit): the sign of `(x >> 1) - (x & a)` with x = a ^ b, `a` being the operand the original ANDs with.
inline bool halfword_greater(std::int32_t a, std::int32_t b) {
    const std::int32_t x = a ^ b;
    return ((x >> 1) - (x & a)) < 0;
}
}
