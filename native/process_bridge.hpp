#pragma once
#include <cstdint>
#include <span>
namespace cm::aot {
struct NativeHardwareValue { std::int16_t error; std::uint32_t value; };
NativeHardwareValue native_hardware_value(std::uint32_t selector);
struct NativeProcess {
    std::int16_t error = 0;
    std::uint32_t high = 0, low = 0, type = 0, signature = 0, mode = 0;
    std::uint32_t launcher_high = 0, launcher_low = 0, launch_date = 0, active_time = 0;
};
NativeProcess current_native_process();
NativeProcess native_process_information(std::uint32_t high, std::uint32_t low);
std::uint32_t native_local_date_time();
std::uint32_t native_tick_count();
std::uint64_t native_microseconds();
double native_extended80(std::span<const std::uint8_t> bytes);
std::int32_t native_fixed(double value);
struct NativeMachineLocation {
    std::uint32_t latitude, longitude, gmt_and_daylight;
};
NativeMachineLocation native_machine_location();
}
