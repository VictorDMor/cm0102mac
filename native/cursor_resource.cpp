#include "cursor_resource.hpp"
#include <map>
#include <stdexcept>
namespace cm::aot {
ColorCursor parse_monochrome_cursor(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != 68) throw std::runtime_error("Invalid original monochrome cursor size");
    ColorCursor cursor{};
    cursor.y = std::uint16_t(bytes[64]) << 8 | bytes[65];
    cursor.x = std::uint16_t(bytes[66]) << 8 | bytes[67];
    if (cursor.x >= 16 || cursor.y >= 16) throw std::runtime_error("Invalid original cursor hotspot");
    for (unsigned i = 0; i < 256; ++i) {
        const bool data = bytes[i / 8] & (0x80 >> (i % 8));
        const bool mask = bytes[32 + i / 8] & (0x80 >> (i % 8));
        if (data && !mask) throw std::runtime_error("Inverting original cursors require a native compositing extension");
        for (unsigned c = 0; c < 3; ++c) cursor.rgba[i * 4 + c] = data ? 0 : 255;
        cursor.rgba[i * 4 + 3] = mask ? 255 : 0;
    }
    return cursor;
}
ColorCursor parse_color_cursor(std::span<const std::uint8_t> bytes) {
    auto read = [&](std::uint64_t at, unsigned size) {
        if (at > bytes.size() || size > bytes.size() - at) throw std::runtime_error("Truncated original color cursor");
        std::uint32_t value = 0;
        for (unsigned i = 0; i < size; ++i) value = (value << 8) | bytes[at + i];
        return value;
    };
    if (bytes.size() < 96 || read(0, 2) != 0x8001) throw std::runtime_error("Unsupported original color cursor type");
    ColorCursor cursor;
    cursor.map_offset = read(2, 4); cursor.pixels_offset = read(6, 4);
    cursor.y = read(84, 2); cursor.x = read(86, 2);
    const std::uint64_t map = cursor.map_offset;
    if (map < 96 || map + 50 > bytes.size() || cursor.x >= 16 || cursor.y >= 16)
        throw std::runtime_error("Invalid original cursor metadata");
    const auto stride = read(map + 4, 2) & 0x3fff;
    if (read(map + 6, 2) || read(map + 8, 2) || read(map + 10, 2) != 16 || read(map + 12, 2) != 16 ||
        read(map + 16, 2) || read(map + 30, 2) || read(map + 34, 2) != 1)
        throw std::runtime_error("Unsupported original cursor PixMap geometry or storage");
    const auto depth = read(map + 32, 2);
    if ((depth != 1 && depth != 2 && depth != 4 && depth != 8) || stride < 16 * depth / 8)
        throw std::runtime_error("Unsupported original cursor palette depth");
    cursor.pixel_bytes = stride * 16;
    cursor.palette_offset = read(map + 42, 4);
    const std::uint64_t table = cursor.palette_offset;
    const auto flags = read(table + 4, 2), count = read(table + 6, 2) + 1;
    if (count > 256) throw std::runtime_error("Excessive original cursor color count");
    cursor.palette_bytes = 8 + count * 8;
    if (cursor.pixels_offset < map + 50 || std::uint64_t(cursor.pixels_offset) + cursor.pixel_bytes > bytes.size() ||
        table < std::uint64_t(cursor.pixels_offset) + cursor.pixel_bytes || table + cursor.palette_bytes != bytes.size())
        throw std::runtime_error("Invalid original cursor data offsets");
    std::map<std::uint32_t, std::array<std::uint8_t, 3>> palette;
    for (unsigned i = 0; i < count; ++i) {
        const auto at = table + 8 + i * 8;
        const auto index = (flags & 0x8000) ? i : read(at, 2);
        std::array<std::uint8_t, 3> color;
        for (unsigned c = 0; c < 3; ++c) color[c] = (read(at + 2 + c * 2, 2) + 128) / 257;
        if (!palette.emplace(index, color).second) throw std::runtime_error("Duplicate original cursor color index");
    }
    for (unsigned y = 0; y < 16; ++y) for (unsigned x = 0; x < 16; ++x) {
        const auto bit = x * depth;
        const auto index = (read(cursor.pixels_offset + y * stride + bit / 8, 1) >> (8 - depth - bit % 8)) & ((1u << depth) - 1);
        const auto found = palette.find(index);
        if (found == palette.end()) throw std::runtime_error("Original cursor references a missing color");
        const auto at = (y * 16 + x) * 4;
        for (unsigned c = 0; c < 3; ++c) cursor.rgba[at + c] = found->second[c];
        cursor.rgba[at + 3] = read(52 + y * 2 + x / 8, 1) & (0x80 >> (x % 8)) ? 255 : 0;
    }
    return cursor;
}
}
