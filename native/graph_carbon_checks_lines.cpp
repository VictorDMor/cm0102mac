// graph_carbon.cpp, block `lines` (0x1076a140, 0x1076a8f0, 0x1076ad50): cases for the line, the circle's octants and the ellipse. Every case
// writes the surface's three variables (graph_carbon_pixels_detail.hpp) into the TOC's small data and points them at a pixel area in the
// scratch memory, which `prepare` fills with a pattern in each compared run (so a pixel written to a wrong place, with a wrong colour or by
// a wrong store size shows in the memory comparison, and the shaded and recoloured pixels of the fills have something to work on). The
// arguments in r9 and r10 are distinct values nothing reads: a routine that changes them differently shows in the full comparison of r3-r10.
// Labels start with "lines:".
#include "graph_carbon_checks.hpp"
#include "graph_carbon_pixels_detail.hpp"
#include <array>
#include <climits>
#include <cstdlib>
#include <random>

namespace cm::aot::graph_carbon_checks {
namespace {
using namespace cm::aot::graph_carbon;
constexpr std::uint32_t game_toc = 0x108bba90;
constexpr std::uint32_t line_entry = 0x1076a140, octant_entry = 0x1076a8f0, ellipse_entry = 0x1076ad50;
constexpr std::uint32_t allocate_block = 0x107735f0, release_block = 0x10773610;
// Pixel (0, 0) of the synthetic surface and the area `prepare` patterns (the surface's rows run both ways from it).
constexpr std::uint32_t surface_at = scratch_base + 0x50000, pattern_from = surface_at - 0x4000, pattern_to = surface_at + 0x8000;
constexpr std::uint32_t table_at = scratch_base + 0x90000, table_size = 0x400;
constexpr std::uint32_t small(std::int32_t offset) { return game_toc + std::uint32_t(offset); }
std::uint64_t mix(std::uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33;
    return x;
}
// A pattern over [from, to) of 8-byte words (a pixel is the low half of a word's part, so neighbours differ).
void patterned(State& s, std::uint32_t from, std::uint32_t to, std::uint64_t seed) {
    for (std::uint32_t at = from; at < to; at += 8) s.memory.write<8>(at, mix(at ^ seed));
}
std::vector<std::pair<unsigned, std::uint32_t>> registers_for(std::initializer_list<std::pair<unsigned, std::uint32_t>> used) {
    std::vector<std::pair<unsigned, std::uint32_t>> registers;
    for (unsigned n = 3; n <= 10; ++n) registers.emplace_back(n, 0x7a000000u + n * 0x01010101u);
    for (const auto& [n, value] : used) registers[n - 3].second = value;
    return registers;
}
// The surface's variables for a case: base, row stride (pixels, a signed halfword) and the suspension byte.
std::vector<Write> surface_variables(std::uint32_t base, std::int32_t stride, std::uint32_t suspended = 0) {
    return {w32(small(surface_variable::base), base), w16(small(surface_variable::row_pixels), std::uint32_t(stride)),
            w8(small(surface_variable::suspended), suspended)};
}
struct Place { std::uint32_t base = surface_at; std::int32_t stride = 64; std::uint32_t suspended = 0; };
Case line_case(const std::string& label, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t style, std::uint32_t colour,
               const Place& place = {}) {
    Case item{.entry = line_entry,
              .registers = registers_for({{3, std::uint32_t(x1)}, {4, std::uint32_t(y1)}, {5, std::uint32_t(x2)}, {6, std::uint32_t(y2)}, {7, style}, {8, colour}}),
              .memory = surface_variables(place.base, place.stride, place.suspended),
              .label = "lines: " + label};
    item.prepare = [](State& s) { patterned(s, pattern_from, pattern_to, 0x1111); };
    return item;
}
}

GRAPH_CARBON_CASES("lines") {
    std::mt19937 random(0x1d3a);
    std::vector<Case> rare, bulk;
    const auto take = [&](Case item) { bulk.push_back(std::move(item)); };

    // ---- 0x1076a140. The first cases are one of each shape (the call trace and the mutations look at the first few of an entry).
    rare.push_back(line_case("horizontal solid", 5, 9, 25, 9, 2, 0x7fff));
    rare.push_back(line_case("vertical solid", 9, 3, 9, 20, 2, 0x1234));
    rare.push_back(line_case("shallow solid", 5, 5, 40, 17, 2, 0x5555));
    rare.push_back(line_case("steep solid", 5, 5, 17, 40, 2, 0x2aaa));
    rare.push_back(line_case("horizontal dotted", 5, 9, 40, 9, 0, 0x7fff));
    rare.push_back(line_case("vertical dotted", 9, 3, 9, 40, 0, 0x1234));
    rare.push_back(line_case("shallow dotted", 40, 17, 5, 5, 0, 0x5555));
    rare.push_back(line_case("steep dotted", 17, 40, 5, 5, 0, 0x2aaa));
    rare.push_back(line_case("point", 12, 12, 12, 12, 0, 0x7fff));
    rare.push_back(line_case("suspended", 5, 5, 40, 17, 2, 0x5555, {.suspended = 1}));
    // The whole fan of slopes: every combination of the deltas below from (30, 25), solid and dotted. Each octant, the axes, the diagonals,
    // the single point and the equal |dx| = |dy| ties (which walk x).
    const std::int32_t deltas[] = {-12, -7, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 7, 12};
    for (const auto dx : deltas)
        for (const auto dy : deltas) {
            const std::string name = "fan " + std::to_string(dx) + "," + std::to_string(dy);
            take(line_case(name + " solid", 30, 25, 30 + dx, 25 + dy, 2, 0x4321));
            take(line_case(name + " dotted", 30, 25, 30 + dx, 25 + dy, 0, 0x0f0f));
        }
    // Longer lines (the dotted pattern repeats, the error terms run a long way) in all directions.
    const std::int32_t far_deltas[] = {-30, -17, -9, 0, 9, 17, 30};
    for (const auto dx : far_deltas)
        for (const auto dy : far_deltas) {
            const auto style = std::uint32_t(random() % 8);
            take(line_case("far " + std::to_string(dx) + "," + std::to_string(dy) + " style " + hex(style), 33, 31, 33 + dx, 31 + dy, style, 0x7c1f));
        }
    // Style bits and colour bits: bit 1 alone (not tested by the line), bit 2, higher bits, and a colour word whose upper half must not
    // reach the pixel (the store is a halfword).
    for (const std::uint32_t style : {0u, 1u, 2u, 3u, 4u, 0x10u, 0xfffdu, 0xffffffffu, 0x80000002u, 0x80000001u})
        for (const std::uint32_t colour : {0u, 0x7fffu, 0xffff1234u, 0x8000u, 0x00010001u})
            take(line_case("style " + hex(style) + " colour " + hex(colour), 20, 20, 49, 36, style, colour));
    for (const std::uint32_t style : {0u, 2u})
        for (const std::uint32_t colour : {0x7fffu, 0xffff8001u}) {
            take(line_case("horizontal style " + hex(style) + " colour " + hex(colour), 8, 14, 40, 14, style, colour));
            take(line_case("vertical style " + hex(style) + " colour " + hex(colour), 14, 8, 14, 40, style, colour));
        }
    // Strides: pixels per row other than the width (one, three, a long row, none, negative: the pixels go up the area).
    for (const std::int32_t stride : {1, 3, 100, 0, -64, -3, 0x1000}) {
        const std::uint32_t base = stride < 0 ? surface_at + 0x4000 : surface_at;
        for (const auto [x1, y1, x2, y2] : {std::array<int, 4>{4, 4, 30, 9}, {4, 4, 9, 30}, {4, 8, 30, 8}, {8, 4, 8, 30}, {6, 6, 6, 6}, {20, 20, 2, 7}})
            for (const std::uint32_t style : {0u, 2u})
                take(line_case("stride " + std::to_string(stride) + " line " + std::to_string(x1) + "," + std::to_string(y1) + " " + std::to_string(x2) + "," + std::to_string(y2) + " style " + hex(style),
                               x1, y1, x2, y2, style, 0x3def, {.base = base, .stride = stride}));
    }
    // Coordinates beyond 16 bits: the bounding box is the halfwords of the extremes, so a line whose raw coordinates differ from the
    // halfwords is clipped to a box that may be empty or inverted. (Large spans: a few of them, the walk is long.) A horizontal or vertical
    // line is decided by the halfwords too: y1 = 0x10007, y2 = 7 is "horizontal".
    for (const std::uint32_t style : {2u, 0u}) {
        take(line_case("halfword horizontal " + hex(style), 5, 0x10007, 25, 7, style, 0x6666));
        take(line_case("halfword vertical " + hex(style), 0x10009, 3, 9, 20, style, 0x6666));
        take(line_case("halfword equal both " + hex(style), 0x10005, 0x20006, 5, 6, style, 0x6666));
        take(line_case("halfword inverted box x " + hex(style), 0x10000 + 40, 10, 5, 30, style, 0x1f1f));
        take(line_case("halfword steep clipped y " + hex(style), 10, 0x10000 + 30, 14, 5, style, 0x1f1f));
        take(line_case("halfword shallow clipped x " + hex(style), 0x10000 + 40, 12, 10, 20, style, 0x0f0f));
        take(line_case("halfword negative " + hex(style), -0x10000 + 20, 10, 5, -0x10000 + 14, style, 0x0f0f));
        take(line_case("halfword sign flip " + hex(style), 0x8005, 8, 0x7ff0, 30, style, 0x0f0f));
        take(line_case("negative coordinates " + hex(style), -5, -3, 20, 9, style, 0x1111));
        take(line_case("negative steep " + hex(style), -3, -9, 4, 20, style, 0x1111));
        take(line_case("negative horizontal " + hex(style), -9, -4, 9, -4, style, 0x1111));
        take(line_case("negative vertical " + hex(style), -4, -9, -4, 9, style, 0x1111));
    }
    take(line_case("minimum far x", INT_MIN, 7, 30, 7, 2, 0x7fff));
    take(line_case("maximum far y", 7, 5, 7, INT_MAX, 0, 0x7fff));
    // Random lines, solid and dotted, any direction, on the synthetic surface and on a wide one.
    for (unsigned n = 0; n < 700; ++n) {
        const auto pick = [&](int low, int high) { return low + int(random() % unsigned(high - low + 1)); };
        const auto x1 = pick(0, 60), y1 = pick(0, 45), x2 = pick(0, 60), y2 = pick(0, 45);
        const std::uint32_t style = random() % 8;
        take(line_case("random " + std::to_string(n) + " " + std::to_string(x1) + "," + std::to_string(y1) + " " + std::to_string(x2) + "," + std::to_string(y2) + " style " + hex(style),
                       x1, y1, x2, y2, style, std::uint32_t(random()) & 0xffff));
    }
    // The snapshot's own surface and its variables, with typical rectangle edges.
    {
        const auto real_base = scenario.r32(small(surface_variable::base)), real_stride = scenario.u16(small(surface_variable::row_pixels));
        if (real_base != 0)
            for (const auto [x1, y1, x2, y2] : {std::array<int, 4>{10, 10, 100, 10}, {10, 10, 10, 70}, {10, 10, 90, 55}, {90, 55, 10, 10}, {20, 60, 70, 12}}) {
                auto item = line_case("snapshot surface " + std::to_string(x1) + "," + std::to_string(y1) + " " + std::to_string(x2) + "," + std::to_string(y2), x1, y1, x2, y2, 0, 0x7fe0);
                item.memory.clear();
                item.prepare = nullptr;
                (void)real_stride;
                take(std::move(item));
            }
    }
    // CM_GRAPH_CARBON_EXHAUSTIVE=1: every delta pair of a 41 x 41 fan, solid and dotted, from two starts (run once by hand).
    if (std::getenv("CM_GRAPH_CARBON_EXHAUSTIVE"))
        for (int dx = -20; dx <= 20; ++dx)
            for (int dy = -20; dy <= 20; ++dy)
                for (const std::uint32_t style : {0u, 2u}) {
                    take(line_case("sweep " + std::to_string(dx) + "," + std::to_string(dy) + " style " + hex(style), 25, 22, 25 + dx, 22 + dy, style, 0x6b5a));
                    take(line_case("sweep back " + std::to_string(dx) + "," + std::to_string(dy) + " style " + hex(style), 25 + dx, 22 + dy, 25, 22, style, 0x2d6b));
                }
    for (auto& item : rare) cases.push_back(std::move(item));
    for (auto& item : bulk) cases.push_back(std::move(item));

    // ---- 0x1076a8f0: the octants of a circle. The ninth argument is the stack word at the entry's sp + 0x38 (a table of rows, or none).
    const auto octant = [&](const std::string& label, std::int32_t i, std::int32_t j, std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom,
                            std::uint32_t colour, std::uint32_t table, const Place& place = {}) {
        Case item{.entry = octant_entry,
                  .registers = registers_for({{3, surface_at}, {4, std::uint32_t(i)}, {5, std::uint32_t(j)}, {6, std::uint32_t(left)}, {7, std::uint32_t(top)},
                                              {8, std::uint32_t(right)}, {9, std::uint32_t(bottom)}, {10, colour}}),
                  .memory = surface_variables(place.base, place.stride, place.suspended),
                  .label = "lines: octants " + label};
        item.memory.push_back(w32(scenario.sp() + 0x38, table));
        item.prepare = [](State& s) {
            patterned(s, pattern_from, pattern_to, 0x2222);
            for (std::uint32_t at = table_at; at < table_at + table_size; at += 8) {
                // Rows start as the ellipse's (800, 0) or as random words, so the minimum and maximum updates take both outcomes.
                const auto word = mix(at ^ 0x3333);
                s.memory.write<4>(at, (word & 1) ? 800 : std::uint32_t(word >> 8) % 90);
                s.memory.write<4>(at + 4, (word & 2) ? 0 : std::uint32_t(word >> 24) % 90);
            }
        };
        return item;
    };
    cases.push_back(octant("first", 0, 6, 10, 10, 30, 28, 0x7fff, table_at));
    cases.push_back(octant("no table", 0, 6, 10, 10, 30, 28, 0x7fff, 0));
    // Each (i, j) step of a circle of radius 9 (the box 10..28 square) as the ellipse walks it, with and without the table, and the clip
    // rectangle smaller than the circle on each side so every plot's four tests take both outcomes.
    for (std::int32_t i = 0; i <= 9; ++i)
        for (std::int32_t j = 9; j >= 0; --j) {
            cases.push_back(octant("circle " + std::to_string(i) + "," + std::to_string(j), i, j, 10, 10, 28, 28, 0x0abc + std::uint32_t(i), (i + j) % 3 ? table_at : 0));
        }
    for (const auto [left, top, right, bottom] : {std::array<int, 4>{14, 10, 28, 28}, {10, 14, 28, 28}, {10, 10, 24, 28}, {10, 10, 28, 24}, {17, 17, 21, 21}, {10, 10, 11, 11}, {10, 10, 10, 10}, {28, 28, 10, 10}, {5, 5, 40, 40}})
        for (const auto [i, j] : {std::array<int, 2>{0, 9}, {3, 8}, {6, 7}, {9, 0}, {4, 4}, {7, 5}})
            cases.push_back(octant("clip " + std::to_string(left) + "," + std::to_string(top) + "," + std::to_string(right) + "," + std::to_string(bottom) + " step " + std::to_string(i) + "," + std::to_string(j),
                                   i, j, left, top, right, bottom, 0x1357, table_at));
    // The loops' bounds: i or j at 16-bit limits (the first loop runs the 16-bit step from i, and not at all when i + 1 leaves 16 bits), and
    // random steps with odd rectangle sizes (the centre rounds toward zero) and negative corners.
    for (const std::int32_t i : {0x7fff, -0x8000, -0x8001, 0x10000, -0x10000, -1, 0x7ffe})
        for (const std::int32_t j : {5, 0x7fff, -0x8000, -1})
            if (i != -0x10000 && i != 0x10000) cases.push_back(octant("limits " + hex(std::uint32_t(i)) + "," + hex(std::uint32_t(j)), i, j, 4, 4, 20, 20, 0x2468, table_at));
    cases.push_back(octant("limit -0x10000 steps", -0x10000, 3, 8, 8, 20, 20, 0x2468, table_at));
    for (unsigned n = 0; n < 200; ++n) {
        const auto pick = [&](int low, int high) { return low + int(random() % unsigned(high - low + 1)); };
        const auto left = pick(-4, 20), top = pick(-4, 20), right = pick(-4, 30), bottom = pick(-4, 30), i = pick(-3, 12), j = pick(-3, 12);
        cases.push_back(octant("random " + std::to_string(n), i, j, left, top, right, bottom, std::uint32_t(random()), (n % 4) ? table_at : 0));
    }
    cases.push_back(octant("suspended is not tested", 3, 8, 10, 10, 28, 28, 0x7fff, table_at, {.suspended = 1}));

    // ---- 0x1076ad50: the ellipse (a circle of the box's height). The allocator runs its original body (the table of rows comes from it).
    const auto ellipse = [&](const std::string& label, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t style, std::uint32_t colour,
                             const Place& place = {}) {
        Case item{.entry = ellipse_entry,
                  .registers = registers_for({{3, std::uint32_t(x1)}, {4, std::uint32_t(y1)}, {5, std::uint32_t(x2)}, {6, std::uint32_t(y2)}, {7, style}, {8, colour}}),
                  .memory = surface_variables(place.base, place.stride, place.suspended),
                  .label = "lines: ellipse " + label,
                  .original_owners = {allocate_block, release_block}};
        item.prepare = [](State& s) { patterned(s, pattern_from, pattern_to, 0x4444); };
        return item;
    };
    std::vector<Case> ellipses;
    ellipses.push_back(ellipse("filled", 10, 8, 30, 28, 4, 0x7fff));
    ellipses.push_back(ellipse("outline solid", 10, 8, 30, 28, 2, 0x7fff));
    ellipses.push_back(ellipse("outline dotted", 10, 8, 30, 28, 0, 0x7fff));
    ellipses.push_back(ellipse("filled and solid", 10, 8, 30, 28, 6, 0x4321));
    for (const std::int32_t height : {-3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20, 25, 33, 40})
        for (const std::int32_t width : {-2, 0, 1, 5, 12, 25})
            for (const std::uint32_t style : {4u, 2u, 0u, 5u}) {
                if (style != 4 && (width != 12 || (height & 1))) continue;
                ellipses.push_back(ellipse("h " + std::to_string(height) + " w " + std::to_string(width) + " style " + hex(style), 9, 6, 9 + width, 6 + height, style, 0x5a5a));
            }
    // The table's allocation fails: the first and only malloc of the routine answers 0 (the owners run their original bodies).
    {
        auto item = ellipse("malloc fails", 10, 8, 30, 28, 4, 0x7fff);
        item.nth_answers = {{allocate_block, 1, 0}};
        ellipses.push_back(std::move(item));
        auto small_item = ellipse("malloc fails small", 10, 8, 18, 15, 5, 0x7fff);
        small_item.nth_answers = {{allocate_block, 1, 0}};
        ellipses.push_back(std::move(small_item));
    }
    ellipses.push_back(ellipse("suspended", 10, 8, 30, 28, 4, 0x7fff, {.suspended = 1}));
    for (const std::int32_t stride : {1, 100, 0, -64}) {
        const std::uint32_t base = stride < 0 ? surface_at + 0x4000 : surface_at;
        ellipses.push_back(ellipse("stride " + std::to_string(stride), 6, 6, 20, 19, 4, 0x3def, {.base = base, .stride = stride}));
        ellipses.push_back(ellipse("stride dotted " + std::to_string(stride), 6, 6, 20, 19, 0, 0x3def, {.base = base, .stride = stride}));
    }
    for (const std::uint32_t colour : {0x0u, 0xffff0001u, 0x8000u})
        ellipses.push_back(ellipse("colour " + hex(colour), 8, 8, 24, 24, 4, colour));
    for (unsigned n = 0; n < 150; ++n) {
        const auto pick = [&](int low, int high) { return low + int(random() % unsigned(high - low + 1)); };
        const auto x1 = pick(0, 30), y1 = pick(0, 20), w = pick(-1, 30), h = pick(-1, 30);
        const std::uint32_t style = random() % 8;
        ellipses.push_back(ellipse("random " + std::to_string(n) + " box " + std::to_string(x1) + "," + std::to_string(y1) + "+" + std::to_string(w) + "," + std::to_string(h) + " style " + hex(style),
                                   x1, y1, x1 + w, y1 + h, style, std::uint32_t(random()) & 0xffff));
    }
    for (auto& item : ellipses) cases.push_back(std::move(item));
}
}
