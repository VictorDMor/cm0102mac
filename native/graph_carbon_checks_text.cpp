// graph_carbon.cpp, block `text` (drawing and measuring): cases for 0x1076d5a0, 0x1076d470, 0x1076d1e0 and 0x1076ca50.
//
// The snapshot (a UI snapshot of the running game) selects the outline fonts and its table of bitmap fonts is whatever the game loaded; the cases
// cover both font systems with real and with built state:
//  - the outline fonts (selection variable not -1) run on the snapshot's real FONT.CPP records: every font number, the callback present (a limit)
//    and absent, empty and real strings, the guards (initialised flag, no surface, no text, font -1, numbers above 7);
//  - the bitmap fonts (selection -1) run on a synthetic table of seven fonts built into the scratch area by `prepare` (the TOC's table slot points at
//    it): glyph widths 0..6 (odd and even rows, zero and one wide), leads and trails, glyphs without pixels, and 4-bit coverage of every value, drawn
//    over a pre-filled surface (also in the scratch area, with margins) whose pixels repeat so that the last-written-pixel cache is hit and missed.
// The registers r3-r10 are compared in full by the checker, so every register the original does not write has a distinct value here. Labels start
// with "text:".
#include "graph_carbon_checks.hpp"
#include "graph_carbon_text_detail.hpp"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>

namespace cm::aot::graph_carbon_checks {
namespace {
using namespace cm::aot::graph_carbon;
constexpr std::uint32_t game_toc = 0x108bba90;
constexpr std::uint32_t gap_entry = 0x1076d5a0, height_entry = 0x1076d470, width_entry = 0x1076d1e0, draw_entry = 0x1076ca50;
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
    const auto flag = scenario.toc(text_toc::message_flag);
    if (std::getenv("CM_GC_TEXT_DUMP")) {
        std::fprintf(stderr, "selection var %x value %x flag %x value %x\n", selection, scenario.r32(selection), flag, scenario.u8(flag));
        const auto table = scenario.toc(text_toc::font_table);
        std::fprintf(stderr, "table %x slots %x\n", table, scenario.toc(text_toc::font_slots));
        for (unsigned f = 0; f < 7; ++f) {
            const auto base = table + f * 0x1404;
            std::fprintf(stderr, "font %u height %u A: width %u rowbytes %u lead %u trail %u pix %x space %u\n", f + 1, scenario.r32(base), scenario.r32(base + 4 + 'A' * 0x14), scenario.r32(base + 8 + 'A' * 0x14),
                         scenario.r32(base + 12 + 'A' * 0x14), scenario.r32(base + 16 + 'A' * 0x14), scenario.r32(base + 20 + 'A' * 0x14), scenario.r32(base + 4 + 0x20 * 0x14));
        }
        for (unsigned n = 0; n < 4; ++n) std::fprintf(stderr, "slot %u -> %x\n", n, scenario.r32(scenario.toc(text_toc::font_slots) + 4 * n));
        std::fprintf(stderr, "surface %x stride %d rowbytes %d initialised %u\n", scenario.r32(slot(text_small::surface)), int(std::int16_t(scenario.u16(slot(text_small::stride)))),
                     int(std::int16_t(scenario.u16(slot(text_small::row_bytes)))), scenario.u8(slot(text_small::initialised)));
        std::fprintf(stderr, "window %u x %u sp %x\n", scenario.r32(scenario.toc(text_toc::window_width)), scenario.r32(scenario.toc(text_toc::window_height)), scenario.sp());
    }
    const std::vector<std::string> strings = {"Continue", "Squad", "Fixtures & Results", "", "|", "a|b", "Hello, World! 0123456789", std::string("tab\there\x01\x1f ok"),
                                              std::string("high \x80\xa0\xe9\xff") , "WWWWWWWWWWWWWWWWWWWW iiiiiiiiiiiiii", "x"};
    const std::vector<std::uint32_t> colours = {0x7fff, 0x0000, 0x2d6b, 0x7c1f, 0x03e0, 0x001f, 0x4210, 0xffff, 0x8000};
    std::uint32_t seed = 0x55aa;
    const auto outline = [&](const char* kind) { return std::string("text: ") + kind; };
    (void)outline;

    // ---- Call sites first: the call trace pairs the first 32 cases of an entry, so the cases that reach each call site (the assertion's three calls, the
    // gap and height calls, the line, FONT.CPP's drawer and width of each of the eight arms) come first for each entry.
    {
        Case item{.entry = height_entry, .registers = registers({{3, minus_one}}), .memory = {w32(selection, minus_one)}, .label = "text: height font -1 assertion (site)"};
        item.prepare = [](State& s) { build_fonts(s, 0); };
        cases.push_back(item);
        Case gap_item{.entry = gap_entry, .registers = registers({{3, 2}, {4, 0}, {5, 3}}), .label = "text: gap text 0 assertion (site)"};
        gap_item.prepare = [](State& s) { build_fonts(s, 0); };
        cases.push_back(gap_item);
        for (std::uint32_t font = 0; font <= 7; ++font)
            cases.push_back({.entry = width_entry, .registers = registers({{3, font}, {4, text_at}}), .memory = [&] { auto m = text(text_at, "Continue"); m.push_back(w32(selection, 0xfffffffeu)); return m; }(),
                             .label = "text: width outline font " + std::to_string(font) + " (site)"});
        for (const std::uint32_t font : {minus_one, 2u})
            for (const bool null_text : {false, true}) {
                if (font != minus_one && null_text == false) continue;
                auto memory = text(text_at, "Continue");
                memory.push_back(w32(selection, minus_one));
                Case item{.entry = width_entry, .registers = registers({{3, font}, {4, null_text ? 0u : text_at}}), .memory = memory, .label = "text: width bitmap assertion (site)"};
                item.prepare = [](State& s) { build_fonts(s, 0); };
                cases.push_back(item);
            }
        {
            auto memory = text(text_at, "A b");
            memory.push_back(w32(selection, minus_one));
            Case item{.entry = width_entry, .registers = registers({{3, 2}, {4, text_at}}), .memory = memory, .label = "text: width bitmap gap (site)"};
            item.prepare = [](State& s) { build_fonts(s, 0); };
            cases.push_back(item);
        }
    }

    // ---- 0x1076d470: the height of a font number. Outline: every number, the table's last entry beyond, the halfword wrap; bitmap: every font, 0
    // (the table before the first), 8 and -1 (the assertion).
    for (const std::uint32_t number : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 0xffffu, minus_one, 0x10003u, 0x1fff7u, 0xffff8002u, 0x80000000u})
        for (const std::uint32_t mode : {0xfffffffeu, 0u, 0x7fffffffu, minus_one}) {
            const auto label = "text: height font " + hex(number) + " selection " + hex(mode);
            Case item{.entry = height_entry, .registers = registers({{3, number}}), .memory = {w32(selection, mode)}, .label = label};
            if (mode == minus_one) item.prepare = [](State& s) { build_fonts(s, 0); };
            cases.push_back(item);
        }
    cases.push_back({.entry = height_entry, .registers = registers({{3, 2}}), .label = "text: height snapshot state"});

    // ---- 0x1076d5a0: the gap after text[index - 1] (index from 1). Every font, every index of strings with bars, controls, high bytes and the end,
    // text 0 and index 0 (reads text[-1]), the font -1; the space's width and the trails edited to the extremes.
    const auto gap = [&](std::uint32_t font, std::uint32_t at, std::uint32_t index, std::string label, std::vector<Write> memory = {}) {
        append(memory, text(text_at, strings[at % strings.size()]));
        Case item{.entry = gap_entry, .registers = registers({{3, font}, {4, text_at}, {5, index}}), .memory = std::move(memory), .label = "text: gap " + label};
        item.prepare = [](State& s) { build_fonts(s, 0); };
        cases.push_back(item);
    };
    for (std::uint32_t font = 1; font <= 7; ++font)
        for (std::uint32_t at = 0; at < strings.size(); ++at)
            for (std::uint32_t index = 0; index <= strings[at].size() + 1; ++index)
                if (font == 1 || font == 7 || index < 3) gap(font, at, index, "font " + std::to_string(font) + " string " + std::to_string(at) + " index " + std::to_string(index));
    for (const std::uint32_t font : {0xffffu, minus_one, 0x1ffffu}) gap(font, 0, 3, "font -1 " + hex(font));
    for (const std::uint32_t font : {0u, 8u, 0x10002u}) gap(font, 0, 3, "font out of the table " + hex(font));
    {
        Case item{.entry = gap_entry, .registers = registers({{3, 2}, {4, 0}, {5, 3}}), .label = "text: gap text 0 (assertion)"};
        item.prepare = [](State& s) { build_fonts(s, 0); };
        cases.push_back(item);
    }
    {
        // The space's width and the two spaces edited: negative products, carry, and a result below zero (kept at 0).
        const std::uint32_t space = table_at + bitmap_font::first_glyph + 0x20 * bitmap_font::glyph_size;
        const std::uint32_t chars[2] = {'a', 'b'};
        const std::uint32_t widths[] = {0, 1, 2, 3, 4, 5, 7, 100, 0x7fffffffu, 0x80000000u, minus_one, 0xfffffffdu, 0xfffffffeu};
        const std::uint32_t edges[] = {0, 1, 5, 0x7fffffffu, 0x80000000u, minus_one};
        for (const auto w : widths)
            for (const auto trail : edges)
                for (const auto lead : {0u, 3u, 0x80000000u}) {
                    std::vector<Write> memory = {w32(space, w), w32(table_at + bitmap_font::first_glyph + chars[0] * bitmap_font::glyph_size + bitmap_font::glyph_field::trail, trail),
                                                 w32(table_at + bitmap_font::first_glyph + chars[1] * bitmap_font::glyph_size + bitmap_font::glyph_field::lead, lead)};
                    append(memory, text(text_at, "ab"));
                    Case item{.entry = gap_entry, .registers = registers({{3, 1}, {4, text_at}, {5, 1}}), .memory = std::move(memory),
                              .label = "text: gap edited space " + hex(w) + " trail " + hex(trail) + " lead " + hex(lead)};
                    // The edits go after the font is built: the memory writes are applied before `prepare`, so they are written by it.
                    item.prepare = [w, trail, lead, chars](State& s) {
                        build_fonts(s, 0);
                        s.memory.write<4>(table_at + bitmap_font::first_glyph + 0x20 * bitmap_font::glyph_size, w);
                        s.memory.write<4>(table_at + bitmap_font::first_glyph + chars[0] * bitmap_font::glyph_size + bitmap_font::glyph_field::trail, trail);
                        s.memory.write<4>(table_at + bitmap_font::first_glyph + chars[1] * bitmap_font::glyph_size + bitmap_font::glyph_field::lead, lead);
                    };
                    cases.push_back(item);
                }
    }

    // ---- 0x1076d1e0: the width of a text. Outline fonts: every number and string (the snapshot's real records), the guard values, text 0;
    // bitmap fonts: every font and string, text 0 (assertion), font -1 (assertion), the font number 0 and 8.
    for (const std::uint32_t mode : {0xfffffffeu, minus_one}) {
        for (const std::uint32_t number : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 0xffffu, minus_one, 0x10002u, 0x10007u, 0xffff8000u}) {
            for (std::uint32_t at = 0; at < strings.size(); ++at) {
                if ((number > 8 && number != 0xffffu) && at > 2) continue;
                std::vector<Write> memory = {w32(selection, mode)};
                append(memory, text(text_at, strings[at]));
                Case item{.entry = width_entry, .registers = registers({{3, number}, {4, text_at}}), .memory = std::move(memory),
                          .label = "text: width " + std::string(mode == minus_one ? "bitmap" : "outline") + " font " + hex(number) + " string " + std::to_string(at)};
                if (mode == minus_one) item.prepare = [](State& s) { build_fonts(s, 0); };
                cases.push_back(item);
            }
            Case item{.entry = width_entry, .registers = registers({{3, number}, {4, 0}}), .memory = {w32(selection, mode)},
                      .label = "text: width " + std::string(mode == minus_one ? "bitmap" : "outline") + " font " + hex(number) + " text 0"};
            if (mode == minus_one) item.prepare = [](State& s) { build_fonts(s, 0); };
            cases.push_back(item);
        }
    }
    {
        // A long string (the halfword result wraps in the outline fonts' extsh; the bitmap sum is a word).
        std::string long_text;
        for (unsigned n = 0; n < 6000; ++n) long_text += char('A' + n % 26);
        std::vector<Write> memory = {w32(selection, minus_one)};
        append(memory, text(text_at, long_text));
        Case item{.entry = width_entry, .registers = registers({{3, 7}, {4, text_at}}), .memory = std::move(memory), .label = "text: width bitmap long string"};
        item.prepare = [](State& s) { build_fonts(s, 0); };
        cases.push_back(item);
        std::vector<Write> outline_memory = {w32(selection, 0xfffffffeu)};
        append(outline_memory, text(text_at, long_text));
        cases.push_back({.entry = width_entry, .registers = registers({{3, 3}, {4, text_at}}), .memory = std::move(outline_memory), .label = "text: width outline long string"});
    }

    // ---- 0x1076ca50: draws a text. Arguments: x, y, font, colour, text, limit.
    // The guards first (outline): initialised, no surface, font -1, no text, numbers above 7.
    const auto draw = [&](std::uint32_t mode, std::uint32_t x, std::uint32_t y, std::uint32_t font, std::uint32_t colour, std::uint32_t at, std::uint32_t limit,
                          const std::string& label, std::function<void(State&)> after = nullptr, bool null_text = false) {
        std::vector<Write> memory;
        memory.push_back(w32(selection, mode));
        if (!null_text) append(memory, text(text_at, strings[at % strings.size()]));
        Case item{.entry = draw_entry, .registers = registers({{3, x}, {4, y}, {5, font}, {6, colour}, {7, null_text ? 0u : text_at}, {8, limit}}), .memory = std::move(memory),
                  .label = "text: draw " + std::string(mode == minus_one ? "bitmap " : "outline ") + label};
        item.prepare = [mode, after](State& s) {
            build_surface(s, 0);
            if (mode == minus_one) build_fonts(s, 0);
            if (after) after(s);
        };
        cases.push_back(item);
    };
    for (std::uint32_t font = 0; font <= 7; ++font) draw(0xfffffffeu, 12, 20 + font, font, 0x4210, 0, 1, "outline arm " + std::to_string(font) + " (site)");
    draw(minus_one, 10, 8, minus_one, 0x7fff, 0, 3, "assertion font -1 (site)");
    draw(minus_one, 10, 8, 3, 0x7fff, 0, 3, "assertion text 0 (site)", nullptr, true);
    draw(minus_one, 10, 8, 3, 0x7fff, 0, 3, "bitmap caret (site)");
    draw(minus_one, 10, 8, 3, 0x7fff, 0, 0, "bitmap caret at the start (site)");
    for (const std::uint32_t font : {1u, 3u, 7u}) {
        draw(0xfffffffeu, 10, 8, font, 0x7fff, 0, minus_one, "guard initialised font " + std::to_string(font), [](State& s) { s.memory.write<1>(slot(text_small::initialised), 1); });
        draw(0xfffffffeu, 10, 8, font, 0x7fff, 0, minus_one, "guard no surface font " + std::to_string(font), [](State& s) { s.memory.write<4>(slot(text_small::surface), 0); });
        draw(0xfffffffeu, 10, 8, font, 0x7fff, 0, 3, "guard no text font " + std::to_string(font), nullptr, true);
    }
    draw(0xfffffffeu, 10, 8, minus_one, 0x7fff, 0, 2, "guard font -1");
    draw(0xfffffffeu, 10, 8, 0xffff, 0x7fff, 0, 2, "guard font 0xffff");
    draw(0xfffffffeu, 10, 8, 0x1ffff, 0x7fff, 0, 2, "guard font 0x1ffff");
    draw(0xfffffffeu, 10, 8, 8, 0x7fff, 0, 2, "guard font 8");
    draw(0xfffffffeu, 10, 8, 0x10003, 0x7fff, 0, 2, "font 0x10003");
    // Every outline font number with and without a limit, real strings, colours.
    for (std::uint32_t font = 0; font <= 7; ++font)
        for (const std::uint32_t limit : {minus_one, 0u, 1u, 4u, 100u, 0x7fffffffu, 0xfffffffeu})
            for (std::uint32_t at : {0u, 3u, 5u, 6u})
                draw(0xfffffffeu, 12 + at, 20 + font, font, colours[(font + at) % colours.size()], at, limit,
                     "font " + std::to_string(font) + " string " + std::to_string(at) + " limit " + hex(limit));
    for (std::uint32_t n = 0; n < 40; ++n) {
        const auto font = 1 + lcg(seed) % 7, x = lcg(seed) % 60, y = 4 + lcg(seed) % 30, at = std::uint32_t(lcg(seed) % strings.size());
        draw(n % 2 ? 0x7fffffffu : 0u, x, y, font, colours[lcg(seed) % colours.size()], at, n % 3 ? minus_one : lcg(seed) % 12, "random " + std::to_string(n));
    }

    // The bitmap fonts (selection -1): guards, the assertions, every font number, strings, colours, limits.
    draw(minus_one, 10, 8, 3, 0x7fff, 0, minus_one, "guard initialised", [](State& s) { s.memory.write<1>(slot(text_small::initialised), 1); });
    draw(minus_one, 10, 8, minus_one, 0x7fff, 0, 3, "font -1 (assertion)");
    draw(minus_one, 10, 8, 0xffff, 0x7fff, 0, 3, "font 0xffff (assertion)");
    draw(minus_one, 10, 8, 3, 0x7fff, 0, 3, "text 0 (assertion)", nullptr, true);
    draw(minus_one, 10, 8, 3, 0x7fff, 3, minus_one, "empty text");
    draw(minus_one, 10, 8, 3, 0x7fff, 3, 0, "empty text limit 0");
    draw(minus_one, 10, 8, 3, 0x7fff, 3, 1, "empty text limit 1");
    for (std::uint32_t font = 1; font <= 7; ++font)
        for (std::uint32_t at = 0; at < strings.size(); ++at) {
            if (at == 3) continue;
            for (const std::uint32_t limit : {minus_one, 0xfffffffdu, 0u, 1u, 3u, 40u})
                if (limit == minus_one || font == 1 || font == 7 || at == 0)
                    draw(minus_one, 6 + at, 5 + font * 2, font, colours[(font * 3 + at) % colours.size()], at, limit,
                         "font " + std::to_string(font) + " string " + std::to_string(at) + " limit " + hex(limit));
        }
    for (std::uint32_t n = 0; n < 160; ++n) {
        const auto font = 1 + lcg(seed) % 7, at = std::uint32_t(lcg(seed) % strings.size());
        const std::uint32_t x = n % 9 == 0 ? 0xfffffffcu + lcg(seed) % 4 : 4 + lcg(seed) % 70, y = n % 11 == 0 ? 0xfffffffeu : 3 + lcg(seed) % 60;
        draw(minus_one, x, y, font, colours[lcg(seed) % colours.size()], at, n % 4 ? minus_one : lcg(seed) % 20, "random " + std::to_string(n));
    }
    // Odd stride and surface variations: the row stride is a halfword read as signed.
    draw(minus_one, 10, 8, 2, 0x7fff, 0, minus_one, "stride 161", [](State& s) { s.memory.write<2>(slot(text_small::stride), 161); });
    draw(minus_one, 10, 8, 2, 0x7fff, 0, 5, "stride 100", [](State& s) { s.memory.write<2>(slot(text_small::stride), 100); });

    // ---- The snapshot's own bitmap fonts (selection -1 on the real table) and real surface: widths, gaps, heights and drawing of real strings.
    for (std::uint32_t font = 1; font <= 7; ++font) {
        cases.push_back({.entry = height_entry, .registers = registers({{3, font}}), .memory = {w32(selection, minus_one)}, .label = "text: height real font " + std::to_string(font)});
        for (std::uint32_t at = 0; at < strings.size(); ++at) {
            std::vector<Write> memory = {w32(selection, minus_one)};
            append(memory, text(text_at, strings[at]));
            cases.push_back({.entry = width_entry, .registers = registers({{3, font}, {4, text_at}}), .memory = memory, .label = "text: width real font " + std::to_string(font) + " string " + std::to_string(at)});
            for (std::uint32_t index : {0u, 1u, 2u, 5u, 40u})
                cases.push_back({.entry = gap_entry, .registers = registers({{3, font}, {4, text_at}, {5, index}}), .memory = memory,
                                 .label = "text: gap real font " + std::to_string(font) + " string " + std::to_string(at) + " index " + std::to_string(index)});
            if (at == 3) continue;
            for (const std::uint32_t limit : {minus_one, 0u, 2u, 6u, 60u})
                cases.push_back({.entry = draw_entry, .registers = registers({{3, 20 + 3 * at}, {4, 60 + 7 * font}, {5, font}, {6, colours[(font + at) % colours.size()]}, {7, text_at}, {8, limit}}),
                                 .memory = memory, .label = "text: draw bitmap real font " + std::to_string(font) + " string " + std::to_string(at) + " limit " + hex(limit)});
        }
    }
    // The outline fonts' width on the real records with the real selection.
    for (std::uint32_t font = 0; font <= 7; ++font)
        for (std::uint32_t at = 0; at < strings.size(); ++at) {
            std::vector<Write> memory;
            append(memory, text(text_at, strings[at]));
            cases.push_back({.entry = width_entry, .registers = registers({{3, font}, {4, text_at}}), .memory = memory, .label = "text: width snapshot outline font " + std::to_string(font) + " string " + std::to_string(at)});
        }
}
}
