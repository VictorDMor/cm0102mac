#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::time_runtime {
inline constexpr std::array<std::uint32_t, 8> entries{
    0x1077f6d0, 0x1077f6e0, 0x1077f760, 0x1077f8c0,
    0x1077f900, 0x1077f940, 0x1077f9a0, 0x1077f9e0};
struct Execution {
    std::uint64_t budget_resumes = 0;
    void (*resume_original)(State&, std::uint32_t, std::uint32_t) = nullptr;
    void (*run_to)(State&, std::uint32_t) = nullptr;
};
void zero_status(State&, Execution* = nullptr);
void leap_year_offset(State&, Execution* = nullptr);
void calendar_fields(State&, Execution* = nullptr);
void initialize_fields(State&, Execution* = nullptr);
void time_value(State&, Execution* = nullptr);
void local_fields(State&, Execution* = nullptr);
void mac_clock_value(State&, Execution* = nullptr);
void daylight_status(State&, Execution* = nullptr);
void invoke(State&, unsigned, Execution* = nullptr);
}
