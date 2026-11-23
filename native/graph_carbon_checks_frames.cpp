// graph_carbon.cpp, block `frames` (0x1076d6e0, the framed rectangle): cases. The routine calls the pixel primitives (0x1076a140 line,
// 0x1076a580 rectangle, 0x1076ad50 ellipse, 0x10769980 dim) on the snapshot's real surface, which both sides run alike (the generated
// bodies, or the other blocks' C++ in production), so every case compares the whole surface, r3-r10, the stack contract (the leaver log
// and the dead stack) and, with --call-trace, the arguments and the held registers at every call.
// Cases cover each stage on its own and in the combinations that can differ: every style bit alone, every pair, the stage product (the
// geometry modifiers 0x20000 / 0x40000 / 0x1000, the fill 0x2 / 0x10 / 0x10|0x10000 / 0x4 / 0x8, the frame of the bevel in each variant,
// the outline / ellipse / shadow / middle-line variants, 0x1000000 and 0x800), the words the game's callers pass (0x30, 0x210, 0x70, 0x60,
// 0x2 and their 0x40 forms), random words with the unused bits set, the colour's every form (zero low half with high bits, saturated
// components, the surface pixel of 0x1000, the module's default and outline colour variables poked), the geometry's every edge
// (the bevel's 50 x 50 and bottom < 100 thresholds, zero size, inverted, outside the surface), the uninitialised-graphics byte, and
// the gradients' division by zero. Labels start with "frames:".
#include "graph_carbon_checks.hpp"
#include "graph_carbon_detail.hpp"
#include <array>
#include <climits>
#include <cstdlib>
#include <random>

namespace cm::aot::graph_carbon_checks {
namespace {
constexpr std::uint32_t game_toc = 0x108bba90, frame_entry = 0x1076d6e0;
constexpr std::int32_t initialised_slot = -0x3550, surface_base_slot = -0x3564, row_stride_slot = -0x3568, default_colour_slot = -0x7bdc,
                       outline_colour_slot = -0x7bd0;
constexpr std::uint32_t bits[] = {0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 0x10000, 0x20000, 0x40000, 0x1000000};
bool exhaustive() { return std::getenv("CM_GRAPH_CARBON_EXHAUSTIVE") != nullptr; }
struct Geometry { std::int32_t left, top, right, bottom; const char* name; };
std::string hex_signed(std::int32_t value) { return value < 0 ? "-" + hex(std::uint32_t(-value)) : hex(std::uint32_t(value)); }
}

GRAPH_CARBON_CASES("frames") {
    std::mt19937 random(0x6c42);
    const auto surface = scenario.r32(game_toc + std::uint32_t(surface_base_slot));
    const auto stride = scenario.u16(game_toc + std::uint32_t(row_stride_slot));
    const auto flag = game_toc + std::uint32_t(initialised_slot);
    const auto default_colour = scenario.r32(game_toc + std::uint32_t(default_colour_slot));
    const auto outline_colour = scenario.r32(game_toc + std::uint32_t(outline_colour_slot));
    if (!surface || !stride || !default_colour || !outline_colour) return;
    const auto pixel_at = [&](std::uint32_t x, std::uint32_t y) { return surface + (y * stride + x) * 2; };

    // A case: the graphics are started (the flag byte 0: the real value while the game draws), the registers distinct where the routine
    // does not read them (r10).
    const auto frame = [&](const std::string& what, const Geometry& g, std::uint32_t style, std::uint32_t colour, std::uint32_t accent = 0x4321,
                           std::vector<Write> memory = {}) {
        memory.push_back(w8(flag, 0));
        return Case{.entry = frame_entry,
                    .registers = {{3, std::uint32_t(g.left)}, {4, std::uint32_t(g.top)}, {5, std::uint32_t(g.right)}, {6, std::uint32_t(g.bottom)}, {7, style},
                                  {8, colour}, {9, accent}, {10, 0x7a00000au}},
                    .memory = std::move(memory),
                    .label = "frames: " + what + " " + g.name + " style " + hex(style) + " colour " + hex(colour)};
    };

    const std::array<Geometry, 18> geometries{{{10, 10, 60, 40, "50x30"}, {100, 100, 110, 108, "small"}, {20, 20, 90, 90, "panel"}, {20, 120, 90, 190, "low panel"},
                                               {10, 10, 59, 59, "width 49"}, {10, 10, 60, 60, "50x50"}, {10, 10, 60, 59, "height 49"}, {10, 50, 60, 100, "bottom 100"},
                                               {10, 49, 60, 99, "bottom 99"}, {50, 50, 50, 50, "point"}, {60, 40, 10, 10, "inverted"}, {10, 10, 10, 40, "zero width"},
                                               {10, 10, 60, 10, "zero height"}, {-20, -10, 30, 20, "negative"}, {780, 580, 830, 640, "beyond"}, {300, 200, 301, 201, "2x2"},
                                               {0, 0, 799, 599, "screen"}, {400, 300, 403, 316, "narrow"}}};
    const auto& medium = geometries[0];
    const auto& panel = geometries[2];
    const std::uint32_t colours[] = {0x4a52, 0x7fff, 0x7c00, 0x03e0, 0x001f, 0x0001, 0x2108, 0x1234, 0x6b5a, 0x8421, 0x0000, 0x10000, 0xdead0000u, 0x12345678u, 0xffffu, 0x8000u};

    // ---- The first cases carry the call trace (the first 32 of an entry): one per call site of the stack contract's callees.
    cases.push_back(frame("dim", medium, 0x2, 0x4a52));
    cases.push_back(frame("fill", medium, 0x10, 0x4a52));
    cases.push_back(frame("ellipse fill", medium, 0x10 | 0x10000, 0x4a52));
    cases.push_back(frame("gradient across", medium, 0x4, 0x4a52));
    cases.push_back(frame("gradient down", medium, 0x8, 0x4a52));
    cases.push_back(frame("ellipse outline", medium, 0x20 | 0x10000, 0x4a52, 0x2222));
    cases.push_back(frame("ellipse outline thin", medium, 0x20 | 0x10000 | 0x2000, 0x4a52, 0x2222));
    cases.push_back(frame("rectangle outline", medium, 0x20 | 0x200, 0x4a52));
    cases.push_back(frame("rectangle outline thin", medium, 0x20 | 0x200 | 0x2000, 0x4a52));
    cases.push_back(frame("bevel", medium, 0x20, 0x4a52));
    cases.push_back(frame("bevel swapped", medium, 0x20 | 0x40, 0x4a52));
    cases.push_back(frame("bevel thick", panel, 0x20 | 0x10, 0x4a52));
    cases.push_back(frame("shadow line", medium, 0x80, 0x4a52));
    cases.push_back(frame("line pair", medium, 0x400, 0x4a52));
    cases.push_back(frame("line pair thin", medium, 0x400 | 0x2000, 0x4a52));
    cases.push_back(frame("middle line", medium, 0x400 | 0x4000, 0x4a52));
    cases.push_back(frame("middle line thin", medium, 0x400 | 0x4000 | 0x2000, 0x4a52));
    cases.push_back(frame("top line", medium, 0x400 | 0x4000 | 0x8000, 0x4a52));
    cases.push_back(frame("top line thin", medium, 0x400 | 0x4000 | 0x8000 | 0x2000, 0x4a52));
    cases.push_back(frame("highlights", medium, 0x1000000, 0x4a52));
    cases.push_back(frame("black outline", medium, 0x800, 0x4a52));
    cases.push_back(frame("inset", medium, 0x20000 | 0x10, 0x4a52));
    cases.push_back(frame("outset", medium, 0x40000 | 0x10, 0x4a52));
    cases.push_back(frame("colour from the surface", medium, 0x1000 | 0x10, 0x4a52));
    cases.push_back(frame("nothing", medium, 0, 0x4a52));
    // The graphics not started: nothing is drawn, whatever the style (the byte is read as a signed byte).
    for (const std::uint32_t value : {1u, 0x7fu, 0x80u, 0xffu, 0x2u}) {
        auto item = frame("not started " + hex(value), medium, 0x20 | 0x10, 0x4a52);
        item.memory.push_back(w8(flag, value));
        cases.push_back(item);
    }
    // The gradients with no width or height: the division by zero (both sides stop alike) and with one step.
    cases.push_back(frame("gradient across, zero width", geometries[11], 0x4, 0x4a52));
    cases.push_back(frame("gradient down, zero height", geometries[12], 0x8, 0x4a52));
    cases.push_back(frame("gradient across, point", geometries[9], 0x4, 0x4a52));
    cases.push_back(frame("gradient down, point", geometries[9], 0x8, 0x4a52));
    cases.push_back(frame("gradient across, one column", {10, 10, 11, 20, "one column"}, 0x4, 0x4a52));
    cases.push_back(frame("gradient down, one row", {10, 10, 20, 11, "one row"}, 0x8, 0x4a52));

    // ---- Every style bit alone and every pair of bits, in the medium rectangle and (alone) in the panel.
    for (const auto bitA : bits) {
        for (const auto& g : {medium, panel}) cases.push_back(frame("bit", g, bitA, 0x4a52));
        cases.push_back(frame("bit, default colour", geometries[1], bitA, 0));
    }
    for (std::size_t i = 0; i < std::size(bits); ++i)
        for (std::size_t j = i + 1; j < std::size(bits); ++j) cases.push_back(frame("pair", geometries[1], bits[i] | bits[j], colours[(i * 7 + j) % std::size(colours)]));
    if (exhaustive())
        for (std::size_t i = 0; i < std::size(bits); ++i)
            for (std::size_t j = i + 1; j < std::size(bits); ++j)
                for (std::size_t k = j + 1; k < std::size(bits); ++k)
                    cases.push_back(frame("triple", geometries[1], bits[i] | bits[j] | bits[k], colours[(i + j + k) % std::size(colours)]));

    // ---- The words the game's callers pass (0x30, 0x210, 0x70, 0x60; the dialog and the buttons) and their variants.
    for (const std::uint32_t word : {0x30u, 0x210u, 0x70u, 0x60u, 0x2u, 0x30u | 0x20000, 0x70u | 0x40000, 0x30u | 0x1000, 0x30u | 0x1000000, 0x70u | 0x800, 0x230u, 0x270u})
        for (const auto& g : geometries) {
            if (g.right - g.left > 400) continue;  // the whole screen below
            cases.push_back(frame("caller", g, word, colours[word % std::size(colours)]));
        }

    // ---- The stage product: geometry modifiers x fill x frame x highlights x outline, over the medium and the panel rectangles.
    {
        const std::uint32_t geometry_modifiers[] = {0, 0x20000, 0x40000, 0x1000, 0x20000 | 0x40000, 0x1000 | 0x20000, 0x1000 | 0x40000, 0x1000 | 0x20000 | 0x40000};
        const std::uint32_t fills[] = {0, 0x2, 0x10, 0x10 | 0x10000, 0x4, 0x8, 0x4 | 0x8, 0x2 | 0x4 | 0x8 | 0x10};
        const std::uint32_t frames[] = {0, 0x20 | 0x10000, 0x20 | 0x10000 | 0x2000, 0x20 | 0x200, 0x20 | 0x200 | 0x2000, 0x20, 0x20 | 0x10, 0x20 | 0x40, 0x20 | 0x10 | 0x40, 0x80,
                                        0x400, 0x400 | 0x2000, 0x400 | 0x4000, 0x400 | 0x4000 | 0x2000, 0x400 | 0x4000 | 0x8000, 0x400 | 0x4000 | 0x8000 | 0x2000, 0x80 | 0x400,
                                        0x20 | 0x80 | 0x400, 0x20 | 0x10000 | 0x200};
        const std::uint32_t highlights[] = {0, 0x1000000};
        const std::uint32_t outlines[] = {0, 0x800};
        unsigned n = 0;
        for (const auto m : geometry_modifiers)
            for (const auto f : fills)
                for (const auto s : frames)
                    for (const auto h : highlights)
                        for (const auto o : outlines) {
                            ++n;
                            if (!exhaustive() && n % 5 != 0) continue;
                            const auto& g = (n % 3 == 0) ? panel : medium;
                            cases.push_back(frame("product", g, m | f | s | h | o, colours[n % std::size(colours)], 0x1357 + n));
                        }
    }

    // ---- Colours: every form of the colour word in each shading, so the components, the clamps at 255 and the low-halfword test are all reached.
    for (const auto colour : colours)
        for (const std::uint32_t word : {0x20u, 0x20u | 0x10, 0x20u | 0x40, 0x4u, 0x8u, 0x80u, 0x1000000u, 0x30u | 0x1000000})
            cases.push_back(frame("colour", geometries[5], word, colour));
    // The module's default colour variable (a halfword): the shadings that start from it when the colour's low half is 0.
    for (const std::uint32_t value : {0x0000u, 0x2108u, 0x7fffu, 0x7c00u, 0x03e0u, 0x001fu, 0x8000u, 0xffffu, 0x5a5au}) {
        for (const std::uint32_t word : {0x20u, 0x20u | 0x10, 0x20u | 0x40, 0x80u, 0x1000000u, 0x20u | 0x1000000})
            for (const std::uint32_t colour : {0u, 0x10000u, 0xabcd0000u}) {
                auto item = frame("default colour " + hex(value), medium, word, colour);
                item.memory.push_back(w16(default_colour, value));
                cases.push_back(item);
            }
    }
    // The outline colour variable of 0x800.
    for (const std::uint32_t value : {0x0000u, 0x7fffu, 0x1234u, 0xffffu, 0x8001u}) {
        auto item = frame("outline colour " + hex(value), medium, 0x800, 0x4a52);
        item.memory.push_back(w16(outline_colour, value));
        cases.push_back(item);
    }
    // 0x1000: the colour is the surface's pixel at the (possibly moved) top-left corner.
    for (const std::uint32_t value : {0x0000u, 0x1234u, 0x7fffu, 0xffffu, 0x8000u, 0x0421u})
        for (const std::uint32_t word : {0x1000u | 0x10, 0x1000u | 0x4, 0x1000u | 0x8, 0x1000u | 0x20, 0x1000u | 0x80, 0x1000u | 0x1000000, 0x1000u | 0x40000 | 0x10, 0x1000u | 0x20000 | 0x2}) {
            const auto left = std::int32_t(30), top = std::int32_t(20);
            const auto moved = (word & 0x40000) ? 2 : 0;
            auto item = frame("surface colour " + hex(value), {left, top, left + 20, top + 16, "surface"}, word, 0x5555,  0x4321,
                              {w16(pixel_at(std::uint32_t(left + moved), std::uint32_t(top + moved)), value)});
            cases.push_back(item);
        }
    // The accent colour (r9) of the ellipse outline.
    for (const std::uint32_t accent : {0u, 0x7fffu, 0xffffu, 0x12340000u, 0x1u})
        cases.push_back(frame("accent", medium, 0x20 | 0x10000, 0x4a52, accent));

    // ---- Random words (every bit of the 32 with a probability of one third, so the unused bits are set too) over random rectangles inside and
    // around the surface, and random small words.
    for (unsigned n = 0; n < (exhaustive() ? 6000u : 1200u); ++n) {
        std::uint32_t word = 0;
        for (unsigned bit = 0; bit < 32; ++bit) if (random() % 3 == 0) word |= 1u << bit;
        if (n % 2) word &= 0x1ffffff;
        if (n % 4 == 1) word &= ~std::uint32_t(0xc | 0x1000);  // fewer gradients and surface colours (the divide stops, the pixel is arbitrary)
        const std::int32_t left = std::int32_t(random() % 740) + 10, top = std::int32_t(random() % 540) + 10;
        const std::int32_t width = std::int32_t(random() % (n % 3 ? 70 : 120)), height = std::int32_t(random() % (n % 3 ? 40 : 110));
        Geometry g{left, top, left + width, top + height, "random"};
        if (n % 16 == 0) g = {left - 500, top - 400, left + width, top + height, "random outside"};
        if (n % 16 == 1) g = {left + width, top + height, left, top, "random inverted"};
        cases.push_back(frame("random " + std::to_string(n), g, word, std::uint32_t(random()) & (n % 5 ? 0xffff : 0xffffffffu), std::uint32_t(random())));
    }

    // ---- Edges of the geometry in each style whose behaviour depends on them: the bevel's thickness (width >= 50, height >= 50, bottom < 100 and 0x10),
    // the middle row of an odd and an even and a negative height, the inset/outset on tiny rectangles.
    for (const std::int32_t right_edge : {58, 59, 60, 61})
        for (const std::int32_t bottom_edge : {58, 59, 60, 98, 99, 100, 101})
            for (const std::uint32_t word : {0x20u | 0x10, 0x20u, 0x20u | 0x10 | 0x40})
                cases.push_back(frame("bevel edges " + hex_signed(right_edge) + "," + hex_signed(bottom_edge), {10, 10, right_edge, bottom_edge, "edge"}, word, 0x4a52));
    for (const std::int32_t bottom_edge : {10, 11, 12, 13, 9, 8, 5, 0, -3})
        for (const std::uint32_t word : {0x400u | 0x4000, 0x400u | 0x4000 | 0x2000, 0x400u | 0x4000 | 0x8000, 0x1000000u, 0x1000000u | 0x20000})
            cases.push_back(frame("middle row " + hex_signed(bottom_edge), {10, 10, 40, bottom_edge, "rows"}, word, 0x4a52));
    for (const std::uint32_t word : {0x20000u | 0x10, 0x40000u | 0x10, 0x20000u | 0x40000 | 0x20, 0x20000u | 0x4, 0x40000u | 0x8, 0x20000u | 0x40000 | 0x4, 0x20000u | 0x40000 | 0x8})
        for (const auto& g : {geometries[15], geometries[9], geometries[1], geometries[13]})
            cases.push_back(frame("tiny", g, word, 0x4a52));
    // Gradients and dims over negative coordinates (the loops and the divisions are signed; the primitives clip).
    for (const std::uint32_t word : {0x4u, 0x8u, 0x4u | 0x20, 0x8u | 0x400, 0x2u, 0x4u | 0x8})
        for (const auto& g : {geometries[13], Geometry{-5, -5, 5, 5, "around zero"}, Geometry{-30, 20, -10, 40, "left of the surface"}})
            cases.push_back(frame("negative", g, word, 0x4a52));
    // The whole screen (heavy: lines and fills of 800 x 600), once.
    for (const std::uint32_t word : {0x10u, 0x30u, 0x2u, 0x4u, 0x8u, 0x70u | 0x800})
        cases.push_back(frame("screen", geometries[16], word, 0x4a52));
}
}
