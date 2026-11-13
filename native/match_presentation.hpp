#pragma once
#include "match_turbo.hpp"
#include "match_speed.hpp"
#include <cstdint>
#include <cstdio>

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

// The commentary routine 0x102021e0 keeps its event's table row (the event
// code minus 8000) at sp+0x206c and reads that row's line table (+8) at its
// end without the checks its text path makes. True when there is no line to
// read: the row is outside the 0x2a5-row table at TOC-0x5298 (an empty event
// has code 0), or its line table is null. The original read such a line from
// page zero. Said once per run, since the fault it replaces was the only
// trace of a match screen drawing an event that has no lines.
inline bool commentary_event_without_lines(const State& state) {
    const auto row = std::int16_t(state.memory.read<2>(std::uint32_t(state.r<4>(4) + 0x206c)));
    const auto in_table = row >= 0 && row < 0x2a5;
    if (in_table) {
        const auto table = std::uint32_t(state.memory.read<4>(std::uint32_t(state.r<4>(8) - 0x5298)));
        if (state.memory.read<4>(table + std::uint32_t(row) * 0x14 + 8) != 0) return false;
    }
    static bool said = false;
    if (!said) {
        said = true;
        std::fprintf(stderr, "Commentary event without lines (table row %d, %s): no sound, pace 0\n",
                     int(row), in_table ? "null line table" : "outside the table");
    }
    return true;
}

}
