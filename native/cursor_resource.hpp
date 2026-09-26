#pragma once
#include <array>
#include <cstdint>
#include <span>
namespace cm::aot {
struct ColorCursor {
    std::array<std::uint8_t, 16 * 16 * 4> rgba{};
    std::uint16_t x, y;
    std::uint32_t map_offset, pixels_offset, palette_offset, pixel_bytes, palette_bytes;
};
ColorCursor parse_color_cursor(std::span<const std::uint8_t> bytes);
ColorCursor parse_monochrome_cursor(std::span<const std::uint8_t> bytes);
}
