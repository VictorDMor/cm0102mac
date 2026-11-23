// FONT.CPP, block `drawer` (0x1074f800..0x107500ec, 570 instructions): the game's text drawing routine. It paints a string into a
// window's 16-bit back buffer (xRGB 1-5-5-5), one glyph at a time, and hands each glyph's rectangle to a callback when the caller gave one.
//
//   r3 font slot (a Font**), r4 window record, r5 x and r6 y (low halfwords: whole pixels), r7 style flags, r8 size byte, r9 colour
//   (1-5-5-5), r10 the text, and a ninth argument on the stack (caller sp + 0x38): 0 or the callback descriptor graph_carbon passes.
//
// Style flags (the low halfword; the selector 0x10750320 and the glyph cache 0x10750580 read the others): 0x80 draws the text twice, first
// in black with flags | 0x50 (a recursive call, no callback), then as asked; 0x100 reads each glyph's bitmap right to left; 0x200 draws
// '|' as a space; 0x20 embosses (the covered pixel is mixed with a lightened copy of itself and the pixel up and to the left with a
// darkened one); 0x40 mixes the pixel with the colour's average; neither blends the colour in by coverage. Coverage is the bitmap byte
// 0..255 against 126 (so a coverage above 126 mixes with a negative weight: the original's arithmetic, kept).
//
// STACK CONTRACT. A leaver (stmw r13): the registers its prologue saves, the link register and the back chain are read by the printf of
// the game (tools/stack_contract.py module FONT.CPP). The frame is the original's (0xa0 bytes, r13-r31 saved), the three words the
// prologue stores into the caller's frame (the font slot at +0x18, the size byte at +0x2f, the text at +0x34) and the locals at 0x38 (the
// recursive call's ninth argument), 0x40 (the baseline) and 0x44 (the font record) are written, and the callees (the style selector, the
// glyph cache, the assertion reporter and the callback's glue) are called at the original's stack pointer with r13-r31 as the original
// holds them (the leaver check compares the saved registers of every entry into 0x1074f800 and the glyph cache and selector).
//
// HOST OBSERVATION (tools/native_drawing_audit.py). The host's HD text layer watches four instructions of this routine and the page runs
// the first one only for a caller that reaches the entry through its page: 0x1074f800 (the entry: the recursive outline call passes it
// itself, with r3-r10 as the original's `bl` leaves them), 0x1074fb58 (a glyph inside the window, after the clipping: r0 the pen's whole x,
// r4 y, r5 r7 r8 r10 the clipped rectangle, r30 the glyph record, r31 the string index, the text at sp + 0xd4), 0x107500b4 (the end of
// every character) and 0x107500e8 (the `blr`, after the epilogue). They are passed here, in the original's order, with those registers
// in the guest state (the checker's boundary log compares everything the observer reads: native/native_drawing_log.hpp).
//
// REGISTERS. r3-r10 are as the original leaves them on every path: the glyph rectangle's clipping and the three pixel loops leave
// their counters and last pixel's temporaries in r3-r10 (derived from the listing, loop by loop). Not reproduced: r14-r20 inside the
// glyph loop (the pixel loops use them as temporaries; no callee reads them: the glyph cache saves r23-r31 only and the epilogue
// restores them) and the diagnostic registers r0, r11 and r12 (r12 holds the callback at its glue call).
#include "font_cpp_detail.hpp"
#include "native_drawing.hpp"
#include <algorithm>

namespace cm::aot::font_cpp {
namespace {
constexpr std::uint32_t entry = 0x1074f800, frame_size = 0xa0;
// The instructions the host observes (see above).
constexpr std::uint32_t glyph_point = 0x1074fb58, character_end = 0x107500b4, return_point = 0x107500e8;
// Routines of other modules or blocks, called as the original calls them (generated, or C++ called through the dispatcher).
constexpr std::uint32_t assertion_report = 0x107755c0, select_style_entry = 0x10750320, glyph_record_entry = 0x10750580, callback_glue = 0x10781320;
// The address after each `bl` of the original.
namespace back {
constexpr std::uint32_t font_assertion = 0x1074f858, window_assertion = 0x1074f888, buffer_assertion = 0x1074f8a4, text_assertion = 0x1074f8c0,
                        shadow = 0x1074f900, style = 0x1074f914, final_callback = 0x1074f9d4, glyph = 0x1074fa14, glyph_callback = 0x1074fa88;
}
// Offsets into the pool the assertions name (pool_text of the TOC's string pool, font_cpp_detail.hpp, is the font list's): the texts
// "font", "window", "window->back_buffer", "string" and the line numbers 0x289, 0x28f, 0x292, 0x298 of FONT.CPP.
namespace assertion {
constexpr std::uint32_t file = 0x12, font_text = 0xd, window_text = 0x61, buffer_text = 0x68, string_text = 0x7c;
constexpr std::uint32_t font_line = 0x289, window_line = 0x28f, buffer_line = 0x292, string_line = 0x298;
}
// The frame's slots: the caller's (the prologue stores the arguments into its parameter area) and the routine's own.
namespace slot {
constexpr std::uint32_t font = 0x18, size = 0x2f, text = 0x34, callback = 0x38;  // from the caller's stack pointer
constexpr std::uint32_t shadow_callback = 0x38, baseline = 0x40, record = 0x44, toc_save = 0x14;  // from the routine's
}
namespace window_record { constexpr std::uint32_t buffer = 0, stride = 4, width = 0xa, height = 0xc; }  // width, height: signed halfwords
namespace glyph_fields { constexpr std::uint32_t left = 0, top = 4, width = 8, row_bytes = 0xc, height = 0x10, bitmap = 0x14, advance = 0x18; }
namespace font_fields { constexpr std::uint32_t loaded = 0, scaler = 0x14; }
// The scaler's two line metrics (16.16): the callback's rectangle runs from the first to the second below the pen, and the baseline is
// the pen plus both.
namespace line_metrics { constexpr std::uint32_t lower = 0x30, upper = 0x38; }
namespace style_bit {
constexpr std::uint32_t emboss = 0x20, average = 0x40, outline = 0x80, mirror = 0x100, bar_as_space = 0x200;
}

std::int32_t sext16(std::uint32_t v) { return std::int32_t(std::int16_t(v & 0xffff)); }
// `srawi 16` then `extsh`: the whole pixel of a 16.16 position with the 0x8000 rounding added.
std::int32_t whole_pixel(std::uint32_t fixed) { return sext16(std::uint32_t(std::int32_t(fixed) >> 16)); }
std::int32_t mulhw(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * b) >> 32); }
std::int32_t sign_bit(std::int32_t v) { return std::int32_t(std::uint32_t(v) >> 31); }
// The compiler's divisions by 126 and by 5 (a multiply-high by a magic number, a shift, and the sign added back): truncating.
struct Quotient { std::int32_t floor; std::int32_t sign; std::int32_t value() const { return std::int32_t(std::uint32_t(floor) + std::uint32_t(sign)); } };
Quotient divide_126(std::int32_t x) {
    const auto q = std::int32_t(std::uint32_t(mulhw(std::int32_t(0x82082083u), x)) + std::uint32_t(x)) >> 6;
    return {q, sign_bit(q)};
}
Quotient divide_5(std::int32_t x) {
    const auto q = mulhw(0x66666667, x) >> 1;
    return {q, sign_bit(q)};
}
std::int32_t product(std::int32_t a, std::int32_t b) { return std::int32_t(std::uint32_t(a) * std::uint32_t(b)); }
std::int32_t sum(std::int32_t a, std::int32_t b) { return std::int32_t(std::uint32_t(a) + std::uint32_t(b)); }

// The colour's channels and a pixel's: the 5-bit fields scaled to 8 bits.
struct Channels { std::int32_t red, green, blue; };
Channels split_colour(std::uint32_t word) {
    return {std::int32_t((word >> 7) & 0xf8), std::int32_t((word >> 2) & 0xf8), std::int32_t((word << 3) & 0xf8)};
}
Channels split_pixel(std::uint32_t pixel) {
    return {std::int32_t((pixel & 0x7c00) >> 7), std::int32_t((pixel & 0x3e0) >> 2), std::int32_t((pixel << 3) & 0xf8)};
}
// Green and blue and red from the 8-bit mixes: each takes its fifth to eighth bits (the stores of the original are rlwimi's of the
// shifted values under the 1-5-5-5 masks).
std::uint32_t join_pixel(std::int32_t red, std::int32_t green, std::int32_t blue) {
    return (std::uint32_t(green) << 2 & 0x3e0) | (std::uint32_t(blue) >> 3 & 0x1f) | (std::uint32_t(red) << 7 & 0x7c00);
}

// r14-r20 as the original holds them: the pixel loops use them as temporaries and leave the last blended pixel's, so the next glyph call
// (whose callees' setjmp saves them into the engine's memory object) sees them. They start as the arguments, the pool and the caller's.
struct Held { std::uint32_t r14, r15, r16, r17, r18, r19, r20; };

// What the glyph loops walk over, as the original's registers hold it when they start.
struct Scan {
    std::uint32_t window, glyph;
    std::int32_t step;                  // +1, or -1 when the glyph is read right to left (r21)
    std::uint32_t src, dest;            // the bitmap byte and the 16-bit pixel the first column reads and writes (r6, r11)
    std::int32_t x_start, x_end, y_start, y_end;  // the clipped rectangle (r8, r7, r5, r10); y_start as r5 holds it
};
// Where the loops ended.
struct Walk {
    std::int32_t y = 0, x = 0;          // the row counter and the column counter at the end
    std::uint32_t src = 0, row_skip = 0, alpha = 0;
    bool rows = false, visited = false;
    std::int32_t count = 0;             // the columns of a row
};
// Both counters and the nested loops all three variants share: a row of `count` columns, each reading a coverage byte from the
// glyph's bitmap and, when it is not zero, blending into the surface through `blend(alpha, x, y, dest)`, which returns the pointer
// the next column continues from. A row ends by moving the surface pointer to the next row and the bitmap pointer by the row's
// bytes less what the columns walked.
template <class Blend>
Walk walk_glyph(Cpu& c, const Scan& s, Blend&& blend) {
    Walk w;
    w.count = s.x_end - s.x_start + 1;
    const std::uint32_t pace = std::uint32_t(s.step) * std::uint32_t(w.count);
    w.src = s.src;
    auto dest = s.dest;
    w.y = s.y_start;
    w.x = s.x_start;
    while (sext16(std::uint32_t(w.y)) <= s.y_end) {
        w.rows = true;
        w.x = s.x_start;
        while (sext16(std::uint32_t(w.x)) <= s.x_end) {
            const std::uint32_t alpha = c.u8(w.src);
            w.alpha = alpha;
            w.visited = true;
            if (alpha != 0) dest = blend(alpha, w.x, w.y, dest);
            w.src += std::uint32_t(s.step);
            ++w.x;
            dest += 2;
        }
        const auto stride = c.u32(s.window + window_record::stride);
        const auto row_bytes = c.u32(s.glyph + glyph_fields::row_bytes);
        w.row_skip = (stride - std::uint32_t(w.count)) << 1;
        ++w.y;
        dest += w.row_skip;
        w.src += row_bytes - pace;
    }
    return w;
}

// Flag 0x20 (0x1074fbb4): the covered pixel is mixed with itself lightened by 7/5 (at most 0xff a channel); when the column and the row
// are not zero the pixel up and to the left is mixed with itself darkened by 3/5 as well.
void paint_embossed(Cpu& c, const Scan& s, std::uint32_t y_start_raw, Held& held) {
    std::uint32_t r5 = y_start_raw;
    held.r19 = std::uint32_t(s.x_end);
    held.r20 = std::uint32_t(s.step) * std::uint32_t(s.x_end - s.x_start + 1);
    const auto w = walk_glyph(c, s, [&](std::uint32_t alpha, std::int32_t x, std::int32_t y, std::uint32_t dest) {
        const std::int32_t a = std::int32_t(alpha), weight = 126 - a;
        const auto lightened = [&](std::int32_t value) {
            const auto bright = std::min(0xff, divide_5(product(value, 7)).value());
            return divide_126(sum(product(weight, value), product(a, bright)));
        };
        const auto pixel = split_pixel(c.u16(dest));
        const auto red = lightened(pixel.red), green = lightened(pixel.green), blue = lightened(pixel.blue);
        const auto joined = join_pixel(red.value(), green.value(), blue.value());
        c.put16(dest, joined);
        r5 = std::uint32_t(red.value()) & 0xff;
        held.r14 = joined, held.r15 = std::uint32_t(blue.value()), held.r16 = std::uint32_t(blue.sign), held.r17 = std::uint32_t(product(weight, pixel.blue));
        held.r18 = std::uint32_t(sum(product(weight, pixel.green), product(a, std::min(0xff, divide_5(product(pixel.green, 7)).value()))));
        if (sext16(std::uint32_t(x)) == 0) return dest;
        r5 = std::uint32_t(sext16(std::uint32_t(y)));
        if (sext16(std::uint32_t(y)) == 0) return dest;
        const auto up = dest - ((c.u32(s.window + window_record::stride) + 1) << 1);
        const auto neighbour = split_pixel(c.u16(up));
        const auto darkened = [&](std::int32_t value) {
            return divide_126(sum(product(weight, value), product(a, divide_5(product(value, 3)).value())));
        };
        const auto dark_red = darkened(neighbour.red), dark_green = darkened(neighbour.green), dark_blue = darkened(neighbour.blue);
        const auto dark_joined = join_pixel(dark_red.value(), dark_green.value(), dark_blue.value());
        c.put16(up, dark_joined);
        r5 = std::uint32_t(dark_red.value()) & 0xff;
        const auto blue_fifth = divide_5(product(neighbour.blue, 3));
        held.r14 = std::uint32_t(product(weight, neighbour.blue)), held.r15 = dark_joined, held.r16 = std::uint32_t(blue_fifth.value()),
        held.r17 = std::uint32_t(blue_fifth.sign), held.r18 = std::uint32_t(product(neighbour.blue, 3));
        return up + ((c.u32(s.window + window_record::stride) + 1) << 1);
    });
    c.set({{3, w.rows ? w.row_skip : std::uint32_t(w.count - 1)}, {4, std::uint32_t(w.count)}, {5, r5}, {6, w.src},
           {7, w.visited ? w.alpha : std::uint32_t(s.x_end)}, {8, std::uint32_t(s.x_start)}, {9, std::uint32_t(w.y)},
           {10, w.rows ? std::uint32_t(w.x) : std::uint32_t(s.y_end)}});
}

// Flag 0x40 (0x1074fe6c): the pixel is mixed, by coverage, with the average of itself and the colour.
void paint_averaged(Cpu& c, const Scan& s, const Channels& ink, std::uint32_t y_start_raw, std::uint32_t y_entry_r4, Held& held) {
    std::uint32_t r4 = y_entry_r4, r5 = y_start_raw;
    held.r15 = std::uint32_t(s.y_end);
    held.r16 = std::uint32_t(s.step) * std::uint32_t(s.x_end - s.x_start + 1);
    held.r17 = std::uint32_t(s.x_end);
    const auto w = walk_glyph(c, s, [&](std::uint32_t alpha, std::int32_t, std::int32_t, std::uint32_t dest) {
        const std::int32_t a = std::int32_t(alpha), weight = 126 - a;
        const auto pixel = split_pixel(c.u16(dest));
        const auto mixed = [&](std::int32_t value, std::int32_t colour) {
            return divide_126(sum(product(weight, value), product(a, (value + colour) >> 1)));
        };
        const auto red = mixed(pixel.red, ink.red), green = mixed(pixel.green, ink.green), blue = mixed(pixel.blue, ink.blue);
        const auto joined = join_pixel(red.value(), green.value(), blue.value());
        c.put16(dest, joined);
        r4 = joined;
        r5 = std::uint32_t(blue.sign);
        held.r14 = std::uint32_t(green.floor), held.r18 = std::uint32_t((pixel.blue + ink.blue) >> 1), held.r19 = std::uint32_t(product(weight, pixel.blue));
        held.r20 = std::uint32_t(red.sign);
        return dest;
    });
    c.set({{3, w.rows ? w.row_skip : std::uint32_t(w.count - 1)}, {4, r4}, {5, r5}, {6, w.src}, {7, std::uint32_t(w.count)},
           {8, std::uint32_t(s.x_start)}, {9, std::uint32_t(w.y)}, {10, w.rows ? std::uint32_t(w.x) : std::uint32_t(s.y_end)}});
}

// Neither flag (0x1074ffa0): the colour mixed in by coverage.
void paint_plain(Cpu& c, const Scan& s, const Channels& ink, std::uint32_t buffer, std::uint32_t y_start_raw, Held& held) {
    std::uint32_t r7 = 0, r9 = buffer;
    held.r14 = std::uint32_t(s.x_end);
    const auto w = walk_glyph(c, s, [&](std::uint32_t alpha, std::int32_t, std::int32_t, std::uint32_t dest) {
        const std::int32_t a = std::int32_t(alpha), weight = 126 - a;
        const auto pixel = split_pixel(c.u16(dest));
        const auto mixed = [&](std::int32_t value, std::int32_t colour) { return divide_126(sum(product(weight, value), product(a, colour))); };
        const auto red = mixed(pixel.red, ink.red), green = mixed(pixel.green, ink.green), blue = mixed(pixel.blue, ink.blue);
        c.put16(dest, join_pixel(red.value(), green.value(), blue.value()));
        r9 = std::uint32_t(red.value());
        r7 = std::uint32_t(blue.sign);
        held.r15 = 0x82082083u, held.r16 = join_pixel(red.value(), green.value(), blue.value()), held.r17 = std::uint32_t(product(weight, pixel.blue));
        held.r18 = std::uint32_t(pixel.blue);
        return dest;
    });
    c.set({{3, std::uint32_t(w.count)}, {4, std::uint32_t(w.y)}, {5, w.rows ? w.row_skip : y_start_raw}, {6, w.src},
           {7, w.visited ? (w.alpha ? r7 : 0) : std::uint32_t(s.x_end)}, {8, std::uint32_t(s.x_start)}, {9, r9}, {10, std::uint32_t(s.y_end)}});
}

// The values the string loop keeps in non-volatile registers.
struct Run {
    std::uint32_t window;      // r28
    std::uint32_t flags16;     // r23 (the low halfword of r13, the flags the loops test)
    std::int32_t step;         // r21
    Channels ink;              // r27 red, r26 green, r25 blue (8 bits each)
    std::uint32_t pen_y;       // r24 (16.16)
    std::uint32_t baseline;    // 0x40(r1): the pen plus both line metrics
    Held held;                 // r14-r20
};

// 0x1074fa94..0x107500b4: one glyph's rectangle clipped to the window and painted. Every path leaves r3-r10 as the original does.
void paint_glyph(Cpu& c, Run& run, std::uint32_t glyph, std::uint32_t pen_x, std::uint32_t index) {
    const auto window = run.window;
    const auto left = c.u32(glyph + glyph_fields::left), top = c.u32(glyph + glyph_fields::top), width = c.u32(glyph + glyph_fields::width),
               height = c.u32(glyph + glyph_fields::height);
    const std::int32_t x0 = whole_pixel(pen_x + (left << 10) + 0x8000), y0 = whole_pixel(run.baseline - (top << 10) + 0x8000);
    const auto window_width = sext16(c.u16(window + window_record::width));
    const std::int32_t x1 = sext16(std::uint32_t(x0) + width - 1), y1 = sext16(std::uint32_t(y0) + height - 1);
    // r3-r10 as the original holds them while it clips: each early exit leaves the registers it has set so far.
    std::uint32_t r3 = std::uint32_t(window_width), r4 = std::uint32_t(y0), r5 = std::uint32_t(y0), r6 = std::uint32_t(y0) + height - 1,
                  r7 = std::uint32_t(x1), r8 = std::uint32_t(x0), r10 = std::uint32_t(y1);
    bool r9_set = false;
    std::uint32_t r9 = 0;
    const auto leave_clipped = [&] {
        c.set({{3, r3}, {4, r4}, {5, r5}, {6, r6}, {7, r7}, {8, r8}, {10, r10}});
        if (r9_set) c.set(9, r9);
    };
    if (x0 >= window_width) return leave_clipped();
    r6 = std::uint32_t(x1);
    if (x1 < 0) return leave_clipped();
    const auto window_height = sext16(c.u16(window + window_record::height));
    r9 = std::uint32_t(window_height), r9_set = true;
    if (y0 >= window_height) return leave_clipped();
    r6 = std::uint32_t(y1);
    if (y1 < 0) return leave_clipped();
    r6 = std::uint32_t(x0);
    if (x0 < 0) r8 = 0;
    r6 = std::uint32_t(y0);
    if (y0 < 0) r5 = 0;
    if (x1 >= window_width) r3 = std::uint32_t(window_width - 1), r7 = std::uint32_t(sext16(r3));
    if (y1 >= window_height) r3 = std::uint32_t(window_height - 1), r10 = std::uint32_t(sext16(r3));

    // 0x1074fb58: the glyph is inside the window. The host's text layer reads the clipped rectangle, the glyph record and the string index.
    c.set({{0, std::uint32_t(x0)}, {3, r3}, {4, r4}, {5, r5}, {6, r6}, {7, r7}, {8, r8}, {9, r9}, {10, r10}, {30, glyph}, {31, index}});
    native_drawing_boundary(c.s, glyph_point);

    // The clipped rectangle's first bitmap byte and surface pixel.
    const auto row_bytes = c.u32(glyph + glyph_fields::row_bytes);
    const std::int32_t y_start = sext16(r5), x_start = std::int32_t(r8), x_end = std::int32_t(r7), y_end = std::int32_t(r10);
    const auto skipped_rows = std::uint32_t(y_start - y0);
    const auto bitmap = c.u32(glyph + glyph_fields::bitmap);
    const std::int32_t first_column = run.step > 0 ? x_start : x_end;
    const auto stride = c.u32(window + window_record::stride);
    const auto src = bitmap + (row_bytes * skipped_rows + std::uint32_t(sext16(std::uint32_t(first_column)))) - std::uint32_t(x0);
    const auto row_offset = std::uint32_t(y_start) * stride;
    const auto buffer = c.u32(window + window_record::buffer);
    const auto dest = buffer + ((row_offset << 1) + (std::uint32_t(sext16(std::uint32_t(x_start))) << 1));
    Scan scan{window, glyph, run.step, src, dest, x_start, x_end, std::int32_t(r5), y_end};
    run.held.r14 = std::uint32_t(y_start);
    if (run.flags16 & style_bit::emboss) paint_embossed(c, scan, r5, run.held);
    else if (run.flags16 & style_bit::average) paint_averaged(c, scan, run.ink, r5, row_offset << 1, run.held);
    else paint_plain(c, scan, run.ink, buffer, r5, run.held);
}

// The assertion reporter: `bl 0x107755c0` with the text, the file name and the line (it does not return in the game).
void report(Cpu& c, std::uint32_t pool, std::uint32_t text, std::uint32_t line, std::uint32_t back_to) {
    c.compare_logical(0, 0, 0);
    c.set({{3, pool + text}, {4, pool + assertion::file}, {5, line}});
    c.call(assertion_report, back_to);
}
}

std::uint32_t draw_text(Cpu& c, const TextRequest& q) {
    Frame frame(c, entry, frame_size, {.first_gpr = 13});
    const auto caller = frame.caller();
    const auto pool = c.toc(toc::pool);
    const auto callback = c.u32(caller + slot::callback);
    c.set({{17, pool}, {28, q.window}, {16, q.x}, {15, q.y}, {13, q.flags}, {14, q.colour}, {29, callback}});
    const auto record = c.u32(q.font_slot);
    c.put32(caller + slot::font, q.font_slot);
    c.put32(frame.at(slot::record), record);
    c.put8(caller + slot::size, q.size);
    c.put32(caller + slot::text, q.text);

    // The arguments' assertions (each reports and, if the reporter returns, goes on: the reads below then use the null pointer).
    if (record == 0) report(c, pool, assertion::font_text, assertion::font_line, back::font_assertion);
    if (c.u8(record + font_fields::loaded) == 0) {
        c.set(3, 0);
        frame.leave();
        native_drawing_boundary(c.s, return_point);
        return 0;
    }
    if (q.window == 0) report(c, pool, assertion::window_text, assertion::window_line, back::window_assertion);
    if (c.u32(q.window + window_record::buffer) == 0) report(c, pool, assertion::buffer_text, assertion::buffer_line, back::buffer_assertion);
    if (c.u32(caller + slot::text) == 0) report(c, pool, assertion::string_text, assertion::string_line, back::text_assertion);

    // The outline flag draws the text in black first (flags | 0x50, colour 0, no callback) and then goes on without the flag; any
    // other style goes to the selector, which makes the font record's scaler match the style and size.
    std::uint32_t flags = q.flags;
    c.compare(0, std::int32_t(flags & style_bit::outline), 0);
    if (flags & style_bit::outline) {
        flags = (flags & ~style_bit::outline) & 0xffff;
        c.set(13, flags);
        c.put32(frame.at(slot::shadow_callback), 0);
        const auto size = c.u8(caller + slot::size);
        const auto text = c.u32(caller + slot::text);
        c.set({{3, c.u32(caller + slot::font)}, {4, q.window}, {5, q.x}, {6, q.y}, {7, (flags | 0x50) & 0xffff}, {8, size}, {9, 0}, {10, text}});
        c.link(back::shadow);
        native_drawing_boundary(c.s, entry);  // the original's `bl` runs the page's observation of the entry
        draw_text(c, {q.font_slot, q.window, q.x, q.y, (flags | 0x50) & 0xffff, size, 0, text});
    } else {
        c.set({{3, c.u32(caller + slot::font)}, {4, flags}, {5, c.u8(caller + slot::size)}});
        c.call(select_style_entry, back::style);
    }

    // The pen's y as a 16.16 value and the baseline: the scaler's two line metrics added to it (the first sum is stored too).
    const auto scaler_record = c.u32(record + font_fields::scaler);
    const auto upper = c.u32(scaler_record + line_metrics::upper), lower = c.u32(scaler_record + line_metrics::lower);
    const auto pen_y = (q.y & 0xffff) << 16;
    c.put32(frame.at(slot::baseline), lower + upper);
    const auto baseline = pen_y + (lower + upper);
    c.put32(frame.at(slot::baseline), baseline);
    c.set(4, upper);
    const std::int32_t step = flags & style_bit::mirror ? -1 : 1;
    Run run{q.window, flags & 0xffff, step, split_colour(q.colour), pen_y, baseline, {q.colour, q.y, q.x, pool, c.r(18), c.r(19), c.r(20)}};
    std::uint32_t pen_x = (q.x & 0xffff) << 16, index = 0, held_glyph = c.r(30);
    // r21-r31 as the original holds them at the string loop's calls (r30: the glyph record of the previous character, 0 after a control
    // character or at the start what the caller had).
    const auto hold = [&](std::uint32_t glyph) {
        const auto& h = run.held;
        c.set({{14, h.r14}, {15, h.r15}, {16, h.r16}, {17, h.r17}, {18, h.r18}, {19, h.r19}, {20, h.r20}});
        c.set({{21, std::uint32_t(run.step)}, {22, pen_x}, {23, run.flags16}, {24, run.pen_y}, {25, std::uint32_t(run.ink.blue)},
               {26, std::uint32_t(run.ink.green)}, {27, std::uint32_t(run.ink.red)}, {30, glyph}, {31, index}});
    };
    // The callback's call: the whole-pixel rectangle (left, top, right, bottom) from the pen and the scaler's two metrics, left and right
    // given; r7 keeps the unextended top of the glyph's call, the glue restores the TOC from its save slot in this frame.
    const auto call_callback = [&](std::uint32_t left, std::uint32_t right, std::uint32_t back_to, std::uint32_t glyph) {
        const auto scaler_now = c.u32(c.u32(frame.at(slot::record)) + font_fields::scaler);
        const auto top_fixed = run.pen_y + c.u32(scaler_now + line_metrics::upper) + 0x8000;
        const auto bottom_fixed = run.pen_y + c.u32(scaler_now + line_metrics::lower) + 0x8000;
        c.compare_logical(0, callback, 0);
        c.set({{12, callback}, {3, left}, {4, std::uint32_t(whole_pixel(top_fixed))}, {5, right}, {6, std::uint32_t(whole_pixel(bottom_fixed))}});
        if (back_to == back::glyph_callback) c.set(7, std::uint32_t(std::int32_t(top_fixed) >> 16));
        hold(glyph);
        c.call(callback_glue, back_to);
        c.set(2, c.u32(frame.at(slot::toc_save)));
    };
    for (;;) {
        const auto text = c.u32(caller + slot::text);
        c.set(3, text);
        std::uint32_t character = c.u8(text + std::uint32_t(sext16(index)));
        c.set(6, character);
        if (character == 0) {
            // The end of the text: the callback gets the rectangle of the pen's own column.
            if (callback != 0) {
                const auto x = std::uint32_t(whole_pixel(pen_x + 0x8000));
                call_callback(x, x, back::final_callback, held_glyph);
            }
            c.set(3, 1);
            frame.leave();
            native_drawing_boundary(c.s, return_point);
            return 1;
        }
        c.set(3, character);
        if (character < 0x20) {
            held_glyph = 0;
        } else {
            if ((run.flags16 & style_bit::bar_as_space) && character == 0x7c) character = 0x20;
            c.compare_logical(0, character, 0x20);
            c.compare(0, std::int32_t(run.flags16 & style_bit::bar_as_space), 0);
            c.set({{3, c.u32(caller + slot::font)}, {4, flags}, {5, c.u8(caller + slot::size)}, {6, character}});
            hold(held_glyph);
            c.call(glyph_record_entry, back::glyph);
            held_glyph = c.r(3);
            bool skip = false;
            if (callback != 0) {
                const auto left = std::uint32_t(whole_pixel(pen_x + 0x8000));
                const auto right = std::uint32_t(whole_pixel(pen_x + c.u32(held_glyph + glyph_fields::advance) + 0x8000));
                call_callback(left, right, back::glyph_callback, held_glyph);
                skip = std::int8_t(c.r(3)) != 0;
            }
            if (!skip) paint_glyph(c, run, held_glyph, pen_x, index);
        }
        native_drawing_boundary(c.s, character_end);
        ++index;
        pen_x += held_glyph ? c.u32(held_glyph + glyph_fields::advance) : 0;
    }
}
}
