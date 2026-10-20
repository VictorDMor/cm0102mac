#pragma once
#include "match_turbo.hpp"
#include "match_speed.hpp"

namespace cm::aot {

// Only the local duration loaded at Mac 0x10203738 is shortened. The original
// millisecond clock, stored start/duration, sound state, and match stop flags
// remain under original-code control. The native UI caps only its new Turbo
// choice; legacy headless comparisons retain the former speed-20 opt-in.
inline std::uint32_t commentary_presentation_duration(
        const State& state, std::uint32_t commentary, std::uint32_t duration) {
    if (!match_turbo_enabled()) return duration;
    const auto speed = state.memory.read<1>(std::uint32_t(commentary + 0x4070));
    if (native_match_speed_enabled() ? speed != turbo_match_speed : speed != 20)
        return duration;
    constexpr std::uint32_t turbo_duration_ms = 20;
    return duration > turbo_duration_ms ? turbo_duration_ms : duration;
}

}
