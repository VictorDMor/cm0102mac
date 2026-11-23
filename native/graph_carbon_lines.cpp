// graph_carbon.cpp, block `lines` (0x1076a140, 0x1076a8f0, 0x1076ad50): the line, the circle's octants and the ellipse. All three paint the
// game's 15-bit surface in guest memory (surface_variable) and leave the loops' last values in r3-r10.
//
//   0x1076a140 draw_line(x1, y1, x2, y2, style, colour): style bit 1 is read by the caller 0x1076a580 only; bit 2 draws every pixel, clear
//     draws four pixels of each six along the line (dotted). A horizontal or vertical line (the 16-bit extents equal) is a plain loop, any
//     other a Bresenham walk along the longer axis; the pixels outside the line's own 16-bit bounding box are skipped (which only differs
//     from the whole line when a coordinate does not fit 16 bits).
//   0x1076a8f0 circle_octants: the four mirrored points of a circle's octant steps, clipped to a rectangle, optionally recording each
//     row's leftmost and rightmost x in a table of {minimum, maximum} pairs (the caller's ninth argument, the word at sp + 0x38).
//   0x1076ad50 draw_ellipse(x1, y1, x2, y2, style, colour): bit 4 fills it (rows of a table the octants fill in, then drawn as solid
//     lines), bit 2 draws every pixel of the outline, clear draws four pixels of six.
//
// STACK CONTRACT. 0x1076a140 is a leaver (stmw r19): the registers its prologue saves, the link register and the back chain are read by
// the game's printf (tools/stack_contract.py module graph_carbon.cpp). Its frame (0x70 bytes, r19-r31, LR at +8) is the original's, and
// every caller of it here (0x1076ad50, 0x1076a580) holds r19-r31 and the link register as the original does at its `bl`. The other two
// build a frame the original has (0x1076ad50: 0x80 bytes, r17-r31) or none (0x1076a8f0 is a leaf whose stmw below the stack pointer is dead).
//
// REGISTERS. r3-r10 are as the original leaves them on every path: each loop's last counters, the last remainder by six, the last
// pixel's offset. The loops are modelled with the original's values in `Volatile`, nothing exempt; r0, r11 and r12 are diagnostics.
#include "graph_carbon_pixels_detail.hpp"
#include <utility>

namespace cm::aot::graph_carbon {
namespace {
constexpr std::uint32_t line_entry = 0x1076a140, line_frame = 0x70, line_exit = 0x1076a570;
constexpr std::uint32_t ellipse_entry = 0x1076ad50, ellipse_frame = 0x80, ellipse_exit = 0x1076b014;
constexpr std::uint32_t allocate_block = 0x107735f0, release_block = 0x10773610;
// The ellipse's own slot for the ninth argument of the octants (the word the callee reads at its sp + 0x38).
constexpr std::uint32_t octant_table_slot = 0x38;
// The address after each `bl` of the ellipse.
namespace back {
constexpr std::uint32_t allocate = 0x1076ad9c, line = 0x1076afec, release = 0x1076b004;
}
// A table row of the ellipse's outline: the leftmost and rightmost x of the row (a row's entry is 8 bytes, the octants' are one row on).
constexpr std::uint32_t unset_left = 800, unset_right = 0, entry_size = 8;
namespace span { constexpr std::uint32_t left = 0, right = 4, octant_left = 8, octant_right = 0xc; }

// 0x10775550 (labs) inline: |x| and the sign mask.
struct Magnitude { std::uint32_t value, mask; };
constexpr Magnitude magnitude(std::uint32_t x) {
    const auto mask = std::uint32_t(std::int32_t(x) >> 31);
    return {(x ^ mask) - mask, mask};
}

// One pixel's byte address from the surface's variables, as the original forms it: row (x + y * stride) * 2 from the base.
std::uint32_t pixel_offset(Cpu& c, std::uint32_t x, std::uint32_t y) {
    const auto stride = std::uint32_t(c.i16(small(c, surface_variable::row_pixels)));
    return (x + y * stride) << 1;
}

// ---- 0x1076a140
struct Extents { std::int32_t x_min, x_max, y_min, y_max; };

// A horizontal (rows false) or vertical line: the pixels from `first` to `last` of one axis at the fixed other coordinate. Both walk a
// pointer from the first pixel (the vertical one adds the row stride, read each time) and test the dotted pattern on the pixel's index.
// r6 is the style's bit 2, which decides whether the remainder is formed at all; when it is, the loop leaves the last high word in r4 and
// the last index in r5.
void straight_line(Cpu& c, Volatile& v, std::int32_t first, std::int32_t last, std::int32_t fixed, std::uint32_t style, std::uint32_t colour, bool vertical) {
    const auto stride = std::int32_t(c.i16(small(c, surface_variable::row_pixels)));
    const std::uint32_t count = word(last) + 1 - word(first);
    // Horizontal: the fixed coordinate is the row, the first pixel's column is `first`; vertical: the other way round.
    const auto column = vertical ? fixed : first, row = vertical ? first : fixed;
    v.r4 = word(stride);
    v.r3 = word(one_sixth);
    v.r5 = word(column) << 1;
    v.r4 = word(row) * word(stride);
    v.r7 = c.u32(small(c, surface_variable::base));
    v.r9 = word(first);
    v.r6 = style & solid_line;
    v.r4 <<= 1;
    v.r8 = v.r7 + (v.r5 + v.r4);
    if (first > last) return;
    for (std::uint32_t n = 0; n < count; ++n) {
        bool draw = v.r6 != 0;
        if (!draw) {
            v.r5 = v.r9 - word(first);
            const auto sixth = remainder_by_six(v.r5);
            v.r4 = word(sixth.high);
            draw = std::int32_t(sixth.rest) < 4;
        }
        if (draw) c.put16(v.r8, colour);
        if (vertical) v.r8 += word(std::int32_t(c.i16(small(c, surface_variable::row_pixels)))) << 1;
        else v.r8 += 2;
        ++v.r9;
    }
}

// A line that is neither horizontal nor vertical, walked along its longer axis one pixel at a time: the major coordinate steps by one, the
// minor one by `step` whenever the error term is not negative. `steep` walks y (the line is longer along y).
void diagonal_line(Cpu& c, Volatile& v, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, const Extents& box, bool steep,
                   std::uint32_t style, std::uint32_t colour) {
    // The walk goes from the smaller major coordinate to the larger.
    if (steep ? y1 > y2 : x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); }
    const std::int32_t step = steep ? (x2 > x1 ? 1 : -1) : (y2 > y1 ? 1 : -1);
    const auto major_span = steep ? word(y2) - word(y1) : word(x2) - word(x1);
    const auto minor_span = magnitude(steep ? word(x2) - word(x1) : word(y2) - word(y1)).value;
    const auto double_minor = minor_span << 1;
    std::uint32_t error = double_minor - major_span, minor = word(steep ? x1 : y1);
    const auto correction = (minor_span - major_span) << 1;
    v.r3 = error;
    v.r4 = correction;
    v.r5 = minor;
    std::uint32_t drawn = 0;
    const auto inside = [&](std::int32_t x, std::int32_t y) { return x >= box.x_min && x <= box.x_max && y >= box.y_min && y <= box.y_max; };
    const auto paint = [&](std::uint32_t x, std::uint32_t y) {
        v.r6 = pixel_offset(c, x, y);
        v.r7 = c.u32(small(c, surface_variable::base));
        c.put16(v.r7 + v.r6, colour);
    };
    if (inside(x1, y1)) {
        // The original reads the stride, then the base, and multiplies row by stride before adding the column.
        v.r6 = word(std::int32_t(c.i16(small(c, surface_variable::row_pixels))));
        drawn = 1;
        v.r7 = c.u32(small(c, surface_variable::base));
        v.r6 = ((word(x1) + word(y1) * v.r6) << 1);
        c.put16(v.r7 + v.r6, colour);
    }
    std::uint32_t major = (steep ? word(y1) : word(x1)) + 1;
    const std::int32_t major_end = steep ? y2 : x2;
    const std::uint32_t count = word(major_end) + 1 - major;
    v.r6 = count;
    v.r8 = major;
    if (std::int32_t(major) > major_end) return;
    for (std::uint32_t n = 0; n < count; ++n) {
        if (std::int32_t(error) >= 0) {
            minor += word(step);
            error += correction;
        } else {
            error += double_minor;
        }
        const auto x = steep ? minor : major, y = steep ? major : minor;
        if (inside(std::int32_t(x), std::int32_t(y))) {
            bool draw = true;
            if (!(style & solid_line)) {
                v.r6 = 0;
                v.r6 = word(one_sixth);
                const auto sixth = remainder_by_six(drawn);
                v.r7 = word(sixth.high);
                v.r6 = sixth.rest;
                draw = std::int32_t(sixth.rest) < 4;
            } else {
                v.r6 = style & solid_line;
            }
            if (draw) paint(x, y);
            ++drawn;
        }
        v.r5 = minor;
        v.r3 = error;
        ++major;
        v.r8 = major;
    }
}
}

void draw_line(Cpu& c, const LineArguments& l) {
    Frame frame(c, line_entry, line_frame, {.first_gpr = 19});
    auto v = Volatile::load(c);
    if (c.u8(small(c, surface_variable::suspended)) == 0) {
        const auto lower = [](std::int32_t a, std::int32_t b) { return sign_extend_16(word(a < b ? a : b)); };
        const auto upper = [](std::int32_t a, std::int32_t b) { return sign_extend_16(word(a > b ? a : b)); };
        const Extents box{lower(l.x1, l.x2), upper(l.x1, l.x2), lower(l.y1, l.y2), upper(l.y1, l.y2)};
        if (box.y_min == box.y_max) {
            straight_line(c, v, box.x_min, box.x_max, box.y_min, l.style, l.colour, false);
        } else if (box.x_min == box.x_max) {
            straight_line(c, v, box.y_min, box.y_max, box.x_min, l.style, l.colour, true);
        } else {
            // |dy| first, then |dx|: the longer axis is the walk's (a tie walks x).
            const auto dy = magnitude(word(l.y2) - word(l.y1)).value, dx = magnitude(word(l.x2) - word(l.x1)).value;
            diagonal_line(c, v, l.x1, l.y1, l.x2, l.y2, box, std::int32_t(dx) < std::int32_t(dy), l.style, l.colour);
        }
        v.store(c);
    }
    frame.leave();
    native_drawing_boundary(c.s, line_exit);
}

void call_draw_line(Cpu& c, const LineArguments& line, std::uint32_t back_to) {
    c.set({{3, word(line.x1)}, {4, word(line.y1)}, {5, word(line.x2)}, {6, word(line.y2)}, {7, line.style}, {8, line.colour}});
    c.link(back_to);
    native_drawing_boundary(c.s, line_entry);
    draw_line(c, line);
}

// ---- 0x1076a8f0
namespace {
struct Octants {
    std::uint32_t surface;
    std::int32_t left, top, right, bottom;
    std::uint32_t colour, table;
};
// One point of the circle: kept when it lies within one pixel of the rectangle, recorded in the table's row and painted.
void plot(Cpu& c, const Octants& o, std::uint32_t y, std::uint32_t x) {
    if (std::int32_t(y) < std::int32_t(word(o.top) - 1) || std::int32_t(y) > std::int32_t(word(o.bottom) + 1) ||
        std::int32_t(x) < std::int32_t(word(o.left) - 1) || std::int32_t(x) > std::int32_t(word(o.right) + 1))
        return;
    if (o.table != 0) {
        const auto entry = o.table + ((y - word(o.top)) << 3);
        if (std::int32_t(x) < c.i32(entry + span::octant_left)) c.put32(entry + span::octant_left, x);
        if (std::int32_t(x) > c.i32(entry + span::octant_right)) c.put32(entry + span::octant_right, x);
    }
    c.put16(o.surface + pixel_offset(c, x, y), o.colour);
}
}

void circle_octants(Cpu& c, std::uint32_t surface, std::int32_t i, std::int32_t j, std::int32_t left, std::int32_t top, std::int32_t right,
                    std::int32_t bottom, std::uint32_t colour, std::uint32_t table, Leftovers& leftovers) {
    // `stmw r24,-0x20(r1)` below the stack pointer (the registers come back unchanged through `lmw`): dead stack, written as the original does.
    for (unsigned n = 24; n < 32; ++n) c.put32(c.sp() - 0x20 + (n - 24) * 4, c.r(n));
    const Octants o{surface, left, top, right, bottom, colour, table};
    // The circle's centre: the middle of the rectangle, each half rounded toward zero.
    const auto half = [](std::uint32_t span_plus_one) { return word(std::int32_t(span_plus_one + (span_plus_one >> 31)) >> 1); };
    const auto centre_x = word(left) + half(word(right) - word(left) + 1), centre_y = word(top) + half(word(bottom) - word(top) + 1);
    // The first loop walks a over [i, extsh(i + 1)) (one step unless i + 1 leaves 16 bits), the second walks b over [j, extsh(j + 1)) likewise,
    // each plotting the four mirrored points.
    const std::int32_t i_end = sign_extend_16(word(i) + 1), j_end = sign_extend_16(word(j) + 1);
    if (i < i_end) {
        const std::uint32_t count = word(i_end) - word(i);
        for (std::uint32_t n = 0; n < count; ++n) {
            const auto a = word(i) + n;
            plot(c, o, word(j) + centre_y, a + centre_x);
            plot(c, o, centre_y - word(j), a + centre_x);
            plot(c, o, centre_y - word(j), centre_x - a);
            plot(c, o, word(j) + centre_y, centre_x - a);
        }
    }
    leftovers.r4 = centre_y - word(i);
    if (j < j_end) {
        const std::uint32_t count = word(j_end) - word(j);
        for (std::uint32_t n = 0; n < count; ++n) {
            const auto b = word(j) + n;
            plot(c, o, word(i) + centre_y, b + centre_x);
            plot(c, o, centre_y - word(i), b + centre_x);
            plot(c, o, centre_y - word(i), centre_x - b);
            plot(c, o, word(i) + centre_y, centre_x - b);
        }
        leftovers.r5 = word(j_end);
    }
}

// ---- 0x1076ad50
void draw_ellipse(Cpu& c, const LineArguments& e) {
    Frame frame(c, ellipse_entry, ellipse_frame, {.first_gpr = 17});
    const auto outline_table = e.style & 4;
    const auto left_edge = word(e.x1), top_edge = word(e.y1), right_edge = word(e.x2), bottom_edge = word(e.y2);
    const auto style = e.style, colour = e.colour;
    // r17-r31 as the original holds them: r17..r21 the arguments, r22 the loop's counter, r23 the table, r24 the surface base, r25..r28 the
    // loop's (error term, radius bound, counters, span), r29 the table flag, r30 and r31. The caller's r24-r28 are what they were at entry
    // until the original loads them.
    std::uint32_t table = 0, rows = 0, inner = 0;
    auto v = Volatile::load(c);
    const auto hold = [&](std::uint32_t r17, std::uint32_t r18, std::uint32_t r19, std::uint32_t r22, std::uint32_t r24, std::uint32_t r25,
                          std::uint32_t r26, std::uint32_t r27, std::uint32_t r28, std::uint32_t r30) {
        c.set({{17, r17}, {18, r18}, {19, r19}, {20, style}, {21, colour}, {22, r22}, {23, table}, {24, r24}, {25, r25}, {26, r26}, {27, r27},
               {28, r28}, {29, outline_table}, {30, r30}, {31, top_edge}});
    };
    // (the registers r24-r28 hold the caller's values until the body loads them: kept in `entry_*` below)
    const std::uint32_t entry_24 = c.r(24), entry_25 = c.r(25), entry_26 = c.r(26), entry_27 = c.r(27), entry_28 = c.r(28);
    if (outline_table != 0) {
        v.r3 = top_edge + 1;
        inner = (bottom_edge - 1) - v.r3;
        rows = inner + 3;
        v.r3 = rows << 3;
        v.store(c);
        c.set({{17, left_edge}, {18, right_edge}, {19, bottom_edge}, {20, style}, {21, colour}, {22, inner}, {23, 0}, {24, entry_24}, {25, entry_25},
               {26, entry_26}, {27, entry_27}, {28, entry_28}, {29, outline_table}, {30, rows}, {31, top_edge}});
        c.call(allocate_block, back::allocate);
        v = Volatile::load(c);
        table = v.r3;
        if (table != 0) {
            v.r6 = 0;
            if (std::int32_t(rows) > 0) {
                v.r3 = inner - 5;
                if (std::int32_t(rows) > 8) {
                    v.r4 = 0;
                    const std::uint32_t blocks = (v.r3 + 7) >> 3;
                    if (std::int32_t(v.r3) > 0) {
                        v.r3 = unset_left;
                        for (std::uint32_t n = 0; n < blocks; ++n) {
                            v.r5 = table + v.r4;
                            v.r4 += 0x40;
                            for (std::uint32_t k = 0; k < 8; ++k) {
                                c.put32(v.r5 + k * entry_size + span::left, unset_left);
                                c.put32(v.r5 + k * entry_size + span::right, unset_right);
                            }
                            v.r6 += 8;
                        }
                    }
                }
                v.r5 = v.r6 << 3;
                v.r4 = unset_left;
                v.r3 = 0;
                if (std::int32_t(v.r6) < std::int32_t(rows)) {
                    const std::uint32_t remaining = rows - v.r6;
                    for (std::uint32_t n = 0; n < remaining; ++n) {
                        v.r6 = table + v.r5;
                        v.r5 += entry_size;
                        c.put32(v.r6 + span::left, unset_left);
                        c.put32(v.r6 + span::right, unset_right);
                    }
                }
            }
        }
    }
    const auto surface = c.u32(small(c, surface_variable::base));
    v.r3 = top_edge + 1;
    inner = (bottom_edge - 1) - v.r3;
    v.r3 = inner + 1;
    const auto half = std::int32_t(v.r3 + (v.r3 >> 31)) >> 1;
    v.r3 = word(half);
    std::uint32_t counter = 0, radius = word(half), index = 0, delta = 0;
    std::uint32_t error = word(sign_extend_16(3u - (word(half) << 1)));
    const auto drawn_this_time = [&] {
        if (outline_table != 0 || (style & solid_line) != 0) return true;
        const auto sixth = remainder_by_six(counter);
        v.r3 = word(sixth.high);
        return std::int32_t(sixth.rest) < 4;
    };
    const auto octants = [&](std::uint32_t i, std::uint32_t j) {
        // The ninth argument: the table pointer in the frame's own slot, which the callee reads from there; r17-r31 as the original holds them.
        c.put32(frame.at(octant_table_slot), table);
        hold(left_edge, right_edge, bottom_edge, counter, surface, error, radius, index, inner, delta);
        v.r3 = surface;
        v.r4 = i;
        v.r5 = j;
        v.r10 = colour;
        v.r6 = left_edge + 1;
        v.r7 = top_edge + 1;
        v.r8 = right_edge - 1;
        v.r9 = bottom_edge - 1;
        Leftovers over{v.r4, v.r5, v.r6, v.r7, v.r8, v.r9, v.r10};
        circle_octants(c, surface, std::int32_t(i), std::int32_t(j), std::int32_t(v.r6), std::int32_t(v.r7), std::int32_t(v.r8), std::int32_t(v.r9), colour, c.u32(frame.at(octant_table_slot)), over);
        v.r4 = over.r4;
        v.r5 = over.r5;
    };
    while (std::int32_t(index) < std::int32_t(radius)) {
        if (drawn_this_time()) octants(index, radius);
        if (std::int32_t(error) < 0) {
            error += word(sign_extend_16(delta + 6));
        } else {
            const auto step = index - radius;
            radius -= 1;
            v.r3 = step << 2;
            error += word(sign_extend_16((step << 2) + 10));
        }
        delta += 4;
        ++index;
        ++counter;
    }
    if (radius != 0 && drawn_this_time()) octants(radius, radius);
    if (table != 0) {
        std::uint32_t row = 0, offset = 0;
        const std::uint32_t rows_to_draw = inner + 3;
        while (std::int32_t(row) < std::int32_t(rows_to_draw)) {
            v.r3 = table + offset;
            v.r5 = c.u32(v.r3 + span::right);
            if (v.r5 != 0) {
                v.r4 = top_edge + row;
                v.r3 = c.u32(table + offset + span::left);
                v.r6 = v.r4;
                v.r8 = colour;
                v.r7 = solid_line;
                v.store(c);
                hold(rows_to_draw, offset, row, counter, surface, error, radius, index, inner, delta);
                call_draw_line(c, {std::int32_t(v.r3), std::int32_t(v.r4), std::int32_t(v.r5), std::int32_t(v.r6), solid_line, colour}, back::line);
                v = Volatile::load(c);
            }
            offset += entry_size;
            ++row;
        }
        v.r3 = table;
        v.store(c);
        hold(rows_to_draw, offset, row, counter, surface, error, radius, index, inner, delta);
        c.call(release_block, back::release);
        v = Volatile::load(c);
    }
    v.store(c);
    frame.leave();
    native_drawing_boundary(c.s, ellipse_exit);
}
}
