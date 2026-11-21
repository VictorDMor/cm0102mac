// COMP_UTIL.CPP, block `interpolation`: cases for 0x10476780, `index * (to - from) / (count - 1)` as a signed
// byte, 0..100 or the game's assertion (answered by the checker's scripted dialog) and a clamp. The routine reads
// only its four argument registers (bytes of r3 and r4, halfwords of r5 and r6, sign-extended; the rest of the
// register is ignored), so the cases are registers only. Floating point does not reach it (integer arithmetic),
// so the "thresholds" here are the integer ones: the values that land on 0, 100, 101, -1, 127, -128 and the byte
// wrap (256 is 0, 356 is a valid 100 again), the count of 1 (mean of the ends), 0 (divisor -1) and the
// halfword extremes. Labels start with "interpolation:" (CM_COMP_UTIL_BLOCK=interpolation).
#include "comp_util_checks.hpp"
#include <set>
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t share = 0x10476780;
Case call(std::uint32_t from, std::uint32_t to, std::uint32_t index, std::uint32_t count, const std::string& what) {
    return {.entry = share, .registers = {{3, from}, {4, to}, {5, index}, {6, count}},
            .label = "interpolation: " + what + " from " + std::to_string(std::int8_t(from)) + " to " + std::to_string(std::int8_t(to)) +
                     " index " + std::to_string(std::int16_t(index)) + " count " + std::to_string(std::int16_t(count))};
}
}

COMP_UTIL_CASES("interpolation") {
    // A count of 1: the mean of the two ends, truncated towards zero, every `to` against a spread of `from`.
    for (const std::int32_t from : {-128, -127, -101, -100, -1, 0, 1, 99, 100, 101, 126, 127})
        for (std::int32_t to = -128; to <= 127; ++to)
            cases.push_back(call(std::uint32_t(from) & 0xff, std::uint32_t(to) & 0xff, 0, 1, "mean"));
    // The same with garbage above the used bits (extsb/extsh ignore them) and a count of 1 spelled with high bits.
    for (const std::uint32_t high : {0x12345600u, 0xffffff00u, 0x80000000u, 0x00ffff00u})
        for (const std::uint32_t low : {0u, 1u, 0x7fu, 0x80u, 0xffu})
            cases.push_back(call(high | low, (~high & 0xffffff00u) | ((0x7fu - low) & 0xff), 0x00050000u | 7u, 0x00010001u, "mean with high bits"));
}

COMP_UTIL_CASES("interpolation") {
    // Two positions: the value is `index * (to - from)` wrapped to a byte, for every difference from -255 to 255
    // (from 0, -128 and 127 against every `to`), so each residue lands on 0..100 (valid), 101..127 and -128..-1.
    for (const std::int32_t from : {0, -128, 127})
        for (std::int32_t to = -128; to <= 127; ++to)
            for (const std::int32_t index : {0, 1, 2, 3, -1, -2, 5, 100, 32767, -32768})
                cases.push_back(call(std::uint32_t(from) & 0xff, std::uint32_t(to) & 0xff, std::uint32_t(index) & 0xffff, 2, "two positions"));
}

COMP_UTIL_CASES("interpolation") {
    // Larger counts: positions along a row of clubs, the way the callers use it (index 0..count-1, then past it),
    // with the ends of the 0..100 range, and counts at the halfword extremes and zero (divisor -1).
    const std::int32_t ends[][2] = {{0, 100}, {0, 50}, {100, 0}, {0, 127}, {-128, 127}, {127, -128}, {10, 90}, {50, 50}, {0, 0}, {0, 1}, {1, 0}, {0, 101}, {0, -1}, {-1, 0}};
    for (const auto& end : ends)
        for (const std::int32_t count : {2, 3, 4, 5, 8, 10, 16, 18, 20, 32, 64, 100, 127, 128, 255, 256, 1000, 32767, 0, -1, -2, -32768, 0x10001 /* 1 in the halfword */})
            for (const std::int32_t index : {-3, -1, 0, 1, 2, 3, 4, 5, 7, 9, 15, 16, 17, 19, 31, 63, 99, 100, 101, 126, 127, 128, 254, 255, 256, 999, 1000, 32767, -32768, count - 2, count - 1, count, count + 1})
                cases.push_back(call(std::uint32_t(end[0]) & 0xff, std::uint32_t(end[1]) & 0xff, std::uint32_t(index) & 0xffff, std::uint32_t(count), "row"));
    // Landing exactly on the thresholds: count 101 with an end of 100 makes the value the index (0..100, then 101 asserts).
    for (std::int32_t index = -2; index <= 130; ++index)
        cases.push_back(call(0, 100, std::uint32_t(index) & 0xffff, 101, "value equals index"));
    // The byte wrap: 256 (0), 356 (100), 357 (101), 255 (-1), 128 (-128): quotient index * 100 / 99 and friends.
    for (const std::int32_t index : {99, 100, 101, 127, 128, 255, 256, 257, 355, 356, 357, 358, 383, 384})
        for (const std::int32_t count : {2, 3, 100})
            cases.push_back(call(0, 100, std::uint32_t(index) & 0xffff, std::uint32_t(count), "byte wrap"));
}
}
