#pragma once
#include <cstdint>

namespace cm::aot {
// Zero is the new presentation choice. The five original values stay intact.
inline constexpr std::uint16_t turbo_match_speed = 0;
// The fastest original speed: Turbo's simulation cadence and its saved value.
inline constexpr std::uint16_t turbo_simulation_speed = 20;
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
    return native_match_speed_enabled() && value == turbo_match_speed ? turbo_simulation_speed : value;
}
struct State;
// The career writer saves the manager record's speed field, and no original
// save holds a zero there (the default is 125, the menu gives 20 to 150). With
// the native speed on, a zero in that field, from choosing Turbo or from a
// career saved before 2026-11-14, becomes the speed Turbo simulates at. The
// choice itself lives in the native preference, which the getter returns
// whatever the record holds. Called after the setter's store and after the
// career loader's read of the field.
void keep_original_match_speed(State&, std::uint32_t field);
void insert_turbo_match_speed(State&);
bool finish_turbo_match_speed_menu(State&);
}
