// CUP.CPP, block `draws`: 0x1047a9e0, a draw below a bound from the game's sample stream.
//
// The original draws a step of the stream, and for a bound within +-65535 reduces the sample (halfword at the
// cursor + salt) to the bound. Beyond that it splits the draw: a part scaled by floating point (bound / 65535.0
// times the sample's remainder modulo 65535) and a finer part drawn by the same routine from a bound a tenth
// the size, `bound * 10 / 65535`, the product wrapped to 32 bits. The original unrolls that recursion four levels
// and calls itself for the fifth (0x1047afcc). The finer bound is at most 32768 in size (|bound * 10| <= 2^31,
// divided by 65535), so the second level always takes the small arm: the deeper levels (0x1047ab74..0x1047b134
// and the call at 0x1047afcc) cannot run (as LEAGUE.CPP's 0x104a3350, league_base_season.cpp).
//
// r4-r10 at the return are the original's: the small arm and zero leave them as the caller had them (the draw's
// own steps use r3 and r0), the large arm leaves what its two levels computed and the tail's sign bit.
#include "cup_base_detail.hpp"
#include "cup_base_draws.hpp"
#include "guest_fpu.hpp"
#include <bit>
#include <stdexcept>

namespace cm::aot::cup_base::draws {
namespace {
std::int32_t multiply_high(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * b) >> 32); }
constexpr std::int32_t magic_65535 = std::int32_t(0x80008001u);
// The floor of the multiply-high sequence's quotient (before its sign correction) and the corrected quotient:
// the original's signed division by 65535 (srawi 15, then add the sign bit).
std::int32_t floor_65535(std::int32_t value) {
    return std::int32_t(std::uint32_t(multiply_high(magic_65535, value)) + std::uint32_t(value)) >> 15;
}
std::int32_t divide_65535(std::int32_t value) {
    const auto floor = floor_65535(value);
    return std::int32_t(std::uint32_t(floor) + (std::uint32_t(floor) >> 31));
}
std::int32_t divide_ten(std::int32_t value, std::uint32_t& sign) {
    const auto floor = multiply_high(0x66666667, value) >> 2;
    sign = std::uint32_t(floor) >> 31;
    return std::int32_t(std::uint32_t(floor) + sign);
}
// divw's quotient without the host's trap for INT_MIN / -1 (undefined on the PowerPC; the dividend is a halfword plus
// the seed).
std::int32_t divide(std::int32_t dividend, std::int32_t divisor) {
    return divisor == -1 ? std::int32_t(0u - std::uint32_t(dividend)) : dividend / divisor;
}
double as_double(std::int32_t value) {
    return std::bit_cast<double>(std::uint64_t(0x43300000u) << 32 | (std::uint32_t(value) ^ 0x80000000u));
}
}

std::int32_t draw_below(Cpu& c, std::int32_t bound) {
    const Frame frame(c, 0x1047a9e0, 0x80, {.first_gpr = 23});
    const auto pool = Pool::at(c);
    const auto doubles = c.toc(toc::doubles);
    // r23 the bound, r27 the pool's doubles, r28-r31 the stream's cells (all loaded before the first test).
    c.set({{23, std::uint32_t(bound)}, {27, doubles}, {28, pool.salt}, {29, pool.start}, {30, pool.end}, {31, pool.cursor}});
    if (bound == 0) return 0;
    pool.advance(0x1047aa34);
    if (bound <= 0xffff && bound >= -0xffff) {
        const auto sample = std::int32_t(pool.sample());
        const auto remainder = std::int32_t(std::uint32_t(sample) - std::uint32_t(divide(sample, bound)) * std::uint32_t(bound));
        return bound > 0 ? remainder : std::int32_t(0u - std::uint32_t(remainder));
    }
    // The first level: the scaled part from this step's sample.
    guest::Fpu fpu(c);
    const auto product = std::int32_t(std::uint32_t(bound) * 10u);
    const auto product_floor = floor_65535(product);
    const auto sample = std::int32_t(pool.sample());
    const auto sample_quotient = divide_65535(sample);
    const auto remainder = std::int32_t(std::uint32_t(sample) - std::uint32_t(sample_quotient) * 0xffffu);
    auto scale = fpu.fsub(as_double(bound), c.f64(doubles + 8));
    scale = fpu.fdiv(scale, c.f64(doubles));
    const auto part = fpu.fmul(scale, fpu.fsub(as_double(remainder), c.f64(doubles + 8)));
    const auto scaled = std::int32_t(std::uint32_t(fpu.fctiwz(0, part)));
    // r4 the sign bit of the product's floor, r5 the sample's quotient times 65535, r6 65535, r7 the magic, r8 the
    // sample, r9 its halfword (0x1047aa5c..0x1047aad4).
    c.set({{4, std::uint32_t(product_floor) >> 31}, {5, std::uint32_t(sample_quotient) * 0xffffu}, {6, 0xffff},
           {7, std::uint32_t(magic_65535)}, {8, std::uint32_t(sample)}, {9, c.u16(c.u32(pool.cursor))}, {24, std::uint32_t(scaled)}});
    const auto finer_bound = divide_65535(product);
    std::int32_t finer = 0;
    if (finer_bound != 0) {
        pool.advance(0x1047ab20);
        if (finer_bound > 0xffff || finer_bound < -0xffff)
            throw std::logic_error("0x1047a9e0: the second level's bound is beyond 65535 (unreachable: |bound * 10 / 65535| <= 32768)");
        const auto second = std::int32_t(pool.sample());
        const auto second_remainder = std::int32_t(std::uint32_t(second) - std::uint32_t(divide(second, finer_bound)) * std::uint32_t(finer_bound));
        finer = finer_bound > 0 ? second_remainder : std::int32_t(0u - std::uint32_t(second_remainder));
        // 0x1047b298..0x1047b2d8: r4 the finer bound, r5 the floor's sign bit, r6 the salt, r7 the halfword.
        c.set({{4, std::uint32_t(finer_bound)}, {5, std::uint32_t(product_floor) >> 31}, {6, c.u32(pool.salt)},
               {7, c.u16(c.u32(pool.cursor))}});
    }
    std::uint32_t sign = 0;
    const auto tenth = divide_ten(std::int32_t(std::uint32_t(finer) + 5u), sign);
    c.set(4, sign);
    auto result = std::int32_t(std::uint32_t(scaled) + std::uint32_t(tenth));
    if (result > std::int32_t(std::uint32_t(bound) - 1u)) result = std::int32_t(std::uint32_t(bound) - 1u);
    return result;
}

}
