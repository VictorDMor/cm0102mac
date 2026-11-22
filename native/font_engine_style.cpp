// The font engine, block `style` (0x1075e0c0..0x1075e960): the emboldening the game's sfnt layer applies through two function
// pointers (the bold style of the game's fonts, selected by the style flags' bits 0 and 1): sfnt +0x40 widens a list of advance widths
// (0x1075e1a0, called from 0x107660c0 with r3 = the advance list), sfnt +0x3c offsets a glyph's outline (0x1075e320, called from
// 0x107666fc with r3 = the glyph): each contour's edges are pushed outwards by a fixed amount, the corners recomputed as the
// intersection of the offset edges and kept within a limit of the original corner, then the points that ended up on the wrong side of
// their neighbours pulled back and runs of equal original coordinates averaged. The third entry is the corner intersection the outline
// offset calls. Called through the indirect glue (0x10781320) with (record, memory object, size, pointer to the 16.16 strength).
//
// Contract. The outline routine allocates a copy of the points through tsi_AllocMem and releases it through tsi_DeAllocMem (a
// stack-contract leaver) at its own frame's sp: it builds the original's frame (stmw r14, 0x90) and holds the original's r14-r31 at
// its calls (the callees save r28-r31 and deeper frames save more: every held register is mirrored by the loop variable that lives in
// it). The callers do not read the volatile registers after the glue call on the paths the checker can name, but the return reaches
// generated code (new_sfnt, FONT.CPP) with r4-r10 unwritten, so both pointer-called routines leave r3-r10 exactly as the original on
// every path and are compared in full: `v` below is the shadow of r3-r10 (v[n] = rn), updated where the original's instructions write
// them (the unrolled copy loops included) and stored into the registers at the calls and at the return.
#include "font_engine_detail.hpp"
#include <array>

namespace cm::aot::font_engine {
namespace {
using namespace style;

// extsh: a halfword sign-extended into a register.
constexpr std::int32_t extsh(std::uint32_t v) { return std::int16_t(v); }
// A register as the original's 32-bit arithmetic leaves it.
constexpr std::uint32_t reg(std::int32_t v) { return std::uint32_t(v); }

// (short)(size / 12) as the original computes it: the high word of 0x2aaaaaab * size, shifted right once with its sign bit added.
std::int32_t twelfth_of(std::uint32_t size) {
    const auto high = std::int32_t((std::int64_t(0x2aaaaaab) * extsh(size)) >> 32);
    auto half = high >> 1;
    half += std::int32_t(std::uint32_t(half) >> 31);
    return extsh(std::uint32_t(half));
}

// r3-r10 as the original holds them.
using Volatile = std::array<std::uint32_t, 11>;
void store(Cpu& c, const Volatile& v) { for (unsigned n = 3; n <= 10; ++n) c.set(n, v[n]); }
void load(Cpu& c, Volatile& v) { for (unsigned n = 3; n <= 10; ++n) v[n] = c.r(n); }
}

// 0x1075e1a0: every advance of the list (+4 count, +0x10 the halfwords) grows by twice the strength.
void embolden_advances(Cpu& c, std::uint32_t advances, std::uint32_t size, std::uint32_t factor_pointer) {
    guest::Frame frame(c, 0x1075e1a0, 0x50, {.first_gpr = 29});
    c.set(31, advances);
    const auto step = twelfth_of(size);
    c.set(29, reg(step));
    Leftovers left;
    const auto scaled = fix_multiply(c.i32(factor_pointer), step, left);
    const auto strength = extsh(reg(scaled - step));
    const auto delta = reg(extsh(reg(2 * strength)));
    const auto count = c.i32(advances + advance_count);
    // After the util call: r3 the product, r4 and r8 its leftovers, r5 = 0, r6 the count, r7 the delta; r9 and r10 as entered.
    Volatile v{};
    v[3] = reg(scaled); v[4] = left.r4; v[5] = 0; v[6] = reg(count); v[7] = delta; v[8] = left.r8; v[9] = c.r(9); v[10] = c.r(10);
    if (count > 0) {
        // The original unrolls by eight (r8-r10 are what its last unrolled pass leaves); the elements are visited in order, the
        // pointer to the halfwords read again for each (and left in r3, r4 the byte offset past the last element).
        std::uint32_t values = 0;
        for (std::int32_t n = 0; n < count; ++n) {
            values = c.u32(advances + advance_values);
            c.put16(values + 2 * std::uint32_t(n), c.u16(values + 2 * std::uint32_t(n)) + delta);
        }
        v[3] = values;
        v[4] = 2 * reg(count);
        if (count > 8) {
            const auto passes = std::uint32_t(count - 1) >> 3;
            v[5] = 8 * passes; v[8] = values; v[9] = values; v[10] = 16 * (passes - 1) + 2;
        }
    }
    store(c, v);
}

// 0x1075e0c0: the corner of two offset edges. (x1,y1)-(x2,y2) is the first edge, the second edge ends at (x4,y4) and passes
// (x3,y3); the corner is written to *out_x and *out_y (halfwords). The edges' cross products use the wrapping 32-bit products of
// the original; parallel edges (a zero cross product) take the midpoint of (x2,y2) and (x3,y3).
void offset_corner(Cpu& c, std::uint32_t x1, std::uint32_t y1, std::uint32_t x2, std::uint32_t y2, std::uint32_t x3, std::uint32_t y3,
                   std::uint32_t x4, std::uint32_t y4, std::uint32_t out_x, std::uint32_t out_y) {
    guest::Frame frame(c, 0x1075e0c0, 0x50, {.first_gpr = 26});
    const auto p1 = extsh(x1), p2 = extsh(y1), p3 = extsh(x2), p4 = extsh(y2), p5 = extsh(x3), p6 = extsh(y3), p7 = extsh(x4), p8 = extsh(y4);
    const auto edge_x = p3 - p1, edge_y = p4 - p2, reach_x = p7 - p5, reach_y = p8 - p6, from_x = p5 - p1, from_y = p6 - p2;
    const auto parallel = reg(reach_x) * reg(edge_y) - reg(reach_y) * reg(edge_x);
    const auto along = reg(edge_x) * reg(from_y) - reg(edge_y) * reg(from_x);
    if (parallel == 0) {
        const auto sum_x = p3 + p5, sum_y = p4 + p6;
        c.put16(out_x, reg((sum_x + std::int32_t(std::uint32_t(sum_x) >> 31)) >> 1));
        c.put16(out_y, reg((sum_y + std::int32_t(std::uint32_t(sum_y) >> 31)) >> 1));
        return;
    }
    Leftovers left;
    const auto fraction = fix_divide(std::int32_t(along), std::int32_t(parallel), left);
    c.put16(out_x, reg(p5 + fix_multiply(reach_x, fraction, left)));
    c.put16(out_y, reg(p6 + fix_multiply(reach_y, fraction, left)));
}

// 0x1075e320: see the head of the file.
void embolden_outline(Cpu& c, std::uint32_t glyph, std::uint32_t mem, std::uint32_t size, std::uint32_t factor_pointer) {
    guest::Frame frame(c, 0x1075e320, 0x90, {.first_gpr = 14});
    c.set({{14, mem}, {31, glyph}});
    Volatile v{};
    load(c, v);  // r9 and r10 are entered with the caller's
    const auto step = twelfth_of(size);
    c.set({{15, reg(step)}});
    Leftovers left;
    const auto scaled = fix_multiply(c.i32(factor_pointer), step, left);
    const auto strength = extsh(reg(scaled - step));
    c.set(30, reg(strength));
    // The limit a corner may move from the original: twice the strength as a halfword, made positive (the halfword minimum stays).
    auto limit = extsh(reg(strength * 2));
    if (limit < 0) limit = extsh(reg(-limit));
    c.set(17, reg(limit));
    const auto contours = [&] { return extsh(c.u16(glyph + contour_count)); };
    v[3] = reg(contours()); v[4] = left.r4; v[5] = left.r5; v[6] = left.r6; v[7] = left.r7; v[8] = left.r8;
    if (!(contours() > 0) || strength == 0) { store(c, v); return; }

    const auto halfword = [&](std::uint32_t array, std::uint32_t at) { return extsh(c.u16(array + at)); };  // lhax
    const auto field = [&](std::uint32_t offset) { return c.u32(glyph + offset); };

    // A copy of the points: the x halfwords, then the y halfwords (4 bytes a point).
    const auto last_end = halfword(field(end_points), 2 * std::uint32_t(contours()) - 2);
    const std::uint32_t points = reg(last_end + 1);
    c.set(15, points);
    v[3] = mem; v[4] = 4 * points;
    store(c, v);
    c.link(0x1075e3b8);
    const auto copy_x = allocate_block(c, mem, 4 * points);
    const auto copy_y = copy_x + 2 * points;
    c.set({{22, copy_x}, {21, copy_y}});
    load(c, v);
    v[6] = 0;
    if (std::int32_t(points) > 0) {
        for (std::uint32_t n = 0; n < points; ++n) {
            c.put16(copy_x + 2 * n, c.u16(field(x_coordinates) + 2 * n));
            c.put16(copy_y + 2 * n, c.u16(field(y_coordinates) + 2 * n));
        }
        // What the unrolled copy leaves (r8-r10 from its last pass; the tail then rewrites r3-r7).
        std::uint32_t done = 0;
        if (points > 8) {
            const auto passes = (points - 1) >> 3;
            done = 8 * passes;
            v[8] = field(x_coordinates); v[9] = 16 * (passes - 1) + 12; v[10] = 16 * (passes - 1) + 10;
        }
        v[3] = copy_y + 2 * points; v[4] = copy_x + 2 * points; v[5] = field(y_coordinates); v[6] = done; v[7] = 2 * points;
    }

    // First pass: each corner offset along the direction its two edges turn (the sign of the y steps), the two offset edges
    // intersected, and the corner kept within `limit` of where it was.
    // Mirrored registers: r15 contour, r16 its byte offset, r18/r23 the contour's first point, r19 the byte offset of the point,
    // r20 the corner's y, r24-r27 the next and current points (x in r26/r27, y in r24/r25), r28 the point index, r29 the last.
    std::int32_t contour = 0, index = 0, last = 0;
    std::uint32_t contour_offset = 0, at = 0;
    std::int32_t first_x = 0, first_y = 0, current_x = 0, current_y = 0, next_x = 0, next_y = 0, corner_y = 0;
    bool corner_seen = false;
    std::uint32_t r15_now = 0;  // r15 holds the contour counter here, the previous point's x in the second pass
    const auto held = [&] {
        c.set({{14, mem}, {15, r15_now}, {16, contour_offset}, {17, reg(limit)}, {18, reg(first_x)}, {19, at}, {21, copy_y}, {22, copy_x}, {23, reg(first_y)},
               {24, reg(next_y)}, {25, reg(current_y)}, {26, reg(next_x)}, {27, reg(current_x)}, {28, reg(index)}, {29, reg(last)}, {30, reg(strength)}, {31, glyph}});
        if (corner_seen) c.set(20, reg(corner_y));
    };
    for (; contour < contours(); ++contour, contour_offset += 2) {
        r15_now = reg(contour);
        v[3] = field(start_points); v[4] = field(end_points);
        index = halfword(v[3], contour_offset);
        last = halfword(v[4], contour_offset);
        const auto xs = field(x_coordinates);
        at = reg(index) << 1;
        const auto ys = field(y_coordinates);
        v[3] = xs; v[4] = ys;
        first_x = halfword(xs, at);
        first_y = halfword(ys, at);
        current_x = halfword(xs, reg(last) << 1);
        next_x = first_x;
        current_y = halfword(ys, reg(last) << 1);
        next_y = first_y;
        for (; index <= last; ++index, at += 2) {
            const auto previous_x = current_x, previous_y = current_y;
            v[3] = reg(previous_x); v[4] = reg(previous_y);
            const auto following = index + 1;
            current_x = next_x;
            current_y = next_y;
            if (following <= last) {
                next_x = halfword(field(x_coordinates), reg(following) << 1);
                next_y = halfword(field(y_coordinates), reg(following) << 1);
            } else {
                next_x = first_x;
                next_y = first_y;
            }
            corner_y = extsh(reg(current_y));
            corner_seen = true;
            v[5] = reg(current_x); v[7] = reg(current_x); v[9] = reg(next_x);
            // Which way the corner's two edges run vertically decides the side its offset edges move to.
            const auto rise = extsh(reg(previous_y - corner_y)), fall = extsh(reg(corner_y - next_y));
            v[6] = reg(fall);
            std::int32_t start_x = previous_x, near_x = current_x, far_x = current_x, end_x = next_x;
            if (rise > 0) {
                start_x = extsh(reg(previous_x + strength));
                near_x = extsh(reg(current_x + strength));
                v[3] = reg(start_x); v[5] = reg(near_x);
            } else if (rise < 0) {
                start_x = extsh(reg(previous_x - strength));
                near_x = extsh(reg(current_x - strength));
                v[3] = reg(start_x); v[5] = reg(near_x);
            }
            if (fall > 0) {
                far_x = extsh(reg(current_x + strength));
                end_x = extsh(reg(next_x + strength));
                v[6] = reg(current_x + strength); v[7] = reg(far_x); v[9] = reg(end_x);
            } else if (fall < 0) {
                far_x = extsh(reg(current_x - strength));
                end_x = extsh(reg(next_x - strength));
                v[6] = reg(current_x - strength); v[7] = reg(far_x); v[9] = reg(end_x);
            }
            v[6] = reg(extsh(v[5]));
            if (extsh(reg(near_x)) == extsh(reg(far_x))) {
                c.put16(field(x_coordinates) + at, reg(near_x));
                v[3] = field(x_coordinates);
            } else {
                // The corner is the intersection of the edge ending at the point and the edge leaving it: its address words go to the
                // callee's parameter words in this frame (sp + 0x38 and + 0x3c).
                c.put32(frame.at(0x38), field(x_coordinates) + at);
                c.put32(frame.at(0x3c), field(y_coordinates) + at);
                held();
                v[6] = reg(current_y); v[8] = reg(current_y); v[10] = reg(next_y);
                store(c, v);
                c.link(0x1075e67c);
                offset_corner(c, reg(start_x), reg(previous_y), reg(near_x), reg(current_y), reg(far_x), reg(current_y), reg(end_x), reg(next_y),
                              field(x_coordinates) + at, field(y_coordinates) + at);
                const auto cx = extsh(reg(current_x));
                const auto moved_x = reg(halfword(field(x_coordinates), at)) - reg(cx);
                const auto moved_y = reg(halfword(field(y_coordinates), at)) - reg(corner_y);
                const auto abs_x = std::int32_t(moved_x ^ std::uint32_t(std::int32_t(moved_x) >> 31)) - std::int32_t(std::int32_t(moved_x) >> 31);
                const auto abs_y = std::int32_t(moved_y ^ std::uint32_t(std::int32_t(moved_y) >> 31)) - std::int32_t(std::int32_t(moved_y) >> 31);
                // The octagonal distance: the longer component plus half the shorter.
                auto distance = abs_y + (abs_x >> 1);
                v[3] = field(x_coordinates); v[4] = moved_x; v[5] = moved_y; v[6] = reg(abs_x >> 1); v[7] = reg(distance);
                v[8] = reg(abs_x); v[9] = reg(abs_y);
                if (abs_x > abs_y) { distance = abs_x + (abs_y >> 1); v[6] = reg(abs_y >> 1); v[7] = reg(distance); }
                const auto bound = extsh(reg(limit));
                v[8] = reg(bound);
                if (distance > bound) {
                    // divw: a zero divisor faults as the generated code does.
                    const auto scale_x = std::int32_t(reg(std::int32_t(moved_x)) * reg(bound)), scale_y = std::int32_t(reg(std::int32_t(moved_y)) * reg(bound));
                    v[6] = reg(scale_x); v[4] = reg(scale_y);
                    const auto quotient_x = std::int32_t(divide_signed<4>(reg(scale_x), reg(distance)));
                    v[5] = reg(quotient_x);
                    c.put16(field(x_coordinates) + at, reg(cx + quotient_x));
                    const auto quotient_y = std::int32_t(divide_signed<4>(reg(scale_y), reg(distance)));
                    v[4] = reg(quotient_y);
                    v[3] = field(y_coordinates);
                    c.put16(field(y_coordinates) + at, reg(corner_y + quotient_y));
                }
            }
        }
    }
    r15_now = reg(contour);

    // Second pass: a coordinate that moved against the direction its original neighbour differs from it goes back to the neighbour's.
    v[3] = 0; v[10] = 0;
    std::int32_t pass_contour = 0;
    std::uint32_t pass_offset = 0;
    for (; pass_contour < contours(); ++pass_contour, pass_offset += 2) {
        v[5] = field(end_points); v[4] = field(start_points);
        std::int32_t previous = halfword(v[5], pass_offset), current = halfword(v[4], pass_offset);
        v[7] = reg(previous); v[4] = reg(current);
        const auto from = reg(current) << 1;
        v[9] = from; v[6] = copy_x + from; v[5] = copy_y + from;
        if (current <= previous) {
            for (std::uint32_t n = reg(previous + 1 - current), at_point = from, k = 0; n != 0; --n, at_point += 2, ++k) {
                const auto previous_at = reg(previous) << 1;
                auto xs = field(x_coordinates);
                const auto copy_previous_x = halfword(copy_x, previous_at), copy_current_x = extsh(c.u16(copy_x + from + 2 * k));
                const auto x_before = halfword(xs, previous_at);
                r15_now = reg(x_before);
                const auto x_now = halfword(xs, at_point);
                v[8] = reg(copy_previous_x); v[7] = reg(copy_current_x - copy_previous_x);
                if (std::int32_t(reg(copy_current_x - copy_previous_x) * reg(x_now - x_before)) < 0) c.put16(xs + at_point, reg(x_before));
                auto ys = field(y_coordinates);
                const auto copy_previous_y = halfword(copy_y, previous_at), copy_current_y = extsh(c.u16(copy_y + from + 2 * k));
                const auto y_previous = halfword(ys, previous_at);
                const auto y_now = halfword(ys, at_point);
                v[8] = reg(copy_previous_y); v[7] = reg(copy_current_y - copy_previous_y);
                if (std::int32_t(reg(copy_current_y - copy_previous_y) * reg(y_now - y_previous)) < 0) c.put16(ys + at_point, reg(y_previous));
                previous = current;
                ++current;
                v[7] = reg(previous); v[9] = at_point + 2; v[5] += 2; v[6] += 2; v[4] = reg(current);
            }
        }
        v[10] += 2; v[3] += 1;
    }

    // Third pass: in a run of points whose original x is equal, the new x of all of them becomes their average. The run starts at the
    // contour's last point; a run is averaged when a point differs from its start (the run that ends with the contour is not), and the
    // scan then resumes inside the run it just averaged (its second point), as the original's loop does.
    v[9] = 0; v[6] = 0;
    std::int32_t third_contour = 0, third_index = 0;
    std::uint32_t third_offset = 0;
    for (; third_contour < contours(); ++third_contour, third_offset += 2) {
        std::int32_t run_count = 1;
        v[4] = field(end_points); v[3] = field(start_points);
        const auto last_point = halfword(v[4], third_offset);
        v[4] = field(x_coordinates);
        third_index = halfword(v[3], third_offset);
        std::int32_t run_sum = halfword(field(x_coordinates), reg(last_point) << 1), run_start = last_point;
        v[7] = reg(last_point); v[10] = reg(third_index); v[8] = reg(last_point);
        for (; third_index <= last_point; ++third_index) {
            const auto point_at = reg(third_index) << 1, start_at = reg(run_start) << 1;
            v[4] = point_at; v[5] = start_at;
            if (halfword(copy_x, point_at) == halfword(copy_x, start_at)) {
                ++run_count;
                v[3] = field(x_coordinates);
                run_sum = std::int32_t(reg(run_sum) + reg(halfword(field(x_coordinates), point_at)));
                v[10] = reg(third_index) + 1;
                continue;
            }
            if (run_count > 1) {
                run_sum = std::int32_t(divide_signed<4>(reg(run_sum), reg(run_count)));
                v[4] = field(x_coordinates);
                --third_index;
                c.put16(field(x_coordinates) + start_at, reg(run_sum));
                auto store_at = reg(third_index) << 1;
                v[4] = store_at; v[5] = field(x_coordinates);
                c.put16(field(x_coordinates) + store_at, reg(run_sum));
                for (std::int32_t more = run_count - 2; more > 0; --more) {
                    store_at -= 2;
                    --third_index;
                    v[5] = field(x_coordinates); v[4] = store_at;
                    c.put16(field(x_coordinates) + store_at, reg(run_sum));
                }
            }
            run_start = third_index;
            run_count = 1;
            v[3] = field(x_coordinates); v[8] = reg(third_index);
            run_sum = halfword(field(x_coordinates), reg(third_index) << 1);
            v[10] = reg(third_index) + 1;
        }
        // Every pass over a contour's points leaves r10 at its index past the last point (a contour that ends before it starts: its start).
        v[10] = reg(third_index);
        v[6] += 2; v[9] += 1;
    }
    held();
    v[3] = mem; v[4] = copy_x;
    store(c, v);
    c.link(0x1075e944);
    release_block(c, mem, copy_x);
}
}
