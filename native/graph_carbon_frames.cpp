// graph_carbon.cpp, block `frames` (0x1076d6e0, 670 instructions): the framed, shaded or filled rectangle of the game's panels and buttons.
//
// 0x1076d6e0 draws the rectangle (left, top, right, bottom) of the surface as its style word says, in stages (a stack-contract leaver: stmw
// r17, frame 0x80; the primitives are called at its own stack pointer with r17-r31 as the original holds them):
//  - geometry and colour: 0x20000 pulls the right and bottom edges in by 2, 0x40000 pushes the left and top edges out by 2 (the interior of
//    a frame), 0x1000 takes the colour from the surface's pixel at (left, top);
//  - the fill: 0x2 dims the area (0x10769980), else 0x10 fills it with the colour (the rectangle 0x1076a580, or the ellipse 0x1076ad50 with
//    0x10000), else 0x4 / 0x8 a gradient of one-pixel lines (0x1076a140) from the colour at 100% to black across / down;
//  - the frame: with 0x20 an ellipse (0x10000, in the accent colour) or a rectangle outline (0x200) or a bevel of 2 pixels (4 for a panel of at
//    least 50 x 50 with bottom < 100 and 0x10): rings of four lines, the light colour (the colour at 100 + 66 ring / n percent) on the top and
//    left, the dark one (100 - 66 ring / n percent) on the bottom and right, swapped by 0x40; without 0x20 a shadow line under the rectangle
//    (0x80, the colour at 90%) or else, with 0x400, an outline of the rectangle (with 0x4000 one line through the middle, with 0x8000 at the top;
//    0x2000 the thin pen);
//  - 0x1000000 two highlight lines through the middle (the colour at 66% and the colour at 133%), and 0x800 a one-pixel rectangle just
//    outside in the colour of the module's variable at TOC -0x7bd0.
// What the primitives do is read from the callers: "mode" 1 and 2 are two pen widths, 4 the fill of a rectangle.
//
// Registers: the original's r17-r31 are the C++'s `Kept` registers (the callees save them into their frames: the leaver log and the checker's
// trace compare them at each call); r3-r8 at each call are its arguments and r9 / r10 the leftovers of the inline shading that precedes the
// call (a callee's leftovers carry on to the next call); the shading is `shade`, the arithmetic of the leaf 0x1076bb40 (the compiler's divide
// by 100 of a product below 2^17, exactly `/ 100`).
#include "graph_carbon_detail.hpp"

namespace cm::aot::graph_carbon {
namespace {
constexpr std::uint32_t entry = 0x1076d6e0;
// The pixel primitives (other blocks) this routine calls through the dispatcher.
namespace primitive {
inline constexpr std::uint32_t line = 0x1076a140, rectangle = 0x1076a580, ellipse = 0x1076ad50, dim = 0x10769980;
}
// The style word's bits.
namespace bit {
inline constexpr std::uint32_t dim = 0x2, gradient_across = 0x4, gradient_down = 0x8, fill = 0x10, bevel_frame = 0x20, bevel_swapped = 0x40,
                               shadow_line = 0x80, outline = 0x200, line_pair = 0x400, black_outline = 0x800, colour_from_surface = 0x1000,
                               thin = 0x2000, middle_line = 0x4000, top_line = 0x8000, ellipse_shape = 0x10000, inset = 0x20000,
                               outset = 0x40000, highlight_pair = 0x1000000;
}
// The module's small data and TOC slots.
constexpr std::int32_t initialised = -0x3550, surface_base = -0x3564, row_stride = -0x3568;
constexpr std::int32_t default_colour_slot = -0x7bdc, outline_colour_slot = -0x7bd0;
constexpr std::uint32_t magic_hundredth = 0x51eb851f;  // the compiler's divide-by-100 constant, left in r9 by the gradient's shading

std::int32_t as_signed(std::uint32_t value) { return std::int32_t(value); }

// The 8-bit components of a 15-bit pixel: red (bits 10-14), green (5-9) and blue (0-4), widened by three bits.
struct Channels { std::uint32_t red, green, blue; };
constexpr Channels widened(std::uint32_t pixel) { return {(pixel & 0x7c00) >> 7, (pixel & 0x3e0) >> 2, (pixel << 3) & 0xf8}; }
// The pixel scaled by `percent` (its low byte), each component clamped at 255: the leaf 0x1076bb40's result.
constexpr Channels scaled(std::uint32_t pixel, std::uint32_t percent) {
    const auto factor = percent & 0xff;
    const auto channel = [&](std::uint32_t value) { const auto result = value * factor / 100; return result < 0xff ? result : 0xffu; };
    const auto source = widened(pixel);
    return {channel(source.red), channel(source.green), channel(source.blue)};
}
constexpr std::uint32_t pixel_of(const Channels& channels) { return pack_pixel(channels.red, channels.green, channels.blue); }
constexpr std::uint32_t shade(std::uint32_t pixel, std::uint32_t percent) { return pixel_of(scaled(pixel, percent)); }

// The rectangle being drawn. Its members are the registers the original keeps its values in while it calls the primitives.
class Rectangle {
public:
    Rectangle(Cpu& cpu, std::uint32_t left_edge, std::uint32_t top_edge, std::uint32_t right_edge, std::uint32_t bottom_edge,
              std::uint32_t style_word, std::uint32_t colour_word, std::uint32_t accent_word)
        : c(cpu), dark_edge(cpu, cpu.r(17)), default_colour(cpu, cpu.toc(default_colour_slot)), style(cpu, style_word), accent(cpu, accent_word),
          bottom(cpu, bottom_edge), right(cpu, right_edge), top(cpu, top_edge), left(cpu, left_edge), scan(cpu, cpu.r(25)),
          inner_top(cpu, cpu.r(26)), inner_right(cpu, cpu.r(27)), ring(cpu, cpu.r(28)), inner_bottom(cpu, cpu.r(29)),
          thickness(cpu, cpu.r(30)), colour(cpu, colour_word) {}

    void draw() {
        if (c.i8(small(c, initialised)) != 0) return;  // the graphics are not started
        geometry();
        fill();
        frame();
        highlights();
        black_outline();
    }

private:
    bool has(std::uint32_t bits) const { return (std::uint32_t(style) & bits) != 0; }
    // `bl primitive` with the arguments in r3-r8.
    void call(std::uint32_t target, std::uint32_t back, std::uint32_t x1, std::uint32_t y1, std::uint32_t x2, std::uint32_t y2, std::uint32_t mode,
              std::uint32_t colour_argument) {
        c.set({{3, x1}, {4, y1}, {5, x2}, {6, y2}, {7, mode}, {8, colour_argument}});
        c.call(target, back);
    }
    // The primitive over the rectangle's own edges.
    void call_over_edges(std::uint32_t target, std::uint32_t back, std::uint32_t mode, std::uint32_t colour_argument) {
        call(target, back, left, top, right, bottom, mode, colour_argument);
    }
    // The colour the shading starts from: the colour, or the module's default (a halfword behind the TOC slot in r18) when its low halfword is 0.
    std::uint32_t lit_colour() const { return (std::uint32_t(colour) & 0xffff) != 0 ? std::uint32_t(colour) : std::uint32_t(c.u16(default_colour)); }

    void geometry() {
        if (has(bit::inset)) {
            right = right - 2;
            bottom = bottom - 2;
        }
        if (has(bit::outset)) {
            left = left + 2;
            top = top + 2;
        }
        if (has(bit::colour_from_surface)) {
            const auto stride = extended(c.i16(small(c, row_stride)));
            const auto base = c.u32(small(c, surface_base));
            // r3 the column's byte offset, r4 the surface's base address (left in the registers).
            c.set({{3, std::uint32_t(left) << 1}, {4, base}});
            colour = c.u16(base + ((std::uint32_t(left) << 1) + ((std::uint32_t(top) * stride) << 1)));
        }
    }

    void fill() {
        if (has(bit::dim)) {
            c.set({{3, left}, {4, top}, {5, right}, {6, bottom}});
            c.call(primitive::dim, 0x1076d77c);
        } else if (has(bit::fill)) {
            if (has(bit::ellipse_shape)) call_over_edges(primitive::ellipse, 0x1076d7ac, 4, colour);
            else call_over_edges(primitive::rectangle, 0x1076d7cc, 4, colour);
        } else if (has(bit::gradient_across)) {
            // Vertical lines from the top to the bottom, the colour fading with the column.
            scan = left;
            while (as_signed(scan) <= as_signed(right)) {
                const auto position = std::uint32_t(divide_signed<4>((scan - left) * 100, right - left));
                gradient_line(scan, top, scan, bottom, 100 - position, 0x1076d8d4);
                scan = scan + 1;
            }
        } else if (has(bit::gradient_down)) {
            scan = top;
            while (as_signed(scan) <= as_signed(bottom)) {
                const auto position = std::uint32_t(divide_signed<4>((scan - top) * 100, bottom - top));
                gradient_line(left, scan, right, scan, 100 - position, 0x1076d9e8);
                scan = scan + 1;
            }
        }
    }
    // One line of a gradient: the colour at `percent`; r9 holds the divide's constant and r10 the colour's green component when it is called.
    void gradient_line(std::uint32_t x1, std::uint32_t y1, std::uint32_t x2, std::uint32_t y2, std::uint32_t percent, std::uint32_t back) {
        c.set({{9, magic_hundredth}, {10, widened(colour).green}});
        call(primitive::line, back, x1, y1, x2, y2, 2, shade(colour, percent));
    }

    void frame() {
        if (has(bit::bevel_frame)) {
            if (has(bit::ellipse_shape)) call_over_edges(primitive::ellipse, has(bit::thin) ? 0x1076da28 : 0x1076da48, has(bit::thin) ? 1 : 2, accent);
            else if (has(bit::outline)) call_over_edges(primitive::rectangle, has(bit::thin) ? 0x1076da78 : 0x1076da98, has(bit::thin) ? 1 : 2, colour);
            else bevel();
        } else if (has(bit::shadow_line)) {
            // A line along the bottom, 2 pixels in from the sides, at 90% of the colour; r9 holds the colour it is made from.
            const auto source = lit_colour();
            c.set(9, source);
            call(primitive::line, 0x1076de70, left + 2, bottom, right - 2, bottom, 1, shade(source, 0x5a));
        } else if (has(bit::line_pair)) {
            if (has(bit::middle_line)) {
                accent = has(bit::top_line) ? std::uint32_t(top) : top + std::uint32_t(as_signed(bottom - top) / 2);
                call(primitive::line, has(bit::thin) ? 0x1076decc : 0x1076deec, left, accent, right, accent, has(bit::thin) ? 1 : 2, colour);
            } else {
                call_over_edges(primitive::line, has(bit::thin) ? 0x1076df14 : 0x1076df34, has(bit::thin) ? 1 : 2, colour);
            }
        }
    }
    // The bevel: rings 1..thickness of four lines each.
    void bevel() {
        const bool panel = as_signed(right - left) >= 0x32 && as_signed(bottom - top) >= 0x32 && as_signed(bottom) < 0x64 && has(bit::fill);
        thickness = panel ? 4 : 2;
        scan = left + thickness;
        inner_top = top + thickness;
        inner_right = right - thickness;
        inner_bottom = bottom - thickness;
        ring = 1;
        while (as_signed(ring) <= as_signed(thickness)) {
            // The ring's share of the way to the full contrast: ring * 66 / thickness percent more light, or less.
            const auto share = std::uint32_t(divide_signed<4>(ring * 0x42, thickness));
            const auto light = shade(lit_colour(), share + 100), dark = shade(colour, 100 - share);
            accent = has(bit::bevel_swapped) ? dark : light;
            dark_edge = has(bit::bevel_swapped) ? light : dark;
            c.set({{9, (100 - share) & 0xff}, {10, widened(colour).green}});
            call(primitive::line, 0x1076dd10, scan - ring, inner_top - ring, inner_right + ring, inner_top - ring, 2, accent);
            call(primitive::line, 0x1076dd2c, scan - ring, inner_top - ring, scan - ring, inner_bottom + ring, 2, accent);
            call(primitive::line, 0x1076dd4c, inner_right + ring, inner_bottom + ring, scan - ring + 1, inner_bottom + ring, 2, dark_edge);
            call(primitive::line, 0x1076dd6c, inner_right + ring, inner_bottom + ring, inner_right + ring, inner_top - ring + 1, 2, dark_edge);
            ring = ring + 1;
        }
    }

    // Two lines through the middle, the lower one at 133% of the lit colour (kept in r18 from here), the upper at 66% of the colour.
    void highlights() {
        if (!has(bit::highlight_pair)) return;
        const auto bright = scaled(lit_colour(), 0x85);
        const auto bright_pixel = pixel_of(bright);
        default_colour = bright_pixel;
        accent = top + std::uint32_t(as_signed(bottom - top) / 2);
        // r9 and r10 are the bright colour's red and green components when the first line is called.
        c.set({{9, bright.red}, {10, bright.green}});
        call(primitive::line, 0x1076e100, left + 2, accent, right - 2, accent, 2, shade(colour, 0x42));
        call(primitive::line, 0x1076e11c, left + 2, accent + 1, right - 2, accent + 1, 2, default_colour);
    }
    // A one-pixel rectangle just outside, in the colour the module keeps for it.
    void black_outline() {
        if (!has(bit::black_outline)) return;
        call(primitive::rectangle, 0x1076e144, left - 1, top - 1, right + 1, bottom + 1, 2, c.u16(c.toc(outline_colour_slot)));
    }

    Cpu& c;
    // r17 the dark edge colour of a bevel, r18 the address of the default colour (then the bright highlight pixel), r19 the style word, r20 the
    // accent colour (the light edge of a bevel, a middle row), r21-r24 the bottom, right, top and left edges, r25 the scan position (a bevel's
    // inner left edge), r26-r29 the bevel's inner top, inner right, ring and inner bottom, r30 its thickness, r31 the colour.
    Kept<17> dark_edge;
    Kept<18> default_colour;
    Kept<19> style;
    Kept<20> accent;
    Kept<21> bottom;
    Kept<22> right;
    Kept<23> top;
    Kept<24> left;
    Kept<25> scan;
    Kept<26> inner_top;
    Kept<27> inner_right;
    Kept<28> ring;
    Kept<29> inner_bottom;
    Kept<30> thickness;
    Kept<31> colour;
};
}

void framed_rectangle(Cpu& c, std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom, std::uint32_t style, std::uint32_t colour,
                      std::uint32_t accent) {
    guest::Frame frame(c, entry, 0x80, {.first_gpr = 17});
    Rectangle(c, std::uint32_t(left), std::uint32_t(top), std::uint32_t(right), std::uint32_t(bottom), style, colour, accent).draw();
}
}
