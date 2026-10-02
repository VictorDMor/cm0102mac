#pragma once
#include "aot_runtime.hpp"
#include <bit>

namespace cm::aot {
// Replace only the complete, audited Mac unsigned 64-bit division routine at
// 0x10773870..0x10773958. Operands and quotient use the r3:r4 / r5:r6 ABI.
// This routine has no memory access or native service. Preserve its otherwise
// caller-clobbered registers, CR0, carry, CTR, and original instruction count.
// P-code unique-space values are instruction-local translation temporaries,
// not architectural registers; no original instruction may consume them.
// The generated wrapper must retain the original path for PC profiling and
// invoke this before constructing its page-local instruction counter.
inline bool try_fast_unsigned_division(State& s) {
    if (s.pc != 0x10773870) return false;
    const auto dividend_high = std::uint32_t(s.r<4>(12));
    const auto divisor_high = std::uint32_t(s.r<4>(20));
    const auto dividend = (std::uint64_t(dividend_high) << 32) | s.r<4>(16);
    const auto divisor = (std::uint64_t(divisor_high) << 32) | s.r<4>(24);
    // The original software loop does not execute a trapping divide. Retain
    // its actual zero-divisor behavior instead of introducing C++ undefined
    // behavior, an invented result, or a new exception.
    if (!divisor) return false;

    const auto dividend_zeros = std::countl_zero(dividend);
    const auto divisor_zeros = std::countl_zero(divisor);
    const bool early_return = dividend_zeros > divisor_zeros;
    const auto quotient = dividend / divisor;
    const auto remainder = dividend % divisor;
    const auto high_word_branches = unsigned(!dividend_high) + unsigned(!divisor_high);
    std::uint64_t instructions = 14 + high_word_branches;
    if (!early_return) {
        const auto iterations = unsigned(divisor_zeros - dividend_zeros + 1);
        // Each restoring-division iteration executes eight instructions plus
        // three for a successful subtraction (one set quotient bit). The two
        // normalization branches save two instructions only at 32 iterations.
        instructions = (iterations == 32 ? 33u : 35u) + high_word_branches +
                       8u * iterations + 3u * std::popcount(quotient);
    }
    // A budget failure must still stop at the exact original instruction,
    // with its partially updated register state and failing step counted.
    // Fall back before making any changes whenever the whole call cannot fit.
    if (s.executed > s.budget || instructions > s.budget - s.executed) return false;

    const auto summary_overflow = s.r<1>(1024) & 1;
    s.wr<4>(12, quotient >> 32);
    s.wr<4>(16, quotient);
    if (early_return) {
        s.wr<4>(0, unsigned(dividend_zeros));
        s.wr<4>(36, unsigned(divisor_zeros));
        s.wr<4>(40, 64u - unsigned(dividend_zeros));
        s.wr<1>(1027, 1); // subfic r10,r0,64 has no unsigned borrow.
        s.wr<1>(2304, 4 | summary_overflow);
        // This branch leaves r7, r8, and CTR untouched.
    } else {
        const bool last_subtraction_succeeded = (quotient & 1) != 0;
        const auto last_difference = last_subtraction_succeeded ? remainder : remainder - divisor;
        const auto difference_high = std::uint32_t(last_difference >> 32);
        s.wr<4>(0, last_subtraction_succeeded ? 0 : std::uint32_t(last_difference));
        s.wr<4>(28, remainder >> 32);
        s.wr<4>(32, remainder);
        s.wr<4>(36, difference_high);
        s.wr<4>(40, UINT32_MAX);
        s.wr<4>(4132, 0);
        s.wr<1>(1027, 0); // The final quotient shift cannot overflow.
        const auto comparison = difference_high & 0x80000000u ? 8u : difference_high ? 4u : 2u;
        s.wr<1>(2304, comparison | summary_overflow);
    }
    s.executed += instructions;
    s.pc = std::uint32_t(s.r<4>(4128));
    return true;
}
}
