#pragma once
// The font engine's records, constants and arithmetic helpers (font_engine.hpp has the routines).
//
// Fixed-point arithmetic. The original's helpers (UTIL, 0x10767ce0..0x10768074) are the only place the engine multiplies
// and divides 16.16 values from halves; the autogridder, the interpreter and the scan converter call them. Every other
// multiply the engine writes inline is a plain mullw or mulhw, wrapping at 32 bits: write those with the helpers below
// (`wrap`, `rlwinm`, `mulhw`) so the width and the sign are explicit, and check each against the original over the edge values
// (font_engine_checks_util.cpp does for the helpers).
#include "font_engine.hpp"
#include <bit>
#include <cstdint>

namespace cm::aot::font_engine {
// TOC slots (`lwz rX,slot(r2)`: the word is a pointer to the unit's literal pool).
namespace toc {
inline constexpr std::int32_t t2kstrm_pool = -0x3c30;  // 0x108a0824: "pos <= t->maxPos", the unit's first assertion
}
// The assertion reporter 0x107755c0 (MAIN.CPP) takes the assertion's text, the unit's file name and the line.
inline constexpr std::uint32_t assertion_report = 0x107755c0;
// The byte move (NETWORK.CPP) read_bytes copies with: (destination, source, count).
inline constexpr std::uint32_t byte_move = 0x10739ef0;
// The unit's file name sits 0x11 bytes after the pool's first string in T2KSTRM.C's pool.
inline constexpr std::uint32_t t2kstrm_name_offset = 0x11;

// rlwinm: rotate left by `shift`, mask from bit `mb` to `me` (PowerPC numbering: bit 0 is the most significant).
constexpr std::uint32_t rlwinm(std::uint32_t value, unsigned shift, unsigned mb, unsigned me) {
    const auto rotated = std::rotl(value, int(shift));
    const std::uint32_t from = 0xffffffffu >> mb, to = 0xffffffffu << (31 - me);
    return rotated & (mb <= me ? (from & to) : (from | to));
}
// A value's low halfword, zero-extended (rlwinm rX,rY,0,16,31) and the high halfword of a signed value, as the original
// extracts them (srawi 16 then the mask).
constexpr std::uint32_t low_half(std::uint32_t v) { return v & 0xffffu; }
constexpr std::uint32_t high_half(std::uint32_t v) { return v >> 16; }
// 32-bit wrapping arithmetic: C++ signed overflow is undefined, the guest wraps.
constexpr std::int32_t wrap(std::uint32_t v) { return std::int32_t(v); }
constexpr std::uint32_t negate(std::uint32_t v) { return 0u - v; }

// The stream record (T2KSTRM.C): the data, the read position and the length.
namespace stream {
inline constexpr std::uint32_t base = 0x0, position = 0x4, length = 0x8;
}
}
