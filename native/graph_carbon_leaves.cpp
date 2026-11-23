// graph_carbon.cpp, block `leaves` (0x10769b50, 0x1076e910, 0x107698d0, 0x10769920, 0x1076bb20, 0x1076bb40, 0x1076ca20): the module's
// leaf routines. None calls anything, so none builds a frame; each leaves r3 and r4-r10 as the original's last instruction does (the
// checker compares them in full) and the registers the original does not write stay as the caller had them.
#include "graph_carbon_detail.hpp"

namespace cm::aot::graph_carbon {
namespace {
// `mulhw`: the high word of the signed 64-bit product.
constexpr std::int32_t high_word(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * std::int64_t(b)) >> 32); }
// The compiler's divide by 100 of a non-negative product: multiply by 0x51eb851f, shift the high word right by 5 (arithmetic) and add
// the sign bit (the correction for a negative dividend, which no product here is).
constexpr std::uint32_t magic_hundredth = 0x51eb851f;
struct Hundredth { std::int32_t shifted; std::uint32_t sign; std::int32_t quotient; };
constexpr Hundredth divide_by_hundred(std::uint32_t product) {
    const auto shifted = high_word(std::int32_t(magic_hundredth), std::int32_t(product)) >> 5;
    const auto sign = (std::uint32_t(shifted) >> 31) & 1;
    return {shifted, sign, std::int32_t(std::uint32_t(shifted) + sign)};
}
constexpr std::int32_t at_most_255(std::int32_t value) { return value < 0xff ? value : 0xff; }
}

std::uint32_t full_screen(Cpu& c) { return c.u8(c.toc(toc::full_screen)); }

std::uint32_t current_cursor(Cpu& c) { return c.u8(small(c, small_data::current_cursor)); }

void rectangle_from_corners(Cpu& c, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t rect, Leftovers& left) {
    Rect box(c, rect);
    box.left = std::int16_t(x1 < x2 ? x1 : x2);
    box.top = std::int16_t(y1 < y2 ? y1 : y2);
    box.right = std::int16_t(x1 > x2 ? x1 : x2);
    box.bottom = std::int16_t(y1 > y2 ? y1 : y2);
    left.r5 = std::uint32_t(x1 > x2 ? x1 : x2);
    left.r6 = std::uint32_t(y1 > y2 ? y1 : y2);
}

std::uint32_t intersection_area(Corners a, Corners b, Leftovers& left) {
    const auto edge_left = a.left > b.left ? a.left : b.left, edge_top = a.top > b.top ? a.top : b.top;
    const auto edge_right = a.right < b.right ? a.right : b.right, edge_bottom = a.bottom < b.bottom ? a.bottom : b.bottom;
    // The differences wrap at 32 bits. The original keeps a side only where its negation and the side differ in sign (-x & ~x has its
    // sign bit set exactly when x > 0), which is `x > 0` for every word, so a non-positive width or height gives an empty area.
    const auto width = std::uint32_t(edge_right) - std::uint32_t(edge_left), height = std::uint32_t(edge_bottom) - std::uint32_t(edge_top);
    left.r7 = std::uint32_t(edge_left);
    left.r8 = std::uint32_t(edge_top);
    left.r9 = std::uint32_t(edge_right);
    left.r10 = std::uint32_t(edge_bottom);
    left.r5 = width;
    left.r4 = height;
    const auto kept_width = std::int32_t(width) > 0 ? width : 0, kept_height = std::int32_t(height) > 0 ? height : 0;
    return kept_width * kept_height;
}

std::uint32_t pack_pixel(std::uint32_t red, std::uint32_t green, std::uint32_t blue) {
    return (((red << 7) & pixel::red_mask) | ((green << 2) & pixel::green_mask) | ((blue >> 3) & pixel::blue_mask)) & 0xffff;
}

std::uint32_t scale_pixel(std::uint32_t value, std::uint32_t percent, Leftovers& left) {
    const auto factor = percent & 0xff;
    // The components widened to 8 bits (the top five bits, shifted up three), each multiplied by the factor and divided by 100.
    const auto red8 = (value & pixel::red_mask) >> 7, green8 = (value & pixel::green_mask) >> 2, blue8 = (value & pixel::blue_mask) << 3;
    const auto red = divide_by_hundred(factor * red8), green = divide_by_hundred(factor * green8), blue = divide_by_hundred(factor * blue8);
    const auto red_out = at_most_255(red.quotient), green_out = at_most_255(green.quotient), blue_out = at_most_255(blue.quotient);
    left.r4 = std::uint32_t(blue_out);
    left.r5 = std::uint32_t(green.quotient);
    left.r6 = std::uint32_t(red.quotient);
    left.r7 = red.sign;
    left.r8 = magic_hundredth;
    left.r9 = factor;
    return pack_pixel(std::uint32_t(red_out), std::uint32_t(green_out), std::uint32_t(blue_out));
}

std::uint32_t probe_text_position(Cpu& c, std::uint32_t value, Leftovers& left) {
    Field<std::int32_t> limit(c, small(c, small_data::text_probe_limit)), count(c, small(c, small_data::text_probe_count)),
        captured(c, small(c, small_data::text_probe_captured));
    const std::int32_t calls = count;
    if (calls == std::int32_t(limit)) captured = std::int32_t(std::int16_t(value & 0xffff));
    left.r4 = std::uint32_t(calls);
    count = std::int32_t(std::uint32_t(calls) + 1);
    return 0;
}
}
