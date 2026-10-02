#pragma once
#include "match_turbo.hpp"

namespace cm::aot {

// Only the local duration loaded at Mac 0x10203738 is shortened. The original
// millisecond clock, stored start/duration, sound state, and match stop flags
// remain under original-code control. Speed 20 is the game's Very Fast setting.
inline std::uint32_t commentary_presentation_duration(
        const State& state, std::uint32_t commentary, std::uint32_t duration) {
    if (!match_turbo_enabled()) return duration;
    if (state.memory.read<1>(std::uint32_t(commentary + 0x4070)) != 20)
        return duration;
    constexpr std::uint32_t turbo_duration_ms = 20;
    return duration > turbo_duration_ms ? turbo_duration_ms : duration;
}

}
