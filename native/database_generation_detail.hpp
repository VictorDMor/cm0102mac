#pragma once
// DATABASE.CPP's generation block: the random generator's entry, the sampling
// arithmetic the compiler emitted at every draw, and the record layouts the
// routines read. Meanings marked "inferred" come from how the routines use them.
#include "guest_abi.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include <bit>

namespace cm::aot::database::generation {
using guest::Cpu;
using guest::compare;
using guest::compare_logical;

// The game's random generator (rng_core): one 15-bit sample per call, from the
// state word in the TOC at -0x34c4. Called through the dispatcher, so the C++
// owner (or the original, under the checker) runs at the original's return address.
constexpr std::uint32_t random_entry = 0x1077b770;

// Registers of one "sample * factor / 32768" expression (the same instruction
// sequence at every draw): mullw lo, mulhw hi, a scratch for the mixed word,
// the register srawi writes and the one that holds the result.
struct Product {
    unsigned lo, hi, scratch, shifted, result;
};
// result = trunc((factor * sample) / 32768) for the signed 64-bit product, as
// mullw/mulhw, rlwinm/rlwimi (the product shifted right by 15 in 32 bits),
// srawi (its carry records a negative product that lost one bits) and addze.
// Every register and XER[CA] is left as the instructions leave them.
inline std::uint32_t scaled_sample(Cpu& c, std::uint32_t factor, std::uint32_t sample, const Product& r) {
    const auto product = std::int64_t(std::int32_t(factor)) * std::int32_t(sample);
    const auto lo = std::uint32_t(product), hi = std::uint32_t(product >> 32);
    c.set(r.lo, lo);
    c.set(r.hi, hi);
    const auto rotated = std::rotl(lo, 17);
    c.set(r.result, rotated);
    c.set(r.scratch, hi);
    const auto mixed = (hi & ~0x7fffu) | (lo & 0x7fffu);  // rlwimi r,lo,0,17,31
    c.set(r.scratch, mixed);
    const auto inserted = (rotated & 0x1ffffu) | (std::rotl(hi, 17) & 0xfffe0000u);  // rlwimi r,hi,17,0,14
    c.set(r.result, inserted);
    const bool carry = std::int32_t(mixed) < 0 && (mixed & 0x7fffu) != 0;  // srawi
    c.set(r.shifted, std::uint32_t(std::int32_t(mixed) >> 15));
    const auto sum = inserted + (carry ? 1u : 0u);
    c.set(r.result, sum);
    c.s.wr<1>(guest::reg::xer_ca, sum < inserted);  // addze's carry out
    return sum;
}

// Loads `address` into register `n` (lwz / lbz / lhz) and answers the value.
inline std::uint32_t load32(Cpu& c, unsigned n, std::uint32_t address) {
    const auto value = c.u32(address);
    c.set(n, value);
    return value;
}
inline std::uint32_t load8(Cpu& c, unsigned n, std::uint32_t address) {
    const auto value = c.u8(address);
    c.set(n, value);
    return value;
}
inline std::uint32_t extsb(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }

// The tables the name pickers (0x100c42b0, 0x100c4ba0) index: a table of 0x24-byte
// rows per nation (inferred: the person's nation at +0x68) that holds the first
// index and the count of its entries, and a table of 0x2c-byte entries
// with a signed byte at +0x28 (inferred: how common the entry is).
namespace nation_row {
constexpr std::int32_t table_slot = -0x7580;  // address of the word that holds the table
constexpr std::uint32_t stride = 0x24;
}
namespace name_entry {
constexpr std::uint32_t stride = 0x2c, weight = 0x28;
}
constexpr std::uint32_t person_nation = 0x68;
}

namespace cm::aot::database::generation {
// Three-slot preference lists: a club keeps two at +0x5c and +0x68 (inferred: favoured and
// disliked staff), a person's preferences record (+0x18) four, at +0x4, +0x10, +0x1c, +0x28.
constexpr std::uint32_t slot_stride = 4, slot_count = 3;
constexpr std::uint32_t person_preferences = 0x18;
// The pool the "which slot to replace" draws read (TOC words that point at words): the
// cursor into a pool of 16-bit samples (-0x7b84), the salt added to each sample (-0x7b90),
// the pool's end (-0x7b88) and its start (-0x7b8c, the value written to the cursor on wrap).
constexpr std::int32_t pool_cursor_slot = -0x7b84, pool_salt_slot = -0x7b90, pool_end_slot = -0x7b88,
                       pool_start_slot = -0x7b8c;

// Where the original keeps the cursor and the end of the pool in registers around the draw.
struct PoolRegisters {
    unsigned cursor, end;
};
// Moves the cursor two bytes on; past the end it restarts at `start` and draws one random
// sample, which becomes the salt (low 16 bits). `back` is the return address of the draw.
inline void advance_pool(Cpu& c, PoolRegisters regs, std::uint32_t cursor_at, std::uint32_t end_at,
                         std::uint32_t salt_at, std::uint32_t start, std::uint32_t back) {
    const auto cursor = load32(c, regs.cursor, cursor_at);
    const auto end = load32(c, regs.end, end_at);
    const auto next = cursor + 2;
    c.set(regs.cursor, next);
    const bool wrapped = compare_logical(c, next, end) > 0;
    c.put32(cursor_at, next);
    if (!wrapped) return;
    c.put32(cursor_at, start);
    c.call(random_entry, back);
    c.set(0, c.r(3) & 0xffff);
    c.put32(salt_at, c.r(0));
}
// The slot (0-2) the pool's current sample picks: (sample + salt) mod 3 with the sign of
// the 32-bit sum (the multiply-high reciprocal of 3), -1 for a negative remainder.
inline int pool_choice(Cpu& c, std::uint32_t cursor_at, std::uint32_t salt_at) {
    const auto cursor = load32(c, 4, cursor_at);
    c.set(3, 0x55550000);
    const auto salt = load32(c, 0, salt_at);
    c.set(3, 0x55555556);
    const auto sample = std::uint32_t(c.u16(cursor));
    c.set(4, sample);
    c.set(4, sample + salt);
    const auto sum = std::int32_t(c.r(4));
    const auto quotient = std::int32_t((std::int64_t(0x55555556) * sum) >> 32);
    c.set(3, std::uint32_t(quotient));
    c.set(0, std::uint32_t(quotient) >> 31);
    c.set(0, std::uint32_t(quotient) + c.r(0));
    c.set(0, c.r(0) * 3);
    const auto remainder = std::int32_t(std::uint32_t(sum) - c.r(0));
    c.set(0, std::uint32_t(remainder));
    const auto against_one = compare(c, remainder, 1);
    if (against_one == 0) return 1;
    if (against_one > 0) return compare(c, remainder, 3) >= 0 ? -1 : 2;
    return compare(c, remainder, 0) >= 0 ? 0 : -1;
}

// PowerPC rotate-and-mask instructions (IBM bit numbering: bit 0 is the most significant) and
// the XER[CA] arithmetic the compiler's 64-bit shifts use.
constexpr std::uint32_t mask_bits(unsigned mb, unsigned me) {
    std::uint32_t mask = 0;
    for (unsigned i = 0; i < 32; ++i)
        if (mb <= me ? (i >= mb && i <= me) : (i >= mb || i <= me)) mask |= 0x80000000u >> i;
    return mask;
}
constexpr std::uint32_t rlwinm(std::uint32_t value, unsigned shift, unsigned mb, unsigned me) {
    return std::rotl(value, int(shift)) & mask_bits(mb, me);
}
constexpr std::uint32_t rlwimi(std::uint32_t into, std::uint32_t value, unsigned shift, unsigned mb, unsigned me) {
    const auto mask = mask_bits(mb, me);
    return (std::rotl(value, int(shift)) & mask) | (into & ~mask);
}
inline std::uint32_t srawi(Cpu& c, std::uint32_t value, unsigned count) {
    c.s.wr<1>(guest::reg::xer_ca, std::int32_t(value) < 0 && (value & ((1u << count) - 1)) != 0);
    return std::uint32_t(std::int32_t(value) >> count);
}
inline std::uint32_t addze(Cpu& c, std::uint32_t value) {
    const std::uint32_t carry = c.s.r<1>(guest::reg::xer_ca) & 1;
    c.s.wr<1>(guest::reg::xer_ca, value + carry < value);
    return value + carry;
}

// The wide-range scaling of a sample (c53e0's and range_sample's first level): the pool sample plus salt, reduced
// modulo 65535 by multiply-high, becomes a fraction of the bound in floating point,
// trunc((bound - bias) / divisor * (remainder - bias)), left in `result_register` (through the stack word it is
// stored to). Also stores the conversion words below the new stack pointer and leaves the registers and f0-f2 as the
// compiler did. Answers the bound * 10 / 65535 (the next level's bound) in two parts: r0 holds its shifted value and
// r3 its sign bit; the caller adds them (the two callers add into different registers).
struct WideScale {
    std::uint32_t shifted, sign;
};
inline WideScale wide_scale(Cpu& c, std::uint32_t constants, std::uint32_t bound, unsigned cursor_register,
                            unsigned salt_register, unsigned result_register) {
    using guest::Fpu;
    constexpr std::uint32_t conversion_high = 0x43300000, reciprocal = 0x80008001, word_range = 0xffff;
    const auto sp = c.sp();
    c.set(3, bound ^ 0x80000000u);
    c.set(0, conversion_high);
    c.put32(sp + 0x4c, c.r(3));
    c.set(6, 0x80010000);
    const auto cursor = load32(c, 7, c.r(cursor_register));
    c.set(4, conversion_high);
    c.put32(sp + 0x48, c.r(0));
    c.set(5, 0x10000);
    c.set_f(2, c.f64(constants + 0x18));
    c.set(3, 0x80010000);
    c.set_f(1, std::bit_cast<double>(std::uint64_t(c.u32(sp + 0x48)) << 32 | c.u32(sp + 0x4c)));
    c.set(6, reciprocal);
    c.set_f(0, c.f64(constants + 0x10));
    c.set(5, word_range);
    Fpu fpu(c);
    c.set_f(1, fpu.fsub(c.f(1), c.f(2)));
    c.set(8, c.u16(cursor));
    const auto salt = load32(c, 7, c.r(salt_register));
    c.set(0, bound * 10);
    c.set(3, reciprocal);
    c.put32(sp + 0x40, c.r(4));
    c.set_f(2, fpu.fdiv(c.f(1), c.f(0)));
    c.set(7, c.r(8) + salt);
    // The remainder of the sum by 65535, as multiply-high by the reciprocal and a shift.
    c.set(4, std::uint32_t((std::int64_t(std::int32_t(reciprocal)) * std::int32_t(c.r(7))) >> 32));
    c.set_f(1, c.f64(constants + 0x18));
    c.set(4, c.r(4) + c.r(7));
    c.set(4, srawi(c, c.r(4), 15));
    c.set(6, c.r(4) >> 31);
    c.set(4, c.r(4) + c.r(6));
    c.set(4, c.r(4) * c.r(5));
    c.set(result_register, c.r(7) - c.r(4));
    c.set(4, c.r(result_register) ^ 0x80000000u);
    c.put32(sp + 0x44, c.r(4));
    c.set(3, std::uint32_t((std::int64_t(std::int32_t(c.r(3))) * std::int32_t(c.r(0))) >> 32));
    c.set_f(0, std::bit_cast<double>(std::uint64_t(c.u32(sp + 0x40)) << 32 | c.u32(sp + 0x44)));
    c.set_f(0, fpu.fsub(c.f(0), c.f(1)));
    c.set(0, c.r(3) + c.r(0));
    c.set(0, srawi(c, c.r(0), 15));
    c.set_f(0, fpu.fmul(c.f(2), c.f(0)));
    c.set(3, c.r(0) >> 31);
    const WideScale next{c.r(0), c.r(3)};
    c.put64(sp + 0x38, fpu.fctiwz(0, c.f(0)));
    c.set(result_register, c.u32(sp + 0x3c));
    return next;
}

// The pool's TOC words (the addresses of the salt, the end and the cursor words, the start value and the constant
// pool): every pool user loads them at its entry and keeps them in non-volatile registers.
inline constexpr std::int32_t narrow_limit = 0xffff;  // up to this bound the sample's remainder is exact
struct Pool {
    std::uint32_t salt_at, start, end_at, cursor_at, constants;
};
inline Pool load_pool(Cpu& c) {
    return {c.toc(pool_salt_slot), c.toc(pool_start_slot), c.toc(pool_end_slot), c.toc(pool_cursor_slot),
            c.toc(-0x5614)};
}
// Moves the cursor on (a wrap draws from rng_core at return address `back`), as advance_pool, with r3 and r0 the
// scratch registers of every inline copy.
inline void advance(Cpu& c, const Pool& pool, std::uint32_t back) {
    advance_pool(c, {3, 0}, pool.cursor_at, pool.end_at, pool.salt_at, pool.start, back);
}
// The inline draw below `bound` once the pool has advanced: |(sample + salt) % bound| (the remainder takes the
// bound's sign, so a bound of 0 or less negates it), from the pool's current sample. Precondition: |bound| <=
// narrow_limit; every caller bounds it by a byte-sized expression, and the original's tests of that limit (r3, cr0)
// are overwritten here by the compare of the bound and the sum. Leaves cr0 = compare(bound, 0), r3 = sample + salt
// and `sample_register` = the sample; the caller keeps the result.
inline std::int32_t draw_below(Cpu& c, const Pool& pool, std::int32_t bound, unsigned sample_register) {
    const auto cursor = c.u32(pool.cursor_at);
    compare(c, bound);
    const auto sample = std::uint32_t(c.u16(cursor));
    c.set(sample_register, sample);
    const auto sum = sample + c.u32(pool.salt_at);
    c.set(3, sum);
    const auto quotient = std::uint32_t(divide_signed<4>(sum, std::uint32_t(bound)));
    const auto remainder = sum - quotient * std::uint32_t(bound);
    return std::int32_t(bound > 0 ? remainder : 0u - remainder);
}
}
