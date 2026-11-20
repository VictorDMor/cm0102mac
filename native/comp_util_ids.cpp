// COMP_UTIL.CPP, block `numeric_ids`: the series a competition id belongs to (0x10464010) and two decimal
// helpers (0x10464880, 0x104648c0). Pure functions of their argument; the two helpers also leave r4 (and r5) as
// the original does, because callers' varargs calls spill r3-r10 into their frames.
#include "comp_util_detail.hpp"
#include <array>

namespace cm::aot::comp_util {
namespace {
struct Range { std::int32_t first, last, series; };
// The original tests these ranges in sequence; none overlaps another, so the order is not significant.
constexpr std::array<Range, 15> series_of = {{
    {1000, 0x3fb, 0x41c},  {0x3fc, 0x3fc, 0x41c}, {0x3fd, 0x406, 0x0a}, {0x407, 0x40b, 0x14}, {0x40c, 0x40e, 0x82},
    {0x413, 0x414, 0x41c}, {0x41f, 0x42e, 0x41c}, {0x43d, 0x444, 0x449}, {0x445, 0x448, 0x44a}, {0x44d, 0x450, 0x451},
    {0x472, 0x472, 0x14},  {0x473, 0x474, 0x410}, {0x475, 0x479, 0x47a}, {0x47b, 0x47b, 0x41c}, {0x47e, 0x47f, 0x1e},
}};
}

std::int32_t parent_series(std::int32_t competition) {
    for (const auto& range : series_of)
        if (competition >= range.first && competition <= range.last) return range.series;
    return -1;
}

std::int8_t last_digit(Cpu& c, std::int32_t value) {
    if (value >= 1000) return 0;
    // The quotient's sign survives in r4 (`rlwinm r4,r0,1,31,31` of the shifted high product).
    c.set(4, value < 0);
    return std::int8_t(value % 10);
}

std::int16_t round_down_to_ten(Cpu& c, std::int16_t value) {
    c.set(5, sext16(std::uint16_t(value)));
    std::int32_t remainder = 0;
    if (value < 1000) {
        c.set(4, value < 0);
        remainder = value % 10;
    }
    return std::int16_t(value - remainder);
}
}
