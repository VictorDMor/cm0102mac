// COMP_UTIL.CPP, block `numeric_ids`: cases for the series of a competition id (0x10464010) and the two decimal
// helpers (0x10464880, 0x104648c0). All three are pure functions of r3, so the cases are registers only: every id
// from -3 to 0x4ff (the whole range the game's ids can take, with both edges of every id range), the decimal
// helpers over each side of 1000 and of zero, and a spread of the full 32-bit and 16-bit domains. The helpers'
// results and the registers they leave (r4, r5) are compared with the original's, on every case. Labels start
// with "numeric_ids:" (CM_COMP_UTIL_BLOCK=numeric_ids).
#include "comp_util_checks.hpp"
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t parent_series = 0x10464010, last_digit = 0x10464880, round_down = 0x104648c0;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
}

COMP_UTIL_CASES("numeric_ids") {
    std::set<std::int32_t> ids;
    for (std::int32_t id = -3; id <= 0x4ff; ++id) ids.insert(id);
    for (const std::int32_t id : {0x7fffffff, std::int32_t(0x80000000), -1000, 0x10000, 0x1ffff, 0x100003fc, 0x2000047e}) ids.insert(id);
    for (const auto id : ids)
        cases.push_back({.entry = parent_series, .registers = {{3, std::uint32_t(id)}}, .label = "numeric_ids: series of " + hex(std::uint32_t(id))});
}

COMP_UTIL_CASES("numeric_ids") {
    // The last digit: the signed remainder below 1000, 0 from 1000; around zero, the powers of ten and the extremes.
    std::set<std::int32_t> values;
    for (std::int32_t v = -1100; v <= 1100; ++v) values.insert(v);
    for (std::int32_t v = 1100; v < 40000; v += 997) values.insert(v);
    for (std::int32_t v = -1100; v > -2000000000; v -= 99991) values.insert(v);
    for (const std::int32_t v : {0x7fffffff, std::int32_t(0x80000000), std::int32_t(0x80000001), 0x3e7, 0x3e8, -0x3e8, -0x3e9}) values.insert(v);
    for (const auto v : values)
        cases.push_back({.entry = last_digit, .registers = {{3, std::uint32_t(v)}}, .label = "numeric_ids: last digit of " + hex(std::uint32_t(v))});
}

COMP_UTIL_CASES("numeric_ids") {
    // Rounded down to a multiple of ten: a halfword in r3's low bits (the high bits are not part of the value).
    std::set<std::int32_t> values;
    for (std::int32_t v = -1100; v <= 1100; ++v) values.insert(v);
    for (std::int32_t v = -32768; v <= 32767; v += 37) values.insert(v);
    for (const std::int32_t v : {32767, -32768, -32767, 999, 1000, 1001, -999, -10, -9, -1}) values.insert(v);
    for (const auto v : values)
        cases.push_back({.entry = round_down, .registers = {{3, std::uint32_t(v)}}, .label = "numeric_ids: round down " + hex(std::uint32_t(v))});
    // The same values with other bits above the halfword: extsh ignores them, as must the rewrite.
    for (const std::int32_t v : {-1001, -10, -1, 0, 7, 10, 999, 1000, 1234, 30000, -32768})
        for (const std::uint32_t high : {0x12340000u, 0xffff0000u, 0x00010000u, 0x80000000u})
            cases.push_back({.entry = round_down, .registers = {{3, high | (std::uint32_t(v) & 0xffff)}},
                             .label = "numeric_ids: round down " + hex(std::uint32_t(v) & 0xffff) + " above " + hex(high)});
}
}
