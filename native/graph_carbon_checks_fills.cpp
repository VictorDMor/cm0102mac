// graph_carbon.cpp, block `fills` (0x107697c0, 0x10769980, 0x1076a580, 0x1076a790): cases for the rectangle primitives. As in the lines block, each
// case points the surface's variables at a pixel area of the scratch memory that `prepare` fills with a pattern in each compared run, so the
// grey, the shade, the fill and the mask have something to change and a pixel in a wrong place shows in the memory comparison. r9 and r10 carry
// values nothing reads (the full comparison of r3-r10 sees a routine that changes them). Labels start with "fills:".
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
constexpr std::uint32_t grey_entry = 0x107697c0, shade_entry = 0x10769980, fill_entry = 0x1076a580, mask_entry = 0x1076a790;
constexpr std::uint32_t surface_at = scratch_base + 0x50000, pattern_from = surface_at - 0x4000, pattern_to = surface_at + 0x8000;
constexpr std::uint32_t bitmap_at = scratch_base + 0xa0000;
constexpr std::uint32_t small(std::int32_t offset) { return game_toc + std::uint32_t(offset); }
std::uint64_t mix(std::uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33;
    return x;
}
void patterned(State& s, std::uint32_t from, std::uint32_t to, std::uint64_t seed) {
    for (std::uint32_t at = from; at < to; at += 8) s.memory.write<8>(at, mix(at ^ seed));
}
std::vector<std::pair<unsigned, std::uint32_t>> registers_for(std::initializer_list<std::pair<unsigned, std::uint32_t>> used) {
    std::vector<std::pair<unsigned, std::uint32_t>> registers;
    for (unsigned n = 3; n <= 10; ++n) registers.emplace_back(n, 0x7a000000u + n * 0x01010101u);
    for (const auto& [n, value] : used) registers[n - 3].second = value;
    return registers;
}
std::vector<Write> surface_variables(std::uint32_t base, std::int32_t stride, std::uint32_t suspended = 0) {
    return {w32(small(surface_variable::base), base), w16(small(surface_variable::row_pixels), std::uint32_t(stride)),
            w8(small(surface_variable::suspended), suspended)};
}
struct Place { std::uint32_t base = surface_at; std::int32_t stride = 64; std::uint32_t suspended = 0; };
std::string text(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2) {
    return std::to_string(x1) + "," + std::to_string(y1) + " " + std::to_string(x2) + "," + std::to_string(y2);
}
Case rectangle_case(std::uint32_t entry, const std::string& label, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, const Place& place,
                    std::initializer_list<std::pair<unsigned, std::uint32_t>> more = {}) {
    std::vector<std::pair<unsigned, std::uint32_t>> used{{3, std::uint32_t(x1)}, {4, std::uint32_t(y1)}, {5, std::uint32_t(x2)}, {6, std::uint32_t(y2)}};
    used.insert(used.end(), more.begin(), more.end());
    auto registers = registers_for({});
    for (const auto& [n, value] : used) registers[n - 3].second = value;
    Case item{.entry = entry, .registers = std::move(registers), .memory = surface_variables(place.base, place.stride, place.suspended), .label = "fills: " + label};
    item.prepare = [](State& s) { patterned(s, pattern_from, pattern_to, 0x5555); };
    return item;
}
}

GRAPH_CARBON_CASES("fills") {
    std::mt19937 random(0x5eed);
    const auto pick = [&](int low, int high) { return low + int(random() % unsigned(high - low + 1)); };
    const auto shade_table = scenario.toc(-0x3bd4);
    // The corners that make a rectangle in each of the four orders, the single pixel, the single row and column, and the extremes of the
    // halfwords (a corner beyond 16 bits is truncated: the rectangle may come out empty or inverted).
    struct Box { int x1, y1, x2, y2; };
    std::vector<Box> boxes = {{10, 10, 40, 30}, {40, 30, 10, 10}, {10, 30, 40, 10}, {40, 10, 10, 30}, {12, 12, 12, 12}, {5, 20, 60, 20}, {20, 5, 20, 40},
                              {0, 0, 63, 47}, {-4, -3, 10, 8}, {30, 20, 38, 27}, {30, 20, 39, 28}, {30, 20, 30 + 15, 20 + 2}, {30, 20, 30 + 16, 20 + 3}, {30, 20, 30 + 17, 20 + 4},
                              {0x10000 + 5, 8, 20, 12}, {5, 0x10000 + 8, 20, 12}, {5, 8, 0x10000 + 20, 12}, {5, 8, 20, 0x10000 + 12}, {0x10009, 0x10004, 7, 7},
                              {0x8003, 6, 20, 14}, {6, 0xffff, 20, 14}, {-0x10000 + 6, 4, 12, 9}, {7, 7, 7 + 7, 7 + 6}};
    for (int width = 0; width <= 20; ++width) boxes.push_back({6, 6, 6 + width, 9});
    for (int n = 0; n < 40; ++n) boxes.push_back({pick(-3, 50), pick(-3, 40), pick(-3, 50), pick(-3, 40)});

    // ---- 0x107697c0: the rectangle turned to grey.
    for (const auto& b : boxes)
        cases.push_back(rectangle_case(grey_entry, "grey " + text(b.x1, b.y1, b.x2, b.y2), b.x1, b.y1, b.x2, b.y2, {}));
    cases.push_back(rectangle_case(grey_entry, "grey suspended", 10, 10, 40, 30, {.suspended = 1}));
    cases.push_back(rectangle_case(grey_entry, "grey suspended byte 0x80", 10, 10, 40, 30, {.suspended = 0x80}));
    for (const std::int32_t stride : {1, 3, 100, 0, -64}) {
        const std::uint32_t base = stride < 0 ? surface_at + 0x4000 : surface_at;
        cases.push_back(rectangle_case(grey_entry, "grey stride " + std::to_string(stride), 4, 4, 20, 12, {.base = base, .stride = stride}));
    }
    {
        // The snapshot's own surface and pixels.
        const auto base = scenario.r32(small(surface_variable::base));
        if (base != 0)
            for (const auto& b : {Box{10, 10, 60, 40}, Box{40, 40, 12, 12}}) {
                auto item = rectangle_case(grey_entry, "grey snapshot surface " + text(b.x1, b.y1, b.x2, b.y2), b.x1, b.y1, b.x2, b.y2, {});
                item.memory.clear();
                item.prepare = nullptr;
                cases.push_back(std::move(item));
            }
    }

    // ---- 0x10769980: the rectangle through the table at 60%. The table's last entry decides whether it is built: unbuilt, the 65,536 calls
    // of the scale routine fill it (a few cases: each is a long run); built, the table is a random one (so the index into it is tested).
    const auto with_table = [&](Case item, bool built) {
        item.prepare = [shade_table, built](State& s) {
            patterned(s, pattern_from, pattern_to, 0x5555);
            for (std::uint32_t at = shade_table; at < shade_table + 0x20000; at += 8) s.memory.write<8>(at, mix(at ^ 0x7777));
            s.memory.write<2>(shade_table + 0x1fffe, built ? 0x1234 : 0);
        };
        return item;
    };
    cases.push_back(with_table(rectangle_case(shade_entry, "shade unbuilt table", 10, 10, 40, 30, {}), false));
    cases.push_back(with_table(rectangle_case(shade_entry, "shade built table", 10, 10, 40, 30, {}), true));
    cases.push_back(with_table(rectangle_case(shade_entry, "shade unbuilt table single pixel", 12, 12, 12, 12, {}), false));
    cases.push_back(with_table(rectangle_case(shade_entry, "shade unbuilt table empty by halfwords", 0x10009, 0x10004, 7, 7, {}), false));
    cases.push_back(with_table(rectangle_case(shade_entry, "shade unbuilt table suspended", 10, 10, 40, 30, {.suspended = 1}), false));
    cases.push_back(with_table(rectangle_case(shade_entry, "shade suspended", 10, 10, 40, 30, {.suspended = 1}), true));
    for (const auto& b : boxes)
        cases.push_back(with_table(rectangle_case(shade_entry, "shade " + text(b.x1, b.y1, b.x2, b.y2), b.x1, b.y1, b.x2, b.y2, {}), true));
    for (const std::int32_t stride : {1, 3, 100, 0, -64}) {
        const std::uint32_t base = stride < 0 ? surface_at + 0x4000 : surface_at;
        cases.push_back(with_table(rectangle_case(shade_entry, "shade stride " + std::to_string(stride), 4, 4, 20, 12, {.base = base, .stride = stride}), true));
    }
    {
        const auto base = scenario.r32(small(surface_variable::base));
        if (base != 0) {
            auto item = rectangle_case(shade_entry, "shade snapshot surface", 10, 10, 60, 40, {});
            item.memory.clear();
            item.prepare = nullptr;
            cases.push_back(std::move(item));
        }
    }

    // ---- 0x1076a580: the rectangle filled (style without bits 1 and 2: [x1, x2) x [y1, y2)) or its four edges as lines (bit 1, else bit 2).
    // The edge styles first (the trace pairs the first cases of an entry and the lines are the calls the leaver check watches).
    for (const std::uint32_t style : {1u, 2u, 3u, 0u})
        cases.push_back(rectangle_case(fill_entry, "fill style " + hex(style) + " 10,10 40,30", 10, 10, 40, 30, {}, {{7, style}, {8, 0x7fff}}));
    for (const auto& b : boxes) {
        for (const std::uint32_t style : {0u, 1u, 2u}) {
            if (style != 0 && (b.x1 > 20 || b.y1 > 20 || b.x1 < 0)) continue;
            cases.push_back(rectangle_case(fill_entry, "fill style " + hex(style) + " " + text(b.x1, b.y1, b.x2, b.y2), b.x1, b.y1, b.x2, b.y2, {},
                                           {{7, style}, {8, 0x2bcd}}));
        }
    }
    for (const std::uint32_t style : {0x4u, 0x8u, 0x10u, 0xfff8u, 0x80000000u, 0xfffffffcu, 0x5u, 0x6u, 0xffffffffu}) {
        cases.push_back(rectangle_case(fill_entry, "fill style " + hex(style) + " 6,6 30,18", 6, 6, 30, 18, {}, {{7, style}, {8, 0x1357}}));
    }
    for (const std::uint32_t colour : {0u, 0x7fffu, 0xffff8001u, 0x00010002u})
        for (const std::uint32_t style : {0u, 1u})
            cases.push_back(rectangle_case(fill_entry, "fill colour " + hex(colour) + " style " + hex(style), 6, 6, 26, 15, {}, {{7, style}, {8, colour}}));
    cases.push_back(rectangle_case(fill_entry, "fill suspended", 10, 10, 40, 30, {.suspended = 1}, {{7, 0}, {8, 0x7fff}}));
    cases.push_back(rectangle_case(fill_entry, "fill suspended edges", 10, 10, 40, 30, {.suspended = 1}, {{7, 1}, {8, 0x7fff}}));
    for (const std::int32_t stride : {1, 3, 100, 0, -64}) {
        const std::uint32_t base = stride < 0 ? surface_at + 0x4000 : surface_at;
        for (const std::uint32_t style : {0u, 1u, 2u})
            cases.push_back(rectangle_case(fill_entry, "fill stride " + std::to_string(stride) + " style " + hex(style), 4, 4, 22, 12, {.base = base, .stride = stride}, {{7, style}, {8, 0x3def}}));
    }
    for (unsigned n = 0; n < 120; ++n) {
        const auto x1 = pick(-2, 40), y1 = pick(-2, 30), x2 = pick(-2, 40), y2 = pick(-2, 30);
        const std::uint32_t style = std::uint32_t(random() % 5);
        cases.push_back(rectangle_case(fill_entry, "fill random " + std::to_string(n) + " style " + hex(style) + " " + text(x1, y1, x2, y2), x1, y1, x2, y2, {},
                                       {{7, style}, {8, std::uint32_t(random()) & 0xffff}}));
    }
    {
        const auto base = scenario.r32(small(surface_variable::base));
        if (base != 0)
            for (const std::uint32_t style : {0u, 1u, 2u}) {
                auto item = rectangle_case(fill_entry, "fill snapshot surface style " + hex(style), 10, 10, 60, 40, {}, {{7, style}, {8, 0x7fe0}});
                item.memory.clear();
                item.prepare = nullptr;
                cases.push_back(std::move(item));
            }
    }

    // CM_GRAPH_CARBON_EXHAUSTIVE=1: every width and height of each rectangle primitive around the unrolled loops' bounds (run once by hand).
    if (std::getenv("CM_GRAPH_CARBON_EXHAUSTIVE")) {
        for (int width = -3; width <= 41; ++width)
            for (int height = -2; height <= 5; ++height) {
                cases.push_back(rectangle_case(fill_entry, "fill sweep " + std::to_string(width) + "x" + std::to_string(height), 6, 5, 6 + width, 5 + height, {}, {{7, 0}, {8, 0x2bcd}}));
                cases.push_back(rectangle_case(grey_entry, "grey sweep " + std::to_string(width) + "x" + std::to_string(height), 6, 5, 6 + width, 5 + height, {}));
                cases.push_back(with_table(rectangle_case(shade_entry, "shade sweep " + std::to_string(width) + "x" + std::to_string(height), 6, 5, 6 + width, 5 + height, {}), true));
            }
    }

    // ---- 0x1076a790: the mask. A bitmap {width, height, width * height bytes} in the scratch area, each byte zero or not at random.
    const auto mask = [&](const std::string& label, std::int32_t x, std::int32_t y, std::uint32_t colour, std::uint32_t width, std::uint32_t height,
                          std::uint32_t density, const Place& place = {}, std::uint32_t bitmap = bitmap_at) {
        Case item{.entry = mask_entry,
                  .registers = registers_for({{3, std::uint32_t(x)}, {4, std::uint32_t(y)}, {5, colour}, {6, bitmap}}),
                  .memory = surface_variables(place.base, place.stride, place.suspended),
                  .label = "fills: mask " + label};
        item.prepare = [width, height, density, bitmap](State& s) {
            patterned(s, pattern_from, pattern_to, 0x6666);
            if (bitmap == 0) return;
            s.memory.write<1>(bitmap, width);
            s.memory.write<1>(bitmap + 1, height);
            for (std::uint32_t n = 0; n < 0x400; ++n) s.memory.write<1>(bitmap + 2 + n, (mix(n ^ 0x99) % 100) < density ? 1 + (mix(n) % 255) : 0);
        };
        return item;
    };
    cases.push_back(mask("first 5x3", 10, 10, 0x7fff, 5, 3, 60));
    cases.push_back(mask("null bitmap", 10, 10, 0x7fff, 0, 0, 0, {}, 0));
    cases.push_back(mask("suspended", 10, 10, 0x7fff, 5, 3, 60, {.suspended = 1}));
    cases.push_back(mask("null bitmap suspended", 10, 10, 0x7fff, 0, 0, 0, {.suspended = 1}, 0));
    for (const std::uint32_t width : {0u, 1u, 2u, 3u, 7u, 8u, 9u, 16u, 17u, 25u, 40u, 255u})
        for (const std::uint32_t height : {0u, 1u, 2u, 5u, 9u})
            for (const std::uint32_t density : {0u, 50u, 100u}) {
                if (width > 40 && (height > 1 || density != 50)) continue;
                cases.push_back(mask("size " + std::to_string(width) + "x" + std::to_string(height) + " density " + std::to_string(density), 8, 6, 0x3c1f, width, height, density));
            }
    for (const auto [x, y] : {std::array<int, 2>{0, 0}, {-3, -2}, {20, 30}, {0x10000 + 4, 5}, {5, 0x10000 + 4}, {0x8001, 2}, {7, 0xffff}, {-0x10000 + 3, 3}})
        cases.push_back(mask("at " + std::to_string(x) + "," + std::to_string(y), x, y, 0x5a5a, 6, 4, 70));
    for (const std::uint32_t colour : {0u, 0xffff8001u, 0x00010002u, 0x7fffu})
        cases.push_back(mask("colour " + hex(colour), 12, 12, colour, 9, 5, 100));
    for (const std::int32_t stride : {1, 3, 100, 0, -64}) {
        const std::uint32_t base = stride < 0 ? surface_at + 0x4000 : surface_at;
        cases.push_back(mask("stride " + std::to_string(stride), 6, 6, 0x4210, 8, 5, 80, {.base = base, .stride = stride}));
    }
    if (std::getenv("CM_GRAPH_CARBON_EXHAUSTIVE"))
        for (std::uint32_t width = 0; width <= 20; ++width)
            for (std::uint32_t height = 0; height <= 10; ++height)
                cases.push_back(mask("sweep " + std::to_string(width) + "x" + std::to_string(height), 9, 7, 0x4a52, width, height, 50));
    for (unsigned n = 0; n < 60; ++n)
        cases.push_back(mask("random " + std::to_string(n), pick(-2, 40), pick(-2, 30), std::uint32_t(random()) & 0xffff, std::uint32_t(random() % 20), std::uint32_t(random() % 12),
                             std::uint32_t(random() % 101)));
}
}
