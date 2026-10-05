#pragma once
#include <cstdint>

namespace cm::rng {
inline constexpr std::uint32_t multiplier = 0x41c64e6d;
inline constexpr std::uint32_t increment = 0x3039;
inline constexpr std::uint32_t pool_samples = 51000;
constexpr std::uint32_t next_state(std::uint32_t state) {
    return state * multiplier + increment;
}
constexpr std::uint32_t sample(std::uint32_t state) {
    return (state >> 16) & 0x7fff;
}
}
