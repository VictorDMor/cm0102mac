#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>
namespace cm::aot {
using LegacyRect = std::array<std::int16_t, 4>; // top, left, bottom, right
std::vector<std::uint8_t> region_union(std::span<const LegacyRect> rectangles);
bool region_contains(std::span<const std::uint8_t> bytes, std::int16_t x, std::int16_t y);
}
