#pragma once
// What the values block shares: the compiler's division by 65535 and by 10, the
// int-to-double stack temporaries, and the press's draw expanded in line (a draw in
// [0, n): a remainder within 16 bits, otherwise a float share of n plus a finer draw
// from 0x10647c00 for its tenth).
#include "guest_fpu.hpp"
#include "media_detail.hpp"
#include "media_values.hpp"

namespace cm::aot::media::detail {
inline std::int32_t multiply_high(std::int32_t a, std::int32_t b) {
    return std::int32_t((std::int64_t(a) * std::int64_t(b)) >> 32);
}
inline std::int32_t half_word(std::uint32_t value) { return std::int32_t(std::int16_t(value)); }
inline std::int32_t byte_word(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
// A quotient as the compiler forms it: the arithmetic shift of the magic multiply, its
// sign bit (added to round toward zero) and the result.
struct Quotient {
    std::int32_t value;
    std::uint32_t shifted, sign;
};
// v / 65535 (magic 0x80008001, shift 15).
inline Quotient by_65535(std::uint32_t v) {
    const auto sum = std::uint32_t(multiply_high(std::int32_t(0x80008001), std::int32_t(v))) + v;
    const auto shifted = std::uint32_t(std::int32_t(sum) >> 15), sign = shifted >> 31;
    return {std::int32_t(shifted + sign), shifted, sign};
}
// (v + 5) / 10 (magic 0x66666667, shift 2): a finer draw rounded to the coarser step.
inline Quotient rounded_tenth(std::int32_t v) {
    const auto shifted = std::uint32_t(multiply_high(0x66666667, std::int32_t(std::uint32_t(v) + 5)) >> 2), sign = shifted >> 31;
    return {std::int32_t(shifted + sign), shifted, sign};
}
// An integer through the pool's bias trick: the stack temporary at `at` holds the double
// 0x43300000:(low word); the original reads it back and subtracts the bias.
inline double stacked_integer(Cpu& c, std::uint32_t at, std::uint32_t low) {
    c.put32(at, 0x43300000);
    c.put32(at + 4, low);
    return c.f64(at);
}
inline std::uint32_t signed_word(std::int32_t value) { return std::uint32_t(value) ^ 0x80000000u; }
// divw by a divisor the caller knows is nonzero, and the remainder idiom after it.
inline std::int32_t divide(std::int32_t a, std::int32_t b) {
    return std::int32_t(std::uint32_t(divide_signed<4>(std::uint32_t(a), std::uint32_t(b))));
}
inline std::int32_t remainder_of(std::int32_t a, std::int32_t b) {
    return std::int32_t(std::uint32_t(a) - std::uint32_t(divide(a, b)) * std::uint32_t(b));
}
// The press item the routines build on their stack and post: 0x102bdf90 starts it with its kind,
// 0x102cb4b0 adds the numbered fields and 0x102cc140 posts it to the shared queue. `back` is the
// address after the original `bl`.
struct Item {
    Cpu& c;
    std::uint32_t at;
    void begin(std::uint32_t kind, std::uint32_t back) const { Program{c}.call(0x102bdf90, back, {at, kind, 0}); }
    void put(std::uint32_t index, std::uint32_t value, std::uint32_t back) const { Program{c}.call(0x102cb4b0, back, {at, index, value}); }
    void post(std::uint32_t subject, std::uint32_t back) const {
        Program{c}.call(0x102cc140, back, {log(), at, subject});
    }
    // The same record handed to the log's other doors: 0x102cc400 for a manager's page, 0x102cc8e0 for the headlines.
    void post_for_manager(std::uint32_t manager, std::uint32_t back) const { Program{c}.call(0x102cc400, back, {log(), at, manager}); }
    void post_headline(std::uint32_t back) const { Program{c}.call(0x102cc8e0, back, {log(), at}); }
    // The log: the object behind the TOC slot -0x7bf0.
    std::uint32_t log() const { return c.u32(c.toc(-0x7bf0)); }
};
// The id behind a link of a record: record -> link at `offset` -> word at +0x20.
inline std::uint32_t linked(Cpu& c, std::uint32_t record, std::uint32_t offset) { return c.u32(c.u32(record + offset) + 0x20); }
// A non-volatile register that already holds a value of the original (a draw's result, say): reading
// and assigning touch only the register. Kept<R> sets it on construction; this does not.
template <unsigned R>
class Held {
public:
    explicit Held(Cpu& c) : c(c) {}
    Held& operator=(std::uint32_t value) {
        c.set(R, value);
        return *this;
    }
    operator std::uint32_t() const { return c.r(R); }

private:
    Cpu& c;
};
// The pool's floats: the 65535 divisor at +0 and the integer bias at +8.
struct Floats {
    double divisor, bias;
    explicit Floats(Cpu& c) : Floats(c, c.toc(slot::floats)) {}
    Floats(Cpu& c, std::uint32_t at) : divisor(c.f64(at)), bias(c.f64(at + 8)) {}
};

// The draw's float part, for n beyond 16 bits: part = trunc(n / 65535.0 * (x mod 65535)) and
// the finer draw's range, n * 10 / 65535 (32-bit wrapping). x is the table's sample.
struct WideDraw {
    std::uint32_t entry, salt, x;
    Quotient whole;
    std::uint32_t multiple, rest;  // whole * 65535, x - multiple
    std::uint32_t tenfold;         // n * 10
    Quotient coarse;               // tenfold / 65535
    std::int32_t part;
};
// The temporaries: n's at `n_at`, the remainder's at `rest_at`, fctiwz's result at `part_at`.
inline WideDraw wide_draw(Cpu& c, Fpu& fpu, const Stream& rng, const Floats& pool, std::int32_t n, std::uint32_t n_at,
                          std::uint32_t rest_at, std::uint32_t part_at) {
    WideDraw d{};
    const double share = fpu.fdiv(fpu.fsub(stacked_integer(c, n_at, signed_word(n)), pool.bias), pool.divisor);
    d.entry = c.u16(c.u32(rng.cursor));
    d.salt = c.u32(rng.seed);
    d.x = d.entry + d.salt;
    d.whole = by_65535(d.x);
    d.multiple = std::uint32_t(d.whole.value) * 0xffff;
    d.rest = d.x - d.multiple;
    d.tenfold = std::uint32_t(n) * 10;
    d.coarse = by_65535(d.tenfold);
    const double rest = fpu.fsub(stacked_integer(c, rest_at, signed_word(std::int32_t(d.rest))), pool.bias);
    const auto bits = fpu.fctiwz(0, fpu.fmul(share, rest));
    c.put64(part_at, bits);
    d.part = std::int32_t(std::uint32_t(bits));
    c.set_f(1, pool.bias);
    return d;
}
// The remainder of the table's sample by n, negated unless n is positive (quirk: the
// sign test is on n, so a negative n gives a draw in (n, 0]).
inline std::int32_t narrow_draw(const Stream& rng, std::int32_t n) {
    const auto rest = remainder_of(std::int32_t(rng.sample()), n);
    return n > 0 ? rest : std::int32_t(0u - std::uint32_t(rest));
}
inline bool beyond_table(std::int32_t n) { return n > 0xffff || n < -0xffff; }

// One inlined random(n) site: the draw in [0, n) the compiler expanded in line, into register
// `result` (kept there across the call, as the original does). The wide form leaves the schedule of
// scratch registers every site shares: r4 the remainder as a signed word, r5 65535, r6 the
// quotient's sign bit, r7 the sample, r8 the table entry.
struct DrawSite {
    std::uint32_t refill_back, call_back;   // after the bl to rand and to 0x10647c00
    std::uint32_t n_at, rest_at, part_at;   // the wide form's stack temporaries (frame offsets)
    unsigned result;
};
// The narrow form of a site, after the refill: the sign test, the remainder and the sample the
// compiler leaves in r3, with the value in register `result`.
inline std::int32_t narrow_site(Cpu& c, const Stream& rng, std::int32_t n, unsigned result) {
    compare(c, n);  // cmpwi n,0 before the remainder: its sign test
    const auto value = narrow_draw(rng, n);
    c.set(3, rng.sample());
    c.set(result, std::uint32_t(value));
    return value;
}
// A site whose range is a byte (|n| <= 128): the table's sample modulo the byte, the sign quirk of
// the remainder as in the wide form. The range is always within 16 bits, so the original's float form
// beside this one can never run here. The value is also left in register R, where the original keeps it.
template <unsigned R>
std::int32_t draw_byte(Cpu& c, const Stream& rng, std::uint32_t refill_back, std::int32_t n) {
    if (!nonzero(c, n)) {
        c.set(R, 0);
        return 0;
    }
    rng.advance(refill_back);
    return narrow_site(c, rng, n, R);
}

// One inlined random(n) site. `pool_of` gives the pool's floats and is called only for the wide form.
template <class PoolOf>
std::int32_t draw_below_with(Cpu& c, const Frame& frame, Fpu& fpu, const Stream& rng, PoolOf&& pool_of, const DrawSite& site, std::int32_t n) {
    if (!nonzero(c, n)) {
        c.set(site.result, 0);
        return 0;
    }
    rng.advance(site.refill_back);
    if (!beyond_table(n)) return narrow_site(c, rng, n, site.result);
    const Floats pool = pool_of();
    const auto d = wide_draw(c, fpu, rng, pool, n, frame.at(std::int32_t(site.n_at)), frame.at(std::int32_t(site.rest_at)), frame.at(std::int32_t(site.part_at)));
    c.set(site.result, std::uint32_t(d.part));
    leave(c, {{4, signed_word(std::int32_t(d.rest))}, {5, 0xffff}, {6, d.whole.sign}, {7, d.x}, {8, d.entry}});
    compare(c, n, n > 0xffff ? 0xffff : -0xffff);
    enter(c, site.call_back, {std::uint32_t(d.coarse.value)});
    const auto finer = random_below(c, d.coarse.value);
    // part + a tenth of the finer draw, at most n - 1 (quirk: no lower bound).
    const auto tenth = rounded_tenth(finer);
    const auto limit = std::int32_t(std::uint32_t(n) - 1);
    const auto total = std::int32_t(std::uint32_t(d.part) + std::uint32_t(tenth.value));
    leave(c, {{3, std::uint32_t(tenth.value)}, {4, tenth.sign}});
    const auto value = compare(c, total, limit) > 0 ? limit : total;
    c.set(site.result, std::uint32_t(value));
    return value;
}
// With the address of the pool's floats (the original keeps it in a register): they are read only by the wide form.
inline std::int32_t draw_below(Cpu& c, const Frame& frame, Fpu& fpu, const Stream& rng, std::uint32_t pool_at, const DrawSite& site, std::int32_t n) {
    return draw_below_with(c, frame, fpu, rng, [&] { return Floats(c, pool_at); }, site, n);
}
// With the floats already read.
inline std::int32_t draw_below(Cpu& c, const Frame& frame, Fpu& fpu, const Stream& rng, const Floats& pool, const DrawSite& site, std::int32_t n) {
    return draw_below_with(c, frame, fpu, rng, [&]() -> const Floats& { return pool; }, site, n);
}

// x mod 2 as the compiler forms it (the sign of x rides along): ((x & 1) ^ s) - s with s the sign bit.
inline std::uint32_t signed_mod2(std::uint32_t x) {
    const auto sign = x >> 31;
    return ((x & 1) ^ sign) - sign;
}

// An inlined random(n) site whose range is a signed byte (or a small sum), held in a register the caller names:
// the draw goes to register `result`. Quirk: a zero n draws nothing and leaves 0 there. The original's wide form
// (|n| beyond 65535) and its call to 0x10647c00 can never run for such a range. `draw_byte<R>` is this with the
// register known when the program is compiled; blocks wrap this for the registers their routine uses.
inline std::int32_t draw_narrow(Cpu& c, const Stream& rng, std::int32_t n, std::uint32_t refill_back, unsigned result) {
    if (!nonzero(c, n)) {
        c.set(result, 0);
        return 0;
    }
    rng.advance(refill_back);
    return narrow_site(c, rng, n, result);
}

// A division by a small constant as the compiler forms it: the magic of the multiply-high, its shift and the divisor.
struct Reciprocal {
    std::int32_t magic;
    unsigned shift;
    std::int32_t divisor;
};
namespace divisors {
constexpr Reciprocal by_3{0x55555556, 0, 3}, by_5{0x66666667, 1, 5}, by_6{0x2aaaaaab, 0, 6}, by_10{0x66666667, 2, 10},
    by_20{0x66666667, 3, 20}, by_25{0x51eb851f, 3, 25};
}
// A sample reduced by such a constant: the multiply-high word, the shifted quotient, its sign bit (added to round
// toward zero) and the remainder: the values the compiler's sequence leaves in the registers around it.
struct Reduced {
    std::uint32_t sample;
    std::int32_t high;
    std::uint32_t shifted, sign;
    std::int32_t rest;
};
inline Reduced reduced_by(std::uint32_t sample, const Reciprocal& by) {
    const auto high = multiply_high(by.magic, std::int32_t(sample));
    const auto shifted = std::uint32_t(high >> by.shift), sign = shifted >> 31;
    return {sample, high, shifted, sign, std::int32_t(sample - (shifted + sign) * std::uint32_t(by.divisor))};
}

// A column (a halfword) of the club's row in the ratings table (76-byte rows, the table behind TOC slot -0x79f0) moved by
// `delta` and held within 1..10000, as the compiler expands it: the table's holder in register `holder` (r3, or r5
// when the routine keeps r3 for something else), the row and the new value left around it. Nothing is done for club 0.
inline void move_club_rating(Cpu& c, std::uint32_t club, unsigned column, std::int32_t delta, unsigned holder = 3) {
    const bool known = nonzero_logical(c, club);
    const auto table = c.u32(c.toc(-0x79f0));
    c.set(holder, table);
    if (!known) return;
    const auto row = c.u32(table) + c.u32(club) * 0x4c;
    c.put16(row + column, extended(c.i16(row + column)) + std::uint32_t(delta));
    std::uint32_t value;
    if (compare(c, c.i16(row + column), 1) < 0) value = 1;
    else if (compare(c, c.i16(row + column), 0x2710) > 0) value = 0x2710;
    else value = extended(c.i16(row + column));
    if (holder == 3) leave(c, {{3, row}, {4, value}, {5, row}});
    else leave(c, {{3, row}, {4, value}});
    c.put16(row + column, value);
}
}

// Other blocks reach these helpers as `values` as well as `detail`.
namespace cm::aot::media { namespace values = detail; }
