#pragma once
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace cm::aot {
struct State;
// Host preference: never stored in original preferences or career records.
inline constexpr bool default_new_screens = true;
inline constexpr unsigned new_screens_action = 30032;
bool new_screens_available();
bool new_screens_enabled();
void initialize_new_screens(const std::filesystem::path& preference = {});
void select_new_screens(bool enabled);
std::string_view new_screens_label(unsigned language);
void begin_new_screens_options();
void append_new_screens_option(State&);
bool dispatch_new_screens_option(State&, unsigned action);
void finish_new_screens_options(unsigned action, std::int32_t result);
}
