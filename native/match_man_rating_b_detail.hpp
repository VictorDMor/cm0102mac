#pragma once
// The rating_b block's record offsets, constants and idioms (0x10216830 and the
// parts 0x10211e70 shares with it). Meanings marked "inferred" come from how
// the values are used; the offsets are the original's.
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "match_man_detail.hpp"
#include <cstdint>

namespace cm::aot::match_man::appraisal {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;
using i32 = std::int32_t;
using u32 = std::uint32_t;

// IBM bit numbering of rlwinm: mask of bit `mb` (0 is the most significant).
constexpr u32 bit(unsigned mb) { return 0x80000000u >> mb; }
constexpr i32 s8(u32 value) { return std::int8_t(value); }    // extsb
constexpr i32 s16(u32 value) { return std::int16_t(value); }  // extsh
constexpr u32 reg(i32 value) { return u32(value); }

// The components a call asks for (r5): the original tests them with rlwinm.
namespace component {
constexpr u32 anchor = bit(0x14);          // 0x800: the value is returned unmixed (cmpwi r17,0x800)
constexpr u32 dated_current = bit(0x19);   // 0x40: current ability, aged by the person's birth date
}

// A person record (0x70 bytes).
namespace person {
constexpr u32 id = 0x0;
constexpr u32 nation = 0xc;          // inferred: the nation record (+0xac reputation, +0x118 region, +0x119)
constexpr u32 second_nation = 0x10;
constexpr u32 player = 0x14;
constexpr u32 staff = 0x1c;
constexpr u32 club = 0x24;
constexpr u32 birth_day = 0x4c;      // 16-bit; the 8-byte date at +0x4c is day (i16), year (u16), then 4 more bytes
constexpr u32 birth_year = 0x4e;
constexpr u32 age = 0x5c;            // 16-bit, used as a signed byte
constexpr u32 byte_5e = 0x5e;
constexpr u32 byte_61 = 0x61;
constexpr u32 byte_65 = 0x65;
constexpr u32 byte_67 = 0x67;
}
// A club record (0x248 bytes).
namespace club {
constexpr u32 nation = 0x2c;
constexpr u32 default_judge = 0x90;
constexpr u32 reputation = 0x1ec;    // 16-bit
}
// The global date (TOC -0x7b80): day (i16) at +0, year (u16) at +2.
constexpr u32 date_day = 0, date_year = 2;
constexpr u32 days_in_year = 365;
// TOC slots of the routine.
namespace slot {
constexpr i32 value_table = -0x5290, position_jump = -0x5294;
constexpr i32 game = -0x7ba0, world = -0x7b3c, lookups = -0x7b7c, last_relation = -0x7b78;
constexpr i32 clubs = -0x7be8, people_count = -0x7be4, date = -0x7b80, extra = -0x7b40;
}
constexpr u32 count_of_people_slot = u32(slot::people_count);

// Entries called. Other modules through Cpu::call.
namespace callee {
constexpr u32 estimate = 0x100cdef0, rating_flags = 0x10117490, judged_ability = 0x10219e40;
constexpr u32 add_days = 0x10717b20, date_days = 0x10718130;
}

// The compiler's division by a constant: mulhw by `magic`, srawi, then the
// sign bit added (rounds toward zero). `x / divisor` for the divisor named at
// each use (verified for all int32 by the checker's self-test).
constexpr i32 mulhw(i32 a, i32 b) { return i32((std::int64_t(a) * b) >> 32); }
constexpr i32 sign_bit(i32 x) { return i32(u32(x) >> 31); }
constexpr i32 magic_div(i32 x, i32 magic, unsigned shift) {
    const i32 q = mulhw(magic, x) >> shift;
    return reg(q) + reg(sign_bit(q));
}
// The same for a magic whose top bit is set: the dividend is added back to the high product.
constexpr i32 magic_div_add(i32 x, i32 magic, unsigned shift) {
    const i32 q = i32(reg(mulhw(magic, x)) + reg(x)) >> shift;
    return reg(q) + reg(sign_bit(q));
}
// srawi by `shift` then addze: division by 2^shift rounding toward zero.
constexpr i32 div_pow2(i32 x, unsigned shift) {
    const i32 q = x >> shift;
    return reg(q) + ((x < 0 && (u32(x) & ((1u << shift) - 1))) ? 1 : 0);
}
// (x + sign) >> 1: halves rounding toward zero.
constexpr i32 half(i32 x) { return i32(reg(x) + reg(sign_bit(x))) >> 1; }

// The int-to-double idiom: the words 0x43300000 and value ^ 0x80000000 at
// `at`, `at`+4; the double loaded from `at`, reduced by `conversion` (the pool's 2^52 + 2^31).
inline double store_pair(Cpu& c, u32 at, i32 value) {
    c.put32(at, 0x43300000);
    c.put32(at + 4, reg(value) ^ 0x80000000u);
    return c.f64(at);
}
// fctiwz of `value` into f0, stored (stfd) at `at`; the low word read back.
inline u32 truncate_at(Cpu& c, Fpu& fpu, u32 at, double value) {
    const auto bits = fpu.fctiwz(0, value);
    c.put64(at, bits);
    return u32(bits);
}
// ble / bge after fcmpo: taken when the unordered or opposite bits are set.
struct Compared { bool less, greater, equal; };
inline Compared fcompare(Fpu& fpu, double a, double b) {
    fpu.fcmp(0, a, b);
    return {a < b, b < a, a == b};
}
}
