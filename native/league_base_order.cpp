// LEAGUE.CPP: three sort orders the league passes to qsort (0x10768300) through its TOC slots -0x4718,
// -0x4728 and -0x472c. Each takes two row addresses and returns the difference the original's `subf` gives,
// wrapping at 32 bits.
#include "league_base_detail.hpp"

namespace cm::aot::league_base {

std::int32_t compare_by_word(Cpu& c, std::uint32_t left, std::uint32_t right) {
    // r4 keeps the right word (0x104aaaf0): qsort hands it back to callers that read r4 after the sort.
    c.set(4, c.u32(right));
    return std::int32_t(c.u32(left) - c.u32(right));
}
std::int32_t compare_by_reputation(Cpu& c, std::uint32_t left, std::uint32_t right) {
    // The halfword +0x1ec of the records the rows point at, larger first (inferred: a club's reputation).
    constexpr std::uint32_t reputation = 0x1ec;
    const auto first = c.u32(left), second = c.u32(right);
    // r5 the left record, r4 its halfword (0x104aab00..0x104aab08); r3 is the result.
    c.set({{5, first}, {4, std::uint32_t(std::int32_t(c.i16(first + reputation)))}});
    return std::int32_t(std::uint32_t(std::int32_t(c.i16(second + reputation))) - std::uint32_t(std::int32_t(c.i16(first + reputation))));
}
std::int32_t compare_by_half_a(Cpu& c, std::uint32_t left, std::uint32_t right) {
    constexpr std::uint32_t value = 0xa;
    return std::int32_t(std::uint32_t(std::int32_t(c.i16(right + value))) - std::uint32_t(std::int32_t(c.i16(left + value))));
}

}
