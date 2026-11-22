#pragma once
// The font engine (tools/font_engine_audit.py): the T2K scaler the game links for its text, rewritten as typed C++.
// font_engine_detail.hpp has the record views, constants and fixed-point helpers; this header lists the routines of the
// blocks written so far (the next blocks append theirs). A routine here takes the guest CPU only where it reads or writes
// guest memory or calls the game; the fixed-point helpers are plain functions.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::font_engine {
using guest::Cpu;

// ---- UTIL (0x10767ce0..0x10768074): the 16.16 and 2.30 fixed-point helpers, a sort of shorts and a distance -------------
// What a helper leaves in the volatile registers besides its result (its callers' indirect calls and varargs spills can read
// them, so the checker compares r4-r8 at return; r9 and r10 pass through untouched).
struct Leftovers {
    std::uint32_t r4 = 0, r5 = 0, r6 = 0, r7 = 0, r8 = 0;
};
// 0x10767d40: the 16.16 product (a * b) >> 16 of two signed values, from 16-bit halves of their magnitudes: the low-by-low
// product's high half is added to the cross products and the high-by-high product's low half shifted up. The halves wrap;
// the product is not rounded.
std::int32_t fix_multiply(std::int32_t a, std::int32_t b, Leftovers& left);
// 0x10767dc0: the same shape for a 2.30 fraction (a * b >> 30, the high-by-high product shifted up 26, the low-by-low product
// down 6, the cross products up 10).
std::int32_t frac_multiply(std::int32_t a, std::int32_t b, Leftovers& left);
// 0x10767e40: a 32-bit value times a signed halfword (the callers pass a sign-extended 16-bit scale) as an 18-bit product:
// (high * |scale| << 2) + (low * |scale| >> 14), signed by the operands' signs.
std::int32_t scale_multiply(std::int32_t value, std::int32_t scale, Leftovers& left);
// 0x10767eb0: the 16.16 quotient (a << 16) / b of two signed values: the integer part by unsigned division, the remainder
// shifted down until it fits 16 bits (the divisor with it) and divided again. A zero divisor faults as the generated code does.
std::int32_t fix_divide(std::int32_t a, std::int32_t b, Leftovers& left);
// 0x10767f20: the length of the vector (a, b) by the interpolation of the larger and the smaller component and three rounds
// of the Newton step (x + (a^2 + b^2) / x) / 2 in 16.16, with the helpers above.
std::int32_t distance(std::int32_t a, std::int32_t b, Leftovers& left);
// 0x10767ce0: bubble sort of `count` halfwords in guest memory, ascending (a pass repeats while one swapped).
void sort_shorts(Cpu& c, std::uint32_t array, std::int32_t count, Leftovers& left);

// ---- T2KSTRM (0x10762f40..0x107630e0): the input stream the font tables are read through ----------------------------------
// A stream is {+0 base, +4 position, +8 length}. A read past the length reports the failed assertion (0x107755c0) and goes on.
std::uint32_t read_unsigned_32(Cpu& c, std::uint32_t stream);
std::int32_t read_signed_16(Cpu& c, std::uint32_t stream);
void read_bytes(Cpu& c, std::uint32_t stream, std::uint32_t destination, std::int32_t count);
}
