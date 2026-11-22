// The font engine, block `style` (0x1075e0c0..0x1075e960): cases for the bold style's three routines. The game runs the outline and
// advance routines (the end-to-end run drives them at every size and style with the shipped fonts' real glyphs: 42,808 outline calls); these
// cases add what a run does not reach: degenerate contours, the strength at its limits, runs of equal coordinates of every length,
// every pairing of the edge directions, the halfword wraps and a zero divisor. Records are built in the scratch area (a glyph record with its
// halfword arrays, a list of advances, a strength word) around a real tsiMemObject of the snapshot, so the copy the outline routine allocates
// and releases goes through the real allocator on both sides. Labels start with "style:" (CM_FONT_ENGINE_BLOCK=style).
#include "font_engine_checks.hpp"
#include <algorithm>
#include <array>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t corner = 0x1075e0c0, advances_entry = 0x1075e1a0, outline = 0x1075e320;
constexpr std::uint32_t game_malloc = 0x10775130, game_free = 0x10775170, reporter = 0x107755c0;
constexpr std::uint32_t area = scratch_base + 0x60000, record_at = area, factor_at = area + 0x80, list_at = area + 0x100, arrays_at = area + 0x1000;

struct Point { int x, y; };
using Contour = std::vector<Point>;

// A tsiMemObject of the snapshot (stamps +0 and +0x178) with room in its table, for the copy the outline routine allocates.
std::uint32_t memory_object(const Scenario& scenario) {
    for (const auto& [low, size] : scenario.s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x17c <= std::uint64_t(low) + size; at += 4)
            if (scenario.r32(std::uint32_t(at)) == 0xab1500ff && scenario.r32(std::uint32_t(at) + 0x178) == 0xa5a55a5a && scenario.r32(std::uint32_t(at) + 8) == 0x1f5)
                return std::uint32_t(at);
    }
    return 0;
}

// A glyph record: the contour count and the four arrays (start points, end points, x, y) laid out in the scratch area.
struct Glyph {
    std::vector<Write> memory;
    std::uint32_t at = record_at;
};
Glyph glyph_of(const std::vector<Contour>& contours, int contour_count_field = -1) {
    Glyph glyph;
    std::uint32_t points = 0;
    for (const auto& contour : contours) points += std::uint32_t(contour.size());
    const std::uint32_t starts = arrays_at, ends = starts + 2 * std::uint32_t(contours.size()) + 4, x = ends + 2 * std::uint32_t(contours.size()) + 4,
                        y = x + 2 * points + 8;
    auto& m = glyph.memory;
    m.push_back(w16(record_at + 0x36, contour_count_field >= 0 ? std::uint32_t(contour_count_field) : std::uint32_t(contours.size())));
    for (const auto offset : {0x3cu, 0x40u, 0x44u, 0x48u}) m.push_back(w32(record_at + offset, offset == 0x3c ? starts : offset == 0x40 ? ends : offset == 0x44 ? x : y));
    std::uint32_t index = 0, contour_index = 0;
    for (const auto& contour : contours) {
        m.push_back(w16(starts + 2 * contour_index, index));
        m.push_back(w16(ends + 2 * contour_index, index + std::uint32_t(contour.size()) - 1));
        for (const auto& point : contour) {
            m.push_back(w16(x + 2 * index, std::uint32_t(point.x)));
            m.push_back(w16(y + 2 * index, std::uint32_t(point.y)));
            ++index;
        }
        ++contour_index;
    }
    return glyph;
}
std::vector<Write> concat(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }
}

FONT_ENGINE_CASES("style") {
    const auto mem = memory_object(scenario);
    if (!mem) return;
    std::mt19937 random(0x57911e);
    const auto fix = [](double value) { return std::uint32_t(std::int32_t(value * 65536)); };
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, const std::string& label) -> Case& {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .label = "style: " + label});
        cases.back().original_owners = {game_malloc, game_free};
        return cases.back();
    };
    // (size, factor) pairs and the strength each makes: size/12 times the factor less size/12, as the original rounds it.
    struct Strength { std::uint32_t size, factor; const char* name; };
    const std::array<Strength, 14> strengths = {{
        {9, 0x14000, "size 9"}, {14, 0x14000, "size 14 (0)"}, {36, 0x18000, "1"}, {48, 0x18000, "2"}, {72, 0x18000, "3"}, {96, 0x20000, "8"}, {120, 0x30000, "20"},
        {132, 0x8000, "-5"}, {200, 0x28000, "25"}, {600, 0x10000, "1.0"}, {0xffff, 0x14000, "size -1"}, {0x8000, 0x14000, "size -32768"},
        {2400, 0x3ffff, "big"}, {0x7fff, 0x7fffffff, "huge"}}};

    // ---- 0x1075e1a0: the advances ----
    for (const std::uint32_t count : {0u, 1u, 2u, 7u, 8u, 9u, 10u, 16u, 17u, 25u, 0xffffffffu, 0x80000000u}) {
        for (const auto& strength : strengths) {
            if (count > 30 && &strength != &strengths[2]) continue;
            std::vector<Write> memory{w32(list_at + 4, count), w32(list_at + 0x10, area + 0x400), w32(factor_at, strength.factor)};
            for (std::uint32_t n = 0; n < 40; ++n) memory.push_back(w16(area + 0x400 + 2 * n, n % 3 == 0 ? 0xffff - n : n % 3 == 1 ? 0x7fff + n : 100 * n));
            add(advances_entry, {{3, list_at}, {4, mem}, {5, strength.size}, {6, factor_at}}, memory,
                "advances count " + hex(count) + " " + strength.name);
        }
    }
    // The pointer to the halfwords is read again for each: a count past the data (the values wrap, the writes go on).
    add(advances_entry, {{3, list_at}, {4, mem}, {5, 36}, {6, factor_at}}, {w32(list_at + 4, 3), w32(list_at + 0x10, area + 0x400), w32(factor_at, 0x18000)}, "advances, empty data");

    // ---- 0x1075e0c0: the corner of two edges ----
    const std::array<int, 12> edges = {0, 1, -1, 5, -5, 100, -100, 0x7fff, -0x8000, 0x1234, -0x4321, 32};
    const auto corner_case = [&](std::array<std::uint32_t, 8> v, const std::string& label) {
        // The caller's frame: the two words the callee reads at sp + 0x38 (the addresses its results go to).
        add(corner, {{3, v[0]}, {4, v[1]}, {5, v[2]}, {6, v[3]}, {7, v[4]}, {8, v[5]}, {9, v[6]}, {10, v[7]}},
            {w32(scenario.sp() + 0x38, area + 0x200), w32(scenario.sp() + 0x3c, area + 0x210)}, "corner " + label);
    };
    for (unsigned n = 0; n < 700; ++n) {
        std::array<std::uint32_t, 8> v;
        for (auto& value : v) {
            const auto kind = random() % 4;
            value = kind == 0 ? std::uint32_t(edges[random() % edges.size()]) : kind == 1 ? std::uint32_t(std::int32_t(random() % 2001) - 1000) : kind == 2 ? std::uint32_t(random()) : std::uint32_t(std::int32_t(random() % 61) - 30);
        }
        corner_case(v, "random " + std::to_string(n));
    }
    // Parallel edges (a zero cross product): the midpoint, and the near parallel ones.
    for (const int offset : {0, 1, -1}) {
        corner_case({0, 0, 10, 10, 20, 20, 30, std::uint32_t(30 + offset)}, "parallel " + std::to_string(offset));
        corner_case({0, 0, 10, 0, 20, 5, 30, 5}, "horizontal");
        corner_case({std::uint32_t(-5), std::uint32_t(-7), std::uint32_t(-3), std::uint32_t(-9), std::uint32_t(-1), std::uint32_t(-11), 1, std::uint32_t(-13 + offset)}, "midpoint of negatives");
    }
    corner_case({0, 0, 0, 0, 0, 0, 0, 0}, "all zero");
    corner_case({0x7fff, 0x7fff, 0x8000, 0x8000, 0x7fff, 0x7fff, 0x8000, 0x8000}, "extremes");

    // ---- 0x1075e320: the outline ----
    const std::vector<std::vector<Contour>> shapes = {
        {{{0, 0}, {100, 0}, {100, 100}, {0, 100}}},                                          // square, counter-clockwise
        {{{0, 0}, {0, 100}, {100, 100}, {100, 0}}},                                          // clockwise
        {{{0, 0}, {120, 0}, {60, 100}}},                                                     // triangle
        {{{0, 0}, {60, 0}, {60, 40}, {100, 40}, {100, 100}, {0, 100}}},                      // L
        {{{40, 0}, {60, 0}, {60, 40}, {100, 40}, {100, 60}, {60, 60}, {60, 100}, {40, 100}, {40, 60}, {0, 60}, {0, 40}, {40, 40}}},  // plus
        {{{0, 0}, {100, 0}, {100, 100}, {0, 100}}, {{20, 20}, {20, 80}, {80, 80}, {80, 20}}},  // a ring
        {{{5, 5}}},                                                                          // one point
        {{{5, 5}, {50, 90}}},                                                                // two points
        {{{5, 5}, {5, 5}, {5, 5}, {5, 5}}},                                                  // the same point
        {{{0, 0}, {10, 10}, {20, 20}, {30, 30}, {40, 40}}},                                  // collinear
        {{{0, 0}, {0, 10}, {0, 20}, {0, 30}, {0, 40}, {0, 50}}},                             // a vertical line: parallel edges, equal x
        {{{0, 0}, {50, 0}, {100, 0}, {150, 0}}},                                             // a horizontal line
        {{{-32768, -32768}, {32767, -32768}, {32767, 32767}, {-32768, 32767}}},              // the halfword extremes
        {{{32767, 0}, {-32768, 100}, {32767, 200}, {-32768, 300}, {32767, 400}}},            // zigzag across the range
        {{{0, 0}, {10, 0}, {10, 10}, {0, 10}}, {{50, 50}}, {{60, 60}, {70, 60}, {70, 70}}},  // three contours
    };
    const std::array<std::size_t, 7> picked = {2, 3, 4, 5, 7, 9, 11};  // strengths used with every shape: the ones that offset (and size 9)
    for (std::size_t s = 0; s < shapes.size(); ++s) {
        const auto glyph = glyph_of(shapes[s]);
        for (const auto index : picked) {
            const auto& strength = strengths[index];
            add(outline, {{3, glyph.at}, {4, mem}, {5, strength.size}, {6, factor_at}}, concat(glyph.memory, {w32(factor_at, strength.factor)}),
                "outline shape " + std::to_string(s) + " strength " + strength.name);
        }
        for (const std::size_t index : {0u, 1u, 12u, 13u}) {
            const auto& strength = strengths[index];
            add(outline, {{3, glyph.at}, {4, mem}, {5, strength.size}, {6, factor_at}}, concat(glyph.memory, {w32(factor_at, strength.factor)}),
                "outline shape " + std::to_string(s) + " strength " + strength.name);
        }
    }
    // The strength that makes the corner limit the halfword minimum (strength +-16384: 2 * strength wraps): a corner at distance zero divides by zero.
    for (const std::uint32_t factor : {0x18000u, 0x28000u}) {
        const auto glyph = glyph_of(shapes[3]);
        add(outline, {{3, glyph.at}, {4, mem}, {5, 12 * 16384u}, {6, factor_at}}, concat(glyph.memory, {w32(factor_at, factor)}), "outline strength 16384 factor " + hex(factor));
    }
    // Contour counts and end points at their edges.
    for (const int count : {0, -1, -32768, 1, 2}) {
        const auto glyph = glyph_of(shapes[5], count);
        add(outline, {{3, glyph.at}, {4, mem}, {5, 72}, {6, factor_at}}, concat(glyph.memory, {w32(factor_at, 0x18000)}), "outline contour count " + std::to_string(count));
    }
    // A contour count past the arrays: the passes read garbage start and end points (the stores of the first passes land on the record itself,
    // so a later pass sees a contour count of zero: r9/r10 then come from the loops that ran).
    for (const int count : {3, 5, 11, 12}) {
        const auto glyph = glyph_of(shapes[7], count);
        add(outline, {{3, glyph.at}, {4, mem}, {5, strengths[12].size}, {6, factor_at}}, concat(glyph.memory, {w32(factor_at, strengths[12].factor)}),
            "outline count " + std::to_string(count) + " over a two-point glyph");
    }
    {   // The last end point is -1 (no points), a contour that ends before it starts, and a start past the data.
        auto glyph = glyph_of(shapes[0]);
        auto memory = glyph.memory;
        memory.push_back(w16(arrays_at + 2 * 1 + 4 + 0, 0xffff));
        add(outline, {{3, glyph.at}, {4, mem}, {5, 72}, {6, factor_at}}, concat(memory, {w32(factor_at, 0x18000)}), "outline end point -1 (shape written over)");
        const auto g2 = glyph_of(shapes[4]);
        std::vector<Write> empty_contour = g2.memory;
        // start 3, end 2 for the only contour: an empty contour
        empty_contour.push_back(w16(arrays_at, 3));
        empty_contour.push_back(w16(arrays_at + 2 + 4, 2));
        add(outline, {{3, g2.at}, {4, mem}, {5, 72}, {6, factor_at}}, concat(empty_contour, {w32(factor_at, 0x18000)}), "outline empty contour");
    }
    // Runs of equal original x of every length around the unrolled store loop (2..20 points on one vertical), and corners that differ.
    for (unsigned run = 1; run <= 20; ++run) {
        Contour contour;
        contour.push_back({0, 0});
        for (unsigned n = 0; n < run; ++n) contour.push_back({50, int(10 * n + 5)});
        contour.push_back({100, 200});
        contour.push_back({-50, 100});
        const auto glyph = glyph_of({contour});
        for (const std::size_t index : {3u, 5u}) add(outline, {{3, glyph.at}, {4, mem}, {5, strengths[index].size}, {6, factor_at}}, concat(glyph.memory, {w32(factor_at, strengths[index].factor)}),
                                                      "outline run of " + std::to_string(run) + " strength " + strengths[index].name);
    }
    // The last point of the contour differs from a run of equal x before it: the final step of the third pass averages (and leaves the registers of its
    // store loops, unrolled by eight, as the routine's return state).
    for (unsigned run = 2; run <= 21; ++run) {
        Contour contour;
        for (unsigned n = 0; n < run; ++n) contour.push_back({0, int(10 * n)});
        contour.push_back({70, 5});
        const auto glyph = glyph_of({contour});
        add(outline, {{3, glyph.at}, {4, mem}, {5, strengths[3].size}, {6, factor_at}}, concat(glyph.memory, {w32(factor_at, strengths[3].factor)}),
            "outline run of " + std::to_string(run) + " before the last point");
    }
    // Random outlines: contours of 1..30 points at glyph-like coordinates, with duplicated x and y values.
    for (unsigned n = 0; n < 220; ++n) {
        std::vector<Contour> contours(1 + random() % 3);
        const int range = random() % 3 == 0 ? 20 : random() % 2 ? 300 : 30000;
        for (auto& contour : contours) {
            contour.resize(1 + random() % 30);
            int last_x = 0, last_y = 0;
            for (auto& point : contour) {
                const auto choice = random() % 5;
                point.x = choice == 0 ? last_x : int(random() % (2 * range + 1)) - range;
                point.y = choice == 1 ? last_y : int(random() % (2 * range + 1)) - range;
                last_x = point.x;
                last_y = point.y;
            }
        }
        const auto glyph = glyph_of(contours);
        const auto& strength = strengths[picked[random() % picked.size()]];
        add(outline, {{3, glyph.at}, {4, mem}, {5, strength.size}, {6, factor_at}}, concat(glyph.memory, {w32(factor_at, strength.factor)}),
            "outline random " + std::to_string(n) + " strength " + strength.name);
    }
    (void)fix;
    (void)reporter;
}
}
