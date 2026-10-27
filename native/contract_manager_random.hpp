#pragma once
#include "contract_manager_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::contract_manager::detail {
struct RandomTable {
    Cpu& c;
    std::uint32_t salt, base, end, cursor;

    std::uint32_t position() const { return c.u32(cursor); }
    std::uint32_t value() const { return c.u16(position()) + c.u32(salt); }
    void advance(std::uint32_t back) const {
        const auto next = position() + 2, last = c.u32(end);
        c.set(3, next); c.set(0, last);
        const bool wraps = compare_logical(c, next, last) > 0;
        c.put32(cursor, next);
        if (wraps) {
            c.put32(cursor, base);
            const auto entropy = Program(c).call(0x1077b770, back);
            c.set(0, entropy & 0xffff);
            c.put32(salt, entropy & 0xffff);
        }
    }
};

inline std::int32_t multiply_high(std::int32_t first, std::int32_t second) {
    return std::int32_t((std::int64_t(first) * second) >> 32);
}
struct SignedQuotient { std::int32_t value, floor; std::uint32_t sign; };
inline SignedQuotient divide_65535(std::int32_t value) {
    const auto floor = std::int32_t(std::uint32_t(multiply_high(std::int32_t(0x80008001), value)) +
                                    std::uint32_t(value)) >> 15;
    const auto sign = std::uint32_t(floor) >> 31;
    return {std::int32_t(std::uint32_t(floor) + sign), floor, sign};
}
inline SignedQuotient rounded_tenth(std::int32_t value) {
    const auto dividend = std::int32_t(std::uint32_t(value) + 5);
    const auto floor = multiply_high(0x66666667, dividend) >> 2;
    const auto sign = std::uint32_t(floor) >> 31;
    return {std::int32_t(std::uint32_t(floor) + sign), floor, sign};
}
}
