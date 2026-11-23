// graph_carbon.cpp, block `text` (drawing and measuring): the bitmap fonts' kerning gap 0x1076d5a0, the glyph height 0x1076d470, the string
// width 0x1076d1e0 and the text drawer 0x1076ca50 (the game's text entry points: every screen draws and measures through them).
//
// The game has two font systems. FONT.CPP's outline fonts (eight font numbers over four loaded records) are used while the selection
// variable (TOC -0x740c) is not -1; they are drawn by FONT.CPP's drawer 0x1074f800 and measured by its string width 0x107500f0. With the
// selection at -1 (the "Traditional" look) the module's own bitmap fonts are used: seven tables (0x1404 bytes: the glyph height, then
// 0x14 bytes a character from 0x20: width, row bytes, the space before and after, the pixels as 4-bit coverage nibbles). The drawer blends
// the colour into the surface by the nibble (15 = the colour itself, else (colour * n + pixel * (15 - n)) / 15 per channel) and keeps the
// last written pixel and the channels it read in registers, so that a pixel equal to the last written one reuses the channels of an
// earlier pixel (a quirk of the original, kept).
//
// STACK CONTRACT. 0x1076ca50, 0x1076d1e0 and 0x1076d470 are leavers (the registers their prologues save are read by the game's printf):
// each builds the original's frame (the saved registers, the link register, the back chain and the stores into its caller's parameter
// area and its own locals), holds the callee-saved registers as the original does at each call that can reach a leaver, and calls the
// leavers at its own stack pointer. 0x1076d5a0 builds its frame too (it is below leavers, whose dead stack is compared).
// REGISTERS. r3-r10 are as the original leaves them on every path (`Scratch`, updated where the original writes a register).
#include "graph_carbon_text_detail.hpp"
#include "native_drawing.hpp"

namespace cm::aot::graph_carbon {
namespace {
// The assertion report every routine here ends an unusable argument in: the file name, the format and the title in the pool.
guest::Assertion assertion_at(const Cpu& c, std::uint32_t at, std::uint32_t name, std::uint32_t extension, std::uint32_t line) {
    const auto pool = c.toc(toc::pool);
    return {at, pool, pool + text_pool::format, pool + text_pool::title, name, extension, line};
}

// The jump tables' arms. The outline fonts: each font number draws and measures through one of the four loaded records at the slot's
// offset (the array of eight slots holds four records) with a size, a vertical offset and the drawer's style flags.
struct OutlineFont { std::uint32_t slot_offset, size, y_offset, flags, draw_back, width_back, height; };
constexpr OutlineFont outline_fonts[8] = {
    {0x0, 9, 3, 0x200, 0x1076cb80, 0x1076d248, 0xf},   {0x4, 9, 3, 0x200, 0x1076cbb4, 0x1076d264, 0xf},
    {0x4, 0xa, 4, 0x200, 0x1076cbe8, 0x1076d280, 0x12}, {0x4, 0xe, 4, 0x200, 0x1076cc1c, 0x1076d29c, 0x15},
    {0x4, 0x10, 4, 0x200, 0x1076cc50, 0x1076d2b8, 0x18}, {0x4, 0x12, 4, 0x200, 0x1076cc84, 0x1076d2d4, 0x1b},
    {0xc, 0x14, 9, 0x200, 0x1076ccb8, 0x1076d2f0, 0x27}, {0xc, 0x16, 0xb, 0x280, 0x1076ccec, 0x1076d30c, 0x2d}};
}

// 0x1076d5a0: the gap the drawer and the width leave between a character and the next (index is 1-based: the pair is text[index - 1] and
// text[index]): three quarters of the space's width less the first one's trailing space and the second one's leading space, never
// negative; 0 for no next character or the font number -1. A bar (0x7c) counts as a space.
std::int32_t glyph_gap(Cpu& c, std::uint32_t font, std::uint32_t text, std::uint32_t index) {
    Frame frame(c, 0x1076d5a0, 0x70, {.first_gpr = 30});
    const auto pool = c.toc(toc::pool);
    c.set({{31, pool}, {30, c.toc(text_toc::message_buffer)}, {6, 0}});
    if (extsh(font) == 0xffffffffu) { c.set(3, 0); return 0; }
    const auto table = c.toc(text_toc::font_table);
    const auto base = table + (extsh(font) - 1) * bitmap_font::size;
    c.set({{3, table}, {7, base}});
    if (text == 0) {
        // Line 0xc95 of graph_carbon.cpp.
        guest::report_assertion(c, assertion_at(c, 0x1076d604, frame.at(0x40), frame.at(0x38), 0xc95),
                                [&](std::uint32_t entry, std::uint32_t back) { c.call(entry, back); });
        c.set({{4, c.toc(text_toc::message_flag)}, {3, 0}});
        return 0;
    }
    if (c.u8(text + index) == 0) { c.set(3, 0); return 0; }
    auto first = c.u8(text + index - 1), next = c.u8(text + index);
    if (first == 0x7c) first = 0x20;
    if (next == 0x7c) next = 0x20;
    const auto three_quarters = srawi(c.u32(base + bitmap_font::first_glyph + bitmap_font::space * bitmap_font::glyph_size) * 3, 2);
    const auto trail = c.u32(bitmap_font::glyph(base, first) + bitmap_font::glyph_field::trail);
    const auto lead = c.u32(bitmap_font::glyph(base, next) + bitmap_font::glyph_field::lead);
    auto gap = three_quarters.value + three_quarters.carry - trail - lead;
    if (std::int32_t(gap) < 0) gap = 0;
    c.set({{3, gap}, {4, trail}, {5, three_quarters.value}, {6, gap}, {7, base}});
    return std::int32_t(gap);
}

// 0x1076d470: the height of a font number's glyphs in pixels: for the outline fonts the height of the number's arm, for the bitmap fonts the
// first word of its table; 0 for a number above 7 (outline) and for -1 (bitmap: the assertion of line 0xc73).
std::uint32_t font_height(Cpu& c, std::uint32_t font) {
    Frame frame(c, 0x1076d470, 0x70, {.first_gpr = 30});
    const auto pool = c.toc(toc::pool);
    const auto selection = c.toc(text_toc::selection);
    c.set({{4, selection}, {30, c.toc(text_toc::message_buffer)}, {31, pool}});
    if (c.u32(selection) != 0xffffffffu) {
        const auto number = extsh(font);
        const auto height = number > 7 ? 0 : outline_fonts[number].height;
        c.set(3, height);
        return height;
    }
    if (extsh(font) == 0xffffffffu) {
        guest::report_assertion(c, assertion_at(c, 0x1076d524, frame.at(0x40), frame.at(0x38), 0xc73),
                                [&](std::uint32_t entry, std::uint32_t back) { c.call(entry, back); });
        c.set({{4, c.toc(text_toc::message_flag)}, {3, 0}});
        return 0;
    }
    const auto table = c.toc(text_toc::font_table);
    const auto height = c.u32(table + extsh(font) * bitmap_font::size - bitmap_font::size + bitmap_font::height);
    c.set(3, height);
    return height;
}

// 0x1076d1e0: the width of a string in pixels (a halfword) for a font number. The outline fonts ask FONT.CPP's string width of the number's
// record; the bitmap fonts add each character's width and gap (characters below 0x20 count nothing).
std::int32_t text_width(Cpu& c, std::uint32_t font, std::uint32_t text) {
    Frame frame(c, 0x1076d1e0, 0xa0, {.first_gpr = 26});
    const auto pool = c.toc(toc::pool);
    const auto flag = c.toc(text_toc::message_flag), buffer = c.toc(text_toc::message_buffer), selection = c.toc(text_toc::selection);
    auto base = c.toc(text_toc::font_slots);
    c.set({{30, font}, {31, text}, {27, flag}, {28, buffer}, {29, pool}, {3, base}, {5, selection}});
    const auto finish = [&](std::uint32_t result) { c.set(3, result); return std::int32_t(result); };
    if (c.u32(selection) != 0xffffffffu) {
        const auto number = extsh(font);
        if (number > 7) return finish(0);
        const auto& arm = outline_fonts[number];
        c.set({{6, text}, {3, base + arm.slot_offset}, {4, 0}, {5, arm.size}});
        c.call(font_width_entry, arm.width_back);
        return finish(extsh(c.r(3)));
    }
    if (extsh(font) == 0xffffffffu) {
        guest::report_assertion(c, assertion_at(c, 0x1076d33c, frame.at(0x68), frame.at(0x40), 0xc1c),
                                [&](std::uint32_t entry, std::uint32_t back) { c.call(entry, back); });
        return finish(0);
    }
    const auto table = c.toc(text_toc::font_table);
    const auto font_base = table + (extsh(font) - 1) * bitmap_font::size;
    c.set({{3, table}, {26, font_base}});
    if (text == 0) {
        guest::report_assertion(c, assertion_at(c, 0x1076d3b0, frame.at(0x48), frame.at(0x38), 0xc24),
                                [&](std::uint32_t entry, std::uint32_t back) { c.call(entry, back); });
        return finish(0);
    }
    std::uint32_t total = 0, index = 1;
    for (;; ++index) {
        auto character = c.u8(text + index - 1);
        c.set(3, character);
        if (character == 0) break;
        if (character == 0x7c) { character = 0x20; c.set(3, 0x20); }
        if (character < 0x20) continue;
        const auto glyph = font_base + character * bitmap_font::glyph_size;
        c.set({{3, font}, {4, text}, {5, index}, {6, glyph}});
        total += c.u32(glyph + bitmap_font::first_glyph + bitmap_font::glyph_field::width);
        c.set({{27, total}, {28, index}});
        direct_call(c, 0x1076d5a0, 0x1076d440);
        total += std::uint32_t(glyph_gap(c, font, text, index));
        c.set(27, total);
    }
    return finish(total);
}

namespace {
constexpr std::int32_t s32(std::uint32_t value) { return std::int32_t(value); }
// The callee-saved registers r13-r31 of 0x1076ca50 by what they hold; the original keeps the drawer's state in them and a callee's
// prologue saves them (the leavers' saved registers are compared, the others' land in the compared dead stack).
struct Held {
    std::uint32_t x, text, pool, buffer, cached, column, row, height, y, colour, width, index, pen, blue_pixel, green_pixel, red_pixel, blue, green, red;
    void apply(Cpu& c) const {
        c.set({{13, x}, {14, text}, {15, pool}, {16, buffer}, {17, cached}, {18, column}, {19, row}, {20, height}, {21, y}, {22, colour},
               {23, width}, {24, index}, {25, pen}, {26, blue_pixel}, {27, green_pixel}, {28, red_pixel}, {29, blue}, {30, green}, {31, red}});
    }
};
// The weighted mean of a channel of the colour (`ink`) and of the pixel under it by the coverage nibble: (ink * n + pixel * (15 - n)) / 15,
// the compiler's multiply by 0x88888889 and shift of a positive sum.
struct Mean { std::uint32_t high, quotient, sign, value; };
Mean mean(std::uint32_t sum) {
    const auto high = mulhw(0x88888889u, sum);
    const auto quotient = srawi(high + sum, 3).value;
    const auto sign = sign_bit(quotient);
    return {high, quotient, sign, quotient + sign};
}
}

// 0x1076ca50: draws a string at (x, y) in a 15-bit colour with a font number into the surface; `limit` (-1: none) is a character position
// at which the pen's x is captured by the probe callback (outline fonts) or kept by the drawer (bitmap fonts), and then a vertical line is
// drawn at it (0x1076a140, style 2, the same colour) over the font's height: the caret of the text fields.
void draw_text(Cpu& c, std::uint32_t x, std::uint32_t y, std::uint32_t font, std::uint32_t colour, std::uint32_t text, std::uint32_t limit) {
    Frame frame(c, 0x1076ca50, 0x100, {.first_gpr = 13});
    const auto pool = c.toc(toc::pool);
    Held h{x, text, pool, c.toc(text_toc::message_buffer), c.r(17), c.toc(text_toc::message_flag), c.r(19), c.r(20), y, colour, c.r(23), c.r(24),
           c.r(25), c.r(26), c.r(27), c.r(28), c.r(29), c.r(30), c.r(31)};
    Scratch g = Scratch::take(c);
    const auto selection = c.toc(text_toc::selection);
    g.r3 = c.toc(text_toc::font_slots);
    g.r9 = selection;
    c.put16(frame.at(0x122), font);
    c.put32(frame.at(0x12c), limit);
    const auto finish = [&] {
        g.put(c);
        native_drawing_boundary(c.s, 0x1076d194);  // the observer's exit point of this routine
    };
    const auto font_number = [&] { return extsh(c.u16(frame.at(0x122))); };
    const auto limit_word = [&] { return c.u32(frame.at(0x12c)); };
    const auto captured = small(c, small_data::text_probe_captured);
    // The vertical line from `at` over the font's height: the caret.
    const auto caret = [&](std::uint32_t at, std::uint32_t height_back, std::uint32_t line_back) {
        g.r3 = font_number();
        h.apply(c); g.put(c);
        direct_call(c, 0x1076d470, height_back);
        const auto height = font_height(c, g.r3);
        g = Scratch::take(c);
        const auto top = at;
        g.r4 = y; g.r3 = top; g.r5 = top; g.r8 = colour; g.r6 = y + height - 1; g.r7 = 2;
        h.apply(c); g.put(c);
        c.call(line_entry, line_back);
        g = Scratch::take(c);
    };

    if (c.u32(selection) != 0xffffffffu) {
        // ---- the outline fonts: FONT.CPP's drawer into a window record built on the stack (back buffer, stride, depth byte, size)
        if (std::int8_t(c.u8(small(c, text_small::initialised))) != 0) return finish();
        if (c.u32(small(c, text_small::surface)) == 0) return finish();
        if (font_number() == 0xffffffffu) return finish();
        if (text == 0) return finish();
        g.r4 = 0;
        g.r5 = extsh(c.u16(small(c, text_small::row_bytes)));
        const auto height_word = c.toc(text_toc::window_height), width_word = c.toc(text_toc::window_width);
        g.r7 = c.u32(height_word);
        g.r5 = c.u32(width_word);
        g.r6 = c.u32(small(c, text_small::surface));
        const auto row_bytes = extsh(c.u16(small(c, text_small::row_bytes)));
        const auto stride = extsh(srawi(sign_bit(row_bytes) + row_bytes, 1).value);
        c.put16(frame.at(0x5c), g.r7);
        c.put32(frame.at(0x50), g.r6);
        c.put16(frame.at(0x5a), g.r5);
        c.put32(frame.at(0x54), stride);
        c.put8(frame.at(0x58), g.r4);
        if (s32(limit_word()) <= -1) {
            g.r6 = 0;
        } else {
            g.r5 = 0;
            g.r4 = 0xffffffffu;
            g.r6 = c.toc(text_toc::callback);
            c.put32(small(c, small_data::text_probe_limit), limit_word());
            c.put32(small(c, small_data::text_probe_count), g.r5);
            c.put32(captured, g.r4);
        }
        if (font_number() <= 7) {
            const auto& arm = outline_fonts[font_number()];
            c.put32(frame.at(0x38), g.r6);  // the drawer's ninth argument: no callback or the probe's descriptor
            g.r9 = colour;
            g.r10 = text;
            g.r3 += arm.slot_offset;
            g.r4 = frame.at(0x50);
            g.r5 = extsh(x);
            g.r6 = extsh(extsh(y) + arm.y_offset);
            g.r7 = arm.flags;
            g.r8 = arm.size;
            h.apply(c); g.put(c);
            c.call(font_draw_entry, arm.draw_back);
            g = Scratch::take(c);
        }
        if (s32(limit_word()) <= -1) return finish();
        if (s32(c.u32(captured)) <= -1) return finish();
        caret(c.u32(captured), 0x1076cd0c, 0x1076cd30);
        return finish();
    }

    // ---- the bitmap fonts
    g.r3 = c.u8(small(c, text_small::initialised));
    c.put32(frame.at(0xa0), 0xffffffffu);
    h.cached = 0xffffffffu;
    if (std::int8_t(g.r3) != 0) return finish();
    if (font_number() == 0xffffffffu) {
        g.put(c);
        guest::report_assertion(c, assertion_at(c, 0x1076cd6c, frame.at(0x80), frame.at(0x48), 0xb65),
                                [&](std::uint32_t entry, std::uint32_t back) { h.apply(c); c.call(entry, back); });
        g = Scratch::take(c);
        return finish();
    }
    const auto table = c.toc(text_toc::font_table);
    g.r3 = table;
    const auto font_base = table + (font_number() - 1) * bitmap_font::size;
    c.put32(frame.at(0xa4), font_base);
    if (text == 0) {
        g.put(c);
        guest::report_assertion(c, assertion_at(c, 0x1076cde0, frame.at(0x60), frame.at(0x40), 0xb6d),
                                [&](std::uint32_t entry, std::uint32_t back) { h.apply(c); c.call(entry, back); });
        g = Scratch::take(c);
        return finish();
    }
    h.pen = x;
    h.index = 1;
    h.green = (colour & 0x3e0) >> 2 & 0xff;
    h.blue = colour << 3 & 0xf8;
    h.red = colour >> 7 & 0xf8;
    for (;; ++h.index) {
        g.r3 = h.text + h.index;
        g.r6 = c.u8(g.r3 - 1);
        if (g.r6 == 0) break;
        if (g.r6 == 0x7c) g.r6 = 0x20;
        if (g.r6 < 0x20) continue;
        const auto character = g.r6;
        h.height = c.u32(font_base + bitmap_font::height);
        g.r4 = character * bitmap_font::glyph_size;
        auto pixels = c.u32(bitmap_font::glyph(font_base, character) + bitmap_font::glyph_field::pixels);  // read ahead of the tests, as the original does
        g.r5 = g.r4;
        g.r3 = font_base + g.r4;
        h.width = c.u32(g.r3 + 4);
        if (pixels != 0 && character > 0x20) {
            const auto right = h.pen + h.width - 1;
            g.r4 = s32(h.pen) >= s32(right) ? right : h.pen;
            const auto bottom = h.y + h.height - 1;
            g.r3 = s32(h.y) >= s32(bottom) ? bottom : h.y;
            g.r4 = extsh(g.r4);
            g.r3 = extsh(g.r3);
            const auto stride = extsh(c.u16(small(c, text_small::stride)));
            g.r5 = c.u32(small(c, text_small::surface));
            const auto row_start = g.r3 * stride;
            h.row = h.y;
            g.r3 = g.r4 << 1;
            h.buffer = g.r5 + (g.r3 + (row_start << 1));
            while (s32(h.row) <= s32(h.y + h.height - 1)) {
                h.column = h.pen;
                while (s32(h.column) <= s32(h.pen + h.width - 1)) {
                    const auto offset = h.column - h.pen;
                    std::uint32_t nibble;
                    if (((offset & 1) ^ sign_bit(offset)) - sign_bit(offset) != 0) {
                        // The low nibble of the byte, which the column then leaves behind.
                        nibble = c.u8(pixels) & 0xf;
                        ++pixels;
                        g.r3 = 0;
                    } else {
                        nibble = c.u8(pixels) >> 4;
                        g.r3 = sign_bit(offset);
                    }
                    if (nibble == 0xf) {
                        c.put16(h.buffer, colour);
                    } else if (nibble != 0) {
                        if (c.u16(h.buffer) != h.cached) {
                            // The channels of the pixel under, which a later equal pixel reuses without reading.
                            const auto under = c.u16(h.buffer);
                            h.blue_pixel = under << 3 & 0xf8;
                            h.green_pixel = (under & 0x3e0) >> 2 & 0xff;
                            h.red_pixel = under >> 7 & 0xf8;
                        }
                        const auto red = mean((h.red & 0xff) * nibble + (h.red_pixel & 0xff) * (0xf - nibble));
                        h.pool = red.high;  // r15 holds the multiply-high of the red mean
                        const auto green = mean((h.green & 0xff) * nibble + (h.green_pixel & 0xff) * (0xf - nibble));
                        const auto blue_sum = (h.blue & 0xff) * nibble + (h.blue_pixel & 0xff) * (0xf - nibble);
                        const auto blue = mean(blue_sum);
                        const auto blue_shifted = (blue.quotient + blue.sign & 0xff) >> 3;
                        const auto word = (blue_shifted & ~0x3e0u) | ((green.value << 2) & 0x3e0);
                        const auto joined = (word & ~0x7c00u) | (((red.value & 0xff) << 7) & 0x7c00);
                        c.put16(h.buffer, joined);
                        h.cached = c.u16(h.buffer);
                        g.r3 = blue.sign;
                        g.r4 = red.value & 0xff;
                        g.r5 = green.value;
                        g.r6 = 0xf - nibble;
                        g.r7 = (h.blue & 0xff) * nibble;
                        g.r8 = 0x88888889u;
                        g.r9 = red.quotient;
                        g.r10 = h.blue_pixel & 0xff;
                    }
                    h.buffer += 2;
                    ++h.column;
                }
                const auto columns = h.column - h.pen;
                g.r3 = sign_bit(columns);
                if (((columns & 1) ^ sign_bit(columns)) - sign_bit(columns) != 0) ++pixels;
                ++h.row;
                h.buffer += (extsh(c.u16(small(c, text_small::stride))) - h.width) << 1;
            }
        }
        g.r3 = limit_word();
        if (s32(h.index) == s32(g.r3 + 1)) {
            g.r3 = h.pen - 1;
            if (s32(h.x) > s32(h.pen - 1)) g.r3 = h.x;
            c.put32(frame.at(0xa0), g.r3);
        }
        g.r3 = font_number();
        g.r4 = h.text;
        g.r5 = h.index;
        h.apply(c); g.put(c);
        direct_call(c, 0x1076d5a0, 0x1076d11c);
        const auto gap = glyph_gap(c, g.r3, g.r4, g.r5);
        g = Scratch::take(c);
        h.pen += std::uint32_t(gap) + h.width;
    }
    g.r3 = limit_word();
    if (s32(h.index) == s32(g.r3 + 1)) c.put32(frame.at(0xa0), h.pen);
    if (s32(c.u32(frame.at(0xa0))) <= -1) return finish();
    caret(c.u32(frame.at(0xa0)), 0x1076d160, 0x1076d184);
    finish();
}
}
