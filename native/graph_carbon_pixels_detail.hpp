#pragma once
// graph_carbon.cpp, blocks `lines` and `fills` (graph_carbon_lines.cpp, graph_carbon_fills.cpp): what the pixel primitives share. They write
// the 15-bit surface the module draws into (graph_carbon_detail.hpp's `pixel::`) through three variables of the module's small data and
// leave their last loop values in the volatile registers, which the checker compares in full.
#include "graph_carbon_detail.hpp"
#include "native_drawing.hpp"

namespace cm::aot::graph_carbon {
// The surface's variables (small data, read as the original reads them: the base each time a pixel's address is formed, the row stride in
// pixels as a signed halfword).
namespace surface_variable {
inline constexpr std::int32_t base = -0x3564,            // u32: the first pixel of the surface (0x108b852c)
    row_pixels = -0x3568,                                // i16: the pixels from one row to the next (0x108b8528)
    suspended = -0x3550;                                 // byte: nonzero while the window is being built (0x10768820): every pixel primitive does nothing
}
// Halfword sign extension (`extsh`), and a signed value as the register word.
inline constexpr std::int32_t sign_extend_16(std::uint32_t value) { return std::int32_t(std::int16_t(value & 0xffff)); }
inline constexpr std::uint32_t word(std::int32_t value) { return std::uint32_t(value); }
// The Mac's `mulhw`: the high word of the signed 64-bit product.
inline constexpr std::int32_t high_word(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * std::int64_t(b)) >> 32); }
// The compiler's remainder by 6 of a count: the high word of the multiply by 0x2aaaaaab, its sign bit added, six times that taken away. The
// routines keep `high` and the remainder in volatile registers, so both are returned.
inline constexpr std::int32_t one_sixth = 0x2aaaaaab;
struct SixthRemainder { std::int32_t high; std::uint32_t quotient_sign, rest; };
inline constexpr SixthRemainder remainder_by_six(std::uint32_t n) {
    const auto high = high_word(one_sixth, std::int32_t(n));
    const auto sign = std::uint32_t(high) >> 31;
    return {high, sign, n - (std::uint32_t(high) + sign) * 6u};
}
// The volatile registers r3-r10 as a routine holds them while it runs (the loops leave their last values there).
struct Volatile {
    std::uint32_t r3, r4, r5, r6, r7, r8, r9, r10;
    static Volatile load(const Cpu& c) { return {c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)}; }
    void store(Cpu& c) const { c.set({{3, r3}, {4, r4}, {5, r5}, {6, r6}, {7, r7}, {8, r8}, {9, r9}, {10, r10}}); }
};
// The style bit a line routine tests: set draws every pixel (a solid line), clear draws four pixels of each six (a dotted one).
inline constexpr std::uint32_t solid_line = 2;
}
