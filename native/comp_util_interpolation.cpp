// COMP_UTIL.CPP, block `interpolation`: 0x10476780, the position-weighted value the cups' draw screens use for
// a 0..100 figure: the value `index * (to - from) / (count - 1)` (the original has no "+ from"; callers add it
// where they want it), or the mean of the two ends when the count is 1. Integer arithmetic only (no floating
// point reaches this routine). A result outside 0..100 reports the game's assertion (line 0x256e) and is
// clamped to the nearest end.
#include "comp_util_detail.hpp"

namespace cm::aot::comp_util {
namespace {
constexpr std::uint32_t report_at = 0x10476814, report_line = 0x256e;
constexpr std::int32_t name_slot = 0x40, extension_slot = 0x38;  // the assertion's two stack buffers
}

std::int8_t interpolated_share(Cpu& c, std::int8_t from, std::int8_t to, std::int16_t index, std::int16_t count) {
    Frame frame(c, 0x10476780, 0x70, {.first_gpr = 29});
    // r31 the unit's string pool, r30 the shared message buffer: both loaded before the first test.
    c.set({{31, c.toc(-0x47e4)}, {30, c.toc(-0x7ba8)}});
    std::int8_t value;
    if (count == 1) {
        value = std::int8_t((std::int32_t(from) + std::int32_t(to)) / 2);
        c.set(7, 1);
    } else {
        // divw: the product of two bounded values, so the quotient never overflows; count - 1 is not 0 here.
        value = std::int8_t((std::int32_t(index) * (std::int32_t(to) - std::int32_t(from))) / (std::int32_t(count) - 1));
        c.set({{4, sext16(std::uint16_t(index))}, {6, sext8(std::uint8_t(from))}, {7, sext16(std::uint16_t(count))}});
    }
    c.set(29, sext8(std::uint8_t(value)));
    if (value < 0 || value > 100) {
        guest::report_assertion(c, {report_at, c.toc(-0x47e4), c.toc(-0x47e4) + 0xe, c.toc(-0x47e4) + 0x1b, frame.at(name_slot), frame.at(extension_slot), report_line});
        // After the report: 0 for a negative value, 100 for a larger one.
        c.set(4, value < 0 ? 0 : 100);
        value = std::int8_t(c.r(4));
        c.set(29, std::uint32_t(value));
    }
    return value;
}
}
