// The font engine, block `t2ksc` (T2KSC.C, 0x10761270..0x10762f40): the scan converter that turns the scaled outlines into bitmaps.
//
// The converter object (scan:: in font_engine_detail.hpp, 0x2228 bytes, the allocator's scratch slot 4) holds, for an outline in 26.6
// coordinates, two families of sorted edge lists: one list per pixel row (scan::row_heads, keyed by the x of each crossing of the row's
// centre line) and, when the bitmap is grey or dropout control is on, one list per pixel column (scan::column_heads, keyed by the y of
// each crossing of the column's centre line). A node is {key, next}: the key is the crossing coordinate (26.6) doubled, its low bit the
// edge's direction (1 when the edge runs towards increasing coordinate along its first axis); nodes come eight bytes at a time from
// chunks (the first is inside the object, the others from the allocator; a table of the chunks is kept for the release).
//
//   0x10762e30 scan converter deleted        0x107620d0 scan converter built from the contours (calls the three below)
//   0x10761730 extents of the points         0x10761ec0 contours -> lines and curves (starts and ends are halfword indices, x and y words)
//   0x10761d50 quadratic curve flattened     0x107617b0 line -> nodes of the two families
//   0x10761270 next chunk of nodes           0x107614c0 extents refined from the lists
//   0x107623c0 the bitmap: coverage (8-bit) or one bit per pixel with the dropout passes
//
// Contract. Three of them are the engine's stack leavers (0x107617b0, 0x10761d50, 0x10762e30: the game's printf reads the words their prologues
// leave): each builds its frame with guest::Frame and the callers hold the original's registers when they enter one. Every routine mirrors the
// registers it uses as the original does in `g` (r15-r31 by number; r9 and r10 too, which the callers read after the call and which the callees
// pass through untouched), sets them from `g` before each call that saves registers into the stack image, and leaves r9/r10 as the original.
// The original's r4-r8 at return are leftovers nobody reads (tools/register_contract.py --after: the callers read r3, r9 and r10 only), except
// 0x10762e30's, which are written as the original leaves them.
#include "font_engine_detail.hpp"
#include <array>

namespace cm::aot::font_engine {
namespace {
using Regs = std::array<std::uint32_t, 32>;

constexpr std::int32_t s32(std::uint32_t v) { return std::int32_t(v); }
// srawi: the arithmetic shift.
constexpr std::uint32_t sra(std::uint32_t v, unsigned n) { return std::uint32_t(s32(v) >> n); }
constexpr std::uint32_t half_signed(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }
// The winding step of a node: +1 for an edge running upward, -1 for one running downward (rlwinm r,key,1,30,30 less one).
constexpr std::uint32_t winding_step(std::uint32_t key) { return ((key << 1) & 2) - 1; }

Regs load_registers(Cpu& c, unsigned first) {
    Regs g{};
    for (unsigned n = first; n < 32; ++n) g[n] = c.r(n);
    g[9] = c.r(9);
    g[10] = c.r(10);
    return g;
}
// The registers the original holds at a call (a callee's prologue saves them into the stack image); r9 and r10 reach the callee.
void hold(Cpu& c, const Regs& g, unsigned first) {
    for (unsigned n = first; n < 32; ++n) c.set(n, g[n]);
    c.set({{9, g[9]}, {10, g[10]}});
}
// What a callee leaves in r9 and r10 (they pass through the allocator, the leaves and the reporter; 0x10761270's table copy sets them).
void settle(Cpu& c, Regs& g) {
    g[9] = c.r(9);
    g[10] = c.r(10);
}
// The pool's assertion: its text at pool + text, the file name at +0x16.
void report(Cpu& c, std::uint32_t pool, std::uint32_t text, std::uint32_t line, std::uint32_t back) {
    c.set({{3, pool + text}, {4, pool + 0x16}, {5, line}});
    c.call(assertion_report, back);
}
// The sum of the two edge crossings' winding steps of a pair: a pair runs from its first node to the one where the winding returns to zero.
// Returns the node that ends the pair; `first_key` is the first node's key, `last_key` the end node's.
struct Pair {
    std::uint32_t last;
    std::uint32_t first_key, last_key;
};
Pair pair_from(Cpu& c, std::uint32_t first, std::uint32_t second) {
    Pair pair{second, c.u32(first), c.u32(second)};
    std::uint32_t winding = winding_step(pair.first_key) + winding_step(pair.last_key);
    while (winding != 0) {
        pair.last = c.u32(pair.last + 4);
        pair.last_key = c.u32(pair.last);
        winding += winding_step(pair.last_key);
    }
    return pair;
}
// The 8-bit coverage blend of the vertical pass: the new coverage `a` and the byte `b` already there, each weighted by 0xf82 less the square of
// its distance from 0x3f; the signed division faults on a zero sum as the original does. Leaves the squares' terms for the r9/r10 mirror.
struct Blend {
    std::uint32_t value, weight_a, weight_b;
};
Blend blend(std::uint32_t a, std::uint32_t b) {
    const std::uint32_t weight_a = 0xf82u - (0x3fu - a) * (0x3fu - a), weight_b = 0xf82u - (0x3fu - b) * (0x3fu - b);
    return {std::uint32_t(divide_signed<4>(weight_a * a + weight_b * b, weight_a + weight_b)), weight_a, weight_b};
}
}

// 0x10761270: the next chunk of nodes. Before the first call (chunk capacity zero) the chunk is the one inside the object; afterwards
// the cursor must have reached the limit (an assertion otherwise), a slot in the chunk table is taken (the table grows into the allocator
// when it was the inline one, by reallocation after that) and a chunk of one 0x100th of the extents' perimeter plus 0x100 nodes is allocated.
std::uint32_t new_edge_chunk(Cpu& c, std::uint32_t sc) {
    using namespace scan;
    guest::Frame frame(c, 0x10761270, 0x40, {.first_gpr = 30});
    std::uint32_t r9 = c.r(9), r10 = c.r(10);
    c.set(30, sc);
    const auto pool = c.toc(toc::t2ksc_pool);
    const auto sync = [&] { c.set({{9, r9}, {10, r10}}); };
    const auto settle_here = [&] { r9 = c.r(9); r10 = c.r(10); };
    std::uint32_t chunk, nodes;
    if (c.u32(sc + chunk_capacity) == 0) {
        c.put32(sc + chunk_capacity, 0x20);
        c.put32(sc + chunks, sc + chunk_slots);
        c.put32(sc + chunk_count, 0);
        nodes = 0x400;
        c.set(31, nodes);
        chunk = sc + first_chunk;
        c.put32(c.u32(sc + chunks), chunk);
    } else {
        if (c.u32(sc + cursor) != c.u32(sc + limit)) {
            report(c, pool, 0, 0x26, 0x107612e0);
            settle_here();
        }
        const auto index = c.u32(sc + chunk_count) + 1;
        c.put32(sc + chunk_count, index);
        nodes = sra(c.u32(sc + point_x_max) - c.u32(sc + point_x_min) + c.u32(sc + point_y_max) - c.u32(sc + point_y_min), 6) + 0x100;
        c.set(31, nodes);
        if (!(s32(index) < s32(c.u32(sc + chunk_capacity)))) {
            const auto capacity = index << 1;
            c.put32(sc + chunk_capacity, capacity);
            const auto old_table = c.u32(sc + chunks);
            if (old_table == sc + chunk_slots) {
                sync();
                c.link(0x10761348);
                const auto fresh = allocate_block(c, c.u32(sc + mem), capacity << 2);
                settle_here();
                c.put32(sc + chunks, fresh);
                // The 32 words of the inline table, copied sixteen a pass: the last pass leaves r10 = 0x64 and r9 = 0x68.
                for (std::uint32_t n = 0; n < 0x80; n += 4) c.put32(fresh + n, c.u32(sc + chunk_slots + n));
                r10 = 0x64;
                r9 = 0x68;
            } else {
                sync();
                c.link(0x10761478);
                const auto grown = reallocate_block(c, c.u32(sc + mem), old_table, capacity << 2);
                settle_here();
                c.put32(sc + chunks, grown);
            }
        }
        sync();
        c.link(0x10761488);
        chunk = allocate_block(c, c.u32(sc + mem), nodes << 3);
        settle_here();
        c.put32(c.u32(sc + chunks) + (c.u32(sc + chunk_count) << 2), chunk);
    }
    c.put32(sc + cursor, chunk);
    c.put32(sc + limit, chunk + (nodes << 3));
    sync();
    return chunk;
}

// 0x107614c0: the extents (x_min..y_max) from the lists: the centre of the point extents grown to the first and last row and column that hold
// an edge (their pixel centres) and to the first and last crossing of every list; the row (and column) ranges shrink to the used ones.
// A leaf: its registers are named as the original's (the callers read r9 and r10, which the loops leave).
void bounds_from_edges(Cpu& c, std::uint32_t sc) {
    using namespace scan;
    std::uint32_t r4 = c.u32(sc + point_x_max), r5 = c.u32(sc + point_y_min), r0 = c.u32(sc + point_y_max), r6 = c.u32(sc + point_x_min), r7 = 0, r8, r9 = c.r(9);
    r4 = sra(r6 + r4, 1);
    std::uint32_t r10 = c.u32(sc + first_row), r11 = c.u32(sc + last_row);
    r0 = r5 + r0;
    r6 = sra(r0, 1);
    r0 = r4;
    r5 = r6;
    r8 = r10 << 2;
    // The first row with an edge, and the last.
    while (s32(r10) <= s32(r11)) {
        r7 = c.u32(c.u32(sc + row_heads) + r8);
        if (r7 != 0) break;
        r8 += 4;
        ++r10;
    }
    if (r7 != 0) {
        r8 = (r10 << 6) + 0x20;
        if (s32(r8) < s32(r5)) r5 = r8;
    }
    r8 = r11 << 2;
    while (s32(r11) >= s32(r10)) {
        r7 = c.u32(c.u32(sc + row_heads) + r8);
        if (r7 != 0) break;
        r8 -= 4;
        --r11;
    }
    if (r7 != 0) {
        r8 = (r11 << 6) + 0x20;
        if (s32(r8) > s32(r6)) r6 = r8;
    }
    // Each row's first crossing for the least x, its last for the greatest.
    r8 = r11 + 1;
    r9 = r10 << 2;
    r8 -= r10;
    if (s32(r10) <= s32(r11)) {
        for (std::uint32_t count = r8; count != 0; --count) {
            r7 = c.u32(c.u32(sc + row_heads) + r9);
            if (r7 != 0) {
                r8 = sra(c.u32(r7), 1);
                if (s32(r8) < s32(r0)) r0 = r8;
                while ((r8 = c.u32(r7 + 4)) != 0) r7 = r8;
                r8 = sra(c.u32(r7), 1);
                if (s32(r8) > s32(r4)) r4 = r8;
            }
            r9 += 4;
        }
    }
    c.put32(sc + first_row, r10);
    c.put32(sc + last_row, r11);
    if (c.u8(sc + columns) != 0) {
        r9 = c.u32(sc + first_column);
        r10 = c.u32(sc + last_column);
        r8 = r9 << 2;
        while (s32(r9) <= s32(r10)) {
            r7 = c.u32(c.u32(sc + column_heads) + r8);
            if (r7 != 0) break;
            r8 += 4;
            ++r9;
        }
        // (r7 is the last node read, or what the rows left when no column was read)
        if (r7 != 0) {
            r8 = (r9 << 6) + 0x20;
            if (s32(r8) < s32(r0)) r0 = r8;
        }
        r8 = r10 << 2;
        while (s32(r10) >= s32(r9)) {
            r7 = c.u32(c.u32(sc + column_heads) + r8);
            if (r7 != 0) break;
            r8 -= 4;
            --r10;
        }
        if (r7 != 0) {
            r7 = (r10 << 6) + 0x20;
            if (s32(r7) > s32(r4)) r4 = r7;
        }
        c.put32(sc + first_column, r9);
        r7 = r10 + 1;
        r7 -= r9;
        r8 = r9 << 2;
        c.put32(sc + last_column, r10);
        if (s32(r9) <= s32(r10)) {
            for (std::uint32_t count = r7; count != 0; --count) {
                r7 = c.u32(sc + column_heads);
                r9 = c.u32(r7 + r8);
                if (r9 != 0) {
                    r7 = sra(c.u32(r9), 1);
                    if (s32(r7) < s32(r5)) r5 = r7;
                    while ((r7 = c.u32(r9 + 4)) != 0) r9 = r7;
                    r7 = sra(c.u32(r9), 1);
                    if (s32(r7) > s32(r6)) r6 = r7;
                }
                r8 += 4;
            }
        }
    }
    c.put32(sc + x_min, r0);
    c.put32(sc + x_max, r4);
    c.put32(sc + y_min, r5);
    c.put32(sc + y_max, r6);
    c.put32(sc + bounds_known, 1);
    c.set({{9, r9}, {10, r10}});
}

// 0x10761730: the least and greatest of the points' x (first pass, from `xs`) and y (second pass, from `ys`) over indices 0..last, into
// point_x_min/max and point_y_min/max. A leaf (its registers r9 = greatest, r10 = the last point read are what the callers see).
void point_extents(Cpu& c, std::uint32_t sc, std::uint32_t xs, std::uint32_t ys, std::int32_t last) {
    using namespace scan;
    std::uint32_t r4 = xs, r0, r9 = c.r(9), r10 = c.r(10);
    for (unsigned pass = 0; pass < 2; ++pass) {
        r9 = c.u32(r4);
        r0 = r9;
        if (last >= 1) {
            std::uint32_t at = r4 + 4;
            for (std::uint32_t count = std::uint32_t(last); count != 0; --count, at += 4) {
                r10 = c.u32(at);
                if (s32(r10) > s32(r9)) r9 = r10;
                else if (s32(r10) < s32(r0)) r0 = r10;
            }
        }
        if (pass == 0) {
            c.put32(sc + point_x_min, r0);
            r4 = ys;
            c.put32(sc + point_x_max, r9);
        } else {
            c.put32(sc + point_y_min, r0);
            c.put32(sc + point_y_max, r9);
        }
    }
    c.set({{9, r9}, {10, r10}});
}

// 0x107617b0: the line from (x1, y1) to (x2, y2) (26.6) into the lists. A stack leaver: the original's frame (stmw r15..., 0x80) and its
// r15-r31 are what the caller holds, mirrored in `g` as the original's registers (the names say what they hold in each pass).
// The column pass (only with scan::columns): the line crosses the vertical centre lines (x = 32 + 64 n) between its ends; each crossing's y
// is interpolated (by the fixed-point helpers when the extents are large, else by a plain multiply and divide) and a node
// {2 * y | direction}, y the interpolated crossing (the end points are odd, the interpolated value is not) is linked into the column's list in order. The row pass does the same over y, keyed by x. The endpoints are
// made odd (| 1) so that a crossing never lands exactly on a pixel centre.
void add_line(Cpu& c, std::uint32_t sc_, std::uint32_t x1, std::uint32_t y1, std::uint32_t x2, std::uint32_t y2) {
    using namespace scan;
    guest::Frame frame(c, 0x107617b0, 0x80, {.first_gpr = 15});
    Regs g = load_registers(c, 15);
    std::uint32_t& sc = g[26];
    sc = sc_;
    g[27] = x1;
    g[28] = y1;
    g[29] = x2;
    g[30] = y2;
    Leftovers leftovers;
    const auto hold_all = [&] { hold(c, g, 15); };
    // One node into a list: the next free node of the chunk (a new chunk when the cursor has reached the limit), its key written, then it is linked
    // after the nodes whose key is not greater. `head_register` is the register that holds the list's first node and walks along it.
    const auto insert_edge = [&](std::uint32_t key, unsigned head_register, std::uint32_t slot, std::uint32_t back) {
        std::uint32_t node = c.u32(sc + cursor);
        if (!(node < c.u32(sc + limit))) {
            hold_all();
            c.link(back);
            new_edge_chunk(c, sc);
            settle(c, g);
            node = c.u32(sc + cursor);
        }
        c.put32(sc + cursor, node + 8);
        c.put32(node, key);
        std::uint32_t& head = g[head_register];
        if (head == 0 || s32(c.u32(head)) > s32(key)) {
            c.put32(node + 4, head);
            c.put32(slot, node);
        } else {
            for (;;) {
                const auto next = c.u32(head + 4);
                if (next == 0 || s32(c.u32(next)) > s32(key)) {
                    c.put32(node + 4, next);
                    c.put32(head + 4, node);
                    break;
                }
                head = next;
            }
        }
    };
    constexpr std::uint32_t large_extent = 0xb504;  // 46340: the products below stay within 31 bits under it
    // Whether the interpolation needs the fixed-point helpers: large extents, and an offset or span at or past the limit.
    const auto fixed_path = [&](std::uint32_t span, std::uint32_t offset) {
        return c.u32(sc + large) != 0 && (s32(span) >= s32(large_extent) || s32(offset) >= s32(large_extent) || s32(offset) <= -s32(large_extent));
    };

    // ---- the column lists (r24 lower x, r23 upper x, r22 y at the lower end, r21 the next centre, r20 the x span, r18 the y span) ----
    if (c.u8(sc + columns) != 0) {
        const std::uint32_t a = g[27] | 1, b = g[29] | 1;
        std::uint32_t upper_y, direction;
        if (s32(a) < s32(b)) {
            g[24] = a; g[23] = b; g[22] = g[28] | 1; upper_y = g[30] | 1; direction = 1;
        } else {
            g[24] = b; g[23] = a; g[22] = g[30] | 1; upper_y = g[28] | 1; direction = 0;
        }
        g[21] = ((g[24] + 0x1f) & 0xffffffc0u) + 0x20;
        if (s32(g[21]) <= s32(g[23])) {
            g[20] = g[23] - g[24];
            g[31] = c.u32(sc + column_heads) + (sra(g[21], 6) << 2);
            if (g[20] == 0) {
                g[15] = (g[22] + g[22]) | (direction & 0xff);
                g[16] = c.u32(g[31]);
                insert_edge(g[15], 16, g[31], 0x10761a24);
            } else {
                g[18] = upper_y - g[22];
                if (fixed_path(g[20], g[18])) {
                    g[25] = direction & 0xff;
                    do {
                        const auto fraction = fix_divide(s32(g[21] - g[24]), s32(g[20]), leftovers);
                        const auto along = fix_multiply(s32(g[18]), fraction, leftovers);
                        g[19] = ((g[22] + std::uint32_t(along)) * 2) | g[25];
                        g[16] = c.u32(g[31]);
                        g[17] = g[31];
                        g[31] += 4;
                        insert_edge(g[19], 16, g[17], 0x107618cc);
                        g[21] += 0x40;
                    } while (s32(g[21]) <= s32(g[23]));
                } else {
                    const auto rise = g[21] - g[24];
                    g[15] = g[18] << 6;
                    g[19] = g[18] * rise;
                    g[25] = direction & 0xff;
                    do {
                        const auto step = std::uint32_t(divide_signed<4>(g[19], g[20]));
                        g[16] = g[31];
                        g[17] = c.u32(g[31]);
                        g[31] += 4;
                        g[24] = ((g[22] + step) * 2) | g[25];
                        insert_edge(g[24], 17, g[16], 0x10761984);
                        g[21] += 0x40;
                        g[19] += g[15];
                    } while (s32(g[21]) <= s32(g[23]));
                }
            }
        }
    }

    // ---- the row lists (r28 lower y, r30 upper y, r25 x at the lower end, r27 the next centre, r29 the y span, r16 the x span) ----
    {
        const std::uint32_t a = g[28] | 1;
        g[30] |= 1;
        std::uint32_t upper_x, direction;
        if (s32(a) < s32(g[30])) {
            g[28] = a; g[25] = g[27] | 1; upper_x = g[29] | 1; direction = 1;
        } else {
            g[28] = g[30]; g[30] = a; g[25] = g[29] | 1; upper_x = g[27] | 1; direction = 0;
        }
        g[27] = ((g[28] + 0x1f) & 0xffffffc0u) + 0x20;
        if (s32(g[27]) <= s32(g[30])) {
            g[29] = g[30] - g[28];
            g[31] = c.u32(sc + row_heads) + (sra(g[27], 6) << 2);
            if (g[29] == 0) {
                g[15] = (g[25] + g[25]) | (direction & 0xff);
                g[16] = c.u32(g[31]);
                insert_edge(g[15], 16, g[31], 0x10761ccc);
            } else {
                g[16] = upper_x - g[25];
                if (fixed_path(g[29], g[16])) {
                    g[17] = direction & 0xff;
                    do {
                        const auto fraction = fix_divide(s32(g[27] - g[28]), s32(g[29]), leftovers);
                        const auto along = fix_multiply(s32(g[16]), fraction, leftovers);
                        g[15] = ((g[25] + std::uint32_t(along)) * 2) | g[17];
                        g[19] = c.u32(g[31]);
                        g[18] = g[31];
                        g[31] += 4;
                        insert_edge(g[15], 19, g[18], 0x10761b70);
                        g[27] += 0x40;
                    } while (s32(g[27]) <= s32(g[30]));
                } else {
                    const auto rise = g[27] - g[28];
                    g[18] = g[16] << 6;
                    g[16] = g[16] * rise;
                    g[19] = direction & 0xff;
                    do {
                        const auto step = std::uint32_t(divide_signed<4>(g[16], g[29]));
                        g[20] = g[31];
                        g[17] = c.u32(g[31]);
                        g[31] += 4;
                        g[15] = ((g[25] + step) * 2) | g[19];
                        insert_edge(g[15], 17, g[20], 0x10761c2c);
                        g[27] += 0x40;
                        g[16] += g[18];
                    } while (s32(g[27]) <= s32(g[30]));
                }
            }
        }
    }
    c.set({{9, g[9]}, {10, g[10]}});
}

// 0x10761d50: the quadratic curve P0 (x0, y0), control P1, end P2 (26.6) as lines. The depth is the number of times the curve must be halved
// for its deviation (the second difference over four, the larger component) to come under scan::depth_limit, shifting two bits a level. Each
// halving keeps the left half in the registers (end point = the curve's midpoint, control = midpoints of the sides) and pushes the right half on
// the frame's own stack (seven words at sp + 0x38: start, control, end, depth); a left half of depth zero is drawn with 0x107617b0 and the next
// pushed half taken. A stack leaver (frame 0x210, r29-r31): r29 holds the last midpoint's y, r30 the converter, r31 the stack pointer of the halves.
void add_curve(Cpu& c, std::uint32_t sc, std::uint32_t x0, std::uint32_t y0, std::uint32_t x1, std::uint32_t y1, std::uint32_t x2, std::uint32_t y2) {
    using namespace scan;
    guest::Frame frame(c, 0x10761d50, 0x210, {.first_gpr = 29});
    std::uint32_t r9 = y2, r10 = c.r(10), r29 = c.r(29);
    c.set(30, sc);
    const std::uint32_t base = frame.at(0x38);
    std::uint32_t r31 = base;
    std::uint32_t depth, r4 = x0, r5 = y0, r6 = x1, r7 = y1, r8 = x2;
    {
        const auto dx = sra(r8 + (r4 - r6 - r6), 2), dy = sra(r9 + (r5 - r7 - r7), 2);
        const auto abs_x = (sra(dx, 31) ^ dx) - sra(dx, 31), abs_y = (sra(dy, 31) ^ dy) - sra(dy, 31);
        r10 = s32(abs_y) > s32(abs_x) ? abs_y : abs_x;
        const auto limit_depth = c.u32(sc + depth_limit);
        depth = 0;
        while (s32(r10) > s32(limit_depth)) {
            r10 = sra(r10, 2);
            ++depth;
        }
    }
    for (;;) {
        while (s32(depth) > 0) {
            const std::uint32_t mid_x = sra(r4 + r6 + r8 + r6 + 2, 2), mid_y = sra(r5 + r7 + r9 + r7 + 2, 2);
            c.put32(r31, mid_x);
            r29 = mid_y;
            c.put32(r31 + 4, r29);
            const std::uint32_t right_x = sra(r6 + (r8 + 1), 1);
            c.put32(r31 + 8, right_x);
            r10 = sra(r7 + (r9 + 1), 1);
            c.put32(r31 + 0xc, r10);
            --depth;
            const std::uint32_t left_x = sra(r4 + (r6 + 1), 1), left_y = sra(r5 + (r7 + 1), 1);
            c.put32(r31 + 0x10, r8);
            c.put32(r31 + 0x14, r9);
            c.put32(r31 + 0x18, depth);
            r6 = left_x;
            r7 = left_y;
            r8 = mid_x;
            r9 = r29;
            r31 += 0x1c;
        }
        c.set({{29, r29}, {31, r31}, {9, r9}, {10, r10}});
        c.link(0x10761e70);
        add_line(c, sc, r4, r5, r8, r9);
        r9 = c.r(9);
        r10 = c.r(10);
        if (!(r31 > base)) break;
        depth = c.u32(r31 - 4);
        r9 = c.u32(r31 - 8);
        r8 = c.u32(r31 - 0xc);
        r7 = c.u32(r31 - 0x10);
        r6 = c.u32(r31 - 0x14);
        r5 = c.u32(r31 - 0x18);
        r31 -= 0x1c;
        r4 = c.u32(r31);
    }
    c.set({{9, r9}, {10, r10}});
}

// 0x10761ec0: every contour of the outline as lines and curves. starts/ends are halfword indices into the point arrays (x and y words, on-curve
// bytes). A contour is walked from its start point: on-curve points are joined by lines (0x107617b0), an off-curve point is the control of a
// curve to the next on-curve point or, between two off-curve points, to their midpoint (0x10761d50); a contour that starts off the curve begins
// at its last point (or at the midpoint of the last and first when that one is off the curve too). The walk is the original's: the registers
// are mirrored in `g` (r16 the last midpoint's y, r17 twice the contour number, r18 the converter, r19 the last midpoint's x, r20-r22 the on-curve
// bytes, the y and x words, r23/r25 the next point's y/x, r24/r26 the control's y/x, r27 the next index, r28 the current index or -1 for a start on
// the curve, r29 the contour number, r30/r31 the contour's last and first index).
void add_contours(Cpu& c, std::uint32_t sc) {
    using namespace scan;
    guest::Frame frame(c, 0x10761ec0, 0x80, {.first_gpr = 16});
    Regs g = load_registers(c, 16);
    g[18] = sc;
    g[29] = 0;
    g[17] = 0;
    g[22] = c.u32(sc + xs);
    g[21] = c.u32(sc + ys);
    g[20] = c.u32(sc + on_curve);
    const auto on = [&](std::uint32_t index) { return c.u8(g[20] + index) != 0; };
    const auto x_of = [&](std::uint32_t index) { return c.u32(g[22] + (index << 2)); };
    const auto y_of = [&](std::uint32_t index) { return c.u32(g[21] + (index << 2)); };
    const auto curve = [&](std::uint32_t x0, std::uint32_t y0, std::uint32_t cx, std::uint32_t cy, std::uint32_t ex, std::uint32_t ey, std::uint32_t back) {
        g[9] = ey;
        hold(c, g, 16);
        c.set({{4, x0}, {5, y0}});
        c.link(back);
        add_curve(c, g[18], x0, y0, cx, cy, ex, ey);
        settle(c, g);
    };
    const auto line = [&](std::uint32_t x0, std::uint32_t y0, std::uint32_t ex, std::uint32_t ey) {
        hold(c, g, 16);
        c.link(0x10762078);
        add_line(c, g[18], x0, y0, ex, ey);
        settle(c, g);
    };
    while (s32(g[29]) < s32(half_signed(c.u16(sc + contour_count)))) {
        g[31] = half_signed(c.u16(c.u32(sc + contour_starts) + g[17]));
        g[30] = half_signed(c.u16(c.u32(sc + contour_ends) + g[17]));
        std::uint32_t index = g[31], x, y;
        if (on(g[31])) {
            g[28] = 0xffffffffu;
            x = x_of(g[31]);
            y = y_of(g[31]);
            g[24] = 0;
            g[26] = 0;
        } else {
            g[26] = x_of(g[31]);
            g[24] = y_of(g[31]);
            x = x_of(g[30]);
            y = y_of(g[30]);
            if (!on(g[30])) {
                x = sra(x + (g[26] + 1), 1);
                y = sra(y + (g[24] + 1), 1);
            }
            g[28] = g[31];
            index = g[30];
        }
        // The two states of the walk: at a control point (the original's 0x10761f70) or after an on-curve point (0x1076203c).
        bool control_state = s32(g[28]) >= 0;
        for (;;) {
            if (control_state) {
                g[27] = g[28] + 1;
                if (s32(g[27]) > s32(g[30])) g[27] = g[31];
                g[25] = x_of(g[27]);
                g[23] = y_of(g[27]);
                if (on(g[27])) {
                    curve(x, y, g[26], g[24], g[25], g[23], 0x10762010);
                    if (g[27] == g[31]) break;
                    index = g[27];
                    x = g[25];
                    y = g[23];
                    control_state = false;
                } else {
                    g[19] = sra(g[26] + (g[25] + 1), 1);
                    g[16] = sra(g[24] + (g[23] + 1), 1);
                    curve(x, y, g[26], g[24], g[19], g[16], 0x10761fc8);
                    if (g[27] == g[31]) break;
                    index = g[28];
                    x = g[19];
                    y = g[16];
                    g[26] = g[25];
                    g[24] = g[23];
                    g[28] = g[27];
                    control_state = s32(g[28]) >= 0;
                }
            } else {
                g[28] = index + 1;
                if (s32(g[28]) > s32(g[30])) g[28] = g[31];
                g[26] = x_of(g[28]);
                g[24] = y_of(g[28]);
                if (on(g[28])) {
                    line(x, y, g[26], g[24]);
                    if (g[28] == g[31]) break;
                    index = g[28];
                    x = g[26];
                    y = g[24];
                } else {
                    if (g[28] == g[31]) break;
                    control_state = s32(g[28]) >= 0;
                }
            }
        }
        ++g[29];
        g[17] += 2;
    }
    c.set({{9, g[9]}, {10, g[10]}});
}

// 0x107620d0: the converter for an outline (contour count, start and end index halfword arrays, x and y words, on-curve bytes), the allocator's
// scratch slot 4. Arguments after r10 are on the caller's stack (the bytes at sp + 0x3b, 0x3f, 0x43 and the word at +0x44): `grey` (r10) says the
// bitmap is 8-bit, `rows` and `columns_pass` the two dropout controls, `skip_contours` (3) leaves the contours unwalked, `depth` the curve limit
// (at least 4, at most 0x10). The row and column list tables come inline (32 slots) or from the allocator, zeroed.
std::uint32_t new_scan_converter(Cpu& c, std::uint32_t mem, std::uint32_t count, std::uint32_t starts, std::uint32_t ends, std::uint32_t xs_,
                                 std::uint32_t ys_, std::uint32_t on_curve_, std::uint32_t grey_) {
    using namespace scan;
    guest::Frame frame(c, 0x107620d0, 0x70, {.first_gpr = 19});
    Regs g = load_registers(c, 19);
    const std::uint32_t caller_sp = frame.caller();
    g[20] = count; g[21] = starts; g[31] = mem; g[22] = ends; g[23] = xs_; g[24] = ys_; g[29] = on_curve_; g[28] = grey_;
    g[25] = c.u8(caller_sp + 0x3b);
    g[27] = c.u8(caller_sp + 0x3f);
    g[26] = c.u8(caller_sp + 0x43);
    g[19] = c.u32(caller_sp + 0x44);
    hold(c, g, 19);
    c.link(0x1076211c);
    const auto sc = scratch_allocate(c, mem, 0x2228, 4);
    settle(c, g);
    g[30] = sc;
    c.put32(sc + scan::mem, g[31]);
    c.put8(sc + grey, g[28]);
    c.put32(sc + depth_limit, 4);
    if (s32(g[19]) > s32(c.u32(sc + depth_limit))) c.put32(sc + depth_limit, g[19]);
    if (s32(c.u32(sc + depth_limit)) > 0x10) c.put32(sc + depth_limit, 0x10);
    c.put8(sc + dropout_rows, g[27]);
    c.put8(sc + dropout_columns, g[26]);
    c.put8(sc + columns, (g[26] & 0xff) != 0 || (g[28] & 0xff) != 0 ? 1 : 0);
    c.put32(sc + found_x, 0);
    c.put32(sc + found_y, 0);
    c.put16(sc + contour_count, g[20]);
    c.put32(sc + contour_starts, g[21]);
    c.put32(sc + contour_ends, g[22]);
    c.put32(sc + xs, g[23]);
    c.put32(sc + ys, g[24]);
    c.put32(sc + on_curve, g[29]);
    c.put32(sc + bounds_known, 0);
    const auto last_end = half_signed(c.u16(g[22] + ((half_signed(g[20]) << 1) - 2)));
    point_extents(c, sc, g[23], g[24], std::int32_t(last_end));
    settle(c, g);
    c.put32(sc + large, 0);
    const std::uint32_t span_limit = 0xb504;
    if (s32(c.u32(sc + point_x_max) - c.u32(sc + point_x_min)) >= s32(span_limit) || !(s32(c.u32(sc + point_y_max) - c.u32(sc + point_y_min)) < s32(span_limit)))
        c.put32(sc + large, 1);
    // The two tables: the columns by x (from the first pass) then the rows by y; a pass allocates 4 * (hi - lo + 1) bytes unless it fits the
    // 32 inline slots, zeroes them, and stores the table (and the pointer shifted so that the first index is at the first slot).
    g[28] = c.u32(sc + point_x_max);
    g[26] = 0;
    g[29] = c.u32(sc + point_x_min);
    do {
        g[28] += 0x40;
        g[29] -= 0x40;
        g[28] = sra(g[28], 6);
        g[29] = sra(g[29], 6);
        g[27] = g[28] - g[29] + 1;
        std::uint32_t list;
        if (g[26] == 0 && c.u8(sc + columns) == 0) {
            list = 0;
        } else {
            if (s32(g[27]) <= 0x20) {
                list = g[26] == 0 ? sc + column_slots : sc + row_slots;
            } else {
                hold(c, g, 19);
                c.link(0x1076226c);
                list = allocate_block(c, g[31], g[27] << 2);
                settle(c, g);
            }
            g[21] = 0;
            if (s32(g[27]) > 0) {
                if (s32(g[27]) > 8) {
                    // Eight words a pass; the registers that hold the zeros (r4-r11, so r9 and r10) are zero afterwards.
                    g[20] = 0;
                    g[9] = 0;
                    g[10] = 0;
                    for (std::uint32_t passes = (g[27] - 8 + 7) >> 3; passes != 0; --passes) {
                        for (std::uint32_t n = 0; n < 0x20; n += 4) c.put32(list + g[20] + n, 0);
                        g[20] += 0x20;
                        g[21] += 8;
                    }
                }
                for (; s32(g[21]) < s32(g[27]); ++g[21]) c.put32(list + (g[21] << 2), 0);
            }
        }
        if (g[26] == 0) {
            c.put32(sc + column_buffer, list);
            c.put32(sc + column_heads, list + (negate(g[29]) << 2));
            c.put32(sc + first_column, g[29]);
            c.put32(sc + last_column, g[28]);
            g[29] = c.u32(sc + point_y_min);
            g[28] = c.u32(sc + point_y_max);
        } else {
            c.put32(sc + row_buffer, list);
            c.put32(sc + row_heads, list + (negate(g[29]) << 2));
            c.put32(sc + first_row, g[29]);
            c.put32(sc + last_row, g[28]);
        }
        ++g[26];
    } while (s32(g[26]) < 2);
    c.put32(sc + chunk_capacity, 0);
    hold(c, g, 19);
    c.link(0x10762384);
    new_edge_chunk(c, sc);
    settle(c, g);
    c.put32(sc + bitmap, 0);
    if (std::int32_t(std::int8_t(g[25])) != 3) {
        hold(c, g, 19);
        c.link(0x107623a0);
        add_contours(c, sc);
        settle(c, g);
    }
    c.set({{3, sc}, {9, g[9]}, {10, g[10]}});
    return sc;
}

// 0x107623c0: the bitmap of the converter. The extents come from the lists (0x107614c0, once); the pixel box and the row size (one byte a pixel
// for the grey bitmap, else a bit) from them; with `skip` (the byte in r5) nothing more is done. The bitmap is the allocator's scratch slot 5, cleared;
// rows run from the bottom. Grey: each row's pairs of crossings (a pair runs while the windings sum to zero) add coverage to the pixels they span
// (0x7e a full pixel, partial ends 2 * the fraction), then each column's pairs are blended in along y (a weighted mean of the new coverage and what
// is there). One bit a pixel: the pixels whose centres a pair spans are set, and the dropout passes set one pixel for each pair thinner than a pixel that
// missed every centre (rows first, then columns), which they note in scan::found_x and found_y.
// The registers are mirrored in `g` as the original's (r18 list head, r19 row pointer, r20 row, r21 the pair's second node, r22 the height less one or the
// list offset, r23 the pool, r24 the bitmap, r25 the converter, r26/r27 the centre y/x or the offset/column, r28 the row size, r29 the height, r30/r31 the
// first row and column).
void render_bitmap(Cpu& c, std::uint32_t sc_, std::uint32_t skip_) {
    using namespace scan;
    guest::Frame frame(c, 0x107623c0, 0x70, {.first_gpr = 18});
    Regs g = load_registers(c, 18);
    g[23] = c.toc(toc::t2ksc_pool);
    g[25] = sc_;
    const std::uint32_t sc = sc_;
    g[19] = skip_;
    const auto hold_all = [&] { hold(c, g, 18); };
    g[20] = c.u8(sc + grey);
    if (c.u32(sc + bounds_known) == 0) {
        hold_all();
        bounds_from_edges(c, sc);
        settle(c, g);
    }
    {
        const auto x_max_ = c.u32(sc + x_max);
        g[31] = c.u32(sc + x_min);
        const auto y_max_ = c.u32(sc + y_max);
        std::uint32_t right_edge = x_max_ + 0x40;
        g[30] = c.u32(sc + y_min);
        g[27] = sra(g[31] + x_max_, 1);
        const std::uint32_t top_edge = y_max_ + 0x40;
        g[26] = sra(g[30] + y_max_, 1);
        c.put32(sc + left_raw, g[31]);
        g[31] = sra(g[31], 6);
        right_edge = sra(right_edge, 6);
        c.put32(sc + left, g[31]);
        g[30] = sra(g[30], 6);
        c.put32(sc + right, right_edge);
        g[22] = sra(top_edge, 6);
        const std::uint32_t width = right_edge - g[31];
        c.put32(sc + top_raw, top_edge);
        g[29] = g[22] - g[30];
        c.put32(sc + top, g[30]);
        c.put32(sc + bottom, g[22]);
        // One byte a pixel when grey, else eight pixels a byte (the signed quotient by eight, rounded towards zero).
        g[28] = g[20] != 0 ? width : std::uint32_t(s32(width + 7) / 8);
    }
    c.put32(sc + row_bytes, g[28]);
    c.put32(sc + bitmap, 0);
    if (std::int8_t(g[19]) == 0) {
        g[19] = g[28] * g[29];
        hold_all();
        c.link(0x1076248c);
        g[24] = scratch_allocate(c, c.u32(sc + scan::mem), g[19], 5);
        settle(c, g);
        c.put32(sc + bitmap, g[24]);
        // The bitmap cleared: words eight at a pass, then single words, then bytes. The registers the loops leave: r18 = 0x20 a pass, r21 the last
        // pointer or the count less eight, and r9 and r10 zero when an unrolled pass ran.
        const std::uint32_t words = sra(g[19], 2);
        std::uint32_t done = 0;
        if (s32(words) > 0) {
            g[21] = words - 8;
            if (s32(words) > 8) {
                g[18] = 0;
                g[9] = 0;
                g[10] = 0;
                for (std::uint32_t passes = (g[21] + 7) >> 3; passes != 0; --passes) {
                    g[21] = g[24] + g[18];
                    for (std::uint32_t n = 0; n < 0x20; n += 4) c.put32(g[21] + n, 0);
                    g[18] += 0x20;
                    done += 8;
                }
            }
            for (; s32(done) < s32(words); ++done) c.put32(g[24] + (done << 2), 0);
        }
        // (The original also has an unrolled byte loop here: it needs more than eight bytes left after the words, which cannot happen.)
        for (std::uint32_t at = done << 2; s32(at) < s32(g[19]); ++at) c.put8(g[24] + at, 0);
        if (!(s32(c.u32(sc + first_row)) > s32(c.u32(sc + last_row))) && !(s32(c.u32(sc + last_row)) < s32(g[22]))) {
            hold_all();
            report(c, g[23], 0x1e, 0x3e6, 0x107625f0);
            settle(c, g);
            c.put32(sc + last_row, g[22] - 1);
        }
        if (g[20] != 0) {
            // ---- grey: coverage along each row ----
            g[20] = c.u32(sc + first_row);
            g[22] = g[29] - 1;
            g[26] = g[20] << 2;
            g[19] = g[24] + g[28] * (g[22] - (g[20] - g[30]));
            g[19] -= g[31];
            while (s32(g[20]) <= s32(c.u32(sc + last_row))) {
                g[18] = c.u32(c.u32(sc + row_heads) + g[26]);
                if (g[18] != 0) {
                    g[21] = c.u32(g[18] + 4);
                    if (g[21] == 0) {
                        hold_all();
                        report(c, g[23], 0x24, 0x3f8, 0x10762650);
                        settle(c, g);
                    }
                    do {
                        const auto pair = pair_from(c, g[18], g[21]);
                        g[21] = pair.last;
                        const std::uint32_t first = sra(pair.first_key, 1), last = sra(pair.last_key, 1);
                        const std::uint32_t first_pixel = sra(pair.first_key, 7), last_pixel = sra(pair.last_key, 7);
                        if (first_pixel == last_pixel) {
                            const auto coverage = ((last & 0x3f) - (first & 0x3f)) << 1;
                            c.put8(g[19] + first_pixel, c.u8(g[19] + first_pixel) + coverage);
                        } else {
                            c.put8(g[19] + first_pixel, c.u8(g[19] + first_pixel) + ((0x3fu - (first & 0x3f)) << 1));
                            for (std::uint32_t pixel = first_pixel + 1; s32(pixel) < s32(last_pixel); ++pixel) c.put8(g[19] + pixel, 0x7e);
                            c.put8(g[19] + last_pixel, (last << 1) & 0x7e);
                        }
                        g[18] = c.u32(g[21] + 4);
                        if (g[18] == 0) break;
                        g[21] = c.u32(g[18] + 4);
                    } while (g[21] != 0);
                }
                g[19] -= g[28];
                g[26] += 4;
                ++g[20];
            }
            // ---- grey: each column's pairs along y, blended into the bytes ----
            g[20] = c.u32(sc + first_column);
            g[26] = g[20] << 2;
            while (s32(g[20]) <= s32(c.u32(sc + last_column))) {
                g[19] = 0;
                g[21] = 0;
                g[18] = c.u32(c.u32(sc + column_heads) + g[26]);
                if (g[18] != 0) {
                    g[27] = c.u32(g[18] + 4);
                    if (g[27] == 0) {
                        hold_all();
                        report(c, g[23], 0x24, 0x427, 0x107627f0);
                        settle(c, g);
                    }
                    do {
                        const auto pair = pair_from(c, g[18], g[27]);
                        g[27] = pair.last;
                        const std::uint32_t y1 = sra(pair.first_key, 1), y2 = sra(pair.last_key, 1), row1 = sra(pair.first_key, 7);
                        const std::uint32_t x_offset = g[20] - g[31];
                        const std::uint32_t at = x_offset + g[28] * (g[22] - (row1 - g[30]));
                        if (g[21] != 0 && g[21] != g[24] + at) {
                            // The pending pixel is blended into the byte it belongs to.
                            const std::uint32_t pending = g[19] & 0xff, old = c.u8(g[21]);
                            const auto mixed = blend(pending, old);
                            g[10] = mixed.weight_a * pending;
                            g[9] = g[10] + mixed.weight_b * old;
                            g[18] = mixed.weight_b;
                            g[19] = 0;
                            c.put8(g[21], mixed.value);
                        }
                        const std::uint32_t row2 = sra(pair.last_key, 7);
                        if (row1 == row2) {
                            g[19] = (g[19] & 0xff) + (((y2 & 0x3f) - (y1 & 0x3f)) << 1);
                            g[19] &= 0xff;
                            g[21] = g[24] + at;
                        } else {
                            const std::uint32_t total = ((g[19] & 0xff) + ((0x3fu - (y1 & 0x3f)) << 1)) & 0xff;
                            g[19] = (y2 << 1) & 0x7e;
                            const std::uint32_t old = c.u8(g[24] + at);
                            const auto mixed = blend(total, old);
                            g[9] = mixed.weight_b;
                            c.put8(g[24] + at, mixed.value);
                            g[21] = g[24] + x_offset + g[28] * (g[22] - (row2 - g[30]));
                        }
                        g[18] = c.u32(g[27] + 4);
                        if (g[18] == 0) break;
                        g[27] = c.u32(g[18] + 4);
                    } while (g[27] != 0);
                    if (g[21] != 0) {
                        const std::uint32_t pending = g[19] & 0xff, old = c.u8(g[21]);
                        c.put8(g[21], blend(pending, old).value);
                    }
                }
                g[26] += 4;
                ++g[20];
            }
        } else {
            // ---- one bit a pixel: the pixels whose centres each pair spans, rows from the last ----
            g[20] = c.u32(sc + last_row);
            g[22] = g[20] << 2;
            g[19] = g[24] + g[28] * ((g[29] - 1) - (g[20] - g[30]));
            for (;;) {
                g[21] = c.u32(sc + first_row);
                if (!(s32(g[20]) >= s32(g[21]))) break;
                g[18] = c.u32(c.u32(sc + row_heads) + g[22]);
                if (g[18] != 0) {
                    g[21] = c.u32(g[18] + 4);
                    if (g[21] == 0) {
                        hold_all();
                        report(c, g[23], 0x24, 0x49d, 0x107629f4);
                        settle(c, g);
                    }
                    do {
                        const auto pair = pair_from(c, g[18], g[21]);
                        g[21] = pair.last;
                        const std::uint32_t from = sra(pair.first_key + 0x40, 7), to = sra(pair.last_key + 0x40, 7);
                        if (s32(from) < s32(to)) {
                            const std::uint32_t start_bit = from - g[31], end_bit = to - (g[31] + 1);
                            const std::uint32_t start_byte = sra(start_bit, 3), end_byte = sra(end_bit, 3);
                            const std::uint32_t start_mask = 0xffu >> (start_bit & 7), end_mask = (0xff80u >> (end_bit & 7)) & 0xff;
                            if (start_byte == end_byte) {
                                c.put8(g[19] + start_byte, c.u8(g[19] + start_byte) | (start_mask & end_mask));
                            } else {
                                c.put8(g[19] + start_byte, c.u8(g[19] + start_byte) | start_mask);
                                for (std::uint32_t byte = start_byte + 1; s32(byte) < s32(end_byte); ++byte) c.put8(g[19] + byte, 0xff);
                                c.put8(g[19] + end_byte, end_mask);
                            }
                        }
                        g[18] = c.u32(g[21] + 4);
                        if (g[18] == 0) break;
                        g[21] = c.u32(g[18] + 4);
                    } while (g[21] != 0);
                }
                g[19] += g[28];
                g[22] -= 4;
                --g[20];
            }
            // ---- dropout control along the rows ----
            if (c.u8(sc + dropout_rows) != 0) {
                g[22] = g[21] << 2;
                g[19] = 0;
                while (s32(g[21]) <= s32(c.u32(sc + last_row))) {
                    g[18] = c.u32(c.u32(sc + row_heads) + g[22]);
                    if (g[18] != 0) {
                        g[20] = c.u32(g[18] + 4);
                        if (g[20] == 0) {
                            hold_all();
                            report(c, g[23], 0x24, 0x4e4, 0x10762bd0);
                            settle(c, g);
                        }
                        do {
                            const std::uint32_t x1 = sra(c.u32(g[18]), 1), x2 = sra(c.u32(g[20]), 1);
                            if (s32(x2 - x1) < 0x40) {
                                const std::uint32_t row = g[24] + g[28] * ((g[29] - 1) - (g[21] - g[30]));
                                const std::uint32_t bit1 = sra(x1, 6) - g[31];
                                g[9] = sra(x2, 6) - g[31];
                                const std::uint32_t bit2 = g[9];
                                const std::uint32_t mask1 = 0x80u >> (bit1 & 7), mask2 = 0x80u >> (bit2 & 7);
                                if ((mask1 & c.u8(row + sra(bit1, 3))) == 0 && (mask2 & c.u8(row + sra(bit2, 3))) == 0) {
                                    std::uint32_t middle = sra(x1 + (x2 + 1), 1);
                                    if (s32(x1) > s32(g[27])) middle = sra(x1 + (x2 - 2), 1);
                                    const std::uint32_t bit = sra(middle, 6) - g[31];
                                    g[19] = 1;
                                    c.put8(row + sra(bit, 3), c.u8(row + sra(bit, 3)) | (0x80u >> (bit & 7)));
                                }
                            }
                            g[18] = c.u32(g[20] + 4);
                            if (g[18] == 0) break;
                            g[20] = c.u32(g[18] + 4);
                        } while (g[20] != 0);
                    }
                    g[22] += 4;
                    ++g[21];
                }
                c.put32(sc + found_x, c.u32(sc + found_x) | g[19]);
            }
            // ---- dropout control along the columns ----
            if (c.u8(sc + dropout_columns) != 0) {
                g[27] = c.u32(sc + first_column);
                g[19] = 0;
                g[22] = g[27] << 2;
                while (s32(g[27]) <= s32(c.u32(sc + last_column))) {
                    g[18] = c.u32(c.u32(sc + column_heads) + g[22]);
                    if (g[18] != 0) {
                        g[20] = c.u32(g[18] + 4);
                        if (g[20] == 0) {
                            hold_all();
                            report(c, g[23], 0x24, 0x514, 0x10762d18);
                            settle(c, g);
                        }
                        do {
                            const std::uint32_t y1 = sra(c.u32(g[18]), 1), y2 = sra(c.u32(g[20]), 1);
                            if (s32(y2 - y1) < 0x40) {
                                g[9] = g[27] - g[31];
                                const std::uint32_t mask = (0x80u >> (g[9] & 7)) & 0xff, byte = sra(g[9], 3);
                                const std::uint32_t row1 = g[28] * ((g[29] - 1) - (sra(y1, 6) - g[30])), row2 = g[28] * ((g[29] - 1) - (sra(y2, 6) - g[30]));
                                if ((mask & c.u8(g[24] + row1 + byte)) == 0 && (mask & c.u8(g[24] + row2 + byte)) == 0) {
                                    std::uint32_t middle = sra(y1 + (y2 + 1), 1);
                                    if (s32(y1) > s32(g[26])) middle = sra(y1 + (y2 - 2), 1);
                                    const std::uint32_t at = g[24] + g[28] * ((g[29] - 1) - (sra(middle, 6) - g[30])) + byte;
                                    g[19] = 1;
                                    c.put8(at, c.u8(at) | mask);
                                }
                            }
                            g[18] = c.u32(g[20] + 4);
                            if (g[18] == 0) break;
                            g[20] = c.u32(g[18] + 4);
                        } while (g[20] != 0);
                    }
                    g[22] += 4;
                    ++g[27];
                }
                c.put32(sc + found_y, c.u32(sc + found_y) | g[19]);
            }
        }
    }
    c.set({{9, g[9]}, {10, g[10]}});
}

// 0x10762e30: the converter given back (a stack leaver: frame 0x50, r29-r31; each release below is the allocator's leaver at this frame's stack
// pointer with r29 the converter and r30/r31 the table position as the original holds them). The bitmap, the chunks (all but the inline first), the
// chunk table, the two list tables when they are not the inline ones, and the converter itself (its scratch slot is only marked free).
void delete_scan_converter(Cpu& c, std::uint32_t sc) {
    using namespace scan;
    guest::Frame frame(c, 0x10762e30, 0x50, {.first_gpr = 29});
    c.set(29, sc);
    if (sc == 0) return;
    const auto release = [&](std::uint32_t mem, std::uint32_t pointer, std::uint32_t back) {
        c.set({{3, mem}, {4, pointer}});
        c.link(back);
        release_block(c, mem, pointer);
    };
    std::uint32_t mem = 0;
    {
        const auto bitmap_ = c.u32(sc + bitmap);
        c.set(4, bitmap_);
        if (bitmap_ != 0) {
            mem = c.u32(sc + scan::mem);
            c.set(3, mem);
            if (bitmap_ == c.u32(mem + tsimem::scratch_pointer + 4 * 5)) c.put32(mem + tsimem::scratch_free + 4 * 5, 1);
            else release(mem, bitmap_, 0x10762e7c);
        }
    }
    c.set({{31, 1}, {30, 4}});
    while (s32(c.r(31)) <= s32(c.u32(sc + chunk_count))) {
        c.set(4, c.u32(sc + chunks));
        mem = c.u32(sc + scan::mem);
        c.set(3, mem);
        const auto chunk = c.u32(c.u32(sc + chunks) + c.r(30));
        c.set(4, chunk);
        release(mem, chunk, 0x10762e9c);
        c.set({{30, c.r(30) + 4}, {31, c.r(31) + 1}});
    }
    {
        const auto table = c.u32(sc + chunks);
        c.set(4, table);
        if (table != sc + chunk_slots) release(c.u32(sc + scan::mem), table, 0x10762ec8);
        c.set(3, 0);
        c.put32(sc + chunk_capacity, 0);
    }
    {
        const auto buffer = c.u32(sc + column_buffer);
        c.set(4, buffer);
        if (buffer != sc + column_slots) release(c.u32(sc + scan::mem), buffer, 0x10762ee8);
    }
    {
        const auto buffer = c.u32(sc + row_buffer);
        c.set(4, buffer);
        if (buffer != sc + row_slots) release(c.u32(sc + scan::mem), buffer, 0x10762f00);
    }
    mem = c.u32(sc + scan::mem);
    c.set(3, mem);
    if (sc == c.u32(mem + tsimem::scratch_pointer + 4 * 4)) c.put32(mem + tsimem::scratch_free + 4 * 4, 1);
    else release(mem, sc, 0x10762f24);
}
}
