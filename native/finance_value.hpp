#pragma once
// FINANCE.CPP, block value: what a club can pay for a player or a contract. The account keeps three
// figures from the club's contracts (+0x0c their total, +0x10 the largest, +0x14 the club's
// budget); 0x100f4f80 recomputes them, 0x100f2680 is the budget itself and 0x100f4790 a cap on
// one spending kind. Contract and quirks: docs/cpp-rewrites.md (see finance.hpp).
#include "finance_detail.hpp"
#include "guest_abi.hpp"
#include "guest_fpu.hpp"
#include <bit>
#include <cstdint>

namespace cm::aot::finance {
using guest::Cpu;

// What the club can spend: `outside` is the deal being weighed (a transfer or contract offer record,
// 0 when the club asks for itself) and `kind` the spending kind (0xff: take it from the deal's
// negotiator). The result is also the cap kept in the account's +0x14.             // 0x100f2680
std::uint32_t club_budget(Cpu&, std::uint32_t account, std::uint32_t outside, std::uint32_t kind);
// The budget for one spending kind, kept within the account's +0x14 and the kind's range.  // 0x100f4790
std::uint32_t club_budget_for_kind(Cpu&, std::uint32_t account, std::int8_t kind);
// Recomputes the account's +0x0c, +0x10 and +0x14 from the club's contracts and reputation. // 0x100f4f80
void refresh_account_budget(Cpu&, std::uint32_t account);

namespace value_detail {
// The original's integer-to-double idiom (the magic word less the pool constant) and its
// conversions back, every operation through the Fpu so that FPSCR ends as the original's.
inline constexpr std::uint32_t pool_magic = 0x50, pool_one = 0x18;
inline double magic_word(std::int32_t value) {
    return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (std::uint32_t(value) ^ 0x80000000u));
}
// The two words the idiom stores on the stack for the load: the magic high word, then the integer
// with its sign bit flipped.
inline void put_magic_pair(Cpu& c, std::uint32_t at, std::int32_t value) {
    c.put32(at, 0x43300000);
    c.put32(at + 4, std::uint32_t(value) ^ 0x80000000u);
}
struct Numbers {
    explicit Numbers(Cpu& cpu) : c(cpu), fpu(cpu), pool(cpu.toc(toc::doubles)) {}
    Cpu& c;
    guest::Fpu fpu;
    std::uint32_t pool;
    double constant(std::uint32_t at) const { return c.f64(pool + at); }
    double widen(std::int32_t value) { return fpu.fsub(magic_word(value), constant(pool_magic)); }
    double widen_single(std::int32_t value) { return fpu.fsubs(magic_word(value), constant(pool_magic)); }
    // `fctiwz` and the load of its low word.
    std::int32_t truncate(double value) { return std::int32_t(fpu.fctiwz(0, value)); }
    // The value times a pool constant, truncated (double or single conversion of the value).
    std::int32_t scale(std::int32_t value, std::uint32_t factor) {
        return truncate(fpu.fmul(constant(factor), widen(value)));
    }
    std::int32_t scale_single(std::int32_t value, std::uint32_t factor) {
        return truncate(fpu.fmul(constant(factor), widen_single(value)));
    }
};
// Whether a record's competition identifier is the one a TOC cell names (`lwz rX,slot(r2); lwz rX,0(rX)`).
inline bool is_competition(Cpu& c, std::uint32_t id, std::int32_t slot) { return id == c.u32(c.toc(slot)); }
inline bool is_any_competition(Cpu& c, std::uint32_t id, std::initializer_list<std::int32_t> slots) {
    for (const auto slot : slots)
        if (is_competition(c, id, slot)) return true;
    return false;
}
// The competition cells that several of the unit's routines compare a club's league against.
inline constexpr std::int32_t cell_a = -0x7a18, cell_b = -0x7a14, cell_c = -0x7a10;  // the top division cells (r27, r28, r29 of 0x100f2680)
inline constexpr std::int32_t cell_d = -0x7af4, cell_e = -0x7a00;                    // r13, r14 of 0x100f2680
// Rounds toward zero, as the compiler's multiply-high divisions do.
inline std::int32_t divide(std::int32_t value, std::int32_t divisor) { return value / divisor; }
// The ceiling a spending kind (or a deal's kind byte) puts on a budget before it is scaled.
inline std::int32_t kind_ceiling(std::int8_t kind) {
    switch (kind) {
        case 8: return 20000;
        case 5: case 6: case 7: return 35000;
        case 10: return 1000;
        case 9: return 1500;
        default: return 750;
    }
}
// The strength level of a league record (+0x11c), limited to 1..3.
inline std::int32_t level_of(Cpu& c, std::uint32_t league) {
    const auto raw = c.i8(league + 0x11c);
    return raw < 1 ? 1 : raw > 3 ? 3 : raw;
}
}
}
