// FONT.CPP, block `glyphs` (0x107500f0..0x10750200 and 0x10750580..0x107509f4, two routines, 350 instructions): the glyph cache and the
// string width.
//
// 0x10750580 returns the glyph record (0x20 bytes: the bitmap's left, top, width, row bytes and height, the pixels, the advance, and the key
// style-low-five-bits / size / character) of a character for a style and size. A font record keeps up to 256 of them in an array, most
// recently used first: a hit moves its record to the front; a miss grows the array by one (realloc), or, with 256 cached, frees the oldest
// record's bitmap and reuses the record, shifts the array down and puts the new record in front, renders the character through the engine
// (render_character with command 2, quality 5) and copies the result: plain (the engine's bitmap) or, with style bit 0x10, smoothed by a
// 5x5 box sum / 25 into a bitmap 4 pixels larger each way. The engine's glyph is then purged (level 1).
// 0x107500f0 sums the advances of a string's characters, for the last one the bitmap's left * 0x400 + width * 0x10000 - 1, and returns the
// sum rounded to whole pixels (a halfword); 0 for a font record that is not loaded.
//
// The engine's render and purge are called through the dispatcher (as the selector's calls are: font_cpp_selector.cpp says why).
//
// Host observation (tools/native_drawing_audit.py): the glyph cache's entry 0x10750580 is observed by the page of every caller that reaches
// it through the dispatcher (the drawer does); the string width's typed call below passes it itself. The two points inside are passed here:
// 0x10750740 (the `bl` of the engine's render, with its arguments in place) and 0x107509f0 (the `blr`, after the epilogue, with the record
// in r3).
//
// Both are stack-contract leavers (stmw r23 / r26): the frames are the original's (0x70 and 0x50, the saved registers, the link register).
// Each call is made with the registers the original holds there. The original's r3-r12 and r23-r31 live in `g` by register number, written
// where the original writes the register (the smoothing loops reuse r23-r28 as temporaries, and a callee's prologue saves them), put in
// the machine's registers before each call and read back after it (a callee leaves r4-r10 as it leaves them, and the original's later
// reads of them, and the registers compared at return, are those).
#include "font_cpp_detail.hpp"
#include "native_drawing.hpp"
#include <array>

namespace cm::aot::font_cpp {
namespace {
using Regs = std::array<std::uint32_t, 32>;

constexpr std::uint32_t assertion_report = 0x107755c0, release_object = 0x10773610, allocate_block = 0x107735f0, new_object = 0x10773520,
                        resize_block = 0x107751b0, byte_move = 0x10739ef0, render_character_entry = 0x10760e30, purge_entry = 0x1075f980;
// The pool's assertion texts (+offset), all with the file name at +0x12.
constexpr std::uint32_t file_name = 0x12, null_font = 0xd, engine_error = 0x5a, width_null_text = 0x7c, cache_resize_failed = 0x83,
                        record_failed = 0x8f, bitmap_failed = 0x95;
constexpr std::int32_t error_word = 0x38;
// The instructions the host observes (see the header comment).
constexpr std::uint32_t cache_entry = 0x10750580, render_point = 0x10750740, cache_return = 0x107509f0;

constexpr std::int32_t s32(std::uint32_t v) { return std::int32_t(v); }
constexpr std::uint32_t half(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }  // extsh

Regs load_registers(Cpu& c) {
    Regs g{};
    for (unsigned n = 3; n <= 10; ++n) g[n] = c.r(n);
    for (unsigned n = 23; n < 32; ++n) g[n] = c.r(n);
    return g;
}
// What the original's registers are at a call: the callee's arguments and the registers its prologue saves.
void hold(Cpu& c, const Regs& g) {
    for (unsigned n = 3; n <= 10; ++n) c.set(n, g[n]);
    for (unsigned n = 23; n < 32; ++n) c.set(n, g[n]);
}
// What a callee leaves in r3-r10.
void settle(Cpu& c, Regs& g) {
    for (unsigned n = 3; n <= 10; ++n) g[n] = c.r(n);
}
// The original's `bl` of a generated or owner routine, the arguments in g.
void call(Cpu& c, Regs& g, std::uint32_t target, std::uint32_t back) {
    hold(c, g);
    c.call(target, back);
    settle(c, g);
}
// The assertion report: text, the unit's file name, the line.
void report(Cpu& c, Regs& g, std::uint32_t pool, std::uint32_t text, std::uint32_t line, std::uint32_t back) {
    g[3] = pool + text;
    g[4] = pool + file_name;
    g[5] = line;
    call(c, g, assertion_report, back);
}
// The loop that moves the cache's first `count` entries up by one place, from the back (0x1075061c / 0x107506f8): r3 the byte offset of the
// last source, r4 of the destination, r6 the count (tested as a halfword).
void shift_entries(Cpu& c, Regs& g) {
    while (s32(half(g[6])) > 0) {
        g[5] = c.u32(g[30] + font_record::cache);
        const auto source = g[3] - 4;
        g[3] -= 4;
        g[6] -= 1;
        c.put32(g[5] + g[4], c.u32(g[5] + source));
        g[4] -= 4;
    }
}
}

std::uint32_t glyph_record(Cpu& c, std::uint32_t slot, std::uint32_t style, std::uint32_t size, std::uint32_t character) {
    Frame frame(c, 0x10750580, 0x70, {.first_gpr = 23});
    Regs g = load_registers(c);
    g[3] = slot, g[4] = style, g[5] = size, g[6] = character;
    g[31] = c.toc(toc::pool);
    g[26] = g[4], g[27] = g[5], g[28] = g[6];
    g[30] = c.u32(g[3]);
    const auto pool = g[31];
    const auto local = [&](std::int32_t offset) { return frame.at(offset); };
    if (g[30] == 0) report(c, g, pool, null_font, 0x478, 0x107505bc);
    // The key the cache is searched by: the character and the size (bytes) and the style's low five bits.
    g[6] = g[28] & 0xff;
    g[5] = g[27] & 0xff;
    g[4] = g[26] & 0x1f;
    g[9] = 0;
    g[7] = 0;
    const auto finish = [&](std::uint32_t record) {
        for (unsigned n = 3; n <= 10; ++n) c.set(n, g[n]);
        frame.leave();
        native_drawing_boundary(c.s, cache_return);
        return record;
    };

    // The search, most recent first.
    for (;;) {
        g[3] = c.u32(g[30] + font_record::cache_count);
        g[8] = half(g[9]);
        if (!(s32(g[8]) < s32(g[3]))) break;
        g[3] = c.u32(g[30] + font_record::cache);
        g[29] = c.u32(g[3] + g[7]);
        if (g[6] == c.u8(g[29] + glyph_cache::character) && g[5] == c.u8(g[29] + glyph_cache::size) &&
            g[4] == (c.u16(g[29] + glyph_cache::flags) & 0x1f)) {
            // A hit: moved to the front, the entries before it one place on.
            g[6] = g[9];
            g[3] = g[8] << 2;
            g[4] = half(g[9]) << 2;
            shift_entries(c, g);
            g[4] = c.u32(g[30] + font_record::cache);
            g[3] = g[29];
            c.put32(g[4], g[29]);
            return finish(g[29]);
        }
        g[7] += 4;
        g[9] += 1;
    }

    // A miss. A full cache frees the oldest record's bitmap and reuses the record; else the array grows by a place and a record is made.
    if (s32(g[3]) == s32(font_record::cache_limit)) {
        g[3] = c.u32(g[29] + glyph_cache::bitmap);
        if (g[3] != 0) call(c, g, release_object, 0x10750684);
    } else {
        c.put32(g[30] + font_record::cache_count, g[3] + 1);
        g[3] = c.u32(g[30] + font_record::cache);
        g[4] = c.u32(g[30] + font_record::cache_count) << 2;
        call(c, g, resize_block, 0x107506a0);
        c.put32(g[30] + font_record::cache, g[3]);
        if (c.u32(g[30] + font_record::cache) == 0) report(c, g, pool, cache_resize_failed, 0x4a5, 0x107506c0);
        g[3] = glyph_cache::record_size;
        call(c, g, new_object, 0x107506c8);
        g[29] = g[3];
        if (g[29] == 0) report(c, g, pool, record_failed, 0x4ab, 0x107506e0);
    }
    // The array moves down by one place; the new record is first.
    g[3] = c.u32(g[30] + font_record::cache_count);
    g[6] = half(g[3] - 1);
    g[3] = g[6] << 2;
    g[4] = g[3];
    shift_entries(c, g);
    g[3] = c.u32(g[30] + font_record::cache);
    g[4] = g[28] & 0xff;
    g[9] = local(error_word);
    g[5] = 0;
    c.put32(g[3], g[29]);
    g[6] = 0, g[7] = 5, g[8] = 2;
    g[3] = c.u32(g[30] + font_record::scaler);
    hold(c, g);
    native_drawing_boundary(c.s, render_point);
    c.call(render_character_entry, 0x10750744);
    settle(c, g);
    if (c.u32(local(error_word)) != 0) report(c, g, pool, engine_error, 0x4b8, 0x10750760);

    const auto scaler_field = [&](std::uint32_t offset) {
        g[3] = c.u32(g[30] + font_record::scaler);
        return c.u32(g[3] + offset);
    };
    const bool smooth = (g[26] & 0x10) != 0;
    // The record's fields from the engine's result: placement, advance and the cache key.
    c.put32(g[29] + glyph_cache::left, scaler_field(scaler::left));
    c.put32(g[29] + glyph_cache::top, scaler_field(scaler::top));
    // The smoothed bitmap has a margin of two pixels on each side.
    const std::uint32_t margin = smooth ? 4 : 0;
    c.put32(g[29] + glyph_cache::width, scaler_field(scaler::bitmap_width) + margin);
    c.put32(g[29] + glyph_cache::row_bytes, scaler_field(scaler::bitmap_stride) + margin);
    c.put32(g[29] + glyph_cache::height, scaler_field(scaler::bitmap_height) + margin);
    c.put32(g[29] + glyph_cache::advance, scaler_field(scaler::advance));
    c.put16(g[29] + glyph_cache::flags, g[26]);
    c.put8(g[29] + glyph_cache::size, g[27]);
    c.put8(g[29] + glyph_cache::character, g[28]);

    if (smooth) {
        // The bitmap, then each pixel the sum of the 5x5 neighbourhood of the engine's bitmap divided by 25 (a multiplication by 0x51eb851f).
        g[3] = c.u32(g[29] + glyph_cache::height);
        g[0] = c.u32(g[29] + glyph_cache::row_bytes);
        g[3] *= g[0];
        call(c, g, allocate_block, 0x107507d8);
        c.put32(g[29] + glyph_cache::bitmap, g[3]);
        if (c.u32(g[29] + glyph_cache::bitmap) == 0) report(c, g, pool, bitmap_failed, 0x4cf, 0x107507f8);
        g[3] = 0x51ec0000;
        g[10] = c.u32(g[29] + glyph_cache::bitmap);
        g[11] = g[3] - 0x7ae1;
        g[5] = 0;
        for (;;) {
            g[0] = c.u32(g[29] + glyph_cache::height);
            g[4] = half(g[5]);
            if (!(s32(g[4]) < s32(g[0]))) break;
            g[6] = 0;
            for (;;) {
                g[3] = c.u32(g[29] + glyph_cache::width);
                g[0] = half(g[6]);
                if (!(s32(g[0]) < s32(g[3]))) break;
                g[7] = std::uint32_t(-2);
                g[9] = 0;
                for (;;) {
                    g[12] = half(g[7]);
                    g[28] = g[4] + g[12];
                    if (s32(g[28]) < 0) break;
                    g[8] = c.u32(g[29] + glyph_cache::height);
                    g[25] = g[4] + g[12];
                    if (s32(g[25]) >= s32(g[8])) break;
                    if (s32(g[12]) > 2) break;
                    g[8] = std::uint32_t(-2);
                    for (;;) {
                        g[23] = half(g[8]);
                        g[26] = g[0] + g[23];
                        if (s32(g[26]) < 0) break;
                        g[27] = g[0] + g[23];
                        if (s32(g[27]) >= s32(g[3])) break;
                        if (s32(g[23]) > 2) break;
                        // The engine's bitmap is offset by the margin: only the pixels inside it count.
                        if (!(s32(g[28]) < 2)) {
                            g[24] = c.u32(g[30] + font_record::scaler);
                            g[12] = c.u32(g[24] + scaler::bitmap_height) + 2;
                            if (!(s32(g[25]) >= s32(g[12])) && !(s32(g[26]) < 2)) {
                                g[12] = c.u32(g[24] + scaler::bitmap_width) + 2;
                                if (!(s32(g[27]) >= s32(g[12]))) {
                                    g[26] = c.u32(g[24] + scaler::bitmap_stride);
                                    g[12] = g[28] - 2;
                                    g[27] = c.u32(g[24] + scaler::bitmap);
                                    g[12] = g[26] * g[12];
                                    g[12] = g[0] + g[12];
                                    g[12] += g[23];
                                    g[12] -= 2;
                                    g[12] = c.u8(g[27] + g[12]);
                                    g[9] += g[12];
                                }
                            }
                        }
                        g[8] += 1;
                    }
                    g[7] += 1;
                }
                // The pixel: the sum (a halfword) times 0x51eb851f, high word, shifted, plus the sign: the quotient by 25.
                g[0] = half(g[9]);
                g[6] += 1;
                g[0] = std::uint32_t((std::int64_t(s32(g[11])) * std::int64_t(s32(g[0]))) >> 32);
                g[0] = std::uint32_t(s32(g[0]) >> 3);
                g[3] = (g[0] >> 31) & 1;
                g[0] += g[3];
                c.put8(g[10], g[0]);
                g[10] += 1;
            }
            g[5] += 1;
        }
    } else {
        // The engine's bitmap copied as it is.
        g[3] = c.u32(g[29] + glyph_cache::height);
        g[0] = c.u32(g[29] + glyph_cache::row_bytes);
        g[23] = g[3] * g[0];
        g[3] = g[23];
        call(c, g, allocate_block, 0x1075097c);
        c.put32(g[29] + glyph_cache::bitmap, g[3]);
        if (c.u32(g[29] + glyph_cache::bitmap) == 0) report(c, g, pool, bitmap_failed, 0x506, 0x1075099c);
        g[4] = c.u32(g[30] + font_record::scaler);
        g[5] = g[23];
        g[3] = c.u32(g[29] + glyph_cache::bitmap);
        g[4] = c.u32(g[4] + scaler::bitmap);
        call(c, g, byte_move, 0x107509b0);
    }

    // The engine's glyph is purged (level 1).
    g[3] = c.u32(g[30] + font_record::scaler);
    g[5] = local(error_word);
    g[4] = 1;
    call(c, g, purge_entry, 0x107509c0);
    if (c.u32(local(error_word)) != 0) report(c, g, pool, engine_error, 0x510, 0x107509dc);
    g[3] = g[29];
    return finish(g[29]);
}

std::int32_t string_width(Cpu& c, std::uint32_t slot, std::uint32_t style, std::uint32_t size, std::uint32_t text) {
    Frame frame(c, 0x107500f0, 0x50, {.first_gpr = 26});
    Regs g = load_registers(c);
    g[3] = slot, g[4] = style, g[5] = size, g[6] = text;
    g[27] = c.toc(toc::pool);
    g[28] = g[3], g[29] = g[4], g[30] = g[5], g[31] = g[6];
    g[26] = c.u32(g[3]);
    const auto pool = g[27];
    const auto finish = [&] {
        for (unsigned n = 3; n <= 10; ++n) c.set(n, g[n]);
        return std::int32_t(std::int16_t(g[3]));
    };
    if (g[26] == 0) report(c, g, pool, null_font, 0x371, 0x10750130);
    // `extsb.` of the record's first byte (the loaded flag): an empty record has no width.
    if (std::int8_t(c.u8(g[26])) == 0) {
        g[3] = 0;
        return finish();
    }
    if (g[31] == 0) report(c, g, pool, width_null_text, 0x377, 0x1075015c);
    g[3] = g[28], g[4] = g[29], g[5] = g[30];
    hold(c, g);
    c.link(0x1075016c);
    select_style(c, c.r(3), c.r(4), c.r(5));
    settle(c, g);
    g[26] = 0;
    g[27] = 0;
    for (;;) {
        g[0] = half(g[27]);
        g[6] = c.u8(g[31] + g[0]);
        if (g[6] == 0) break;
        g[3] = g[28], g[4] = g[29], g[5] = g[30];
        hold(c, g);
        c.link(0x1075018c);
        native_drawing_boundary(c.s, cache_entry);  // the original's `bl` runs the page's observation of the cache's entry
        g[3] = glyph_record(c, c.r(3), c.r(4), c.r(5), c.r(6));
        settle(c, g);
        g[4] = half(g[27]);
        g[0] = g[4] + 1;
        g[0] = c.u8(g[31] + g[0]);
        if (std::int8_t(g[0]) != 0) {
            // Not the last character: its advance.
            g[0] = c.u32(g[3] + glyph_cache::advance);
            g[26] += g[0];
        } else {
            // The last: its left bearing and width (the bitmap's extent) instead.
            g[0] = c.u32(g[3] + glyph_cache::width);
            g[4] = c.u32(g[3] + glyph_cache::left);
            g[3] = g[0] << 16;
            g[0] = g[3] - 1;
            g[3] = g[4] << 10;
            g[26] = g[0] + g[26];
            g[26] = g[3] + g[26];
        }
        g[27] += 1;
    }
    g[3] = g[26] + 0x10000;
    g[0] = g[3] - 0x8000;
    g[0] = std::uint32_t(s32(g[0]) >> 16);
    g[3] = half(g[0]);
    return finish();
}
}
