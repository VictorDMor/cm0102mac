#pragma once
#include "news_detail.hpp"

namespace cm::aot::news::detail {
// Addresses captured by the caller's original prologue. The pool base is a
// direct address; the end and cursor addresses contain 32-bit guest pointers.
struct SamplePool {
    Cpu& c;
    std::uint32_t bias_address, base, end_address, cursor_address;
    void advance(std::uint32_t rand_back, bool expose_previous = false) const {
        const auto cursor = c.u32(cursor_address), end = c.u32(end_address);
        if (expose_previous) c.set(4,cursor);
        const auto next = cursor + 2;
        c.set(3, next);
        const auto ordering = compare_logical(c, next, end);
        c.put32(cursor_address, next);
        if (ordering > 0) {
            c.put32(cursor_address, base);
            c.call(0x1077b770, rand_back);
            c.put32(bias_address, c.r(3) & 0xffff);
        }
    }
    std::uint32_t position() const { return c.u32(cursor_address); }
    std::uint32_t bias() const { return c.u32(bias_address); }
};
inline std::uint32_t remainder_word(std::uint32_t value, std::uint32_t divisor) {
    // Match divw's generated zero-divisor convention and wrapping product.
    const auto quotient = divisor ? std::uint32_t(divide_signed<4>(value, divisor)) : 0;
    return value - quotient * divisor;
}
inline std::uint32_t oriented_remainder(std::uint32_t value, std::int32_t bound) {
    const auto remainder = remainder_word(value, std::uint32_t(bound));
    return bound > 0 ? remainder : 0u - remainder;
}
}
