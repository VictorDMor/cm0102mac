// The font engine, block `glyph` (GLYPH.C, 0x1075b970..0x1075c97c): the glyph record the scaler outlines are built in. A record
// (glyph_record:: in font_engine_detail.hpp) holds the contours' start and end points, the points' original (26.6) and font-unit
// coordinates and their on-curve flags, all in one block of 13 bytes a point; a glyph read from a font is simple (points) or
// composite (a list of components the caller assembles with append_glyph).
//
// Registers. A routine here leaves the volatile registers r4-r10 as the original does where its callers can read them
// (0x1075c0f0 leaves r9/r10 as its offset loop or its copy loops do: its callers' callers read them, compared with mask 0x60 in
// font_engine_volatile_read.inc); the callee's saved registers are set where the original
// holds them at a call that reaches a leaver (0x1075c830 here, 0x10767bb0 in the allocator).
#include "font_engine_detail.hpp"

namespace cm::aot::font_engine {
namespace {
// What the generated stream readers (0x10762f40, 0x10762fd0) store in their caller's frame before they run: r29-r31 below the stack pointer, the
// link register and the back chain. The C++ readers build a frame only to report an assertion, so a routine that calls one directly writes
// these words itself (the dead stack is then byte for byte the original's); the stack pointer stays the caller's.
void reader_prologue(Cpu& c) {
    const auto sp = c.sp();
    c.put32(sp - 4, c.r(31));
    c.put32(sp - 8, c.r(30));
    c.put32(sp - 0xc, c.r(29));
    c.put32(sp + 8, c.lr());
    c.put32(sp - 0x50, sp);
}
// Callees outside the block, still generated: the allocator's cached-slot allocation (mem, size, slot), plain allocation (mem, size),
// reallocation (mem, block, size) and release (mem, block); and the stream's end-point reader.
constexpr std::uint32_t allocate_cached = 0x107675c0, allocate = 0x107678a0, reallocate = 0x10767a00, release = 0x10767bb0,
                        read_end_point = 0x10763350;
// The memory object's cache of the blocks the allocator keeps for reuse: a pointer a slot (0-5) from +0x128 and the slot's free
// flag from +0x158 (a block freed while it is the kept one only sets the flag).
constexpr std::uint32_t unused_0c = 0x0c, unused_10 = 0x10;  // words the constructors zero and this block never reads
constexpr std::uint32_t mulhwu(std::uint32_t a, std::uint32_t b) { return std::uint32_t(std::uint64_t(a) * b >> 32); }
constexpr std::uint32_t sext16(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }
// The assertion reporter's text, file name and line, from the unit's pool.
void report(Cpu& c, std::uint32_t pool, std::uint32_t text, std::uint32_t line, std::uint32_t back) {
    c.set({{3, pool + text}, {4, pool + 0x16}, {5, line}});
    c.call(assertion_report, back);
}
}

// 0x1075b970: one point of the compact glyph format, read from the stream (no length check): two bytes give 14 bits of a value
// and a 2-bit code. Values below 0x898 stand for themselves (a zero value with a code of 0 or 1 introduces two further
// halfwords, the point stored as read); larger values pack a pair (a, b) of coordinates in a mixed radix (0x66 per row from
// 0x898, 0x2d4 per row with one more byte from 0x313c, 0x299a with two from 0x393c). The code picks how the pair becomes the
// point: (a, b), (-b, a), (-a, -b), (b, -a). The x written is the point's x halved (the low bit is the on-curve flag the
// function returns, inverted); the y is written whole.
std::uint32_t read_compact_point(Cpu& c, std::uint32_t stream, std::uint32_t x_out, std::uint32_t y_out) {
    const auto read_byte = [&](std::uint32_t& position) {
        position = c.u32(stream + stream::position);
        c.put32(stream + stream::position, position + 1);
        return c.u8(c.u32(stream + stream::base) + position);
    };
    std::uint32_t p0, p1;
    const auto b0 = read_byte(p0);
    const auto b1 = read_byte(p1);
    const std::uint32_t word = b0 << 8 | b1, code = word >> 14, value = word & 0x3fff;
    std::uint32_t a = 0, b = 0, r9 = b0, r10 = c.r(10);
    if (value < 0x898) {
        if (value == 0 && std::int32_t(code) <= 1) {
            // Two halfwords follow: the x and the y of the point as stored.
            std::uint32_t p2, p3, p4, p5;
            const auto b2 = read_byte(p2), b3 = read_byte(p3), b4 = read_byte(p4), b5 = read_byte(p5);
            c.put16(x_out, b2 << 8 | b3);
            c.put16(y_out, b4 << 8 | b5);
            c.set({{3, code == 0 ? 1u : 0u}, {6, sext16(b4 << 8) | b5}, {7, p5}, {8, c.u32(stream + stream::base)}, {9, b4},
                   {10, sext16(b2 << 8) | b3}});
            return code == 0;
        }
        a = value;
    } else if (value < 0x313c) {
        const auto rest = value - 0x898, quotient = mulhwu(0xa0a0a0a1, rest) >> 6;
        a = quotient + 1;
        b = rest - quotient * 0x66 + 1;
    } else if (value < 0x393c) {
        std::uint32_t p2;
        const auto b2 = read_byte(p2);
        const auto joined = rlwinm(value - 0x313c, 8, 0, 23) | b2, quotient = mulhwu(0xb509e68b, joined) >> 9;
        a = quotient + 1;
        b = joined - quotient * 0x2d4 + 1;
        r9 = p2;
        r10 = c.u32(stream + stream::base);
    } else {
        std::uint32_t p2, p3;
        const auto b2 = read_byte(p2), b3 = read_byte(p3);
        const auto joined = rlwinm(rlwinm(value - 0x393c, 8, 0, 23) | b2, 8, 0, 23) | b3, quotient = mulhwu(0x62753507, joined) >> 12;
        a = quotient;
        b = joined - quotient * 0x299a;
        r9 = p3;
        r10 = c.u32(stream + stream::base);
    }
    std::uint32_t x = 0, y = 0;
    switch (code) {
    case 0: x = a; y = b; break;
    case 1: x = negate(b); y = a; break;
    case 2: x = negate(a); y = negate(b); break;
    default: x = b; y = negate(a); break;
    }
    c.put16(x_out, std::uint32_t(std::int32_t(x) >> 1));
    c.put16(y_out, y);
    const auto result = 1 - (x & 1);
    c.set({{3, result}, {6, x}, {7, y}, {8, a}, {9, r9}, {10, r10}});
    return result;
}

// 0x1075c830: the point arrays of a glyph for `count` points and the four phantom points after them, in one block
// (13 bytes a point from the allocator's slot 1, not cleared: 0x107675c0 reuses a kept buffer as it is): the original x and y (4 bytes), the x and y (2) and the on-curve flags (1).
// A stack-contract leaver: its frame (r29-r31, LR, back chain) is what the printf readers find in the dead stack.
void allocate_glyph_points(Cpu& c, std::uint32_t g, std::int32_t count) {
    guest::Frame frame(c, 0x1075c830, 0x50, {.first_gpr = 29});
    const std::uint32_t slots = std::uint32_t(count) + 4;
    c.set({{29, g}, {30, std::uint32_t(count)}, {31, slots}});
    c.set({{3, c.u32(g + glyph_record::mem)}, {4, slots * 13}, {5, 1}});
    c.call(allocate_cached, 0x1075c864);
    const auto block = c.r(3);
    const auto by_four = slots << 2, by_two = slots << 1;
    c.put32(g + glyph_record::original_xs, block);
    c.put32(g + glyph_record::original_ys, c.u32(g + glyph_record::original_xs) + by_four);
    c.put32(g + glyph_record::xs, c.u32(g + glyph_record::original_ys) + by_four);
    c.put32(g + glyph_record::ys, c.u32(g + glyph_record::xs) + by_two);
    c.put32(g + glyph_record::on_curve, c.u32(g + glyph_record::ys) + by_two);
    c.put32(g + glyph_record::point_count, std::uint32_t(count));
    c.set({{3, by_four}, {4, by_two}});
}

// 0x1075bba0: a glyph without outline points: only the four phantom points, the advance's point at x = advance_width and the
// vertical pair at y = top_bearing and top_bearing - advance_height.
std::uint32_t new_empty_glyph(Cpu& c, std::uint32_t mem, std::uint32_t advance_width, std::uint32_t top_bearing, std::uint32_t advance_height) {
    guest::Frame frame(c, 0x1075bba0, 0x50, {.first_gpr = 27});
    c.set({{27, mem}, {28, advance_width}, {29, top_bearing}, {30, advance_height}});
    c.set({{3, mem}, {4, glyph_record::size}, {5, 0}});
    c.call(allocate_cached, 0x1075bbcc);
    const auto g = c.r(3);
    c.put32(g + glyph_record::mem, mem);
    c.set(31, g);
    for (const auto field : {glyph_record::contour_ends, glyph_record::contour_starts, glyph_record::components, glyph_record::original_ys, glyph_record::original_xs, unused_0c, unused_10})
        c.put32(g + field, 0);
    c.put16(g + glyph_record::shape, 2);
    c.put16(g + glyph_record::contour_count, 0);
    c.put32(g + glyph_record::point_count, 0);
    c.put32(g + glyph_record::component_capacity, 0);
    c.put16(g + glyph_record::contours, 0);
    for (const auto field : {glyph_record::x_min, glyph_record::y_min, glyph_record::x_max, glyph_record::y_max}) c.put16(g + field, 0);
    c.put32(g + glyph_record::component_words, 0);
    c.put32(g + glyph_record::instruction_length, 0);
    c.put32(g + glyph_record::instructions, 0);
    c.set({{3, g}, {4, 0}});
    c.link(0x1075bc30);
    allocate_glyph_points(c, g, 0);
    const auto x = c.u32(g + glyph_record::xs), y = c.u32(g + glyph_record::ys);
    c.put16(y, 0);
    c.put16(x, 0);
    c.put16(y + 2, 0);
    c.put16(x + 2, advance_width + sext16(c.u16(x)));
    c.put16(y + 4, sext16(c.u16(g + glyph_record::y_max)) + top_bearing);
    c.put16(x + 4, 0);
    c.put16(y + 6, sext16(c.u16(y + 4)) - advance_height);
    c.put16(x + 6, 0);
    c.put16(g + glyph_record::outline_points, 0);
    c.set({{3, g}, {4, 0}, {5, x}, {6, y}});
    return g;
}

// 0x1075bc90: a glyph read from the font at `stream`: a signed halfword counts the contours; a negative count is a composite
// (a list of component records the caller assembles: flags, glyph index, two arguments and an optional scale, each as
// halfwords, in a buffer that grows by half when it comes within ten words of full), a positive one a simple glyph (the
// contours' end points as deltas, then each point in the compact format). The four phantom points follow the outline:
// (x_min - left_bearing, 0), that + advance_width, and the vertical pair from y_max + top_bearing.
// A zero count falls through to the phantom points with null arrays: the stores land at the low addresses, as the original's.
std::uint32_t new_compact_glyph(Cpu& c, std::uint32_t mem, std::uint32_t stream, std::uint32_t left_bearing, std::uint32_t advance_width,
                        std::uint32_t top_bearing, std::uint32_t advance_height) {
    guest::Frame frame(c, 0x1075bc90, 0x90, {.first_gpr = 15});
    // r15-r26 as the original holds them (their roles change as the routine goes on), r27-r31 the arguments.
    std::uint32_t r15 = mem, r16 = c.r(16), r17 = c.r(17), r18 = c.r(18), r19 = c.r(19), r20 = c.r(20), r21 = c.r(21), r22 = c.r(22),
                  r23 = c.r(23), r24 = c.r(24), r25 = c.toc(toc::glyph_pool), r26 = c.r(26);
    const std::uint32_t pool = r25;
    const auto hold = [&] {
        c.set({{15, r15}, {16, r16}, {17, r17}, {18, r18}, {19, r19}, {20, r20}, {21, r21}, {22, r22}, {23, r23}, {24, r24}, {25, r25},
               {26, r26}, {27, stream}, {28, left_bearing}, {29, advance_width}, {30, top_bearing}, {31, advance_height}});
    };
    // A halfword of the stream (the pilot reader called directly: only its link register and r3, the stream, matter).
    const auto read_16 = [&](std::uint32_t back) {
        c.set(3, stream);
        hold();  // the reader's overrun report runs deep calls that save these registers
        c.link(back);
        reader_prologue(c);
        const auto value = std::uint32_t(read_signed_16(c, stream));
        c.set(3, value);
        return value;
    };
    hold();
    c.set({{3, mem}, {4, glyph_record::size}, {5, 0}});
    c.call(allocate_cached, 0x1075bcc8);
    const auto g = r21 = c.r(3);
    c.put32(g + glyph_record::mem, r15);
    r23 = 0;
    r24 = 0;
    c.set({{4, 0}, {3, stream}});
    for (const auto field : {glyph_record::contour_ends, glyph_record::contour_starts, glyph_record::components, glyph_record::original_ys, glyph_record::original_xs, unused_0c, unused_10})
        c.put32(g + field, 0);
    c.put16(g + glyph_record::shape, 2);
    c.put16(g + glyph_record::contour_count, 0);
    c.put32(g + glyph_record::point_count, 0);
    c.put16(g + glyph_record::contours, read_16(0x1075bd10));
    r26 = 0;
    r22 = 0;
    for (const auto field : {glyph_record::x_min, glyph_record::y_min, glyph_record::x_max, glyph_record::y_max}) c.put16(g + field, 0);
    c.put32(g + glyph_record::component_words, 0);
    c.put32(g + glyph_record::instruction_length, 0);
    c.put32(g + glyph_record::instructions, 0);
    const auto declared = sext16(c.u16(g + glyph_record::contours));
    c.set(4, declared);
    if (std::int32_t(declared) < 0) {
        // The component list: records of 3 or 4 halfwords (flags, glyph, one or two arguments), then a scale of 0, 1, 2 or 4.
        c.put32(g + glyph_record::component_capacity, 0x400);
        r19 = 0;
        c.set({{3, c.u32(g + glyph_record::mem)}, {4, c.u32(g + glyph_record::component_capacity) << 1}});
        hold();
        c.call(allocate, 0x1075bd60);
        r20 = c.r(3);
        std::uint32_t flags;
        do {
            const auto capacity = c.u32(g + glyph_record::component_capacity);
            c.set(3, capacity);
            if (std::int32_t(r19) >= std::int32_t(capacity - 10)) {
                c.put32(g + glyph_record::component_capacity, capacity + std::uint32_t(std::int32_t(capacity + (capacity >> 31)) >> 1));
                c.set({{3, c.u32(g + glyph_record::mem)}, {4, r20}, {5, c.u32(g + glyph_record::component_capacity) << 1}});
                hold();
                c.call(reallocate, 0x1075bd9c);
                r20 = c.r(3);
                if (r20 == 0) {
                    hold();
                    report(c, pool, 0, 0x21f, 0x1075bdb4);
                }
            }
            r15 = flags = read_16(0x1075bdbc);
            c.set(3, stream);
            c.put16(r20 + r26, flags);
            r26 += 2;
            r16 = sext16(read_16(0x1075bdd0));
            if (std::int32_t(r16) < 0) {
                hold();
                report(c, pool, 0x1e, 0x226, 0x1075bde8);
            }
            r15 = flags = sext16(flags);
            c.put16(r20 + r26, r16);
            r26 += 2;
            const auto put_words = [&](std::initializer_list<std::uint32_t> backs) {
                for (const auto back : backs) {
                    c.put16(r20 + r26, read_16(back));
                    c.set(3, stream);
                    r26 += 2;
                }
            };
            if (flags & 1) { put_words({0x1075be04, 0x1075be14}); r19 += 4; }
            else { put_words({0x1075be2c}); r19 += 3; }
            if (flags & 8) { put_words({0x1075be48}); r19 += 1; }
            else if (flags & 0x40) { put_words({0x1075be68, 0x1075be78}); r19 += 2; }
            else if (flags & 0x80) { put_words({0x1075be98, 0x1075bea8, 0x1075beb8, 0x1075bec8}); r19 += 4; }
        } while (flags & 0x20);
        c.put32(g + glyph_record::instruction_length, 0);
        c.set({{3, g}, {4, 0}});
        hold();
        c.link(0x1075bef0);
        allocate_glyph_points(c, g, 0);
        r24 = c.u32(g + glyph_record::xs);
        r23 = c.u32(g + glyph_record::ys);
        c.put32(g + glyph_record::components, r20);
        c.put32(g + glyph_record::component_words, r19);
    } else if (std::int32_t(declared) > 0) {
        // A simple glyph: the end points, then the points.
        r18 = 0;
        if (std::int32_t(declared) <= 8) {
            c.put32(g + glyph_record::contour_starts, g + glyph_record::inline_ends);
            c.put32(g + glyph_record::contour_ends, c.u32(g + glyph_record::contour_starts) + 0x10);
        } else {
            c.set({{3, c.u32(g + glyph_record::mem)}, {4, declared << 2}});
            hold();
            c.call(allocate, 0x1075bf3c);
            c.put32(g + glyph_record::contour_starts, c.r(3));
            c.put32(g + glyph_record::contour_ends, c.u32(g + glyph_record::contour_starts) + (declared << 1));
        }
        r16 = 0;
        r15 = 0;
        while (std::int32_t(r16) < std::int32_t(sext16(c.u16(g + glyph_record::contours)))) {
            c.put16(c.u32(g + glyph_record::contour_starts) + r15, r18);
            c.set(3, stream);
            hold();
            c.call(read_end_point, 0x1075bf70);
            c.put16(c.u32(g + glyph_record::contour_ends) + r15, sext16(r18) + c.r(3));
            ++r16;
            r18 = sext16(sext16(c.u16(c.u32(g + glyph_record::contour_ends) + r15)) + 1);
            r15 += 2;
        }
        r22 = sext16(r18);
        c.put32(g + glyph_record::instruction_length, 0);
        c.set({{3, g}, {4, r22}});
        hold();
        c.link(0x1075bfbc);
        allocate_glyph_points(c, g, std::int32_t(r22));
        r24 = c.u32(g + glyph_record::xs);
        r18 = 0;
        r23 = c.u32(g + glyph_record::ys);
        r19 = 0;
        r15 = c.u32(g + glyph_record::on_curve);
        r16 = r24;
        r17 = r23;
        c.put16(g + glyph_record::contour_count, c.u16(g + glyph_record::contours));
        r26 = 0x7fff;
        r20 = negate(0x7fff);
        r25 = 0;
        c.put32(g + glyph_record::point_count, r22);
        const auto dx_at = frame.at(0x3a), dy_at = frame.at(0x38);
        for (; std::int32_t(r25) < std::int32_t(r22); ++r25, ++r15) {
            c.set({{3, stream}, {4, dx_at}, {5, dy_at}});
            const auto on_curve = read_compact_point(c, stream, dx_at, dy_at);
            c.put8(r15, on_curve);
            r19 = sext16(r19 + sext16(c.u16(dx_at)));
            r18 = sext16(r18 + sext16(c.u16(dy_at)));
            if (std::int32_t(r19) < std::int32_t(sext16(r26))) r26 = r19;
            if (std::int32_t(r18) > std::int32_t(sext16(r20))) r20 = r18;
            c.put16(r16, r19);
            r16 += 2;
            c.put16(r17, r18);
            r17 += 2;
        }
        c.put16(g + glyph_record::x_min, r26);
        c.put16(g + glyph_record::y_max, r20);
    }
    // The phantom points after the outline's `r22` points (the arrays are null for a glyph with no contours).
    const auto at = r22 << 1;
    c.put16(r23 + at, 0);
    const auto x = r24 + at, y = r23 + at;
    c.put16(x, sext16(c.u16(g + glyph_record::x_min)) - left_bearing);
    c.put16(y + 2, 0);
    c.put16(x + 2, advance_width + sext16(c.u16(x)));
    c.put16(y + 4, sext16(c.u16(g + glyph_record::y_max)) + top_bearing);
    const auto middle = std::uint32_t(std::int32_t(sext16(c.u16(x)) + sext16(c.u16(x + 2))) >> 1);
    c.put16(x + 4, middle);
    c.put16(y + 6, sext16(c.u16(y + 4)) - advance_height);
    c.put16(x + 6, middle);
    c.put16(g + glyph_record::outline_points, r22);
    c.set({{3, g}, {4, middle}, {5, sext16(c.u16(x))}, {6, y}, {7, x}});
    return g;
}

// 0x1075c0f0: add the glyph `source` (a component) to the glyph a composite builds in `*slot`. The first component becomes the
// glyph (offset by (dx, dy) when flag 2, ARGS_ARE_XY_VALUES, is set, rounded to 64 first when flag 4 is). Later ones are merged
// into it: new arrays for the union of the points and contours are allocated, the destination's points copied, the component's
// points added at the offset, and the destination's arrays released and replaced. Flag 0x200 (USE_MY_METRICS) leaves the new
// arrays' two phantom points unset; without it the destination's are carried over. Without flag 2 the offset is the difference
// of the matched points: point dx of the destination and point dy of the component. A destination whose last end point is not
// its last point reports the assertion and goes on. Each routine's callers ignore r4-r8 (r9/r10: font_engine_volatile_read.inc for 0x1075c0f0).
void append_glyph(Cpu& c, std::uint32_t slot, std::uint32_t source, std::uint32_t flags, std::uint32_t dx, std::uint32_t dy) {
    guest::Frame frame(c, 0x1075c0f0, 0x80, {.first_gpr = 17});
    std::uint32_t r17 = flags, r18 = dx, r19 = dy, r20 = slot, r21 = c.r(21), r22 = c.r(22), r23 = c.toc(toc::glyph_pool), r24 = c.r(24),
                  r25 = c.r(25), r26 = source, r27 = c.r(27), r28 = c.r(28), r29 = c.r(29), r30 = c.r(30), r31 = c.r(31);
    const std::uint32_t pool = r23;
    const auto hold = [&] {
        c.set({{17, r17}, {18, r18}, {19, r19}, {20, r20}, {21, r21}, {22, r22}, {23, r23}, {24, r24}, {25, r25}, {26, r26},
               {27, r27}, {28, r28}, {29, r29}, {30, r30}, {31, r31}});
    };
    if (source == 0) {
        hold();
        report(c, pool, 0x2e, 0x2df, 0x1075c12c);
    }
    r22 = c.u32(slot);
    const auto destination = r22;
    // The offset a component gets from its flags: the arguments themselves, rounded to the grid when flag 4 says so.
    const auto rounded = [](std::uint32_t value) { return (value + 0x20) & 0xffffffc0u; };
    if (destination == 0) {
        if (flags & 2) {
            const std::uint32_t offset_x = flags & 4 ? rounded(dx) : dx, offset_y = flags & 4 ? rounded(dy) : dy;
            const auto count = std::int32_t(sext16(c.u16(source + glyph_record::outline_points)));
            if ((offset_x != 0 || offset_y != 0) && count > 0) {
                for (std::int32_t n = 0; n < count; ++n) {
                    const auto at = std::uint32_t(n) << 2;
                    const auto x = c.u32(source + glyph_record::original_xs) + at, y = c.u32(source + glyph_record::original_ys) + at;
                    c.put32(x, c.u32(x) + offset_x);
                    c.put32(y, c.u32(y) + offset_y);
                }
                // The unrolled-by-eight loop (more than eight points) leaves its last pass's offset constants: r10 = 0x18 + 0x20 * (passes - 1), r9 = r10 + 4
                // (this routine's callers read r9 and r10 after it: 0x1075fa10 passes them on to its own callers).
                if (count > 8) {
                    const auto passes = std::uint32_t((count - 1) >> 3);
                    c.set({{10, 0x18 + 0x20 * (passes - 1)}, {9, 0x1c + 0x20 * (passes - 1)}});
                }
            }
        }
        c.put32(slot, source);
        return;
    }
    const auto destination_points = sext16(c.u16(destination + glyph_record::outline_points));
    const auto source_points = sext16(c.u16(source + glyph_record::outline_points));
    const auto destination_contours = sext16(c.u16(destination + glyph_record::contours));
    r21 = destination_points + source_points;
    r25 = r21 + 4;
    r20 = destination_contours + sext16(c.u16(source + glyph_record::contours));
    c.set({{3, c.u32(destination + glyph_record::mem)}, {4, r25 * 13}});
    hold();
    c.call(allocate, 0x1075c33c);
    r24 = c.r(3);
    const auto by_four = r25 << 2, by_two = r25 << 1;
    r27 = r24 + by_four;
    r30 = r27 + by_four;
    r29 = r30 + by_two;
    r28 = r29 + by_two;
    c.set({{3, c.u32(destination + glyph_record::mem)}, {4, r20 << 2}});
    hold();
    c.call(allocate, 0x1075c364);
    r25 = c.r(3);
    c.put32(destination + glyph_record::point_count, sext16(r21));
    c.put16(destination + glyph_record::contour_count, r20);
    r31 = r25 + (r20 << 1);
    // The destination's points first.
    for (std::uint32_t n = 0; std::int32_t(n) < std::int32_t(sext16(c.u16(destination + glyph_record::outline_points))); ++n) {
        const auto at = n << 2;
        c.put32(r24 + at, c.u32(c.u32(destination + glyph_record::original_xs) + at));
        c.put32(r27 + at, c.u32(c.u32(destination + glyph_record::original_ys) + at));
        c.put8(r28 + n, c.u8(c.u32(destination + glyph_record::on_curve) + n));
    }
    const auto old_points = sext16(c.u16(destination + glyph_record::outline_points));
    if (!(flags & 0x200)) {
        // The destination's phantom points (the first two) move to the end of the new outline.
        const auto from = old_points << 2, to = r21 << 2;
        c.put32(r24 + to, c.u32(c.u32(destination + glyph_record::original_xs) + from));
        c.put32(r27 + to, c.u32(c.u32(destination + glyph_record::original_ys) + from));
        c.put32(r24 + to + 4, c.u32(c.u32(destination + glyph_record::original_xs) + from + 4));
        c.put32(r27 + to + 4, c.u32(c.u32(destination + glyph_record::original_ys) + from + 4));
    }
    std::uint32_t offset_x, offset_y;
    if (flags & 2) {
        offset_x = r18;
        offset_y = r19;
        if (flags & 4) {
            offset_x = rounded(r18);
            offset_y = rounded(r19);
        }
    } else {
        const auto from = r19 << 2, to = r18 << 2;
        const auto source_x = c.u32(c.u32(source + glyph_record::original_xs) + from), destination_x = c.u32(c.u32(destination + glyph_record::original_xs) + to);
        const auto source_y = c.u32(c.u32(source + glyph_record::original_ys) + from), destination_y = c.u32(c.u32(destination + glyph_record::original_ys) + to);
        offset_x = destination_x - source_x;
        offset_y = destination_y - source_y;
    }
    // The component's points, moved by the offset, after the destination's.
    const auto first = sext16(c.u16(destination + glyph_record::outline_points));
    // The copy loops' leftovers: r9 the on-curve cursor, r10 the byte offset into the component's arrays (the assertion below spills them).
    c.set({{9, r28 + first}, {10, 0}});
    if (std::int32_t(first) < std::int32_t(r21)) {
        c.set({{9, r28 + r21}, {10, (r21 - first) << 2}});
        for (std::uint32_t n = 0; n < r21 - first; ++n) {
            const auto at = n << 2, into = (first + n) << 2;
            c.put32(r24 + into, offset_x + c.u32(c.u32(source + glyph_record::original_xs) + at));
            c.put32(r27 + into, offset_y + c.u32(c.u32(source + glyph_record::original_ys) + at));
            c.put8(r28 + first + n, c.u8(c.u32(source + glyph_record::on_curve) + n));
        }
    }
    {
        const auto end_points = c.u32(destination + glyph_record::contour_ends) + (sext16(c.u16(destination + glyph_record::contours)) << 1);
        if (sext16(c.u16(destination + glyph_record::outline_points)) - 1 != sext16(c.u16(end_points - 2))) {
            hold();
            report(c, pool, 0x3c, 0x33f, 0x1075c638);
        }
    }
    for (std::uint32_t n = 0; std::int32_t(n) < std::int32_t(sext16(c.u16(destination + glyph_record::contours))); ++n) {
        c.put16(r25 + (n << 1), c.u16(c.u32(destination + glyph_record::contour_starts) + (n << 1)));
        c.put16(r31 + (n << 1), c.u16(c.u32(destination + glyph_record::contour_ends) + (n << 1)));
    }
    const auto own = sext16(c.u16(destination + glyph_record::contours));
    if (std::int32_t(own) < std::int32_t(r20)) {
        for (std::uint32_t n = 0; n < r20 - own; ++n) {
            const auto at = n << 1, into = (own + n) << 1;
            c.put16(r25 + into, sext16(c.u16(destination + glyph_record::outline_points)) + sext16(c.u16(c.u32(source + glyph_record::contour_starts) + at)));
            c.put16(r31 + into, sext16(c.u16(destination + glyph_record::outline_points)) + sext16(c.u16(c.u32(source + glyph_record::contour_ends) + at)));
        }
    }
    c.put16(destination + glyph_record::outline_points, r21);
    c.put16(destination + glyph_record::contours, r20);
    // The destination's own arrays go (to the allocator's slot when its block is the one it keeps), the new ones take over.
    {
        const auto mem = c.u32(destination);
        const auto block = c.u32(destination + glyph_record::original_xs);
        if (block == c.u32(mem + tsimem::scratch_pointer + 4)) c.put32(mem + tsimem::scratch_free + 4, 1);
        else { c.set({{3, mem}, {4, block}}); hold(); c.call(release, 0x1075c7d4); }
    }
    if (c.u32(destination + glyph_record::contour_starts) != destination + glyph_record::inline_ends) {
        c.set({{3, c.u32(destination)}, {4, c.u32(destination + glyph_record::contour_starts)}});
        hold();
        c.call(release, 0x1075c7ec);
    }
    c.put32(destination + glyph_record::original_xs, r24);
    c.put32(destination + glyph_record::original_ys, r27);
    c.put32(destination + glyph_record::xs, r30);
    c.put32(destination + glyph_record::ys, r29);
    c.put32(destination + glyph_record::on_curve, r28);
    c.put32(destination + glyph_record::contour_starts, r25);
    c.put32(destination + glyph_record::contour_ends, r31);
    c.put16(destination + glyph_record::shape, c.u16(source + glyph_record::shape));
}

// 0x1075c8c0: release a glyph: the point block and the component buffer go back to the allocator's cached slots when they are
// the blocks it keeps (a flag marks the slot free again), else they are freed; the contours' arrays too unless they are the
// record's own inline ones; then the record itself (slot 0).
void delete_glyph(Cpu& c, std::uint32_t g) {
    guest::Frame frame(c, 0x1075c8c0, 0x40, {.first_gpr = 31});
    c.set(31, g);
    if (g == 0) return;
    const auto release_or_keep = [&](std::uint32_t field, std::uint32_t slot, std::uint32_t back) {
        const auto mem = c.u32(g);
        const auto block = c.u32(g + field);
        c.set({{3, mem}, {4, block}});
        if (block == c.u32(mem + tsimem::scratch_pointer + 4 * slot)) c.put32(mem + tsimem::scratch_free + 4 * slot, 1);
        else c.call(release, back);
    };
    release_or_keep(glyph_record::original_xs, 1, 0x1075c8fc);
    c.set(4, c.u32(g + glyph_record::contour_starts));
    if (c.r(4) != g + glyph_record::inline_ends) {
        c.set(3, c.u32(g));
        c.call(release, 0x1075c914);
    }
    release_or_keep(glyph_record::instructions, 3, 0x1075c938);
    c.set({{3, c.u32(g)}, {4, c.u32(g + glyph_record::components)}});
    c.call(release, 0x1075c944);
    const auto mem = c.u32(g);
    c.set(3, mem);
    if (g == c.u32(mem + tsimem::scratch_pointer)) c.put32(mem + tsimem::scratch_free, 1);
    else { c.set(4, g); c.call(release, 0x1075c968); }
}
}
