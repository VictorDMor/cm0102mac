// graph_carbon.cpp, block `text` (the text box 0x1076e160): cases. The callees run for real on both sides (the text width and height, the
// drawer, the list object, the C library): the outline fonts on the snapshot's FONT.CPP records and the bitmap fonts (selection -1) on a synthetic
// table of seven built by `prepare`, over a surface of 160 x 80 pixels in scratch memory (the same state as the drawer's cases), so the pixels the lines
// draw are compared too. A group answers the width (every character fits / every one is cut) and the drawer, to reach the cutting arithmetic without
// the fonts. Registers: r3-r10 are the arguments; the ninth (the drawer's limit) is the word at sp + 0x38. Labels start with "text: box".
#include "graph_carbon_checks.hpp"
#include "graph_carbon_text_detail.hpp"
#include <string>

namespace cm::aot::graph_carbon_checks {
namespace {
using namespace cm::aot::graph_carbon;
constexpr std::uint32_t game_toc = 0x108bba90;
constexpr std::uint32_t box_entry = 0x1076e160;
constexpr std::uint32_t text_at = scratch_base + 0x20000, table_at = scratch_base + 0x30000, pixels_at = scratch_base + 0x40000, surface_at = scratch_base + 0x72000;
constexpr std::uint32_t surface_stride = 160, surface_rows = 80;
constexpr std::uint32_t slot(std::int32_t offset) { return game_toc + std::uint32_t(offset); }
constexpr std::uint32_t minus_one = 0xffffffffu;

std::vector<std::pair<unsigned, std::uint32_t>> registers(std::initializer_list<std::pair<unsigned, std::uint32_t>> used) {
    std::vector<std::pair<unsigned, std::uint32_t>> all;
    for (unsigned n = 3; n <= 10; ++n) all.emplace_back(n, 0x7a000000u + n * 0x01010101u);
    for (const auto& [n, value] : used) all[n - 3].second = value;
    return all;
}
// A C string at `at` (a pattern behind it, so a read past the terminator shows).
std::vector<Write> text(std::uint32_t at, const std::string& value) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < value.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), std::uint8_t(value[n])));
    memory.push_back(w8(at + std::uint32_t(value.size()), 0));
    for (unsigned n = 1; n <= 4; ++n) memory.push_back(w8(at + std::uint32_t(value.size()) + n, 0x41 + n));
    memory.push_back(w8(at - 1, 0x61));
    return memory;
}
std::uint32_t lcg(std::uint32_t& state) { state = state * 1664525u + 1013904223u; return state >> 8; }

// The synthetic bitmap fonts: the TOC's table slot points at seven fonts of 0x1404 bytes in the scratch area, and the pixels at the surface.
void build_fonts(State& s, unsigned variant) {
    s.memory.write<4>(slot(text_toc::font_table), table_at);
    std::uint32_t seed = 0x1234 + variant;
    for (std::uint32_t font = 0; font < 7; ++font) {
        const auto base = table_at + font * bitmap_font::size;
        s.memory.write<4>(base + bitmap_font::height, 4 + font);
        for (std::uint32_t character = 0x20; character <= 0xff; ++character) {
            const auto record = bitmap_font::glyph(base, character);
            std::uint32_t width = (character * 7 + font) % 7;  // 0..6
            if (character == 0x20) width = 4 + font;
            const auto row_bytes = (width + 1) / 2;
            const auto pixels = pixels_at + (font * 224 + character - 0x20) * 0x20;
            const bool has_pixels = character != 0x20 && character % 11 != 3 && (row_bytes != 0 || character % 13 == 5);
            s.memory.write<4>(record + bitmap_font::glyph_field::width, width);
            s.memory.write<4>(record + bitmap_font::glyph_field::row_bytes, row_bytes);
            s.memory.write<4>(record + bitmap_font::glyph_field::lead, (character * 3 + font) % 5);
            s.memory.write<4>(record + bitmap_font::glyph_field::trail, (character * 5 + font) % 4);
            s.memory.write<4>(record + bitmap_font::glyph_field::pixels, has_pixels ? pixels : 0);
            if (has_pixels)
                for (std::uint32_t n = 0; n < 0x20; ++n) {
                    const auto high = lcg(seed) % 3 == 0 ? 0xf : lcg(seed) % 16, low = lcg(seed) % 4 == 0 ? 0 : lcg(seed) % 16;
                    s.memory.write<1>(pixels + n, high << 4 | low);
                }
        }
    }
}
// The surface: pixels in 15-bit colours, runs of equal ones.
void build_surface(State& s, unsigned variant) {
    std::uint32_t seed = 0x777 + variant;
    s.memory.write<4>(slot(text_small::surface), surface_at);
    s.memory.write<2>(slot(text_small::stride), surface_stride);
    s.memory.write<2>(slot(text_small::row_bytes), surface_stride * 2);
    std::uint32_t pixel = 0x2d6b;
    for (std::uint32_t n = 0; n < surface_stride * surface_rows; ++n) {
        if (lcg(seed) % 3 != 0) pixel = lcg(seed) & 0x7fff;
        s.memory.write<2>(surface_at + 2 * n, pixel);
    }
}
}


GRAPH_CARBON_CASES("text") {
    const auto selection = scenario.toc(text_toc::selection);
    const auto limit_at = scenario.sp() + 0x38;
    const std::vector<std::string> strings = {
        "Hello world", "A", "", "Line one\nLine two\nThree", "A very long line of text that does not fit the box at all, it wraps over and over again",
        std::string("tab\there\x01\x1f ok"), "\n\n", "x\n", "  spaces  ", std::string("high \x80\xa0\xe9\xff end"), "WWWWWWWWWWWWWWWWWWWW iiiiiiiiiiiiii",
        "Squad - Fixtures & Results - Transfers", "one\ntwo\nthree\nfour\nfive\nsix\nseven"};
    struct Rectangle { std::uint32_t x1, y1, x2, y2; };
    const std::vector<Rectangle> rectangles = {{4, 4, 150, 70}, {6, 6, 40, 70}, {10, 8, 14, 12}, {5, 20, 120, 20}, {30, 10, 20, 60}};
    const std::vector<std::uint32_t> styles = {0, 1, 2, 3, 0x40, 0x41, 0x42, 0x20, 0x22, 0x23, 0x80, 0x81, 0x100, 0x102, 0x1a3, 0x1e0, 0xffff, 0x140, 0x60, 0xa0};
    const auto make = [&](std::uint32_t mode, Rectangle box, std::uint32_t style, std::uint32_t font, std::uint32_t colour, std::uint32_t limit, std::uint32_t at,
                          const std::string& label, std::function<void(State&)> after = nullptr, bool null_text = false) {
        std::vector<Write> memory = {w32(selection, mode), w32(limit_at, limit)};
        if (!null_text) append(memory, text(text_at, strings[at % strings.size()]));
        Case item{.entry = box_entry, .registers = registers({{3, box.x1}, {4, box.y1}, {5, box.x2}, {6, box.y2}, {7, style}, {8, font}, {9, colour}, {10, null_text ? 0u : text_at}}),
                  .memory = std::move(memory), .label = "text: box " + std::string(mode == minus_one ? "bitmap " : "outline ") + label};
        item.prepare = [mode, after](State& s) {
            build_surface(s, 0);
            if (mode == minus_one) build_fonts(s, 0);
            if (after) after(s);
        };
        return item;
    };
    const Rectangle wide = rectangles[0], narrow = rectangles[1];
    // ---- Rare paths first: the guards (initialising, font -1, no text), the font number's halfword, the empty text with and without a limit.
    cases.push_back(make(0xfffffffeu, wide, 0, 2, 0x7fff, minus_one, 0, "guard initialising", [](State& s) { s.memory.write<1>(slot(text_small::initialised), 1); }));
    cases.push_back(make(0xfffffffeu, wide, 0, 0xffff, 0x7fff, minus_one, 0, "guard font 0xffff"));
    cases.push_back(make(0xfffffffeu, wide, 0, minus_one, 0x7fff, 3, 0, "guard font -1"));
    cases.push_back(make(0xfffffffeu, wide, 0, 2, 0x7fff, 3, 0, "guard no text", nullptr, true));
    cases.push_back(make(0xfffffffeu, wide, 0, 0x10001, 0x7fff, minus_one, 0, "font 0x10001"));
    cases.push_back(make(0xfffffffeu, wide, 0, 8, 0x7fff, minus_one, 0, "font 8"));
    cases.push_back(make(0xfffffffeu, wide, 0, 0, 0x7fff, minus_one, 0, "font 0"));
    for (const std::uint32_t mode : {0xfffffffeu, minus_one})
        for (const std::uint32_t limit : {minus_one, 0u, 1u, 0x80000000u, 0x7fffffffu, 0xfffffffeu})
            for (const std::uint32_t style : {0u, 2u, 1u, 0x20u})
                cases.push_back(make(mode, wide, style, 3, 0x7fff, limit, 2, "empty text limit " + hex(limit) + " style " + hex(style)));

    // ---- Every style over every text, rectangle and font (outline and bitmap), limits and colours rotating.
    for (const std::uint32_t mode : {0xfffffffeu, minus_one}) {
        std::uint32_t pick = 0;
        for (std::size_t text_number = 0; text_number < strings.size(); ++text_number)
            for (std::size_t box = 0; box < rectangles.size(); ++box)
                for (const auto style : styles) {
                    // The inverted rectangle (box 4) with a shadow style reads and draws outside the 160 x 80 surface: a fault on both sides, not a case.
                    if (box == 4 && (style & 0x20) != 0) continue;
                    // The tiny box (2) with the bitmap fonts and vertical centring puts a tall text above the surface's first row: the same.
                    if (mode == minus_one && box == 2 && (style & 2) == 0) continue;
                    if (box == 2 && style == 0xffff && text_number == 4) continue;  // the same, with every style bit and the long text
                    ++pick;
                    const std::uint32_t font = 1 + pick % 7, limit = pick % 5 == 0 ? 2u : minus_one, colour = 0x2000u + (pick * 0x137) % 0x5fff;
                    cases.push_back(make(mode, rectangles[box], style, font, colour, limit, text_number,
                                         "text " + std::to_string(text_number) + " box " + std::to_string(box) + " style " + hex(style) + " font " + std::to_string(font)));
                }
        // Every font number on a wrapping text with and without the shadow.
        for (std::uint32_t font = 1; font <= 7; ++font)
            for (const std::uint32_t style : {0u, 0x20u, 0x100u, 0x82u})
                cases.push_back(make(mode, narrow, style, font, 0x7fe0, minus_one, 4, "wrap font " + std::to_string(font) + " style " + hex(style)));
    }

    // ---- The cutting arithmetic with the width answered: every prefix fits (0), a width equal to the box, one above it, and every one cut.
    for (const std::uint32_t answer : {0u, 4u, 5u, 6u, 0x7fffu, 0xffffffffu, 0x80000000u}) {
        for (const std::uint32_t style : {0u, 0x100u, 0x1u}) {
            auto item = make(0xfffffffeu, rectangles[1], style, 2, 0x7fff, minus_one, 4, "answered width " + hex(answer) + " style " + hex(style));
            item.answers = {{0x1076d1e0, answer}};
            cases.push_back(item);
        }
    }
    // The drawer and the height answered: the arithmetic of the lines' places without any pixel.
    for (const std::uint32_t height : {0u, 1u, 12u, 0x7fffffffu, 0xffffffffu})
        for (const std::uint32_t style : {0u, 2u, 0x20u, 0x42u}) {
            auto item = make(0xfffffffeu, rectangles[0], style, 3, 0x7fff, 1, 3, "answered height " + hex(height) + " style " + hex(style));
            item.answers = {{0x1076ca50, 0x5a}, {0x1076d470, height}};
            cases.push_back(item);
        }
    // Mask style: asterisks over every character.
    for (const std::uint32_t mode : {0xfffffffeu, minus_one})
        cases.push_back(make(mode, wide, 0x81, 4, 0x7fff, minus_one, 9, "mask high bytes"));
}
}
