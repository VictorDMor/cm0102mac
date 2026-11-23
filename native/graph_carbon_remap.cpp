// graph_carbon.cpp, block `remap` (0x10769460, 213 instructions): the screen fade.
//
// 0x10769460 fades the rectangle (x1, y1, x2, y2) of the off-screen surface to black (steps > 0: from black up to the screen) or from the
// screen to black (steps < 0), in |steps| + 1 frames. It captures the rectangle's pixels (0x1076b020, a bitmap record), builds a remap table
// of 65,536 entries through the allocator (calloc 0x10775200; one entry per 15-bit pixel value present in the capture, allocated on first
// use: |steps| + 1 pixels, the pixel scaled by 0, 100/n, ... 100 percent, ascending for a positive step count and descending for a negative
// one), then for each frame writes the capture's pixels through the table back to the surface and puts the rectangle on the window
// (GetPort / SetPort / SetRect / CopyBits / QDFlushPortBuffer through the Toolbox glue). Everything is released at the end (each entry and
// the table with 0x10775170, the capture with 0x1076b7e0), also when an allocation or the capture fails. Not a stack-contract leaver: the
// frame (0x90 bytes, r19-r31 saved) is built anyway, the host's drawing view reads the stack pointer, the rectangle at +0x44 and the
// registers r26, r28 and r30 at the observation point 0x107696cc (once a frame) and the entry's register r7 at 0x10769460 (observed by the
// generated page before the hook); this routine runs native_drawing_boundary at 0x107696cc and at its `blr`, 0x107697b0.
//
// Registers: the original's r19-r31 are the C++'s `Kept` locals (the callees save them into their frames, the checker's trace compares them
// at each call), r3-r10 at each call are what the original leaves (the loops' temporaries reach the import log of the checker), the
// results are what the last callee left. The allocator, the capture, the free of the capture and the Toolbox glue are called through the
// dispatcher (`c.call`) with the original's return addresses; the two leaves (0x107698d0, 0x1076bb40) are called directly.
#include "graph_carbon_detail.hpp"
#include "native_drawing.hpp"

namespace cm::aot::graph_carbon {
namespace {
constexpr std::uint32_t entry = 0x10769460;
// Callees of other blocks (generated or rewritten, always reached through the dispatcher): the absolute value, calloc, the rectangle capture
// (a bitmap record: +0 width, +4 height, +0xc the pixels), the allocator's new[] and delete[], the capture's release.
namespace callee {
inline constexpr std::uint32_t absolute = 0x10775550, calloc = 0x10775200, capture = 0x1076b020, allocate = 0x10775130,
                               release = 0x10775170, free_capture = 0x1076b7e0;
}
// The Toolbox glue (one `bl` each; the original reloads r2 from the frame after each).
namespace glue {
inline constexpr std::uint32_t get_port = 0x107803f0, get_window_port = 0x107805a0, set_port = 0x107804f8, set_rect = 0x10780588,
                               bitmap_for_copy = 0x107805b8, copy_bits = 0x107805d0, flush_buffer = 0x107805e8;
}
namespace bitmap {  // the capture's record
inline constexpr std::uint32_t width = 0, height = 4, pixels = 0xc;
}
namespace local {  // the frame's own slots (offsets from sp)
inline constexpr std::int32_t saved_port = 0x38, source_rect = 0x3c, region = 0x44;
}
// The module's small data the routine reads: the surface (base address, row stride in pixels) and the off-screen port.
constexpr std::int32_t surface_base = -0x3564, row_stride = -0x3568, off_screen_port = -0x355c;
constexpr std::uint32_t table_entries = 0x10000;
constexpr std::int32_t window_slot = -0x5af8;  // the TOC slot of the address of the window variable
constexpr std::uint32_t observe_frame = 0x107696cc, observe_exit = 0x107697b0;

void reload_toc(Cpu& c) { c.set(2, c.u32(c.sp() + 0x14)); }
// `bl glue; lwz r2,0x14(r1)`.
void toolbox(Cpu& c, std::uint32_t glue_entry, std::uint32_t back) {
    c.call(glue_entry, back);
    reload_toc(c);
}
// `bl 0x1076bb40` (the pixel scaled by a percentage) and `bl 0x107698d0` (the rectangle of two corners), the module's leaves called
// directly: the link register, then r3 and r4-r9 (or r5 and r6) as the leaf's own hook leaves them.
std::uint32_t scaled_pixel(Cpu& c, std::uint32_t pixel, std::uint32_t percent, std::uint32_t back) {
    c.set({{3, pixel}, {4, percent}, {5, 0}});
    c.link(back);
    Leftovers left{c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)};
    const auto scaled = scale_pixel(pixel, percent, left);
    c.set({{3, scaled}, {4, left.r4}, {5, left.r5}, {6, left.r6}, {7, left.r7}, {8, left.r8}, {9, left.r9}, {10, left.r10}});
    return scaled;
}
void rectangle_of_corners(Cpu& c, std::uint32_t x1, std::uint32_t y1, std::uint32_t x2, std::uint32_t y2, std::uint32_t rect, std::uint32_t back) {
    c.set({{3, x1}, {4, y1}, {5, x2}, {6, y2}, {7, rect}});
    c.link(back);
    Leftovers left{c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)};
    rectangle_from_corners(c, std::int32_t(x1), std::int32_t(y1), std::int32_t(x2), std::int32_t(y2), rect, left);
    c.set({{4, left.r4}, {5, left.r5}, {6, left.r6}, {7, left.r7}, {8, left.r8}, {9, left.r9}, {10, left.r10}});
}
std::int32_t as_signed(std::uint32_t v) { return std::int32_t(v); }

// The routine's state: its rectangle, step count and loop variables live in the registers the original keeps them in.
class Fade {
public:
    Fade(Cpu& cpu, const Frame& frame, std::uint32_t x1, std::uint32_t y1, std::uint32_t x2, std::uint32_t y2, std::uint32_t step_count)
        : c(cpu), frame(frame), steps(cpu, step_count), bottom_or_ramp_index(cpu, y2), right_or_column(cpu, x2), top_or_row(cpu, y1),
          table(cpu, 0), left_or_pixel(cpu, x1), pixels(cpu, cpu.r(25)), capture(cpu, 0), ramp_offset(cpu, cpu.r(27)), ramp(cpu, cpu.r(28)),
          window_variable(cpu, cpu.toc(window_slot)), span(cpu, cpu.r(30)), slot(cpu, cpu.r(31)) {}

    // Everything after the prologue: the table and capture, the frames, the release.
    void run() {
        c.set(3, steps);
        c.call(callee::absolute, 0x10769498);
        span = c.r(3);
        if (as_signed(steps) != 0) show();
        release();
    }

private:
    // The capture and the table, then the frames; any failure goes to the release.
    void show() {
        c.set({{3, table_entries}, {4, 4}});
        c.call(callee::calloc, 0x107694b4);
        table = c.r(3);
        if (table == 0) return;
        rectangle_of_corners(c, std::uint32_t(left_or_pixel), std::uint32_t(top_or_row), std::uint32_t(right_or_column), std::uint32_t(bottom_or_ramp_index), frame.at(local::region), 0x107694d4);
        const Rect region(c, frame.at(local::region));
        c.set({{3, extended(region.left)}, {7, 0}, {4, extended(region.top)}, {5, extended(region.right)}, {6, extended(region.bottom)}});
        c.call(callee::capture, 0x107694ec);
        capture = c.r(3);
        if (capture == 0) return;
        if (!build_table()) return;
        put_frames();
    }

    // For each pixel of the capture (row by row) that has no table entry yet, the ramp of its fades.
    bool build_table() {
        pixels = c.u32(capture + bitmap::pixels);
        top_or_row = 0;
        while (as_signed(top_or_row) <= as_signed(c.u32(capture + bitmap::height) - 1)) {
            right_or_column = 0;
            while (as_signed(right_or_column) <= as_signed(c.u32(capture + bitmap::width) - 1)) {
                left_or_pixel = c.u16(pixels);
                slot = left_or_pixel << 2;
                if (c.u32(table + slot) == 0 && !build_ramp()) return false;
                pixels = pixels + 2;
                right_or_column = right_or_column + 1;
            }
            top_or_row = top_or_row + 1;
        }
        return true;
    }
    // One table entry: |steps| + 1 halfwords, the pixel scaled by (ramp * 100 / |steps|) percent, stored from the front for a positive step
    // count and from the back for a negative one.
    bool build_ramp() {
        c.set(3, (span + 1) << 1);
        c.call(callee::allocate, 0x10769528);
        c.put32(table + slot, c.r(3));
        if (c.u32(table + slot) == 0) return false;
        ramp = 0;
        bottom_or_ramp_index = 0;
        ramp_offset = ramp;
        while (as_signed(bottom_or_ramp_index) <= as_signed(span)) {
            const auto percent = std::uint32_t(divide_signed<4>(ramp, span));
            const bool rising = as_signed(steps) > 0;
            const auto shaded = scaled_pixel(c, left_or_pixel, percent & 0xff, rising ? 0x10769564 : 0x10769584);
            const auto ramp_entry = c.u32(table + slot);
            c.set(4, ramp_entry);
            c.put16(rising ? ramp_entry + ramp_offset : ramp_entry + ((span - bottom_or_ramp_index) << 1), shaded);
            ramp = ramp + 100;
            ramp_offset = ramp_offset + 2;
            bottom_or_ramp_index = bottom_or_ramp_index + 1;
        }
        return true;
    }

    // The frames: the port set up once, then per frame the capture's pixels through the table at the frame's offset into the surface and
    // the rectangle onto the window.
    void put_frames() {
        c.set(3, frame.at(local::saved_port));
        toolbox(c, glue::get_port, 0x107695dc);
        c.set(3, c.u32(window_variable));
        toolbox(c, glue::get_window_port, 0x107695e8);
        toolbox(c, glue::set_port, 0x107695f0);
        const Rect region(c, frame.at(local::region));
        const auto right = extended(region.right), bottom = extended(region.bottom);
        c.set({{3, frame.at(local::source_rect)}, {4, extended(region.left)}, {5, extended(region.top)}, {6, extended(std::int16_t(right + 1))}, {7, extended(std::int16_t(bottom + 1))}});
        toolbox(c, glue::set_rect, 0x1076961c);
        ramp = 0;
        ramp_offset = ramp;
        while (as_signed(ramp) <= as_signed(span)) {
            copy_through_table();
            native_drawing_boundary(c.s, observe_frame);
            show_rectangle();
            ramp_offset = ramp_offset + 2;
            ramp = ramp + 1;
        }
        c.set(3, c.u32(frame.at(local::saved_port)));
        toolbox(c, glue::set_port, 0x10769748);
    }
    // The capture's rows written through the table into the rectangle of the surface. Leaves r3-r8 as the loops do (the import log of the
    // checker records them at the next call): the row, the byte width of a row of the rectangle, the surface's base and the row stride,
    // the right edge, the source and the destination pointers.
    void copy_through_table() {
        const Rect region(c, frame.at(local::region));
        const auto stride = c.i16(small(c, row_stride));
        const auto base = c.u32(small(c, surface_base));
        std::uint32_t row = extended(region.top);
        Leftovers left{c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)};
        left.r4 = extended(region.left) << 1;
        left.r5 = base;
        left.r7 = c.u32(capture + bitmap::pixels);
        left.r8 = base + (left.r4 + ((row * extended(stride)) << 1));
        while (as_signed(row) <= as_signed(extended(region.bottom))) {
            std::uint32_t column = extended(region.left);
            for (;;) {
                left.r6 = extended(region.right);
                if (!(as_signed(column) <= as_signed(left.r6))) break;
                const auto value = c.u16(left.r7);
                left.r7 += 2;
                column += 1;
                c.put16(left.r8, c.u16(ramp_offset + c.u32(table + (std::uint32_t(value) << 2))));
                left.r8 += 2;
            }
            row += 1;
            const auto row_bytes = (left.r6 - extended(region.left) + 1) << 1;
            left.r4 = row_bytes;
            left.r5 = extended(stride) << 1;
            left.r8 = left.r8 + left.r5 - row_bytes;
            left.r7 = left.r7 + (c.u32(capture + bitmap::width) << 1) - row_bytes;
        }
        c.set({{3, row}, {4, left.r4}, {5, left.r5}, {6, left.r6}, {7, left.r7}, {8, left.r8}});
    }
    // The window port is the destination of CopyBits from the off-screen port; the steps count's register is the window's port from here.
    void show_rectangle() {
        c.set(3, c.u32(window_variable));
        toolbox(c, glue::get_window_port, 0x107696d4);
        const auto window_port = c.r(3);
        c.set(3, c.u32(small(c, off_screen_port)));
        steps = window_port;
        toolbox(c, glue::bitmap_for_copy, 0x107696e8);
        const auto off_screen_bitmap = c.r(3);
        c.set(3, steps);
        bottom_or_ramp_index = off_screen_bitmap;
        toolbox(c, glue::bitmap_for_copy, 0x107696fc);
        const auto source_rect = frame.at(local::source_rect);
        c.set({{5, source_rect}, {4, c.r(3)}, {3, bottom_or_ramp_index}, {7, 0}, {6, source_rect}, {8, 0}});
        toolbox(c, glue::copy_bits, 0x1076971c);
        c.set({{3, steps}, {4, 0}});
        toolbox(c, glue::flush_buffer, 0x1076972c);
    }

    // The release: each entry of the table, the table, the capture.
    void release() {
        if (table != 0) {
            c.set(3, table_entries);
            steps = table;
            bottom_or_ramp_index = c.r(3) - 1;
            right_or_column = 0;
            while (as_signed(right_or_column) <= as_signed(bottom_or_ramp_index)) {
                const auto block = c.u32(steps);
                c.set(3, block);
                if (block != 0) c.call(callee::release, 0x10769778);
                steps = steps + 4;
                right_or_column = right_or_column + 1;
            }
            c.set(3, table);
            c.call(callee::release, 0x10769790);
        }
        if (capture != 0) {
            c.set(3, capture);
            c.call(callee::free_capture, 0x107697a0);
        }
    }

    Cpu& c;
    const Frame& frame;
    // The original's registers: r19 the step count (the window's port while the frames show), r20 the rectangle's bottom (then the ramp index),
    // r21 its right edge (the column), r22 its top (the row), r23 the table, r24 its left (the pixel), r25 the capture's pixel pointer,
    // r26 the capture, r27 the ramp offset in bytes, r28 the ramp numerator (the frame index), r29 the address of the window variable,
    // r30 |steps|, r31 the table offset of the pixel.
    Kept<19> steps;
    Kept<20> bottom_or_ramp_index;
    Kept<21> right_or_column;
    Kept<22> top_or_row;
    Kept<23> table;
    Kept<24> left_or_pixel;
    Kept<25> pixels;
    Kept<26> capture;
    Kept<27> ramp_offset;
    Kept<28> ramp;
    Kept<29> window_variable;
    Kept<30> span;
    Kept<31> slot;
};
}

void fade_rectangle(Cpu& c, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::int32_t steps) {
    Frame frame(c, entry, 0x90, {.first_gpr = 19});
    Fade(c, frame, std::uint32_t(x1), std::uint32_t(y1), std::uint32_t(x2), std::uint32_t(y2), std::uint32_t(steps)).run();
    frame.leave();
    native_drawing_boundary(c.s, observe_exit);
}
}
