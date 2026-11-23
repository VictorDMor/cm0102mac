// graph_carbon.cpp, block `leaves` (0x10769b50, 0x1076e910, 0x107698d0, 0x10769920, 0x1076bb20, 0x1076bb40, 0x1076ca20): cases for the
// leaf routines. None calls anything, so every case is a plain call from the snapshot's registers: the arguments are set (r3-r10 distinct
// wherever the routine does not write them, so that the full comparison of r3-r10 sees a register it should have kept) and the variables
// the routines read are written in guest memory (the TOC's small data and the display flag), the real values first. A rectangle is
// written into the scratch area over a pattern, so a store of the wrong size, order or place shows in the bytes. Labels start with "leaves:".
#include "graph_carbon_checks.hpp"
#include "graph_carbon_detail.hpp"
#include <climits>
#include <cstdlib>
#include <random>

namespace cm::aot::graph_carbon_checks {
namespace {
using namespace cm::aot::graph_carbon;
constexpr std::uint32_t game_toc = 0x108bba90;
constexpr std::uint32_t full_screen_entry = 0x10769b50, cursor_entry = 0x1076e910, corners_entry = 0x107698d0, area_entry = 0x10769920,
                        pack_entry = 0x1076bb20, scale_entry = 0x1076bb40, probe_entry = 0x1076ca20;
constexpr std::uint32_t rect_at = scratch_base + 0x10000;
constexpr std::uint32_t small(std::int32_t offset) { return game_toc + std::uint32_t(offset); }
// Registers a routine that does not use them must leave alone: distinct values, none of them a plausible argument.
std::vector<std::pair<unsigned, std::uint32_t>> others(std::initializer_list<std::pair<unsigned, std::uint32_t>> used) {
    std::vector<std::pair<unsigned, std::uint32_t>> registers;
    for (unsigned n = 3; n <= 10; ++n) registers.emplace_back(n, 0x7a000000u + n * 0x01010101u);
    for (const auto& [n, value] : used) registers[n - 3].second = value;
    return registers;
}
std::vector<Write> rectangle_pattern() {
    std::vector<Write> memory;
    for (std::uint32_t n = 0; n < 16; ++n) memory.push_back(w8(rect_at - 4 + n, 0xa0 + n));
    return memory;
}
bool exhaustive() { return std::getenv("CM_GRAPH_CARBON_EXHAUSTIVE") != nullptr; }
}

GRAPH_CARBON_CASES("leaves") {
    std::mt19937 random(0x6c41);
    const auto full_screen_variable = scenario.toc(toc::full_screen);
    // ---- 0x10769b50: the display mode flag. The snapshot's own value, then bytes with each bit and the high bits of the word around it set.
    cases.push_back({.entry = full_screen_entry, .registers = others({}), .label = "leaves: full screen, the snapshot's value"});
    for (const std::uint32_t value : {0u, 1u, 2u, 0x7fu, 0x80u, 0xffu, 0x10u})
        cases.push_back({.entry = full_screen_entry, .registers = others({}),
                         .memory = {w8(full_screen_variable, value), w8(full_screen_variable + 1, 0xee), w8(full_screen_variable - 1, 0xdd)},
                         .label = "leaves: full screen byte " + hex(value)});
    // ---- 0x1076e910: the cursor index, a byte of the small data.
    cases.push_back({.entry = cursor_entry, .registers = others({}), .label = "leaves: cursor, the snapshot's value"});
    for (const std::uint32_t value : {0u, 1u, 2u, 3u, 0x7fu, 0x80u, 0xffu})
        cases.push_back({.entry = cursor_entry, .registers = others({}),
                         .memory = {w8(small(small_data::current_cursor), value), w8(small(small_data::current_cursor) + 1, 0xee)},
                         .label = "leaves: cursor byte " + hex(value)});

    // ---- 0x107698d0: the two corners into a Rect {top, left, bottom, right}. Every order of the two points on each axis with edge values
    // (halfword truncation: 0x8000, 0x10000, 0x12345; the word's sign: INT_MIN, INT_MAX), then random points.
    const std::int32_t edges[] = {INT_MIN, -0x10001, -0x8000, -1, 0, 1, 0x7fff, 0x8000, 0x12345, INT_MAX};
    for (const auto x1 : edges)
        for (const auto x2 : edges)
            for (const auto y1 : {INT_MIN, -1, 0, 0x8000, INT_MAX})
                for (const auto y2 : {-0x8000, 0, 0x7fff, 0x10000}) {
                    if (!exhaustive() && ((std::uint32_t(x1) ^ std::uint32_t(x2) ^ std::uint32_t(y1) ^ std::uint32_t(y2)) % 5) != 0) continue;
                    cases.push_back({.entry = corners_entry, .registers = others({{3, std::uint32_t(x1)}, {4, std::uint32_t(y1)}, {5, std::uint32_t(x2)}, {6, std::uint32_t(y2)}, {7, rect_at}}),
                                     .memory = rectangle_pattern(),
                                     .label = "leaves: corners " + hex(std::uint32_t(x1)) + "," + hex(std::uint32_t(y1)) + " " + hex(std::uint32_t(x2)) + "," + hex(std::uint32_t(y2))});
                }
    for (unsigned n = 0; n < 200; ++n) {
        const auto x1 = std::int32_t(random()), y1 = std::int32_t(random() % 400) - 200, x2 = std::int32_t(random() % 600) - 300, y2 = std::int32_t(random());
        cases.push_back({.entry = corners_entry, .registers = others({{3, std::uint32_t(x1)}, {4, std::uint32_t(y1)}, {5, std::uint32_t(x2)}, {6, std::uint32_t(y2)}, {7, rect_at + 2 * (n % 3)}}),
                         .memory = rectangle_pattern(), .label = "leaves: corners random " + std::to_string(n)});
    }
    // The rectangle over the argument registers' own source: the pointer is the same as a corner (nothing is read back).
    cases.push_back({.entry = corners_entry, .registers = others({{3, 5}, {4, 9}, {5, 2}, {6, 1}, {7, rect_at}}), .memory = rectangle_pattern(),
                     .label = "leaves: corners small"});

    // ---- 0x10769920: the area of the intersection of two rectangles (left, top, right, bottom each). Small coordinates make every
    // relation (disjoint on one axis, touching, contained, equal, empty or inverted rectangles); the extremes wrap the differences and the
    // product (0x10000 x 0x10000 is 0).
    for (unsigned n = 0; n < 1200; ++n) {
        const auto pick = [&] { return std::int32_t(random() % 9) - 4; };
        const std::int32_t a[4] = {pick(), pick(), pick(), pick()}, b[4] = {pick(), pick(), pick(), pick()};
        cases.push_back({.entry = area_entry, .registers = others({{3, std::uint32_t(a[0])}, {4, std::uint32_t(a[1])}, {5, std::uint32_t(a[2])}, {6, std::uint32_t(a[3])},
                                                                    {7, std::uint32_t(b[0])}, {8, std::uint32_t(b[1])}, {9, std::uint32_t(b[2])}, {10, std::uint32_t(b[3])}}),
                         .label = "leaves: area small " + std::to_string(n)});
    }
    const std::int32_t large[] = {INT_MIN, INT_MIN + 1, -0x10000, -1, 0, 1, 0x10000, 0x7fff0000, INT_MAX};
    for (unsigned n = 0; n < 1500; ++n) {
        const auto pick = [&] { return large[random() % 9]; };
        const std::int32_t a[4] = {pick(), pick(), pick(), pick()}, b[4] = {pick(), pick(), pick(), pick()};
        cases.push_back({.entry = area_entry, .registers = others({{3, std::uint32_t(a[0])}, {4, std::uint32_t(a[1])}, {5, std::uint32_t(a[2])}, {6, std::uint32_t(a[3])},
                                                                    {7, std::uint32_t(b[0])}, {8, std::uint32_t(b[1])}, {9, std::uint32_t(b[2])}, {10, std::uint32_t(b[3])}}),
                         .label = "leaves: area large " + std::to_string(n)});
    }
    // Wide rectangles whose product wraps: (2^16) x (2^16) = 0 and (65537) x (65537).
    for (const std::int32_t side : {0x10000, 0x10001, 0x20000, 0x7fffffff}) {
        cases.push_back({.entry = area_entry, .registers = others({{3, 0}, {4, 0}, {5, std::uint32_t(side)}, {6, std::uint32_t(side)}, {7, 0}, {8, 0}, {9, std::uint32_t(side)}, {10, std::uint32_t(side)}}),
                         .label = "leaves: area wrapping " + hex(std::uint32_t(side))});
    }

    // ---- 0x1076bb20: the 15-bit pixel of a red, green and blue. Only bits 3-7 of each count; every byte of each component alone, the three
    // together, and random words (the high bits of the registers must not leak in).
    for (std::uint32_t v = 0; v < 256; ++v) {
        cases.push_back({.entry = pack_entry, .registers = others({{3, v}, {4, 0}, {5, 0}}), .label = "leaves: pack red " + hex(v)});
        cases.push_back({.entry = pack_entry, .registers = others({{3, 0}, {4, v}, {5, 0}}), .label = "leaves: pack green " + hex(v)});
        cases.push_back({.entry = pack_entry, .registers = others({{3, 0}, {4, 0}, {5, v}}), .label = "leaves: pack blue " + hex(v)});
        cases.push_back({.entry = pack_entry, .registers = others({{3, v}, {4, v}, {5, v}}), .label = "leaves: pack grey " + hex(v)});
    }
    for (unsigned n = 0; n < 400; ++n)
        cases.push_back({.entry = pack_entry, .registers = others({{3, std::uint32_t(random())}, {4, std::uint32_t(random())}, {5, std::uint32_t(random())}}),
                         .label = "leaves: pack random " + std::to_string(n)});

    // ---- 0x1076bb40: the pixel scaled by a percentage. The first cases are the compiler's divide by 100 over every product the routine can
    // form (a component widened to 8 bits is a multiple of 8, 0..248, times a factor 0..255: all 32 x 256 in the grey pixels), then pixels
    // with every component alone at every factor step, the full 15 bits, bits above the pixel (ignored) and the factor's high bytes (ignored).
    for (std::uint32_t level = 0; level < 32; ++level)
        for (std::uint32_t factor = 0; factor < 256; ++factor)
            cases.push_back({.entry = scale_entry, .registers = others({{3, level << 10 | level << 5 | level}, {4, factor}}),
                             .label = "leaves: scale grey " + hex(level) + " by " + hex(factor)});
    for (std::uint32_t level = 0; level < 32; ++level)
        for (const std::uint32_t factor : {0u, 1u, 13u, 50u, 99u, 100u, 101u, 150u, 199u, 200u, 255u}) {
            cases.push_back({.entry = scale_entry, .registers = others({{3, level << 10}, {4, factor}}), .label = "leaves: scale red " + hex(level) + " by " + hex(factor)});
            cases.push_back({.entry = scale_entry, .registers = others({{3, level << 5}, {4, factor}}), .label = "leaves: scale green " + hex(level) + " by " + hex(factor)});
            cases.push_back({.entry = scale_entry, .registers = others({{3, level}, {4, factor}}), .label = "leaves: scale blue " + hex(level) + " by " + hex(factor)});
        }
    for (unsigned n = 0; n < 1500; ++n)
        cases.push_back({.entry = scale_entry, .registers = others({{3, std::uint32_t(random())}, {4, std::uint32_t(random())}}),
                         .label = "leaves: scale random " + std::to_string(n)});
    if (exhaustive())
        for (std::uint32_t pixel = 0; pixel < 0x8000; pixel += 7)
            for (const std::uint32_t factor : {75u, 255u})
                cases.push_back({.entry = scale_entry, .registers = others({{3, pixel}, {4, factor}}), .label = "leaves: scale sweep " + hex(pixel) + " by " + hex(factor)});

    // ---- 0x1076ca20: the text callback: a counter, the limit and the captured value in the small data. The count at, below and above the
    // limit, negative and at the word's edges (the increment wraps), and values whose halfword has its sign bit set or clear, with high
    // bits that must be dropped.
    const auto probe = [&](std::int32_t count, std::int32_t limit, std::uint32_t value, std::uint32_t captured) {
        cases.push_back({.entry = probe_entry, .registers = others({{3, value}}),
                         .memory = {w32(small(small_data::text_probe_count), std::uint32_t(count)), w32(small(small_data::text_probe_limit), std::uint32_t(limit)),
                                    w32(small(small_data::text_probe_captured), captured)},
                         .label = "leaves: probe count " + hex(std::uint32_t(count)) + " limit " + hex(std::uint32_t(limit)) + " value " + hex(value)});
    };
    cases.push_back({.entry = probe_entry, .registers = others({{3, 0x1234}}), .label = "leaves: probe, the snapshot's state"});
    for (const std::uint32_t value : {0u, 1u, 0x7fffu, 0x8000u, 0xffffu, 0x12345678u, 0xfedc8765u, 0xffff8000u, 0x7fff8000u}) {
        probe(0, 0, value, 0x5555aaaau);
        probe(3, 3, value, 0xffffffffu);
        probe(3, 4, value, 0x5555aaaau);
        probe(4, 3, value, 0x5555aaaau);
        probe(-1, -1, value, 0x1234u);
        probe(INT_MAX, INT_MAX, value, 0);
        probe(INT_MIN, INT_MIN, value, 0);
        probe(-1, 0, value, 7);
    }
    for (unsigned n = 0; n < 200; ++n) {
        const auto count = std::int32_t(random() % 7) - 2;
        probe(count, std::int32_t(random() % 5) - 1, std::uint32_t(random()), std::uint32_t(random()));
    }
}
}
