#pragma once
#include <cstdint>

namespace cm::aot {
// Zero is the new presentation choice. The five original values stay intact.
inline constexpr std::uint16_t turbo_match_speed = 0;
constexpr bool valid_match_speed(std::uint16_t value) {
    return value == 0 || value == 20 || value == 55 || value == 90 || value == 125 || value == 150;
}
bool native_match_speed_enabled();
std::uint16_t native_match_speed();
void initialize_native_match_speed(std::uint16_t value = turbo_match_speed);
void select_native_match_speed(std::uint16_t value);
// Turbo uses the original maximum-speed simulation cadence. Only commentary
// presentation waits are shortened; match ticks and sound barriers are intact.
inline std::uint32_t match_simulation_speed(std::uint32_t value) {
    return native_match_speed_enabled() && value == turbo_match_speed ? 20 : value;
}
struct State;
bool append_turbo_match_speed(State&);
}
