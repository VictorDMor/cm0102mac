#pragma once
#include "aot_runtime.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

namespace cm::aot {
// PowerPC multiply-add instructions round a*c±b once ("with at most a single
// roundoff error", PowerPC Numerics, ch. 14). Ghidra's p-code rounds the
// product first, so generated pages and rewritten routines use these helpers.
// The fnm* forms negate the rounded result, which is exact.
// A NaN addend keeps its sign when subtracted, as with the PowerPC and ARM
// subtraction it replaces.
inline double fused_addend(std::uint64_t b, bool subtract) {
    const auto addend = float_value<8>(b);
    return subtract && !std::isnan(addend) ? -addend : addend;
}
inline std::uint64_t fused_multiply_add(std::uint64_t a, std::uint64_t c, std::uint64_t b, bool subtract) {
    return float_bits<8>(std::fma(float_value<8>(a), float_value<8>(c), fused_addend(b, subtract)));
}
// The single-precision forms round once, to single precision, and return that
// value widened (exactly) to double. For single operands fmaf rounds exactly
// once. Operands that are not single (rare: e.g. integers above 2^24 from the
// 0x43300000 conversion idiom) round to double, then to single: in a
// midpoint case that can differ from the PowerPC by one ulp.
inline std::uint64_t fused_multiply_add_single(std::uint64_t a, std::uint64_t c, std::uint64_t b, bool subtract) {
    const auto x = float_value<8>(a), y = float_value<8>(c), z = fused_addend(b, subtract);
    const auto single = [](double value) { return std::isnan(value) || double(float(value)) == value; };
    if (single(x) && single(y) && single(z))
        return float_bits<8>(double(std::fmaf(float(x), float(y), float(z))));
    return float_bits<8>(double(float(std::fma(x, y, z))));
}

enum FusedKind : std::uint8_t { fused_subtract = 1, fused_negate = 2, fused_single = 4 };
struct FusedSite { std::uint32_t address; std::uint8_t kind; };
// Every multiply-add in the original program, sorted by address; written by
// tools/compile_recomp.py into the generated fused_sites.cpp.
extern const FusedSite fused_sites[];
extern const std::size_t fused_site_count;

// The kind of the original multiply-add at `address`. Rewritten routines ask
// the original instruction rather than restating it, so they cannot drift
// from the generated code.
inline std::uint8_t fused_site_kind(std::uint32_t address) {
    const auto end = fused_sites + fused_site_count;
    const auto found = std::lower_bound(fused_sites, end, address,
        [](const FusedSite& site, std::uint32_t value) { return site.address < value; });
    if (found == end || found->address != address) {
        char message[64];
        std::snprintf(message, sizeof message, "No original multiply-add at 0x%08x", address);
        throw std::logic_error(message);
    }
    return found->kind;
}
inline double original_multiply_add(std::uint8_t kind, std::uint64_t a, std::uint64_t c, std::uint64_t b) {
    const bool subtract = kind & fused_subtract;
    const auto result = float_value<8>((kind & fused_single) ? fused_multiply_add_single(a, c, b, subtract)
                                                             : fused_multiply_add(a, c, b, subtract));
    return (kind & fused_negate) ? -result : result;
}
}
