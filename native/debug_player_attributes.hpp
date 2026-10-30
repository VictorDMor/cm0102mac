#pragma once
#include <array>
#include <cstdint>
#include <utility>

namespace cm::aot::debug_player_attributes {
// The profile getter decodes these signed technical bytes through current
// ability. The original conversion saturates at 20 for raw 100.
inline constexpr std::array technical{
    0x1e, 0x23, 0x24, 0x26, 0x27, 0x2a, 0x2b, 0x31, 0x32,
    0x33, 0x35, 0x37, 0x38, 0x39, 0x3a, 0x3e, 0x41, 0x43};
inline constexpr std::array positive{
    0x1b, 0x1c, 0x1d, 0x1f, 0x20, 0x21, 0x22, 0x28, 0x29, 0x2c,
    0x2e, 0x2f, 0x30, 0x34, 0x36, 0x3b, 0x3c, 0x3d, 0x3f, 0x40, 0x42, 0x44};
inline constexpr std::array negative{0x25, 0x2d}; // Dirtiness, injury proneness.
// Original staff initialization and database loading populate these eight
// personality bytes as a group; +0x62 between them is the club job.
inline constexpr std::array personality{0x61, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69};

template <class F> void for_each(std::uint32_t staff, std::uint32_t player, F&& apply) {
    for (auto offset : technical) apply(player + offset, std::uint8_t{100});
    for (auto offset : positive) apply(player + offset, std::uint8_t{20});
    for (auto offset : negative) apply(player + offset, std::uint8_t{1});
    for (auto offset : personality) apply(staff + offset, std::uint8_t{20});
}
}
