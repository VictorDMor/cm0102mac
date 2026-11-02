#pragma once
// What the press's event pass shares: divisors the compiler turned into a magic multiply and the
// inline date compare of the game's date type.
#include "media_detail.hpp"
#include "media_store_detail.hpp"
#include "media_values_detail.hpp"

namespace cm::aot::media::detail::pick_b {
// Divisions by a constant the event pass makes besides values' divisors (see reduced_by).
namespace event_divisors {
constexpr Reciprocal by_50{0x51eb851f, 4, 50}, by_12500{0x14f8b589, 10, 0x30d4};
// A negative magic: the multiply-high word has the sample added back before the shift.
constexpr Reciprocal by_30{std::int32_t(0x88888889), 4, 30};
}
// reduced_by for a magic whose multiply-high word the compiler adds the sample to.
inline Reduced reduced_adding_sample(std::uint32_t sample, const Reciprocal& by) {
    const auto high = std::int32_t(std::uint32_t(multiply_high(by.magic, std::int32_t(sample))) + sample);
    const auto shifted = std::uint32_t(high >> by.shift), sign = shifted >> 31;
    return {sample, high, shifted, sign, std::int32_t(sample - (shifted + sign) * std::uint32_t(by.divisor))};
}
// A date of the game's type at `at`: the day (signed) and the year, as the inline compare reads them.
// Whether `a` is later than `b`: the years first, then the days through the compiler's xor idiom
// ((a ^ b) >> 1) - ((a ^ b) & a), its sign bit.
inline std::int32_t date_later(Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t other) {
    const auto a_year = std::uint32_t(c.u16(a + 2)), b_year = std::uint32_t(c.u16(b + 2));
    // r3 holds the year of `a`; r4 what the caller had there (`other`) unless the days are compared.
    leave(c, {{3, a_year}, {4, other}});
    if (compare_logical(c, a_year, b_year) > 0) return 1;
    if (compare_logical(c, a_year, b_year) < 0) return 0;
    const auto day = std::int32_t(c.i16(a)), x = day ^ std::int32_t(c.i16(b));
    leave(c, {{3, std::uint32_t(x >> 1)}, {4, std::uint32_t(day)}});
    return std::int32_t((std::uint32_t((x >> 1) - (x & day)) >> 31));
}
}
