// graph_carbon.cpp, block `fills` (0x107697c0, 0x10769980, 0x1076a580, 0x1076a790): the pixel primitives that work on rectangles of the
// 15-bit surface (the game's off-screen screen, guest memory) and leave their loops' last values in r3-r10.
//
//   0x107697c0 shade_to_grey(x1, y1, x2, y2): every pixel of the rectangle (corners in any order, bounds inclusive) becomes the grey of
//     its three 5-bit components' average.
//   0x10769980 shade_rectangle(x1, y1, x2, y2) (the one that ignores the suspension byte): every pixel goes through a 64K-entry table of the pixels at 60% (built on first use by
//     0x1076bb40 over every 16-bit value, recognised as built by a nonzero last entry).
//   0x1076a580 fill_rectangle(x1, y1, x2, y2, style, colour): style bit 1 or 2 draws the four edges as lines (0x1076a140 with that style),
//     otherwise the rectangle [x1, x2) x [y1, y2) is filled with the colour (the exclusive right and bottom edges are the original's).
//   0x1076a790 draw_mask(x, y, colour, bitmap): the bitmap {width byte, height byte, width * height bytes} paints the colour wherever
//     its byte is not zero; a null bitmap is the game's assertion (line 0x61d).
//
// STACK CONTRACT. 0x10769980 and 0x1076a790 are leavers (stmw r24 / r30): the registers their prologues save, the link register and the
// back chain are read by the game's printf (tools/stack_contract.py module graph_carbon.cpp), so their frames are the original's (0x60
// and 0x70 bytes) and the assertion's callees run at their stack pointer with r30 and r31 as the original holds them. 0x107697c0
// (0x50 bytes, r28-r31) and 0x1076a580 (0x50 bytes, r27-r31) are not leavers but the second calls the leaver 0x1076a140 four times and
// holds r27-r31 as the original does there (the other registers r19-r26 are the caller's, never touched).
//
// REGISTERS. r3-r10 are as the original leaves them on every path (each loop's last counters and temporaries): `Volatile` carries them,
// nothing exempt. r0, r11 and r12 are diagnostics.
//
// NATIVE DRAWING. The entries are observed by the generated page (before the hook); the exits (0x107698c4, 0x10769b48, 0x1076a784) are
// the instruction after the epilogue, which the C++ passes through native_drawing_boundary itself.
#include "graph_carbon_pixels_detail.hpp"
#include <utility>

namespace cm::aot::graph_carbon {
namespace {
constexpr std::uint32_t grey_entry = 0x107697c0, grey_frame = 0x50, grey_exit = 0x107698c4;
constexpr std::uint32_t shade_entry = 0x10769980, shade_frame = 0x60, shade_exit = 0x10769b48;
constexpr std::uint32_t fill_entry = 0x1076a580, fill_frame = 0x50, fill_exit = 0x1076a784;
constexpr std::uint32_t mask_entry = 0x1076a790, mask_frame = 0x70;
// The table of shaded pixels (the TOC slot -0x3bd4 holds its address): 0x10000 halfwords, the entry of pixel p at 2 * p.
namespace shade_table { constexpr std::int32_t slot = -0x3bd4; constexpr std::uint32_t pixels = 0x10000, last = 0x1fffe; constexpr std::uint32_t percent = 0x3c; }
// The shared message buffer and success flag of the assertions (TOC slots), and the pool offsets of the message.
namespace assertion_slot { constexpr std::int32_t buffer = -0x7ba8, flag = -0x7bac; }
namespace mask_assertion { constexpr std::uint32_t at = 0x1076a7d4, format = 0x11, title = 0x1e, line = 0x61d; }
// The address after each `bl` of 0x1076a580.
constexpr std::uint32_t edge_returns[2][4] = {{0x1076a5c4, 0x1076a5e0, 0x1076a5fc, 0x1076a618}, {0x1076a630, 0x1076a64c, 0x1076a668, 0x1076a684}};

// The Rect the corners fall into (what 0x107698d0 stores): top, left, bottom, right as halfwords.
struct Corners4 { std::int32_t top, left, bottom, right; };

// The grey of a 15-bit pixel: the three 5-bit components widened to 8 bits and averaged (the high word of the multiply by 0x55555556 with
// its sign added is the division by 3).
std::uint32_t grey_of(std::uint32_t pixel) {
    const auto blue = (pixel << 3) & 0xf8, green = (pixel >> 2) & 0xf8, red = (pixel >> 7) & 0xf8;
    const auto high = high_word(0x55555556, std::int32_t(red + (green + blue)));
    return (std::uint32_t(high) + (std::uint32_t(high) >> 31)) & 0xff;
}
}

void shade_to_grey(Cpu& c, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2) {
    Frame frame(c, grey_entry, grey_frame, {.first_gpr = 28});
    auto v = Volatile::load(c);
    if (c.u8(small(c, surface_variable::suspended)) == 0) {
        // The corners' rectangle, built in the frame by the module's own routine (r7 = its address, r5 and r6 come back as the larger
        // coordinates).
        const auto rect = frame.at(0x38);
        v.r7 = rect;
        Leftovers corners{v.r4, v.r5, v.r6, v.r7, v.r8, v.r9, v.r10};
        rectangle_from_corners(c, x1, y1, x2, y2, rect, corners);
        v.r4 = corners.r4, v.r5 = corners.r5, v.r6 = corners.r6, v.r7 = corners.r7, v.r8 = corners.r8, v.r9 = corners.r9, v.r10 = corners.r10;
        const Rect box(c, rect);
        std::int32_t row = box.top;
        v.r4 = word(row);
        const auto stride = std::uint32_t(c.i16(small(c, surface_variable::row_pixels)));
        v.r3 = word(std::int32_t(box.left));
        v.r4 = c.u32(small(c, surface_variable::base));
        v.r3 <<= 1;
        auto pixel_at = v.r4 + (v.r3 + ((word(row) * stride) << 1));
        while (row <= std::int32_t(box.bottom)) {
            std::int32_t column = box.left;
            for (;;) {
                v.r4 = word(std::int32_t(box.right));
                if (column > std::int32_t(box.right)) break;
                const auto grey = grey_of(c.u16(pixel_at));
                v.r6 = 0;
                v.r4 = grey;
                v.r5 = grey;
                v.r3 = pack_pixel(grey, grey, grey);
                c.put16(pixel_at, v.r3);
                pixel_at += 2;
                ++column;
            }
            v.r3 = word(std::int32_t(box.left));
            ++row;
            v.r3 = v.r4 - v.r3 + 1;
            pixel_at += (std::uint32_t(c.i16(small(c, surface_variable::row_pixels))) - v.r3) << 1;
        }
        v.store(c);
    }
    frame.leave();
    native_drawing_boundary(c.s, grey_exit);
}

void shade_rectangle(Cpu& c, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2) {
    Frame frame(c, shade_entry, shade_frame, {.first_gpr = 24});
    auto v = Volatile::load(c);
    {
        // (This one does not test the suspension byte: the original draws whatever it holds.)
        // The rectangle's bounds: the smaller and larger corners, the raw words kept in registers and their halfwords used.
        v.r7 = word(x2 < x1 ? x2 : x1);
        const auto y_low = word(y2 < y1 ? y2 : y1);
        v.r5 = word(x1 > x2 ? x1 : x2);
        v.r6 = word(y1 > y2 ? y1 : y2);
        const std::int32_t x_min = sign_extend_16(v.r7), y_min = sign_extend_16(y_low), x_max = sign_extend_16(v.r5), y_max = sign_extend_16(v.r6);
        const auto table = c.toc(shade_table::slot);
        v.r3 = table + 0x20000;
        if (c.u16(table + shade_table::last) == 0) {
            // First use: every 16-bit pixel at 60%, by the module's own scale routine (r4 = 60, r5 = 0 at each call, the rest as the last
            // call left it).
            Leftovers last{v.r4, v.r5, v.r6, v.r7, v.r8, v.r9, v.r10};
            for (std::uint32_t pixel = 0; pixel < shade_table::pixels; ++pixel) {
                last.r4 = shade_table::percent;
                last.r5 = 0;
                v.r3 = scale_pixel(pixel, shade_table::percent, last);
                c.put16(table + pixel * 2, v.r3);
            }
            v.r4 = last.r4, v.r5 = last.r5, v.r6 = last.r6, v.r7 = last.r7, v.r8 = last.r8, v.r9 = last.r9, v.r10 = last.r10;
        }
        const auto stride = std::uint32_t(c.i16(small(c, surface_variable::row_pixels)));
        v.r3 = word(x_max) - word(x_min);
        v.r5 = word(x_min) << 1;
        v.r6 = c.u32(small(c, surface_variable::base));
        v.r7 = word(y_min);
        v.r4 = v.r3 + 1;
        v.r5 = v.r6 + (v.r5 + ((word(y_min) * stride) << 1));
        const auto shade = [&](std::uint32_t at) { c.put16(at, c.u16(table + (std::uint32_t(c.u16(at)) << 1))); };
        while (std::int32_t(v.r7) <= y_max) {
            v.r3 = word(x_max) + 1 - word(x_min);
            if (x_min <= x_max) {
                const std::uint32_t blocks = v.r3 >> 3;
                bool tail = true;
                if (blocks != 0) {
                    for (std::uint32_t block = 0; block < blocks; ++block) {
                        for (std::uint32_t k = 0; k < 8; ++k) shade(v.r5 + 2 * k);
                        v.r5 += 16;
                    }
                    v.r3 &= 7;
                    tail = v.r3 != 0;
                }
                if (tail)
                    for (std::uint32_t n = 0; n < v.r3; ++n) {
                        shade(v.r5);
                        v.r5 += 2;
                    }
            }
            ++v.r7;
            v.r5 += (std::uint32_t(c.i16(small(c, surface_variable::row_pixels))) - v.r4) << 1;
        }
        v.store(c);
    }
    frame.leave();
    native_drawing_boundary(c.s, shade_exit);
}

void fill_rectangle(Cpu& c, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t style, std::uint32_t colour) {
    Frame frame(c, fill_entry, fill_frame, {.first_gpr = 27});
    c.set({{27, word(x1)}, {28, word(y1)}, {29, word(x2)}, {30, word(y2)}, {31, colour}});
    auto v = Volatile::load(c);
    if (c.u8(small(c, surface_variable::suspended)) == 0) {
        if (style & 1 || style & 2) {
            // The four edges as lines of the same style: top, right, bottom, left.
            const auto& returns = edge_returns[(style & 1) ? 0 : 1];
            const auto line_style = (style & 1) ? 1u : 2u;
            call_draw_line(c, {x1, y1, x2, y1, line_style, colour}, returns[0]);
            call_draw_line(c, {x2, y1, x2, y2, line_style, colour}, returns[1]);
            call_draw_line(c, {x2, y2, x1, y2, line_style, colour}, returns[2]);
            call_draw_line(c, {x1, y2, x1, y1, line_style, colour}, returns[3]);
        } else {
            // The corners in order (the halfword of the one that moves), then the exclusive rectangle.
            std::int32_t left = x1, top = y1, right = x2, bottom = y2;
            if (left > right) { const auto moved = sign_extend_16(word(left)); left = right; right = moved; }
            if (top > bottom) { const auto moved = sign_extend_16(word(top)); top = bottom; bottom = moved; }
            const auto stride = std::uint32_t(c.i16(small(c, surface_variable::row_pixels)));
            v.r6 = word(sign_extend_16(word(right) - word(left)));
            v.r7 = word(sign_extend_16(word(bottom) - word(top)));
            v.r3 = word(left) << 1;
            v.r4 = c.u32(small(c, surface_variable::base));
            v.r5 = 0;
            v.r8 = v.r4 + (v.r3 + ((word(top) * stride) << 1));
            const std::int32_t width = std::int32_t(v.r6);
            while (sign_extend_16(v.r5) < std::int32_t(v.r7)) {
                v.r4 = 0;
                if (width > 0) {
                    if (width > 8) {
                        const std::int32_t limit = sign_extend_16(v.r6 - 8);
                        for (;;) {
                            v.r3 = word(sign_extend_16(v.r4));
                            if (std::int32_t(v.r3) >= limit) break;
                            for (std::uint32_t k = 0; k < 8; ++k) c.put16(v.r8 + 2 * k, colour);
                            v.r4 += 8;
                            v.r8 += 16;
                        }
                    }
                    while (sign_extend_16(v.r4) < width) {
                        c.put16(v.r8, colour);
                        v.r8 += 2;
                        ++v.r4;
                    }
                }
                ++v.r5;
                v.r8 += (std::uint32_t(c.i16(small(c, surface_variable::row_pixels))) - v.r6) << 1;
            }
            v.store(c);
        }
    }
    frame.leave();
    native_drawing_boundary(c.s, fill_exit);
}

void draw_mask(Cpu& c, std::int32_t x, std::int32_t y, std::uint32_t colour, std::uint32_t bitmap) {
    Frame frame(c, mask_entry, mask_frame, {.first_gpr = 30});
    const auto pool = c.toc(toc::pool), buffer = c.toc(assertion_slot::buffer);
    c.set({{31, pool}, {30, buffer}});
    if (c.u8(small(c, surface_variable::suspended)) == 0) {
        if (bitmap == 0) {
            // The null bitmap: the game's assertion, the success flag cleared (r3 holds the flag's address afterwards).
            report_assertion(c, {mask_assertion::at, pool, pool + mask_assertion::format, pool + mask_assertion::title, frame.at(0x40), frame.at(0x38),
                                 mask_assertion::line});
            c.set(3, c.toc(assertion_slot::flag));
        } else {
            auto v = Volatile::load(c);
            const std::uint32_t width = c.u8(bitmap), height = c.u8(bitmap + 1);
            v.r10 = width;
            // The upper left pixel: x and y as the smaller of the position and the far corner (width and height less one), halfwords.
            const auto far_x = word(x) + (width - 1), far_y = word(y) + (height - 1);
            v.r3 = word(sign_extend_16(std::int32_t(word(x)) < std::int32_t(far_x) ? word(x) : far_x));
            v.r7 = std::int32_t(word(y)) < std::int32_t(far_y) ? word(y) : far_y;
            const auto stride = std::uint32_t(c.i16(small(c, surface_variable::row_pixels)));
            v.r4 = word(sign_extend_16(v.r7));
            v.r7 = width - 1;
            v.r8 = v.r3 << 1;
            v.r4 *= stride;
            v.r9 = c.u32(small(c, surface_variable::base));
            v.r3 = height - 1;
            v.r4 <<= 1;
            auto pixel_at = v.r9 + (v.r8 + v.r4);
            for (std::uint32_t row = 0; std::int32_t(row) <= std::int32_t(v.r3); ++row) {
                v.r8 = row * width;
                v.r9 = 0;
                if (std::int32_t(v.r7) >= 0) {
                    for (std::uint32_t n = 0; n < width; ++n) {
                        v.r4 = c.u8(bitmap + (v.r8 + v.r9 + 2));
                        if (v.r4 != 0) c.put16(pixel_at, colour);
                        pixel_at += 2;
                        ++v.r9;
                    }
                }
                v.r4 = (std::uint32_t(c.i16(small(c, surface_variable::row_pixels))) - width) << 1;
                pixel_at += v.r4;
            }
            v.store(c);
        }
    }
    frame.leave();
}
}
