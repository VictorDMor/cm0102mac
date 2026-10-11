#pragma once
#include "aot_runtime.hpp"
namespace cm::aot {
inline constexpr std::array<std::uint32_t, 8> time_runtime_entries{0x1077f6d0, 0x1077f6e0, 0x1077f760, 0x1077f8c0, 0x1077f900, 0x1077f940, 0x1077f9a0, 0x1077f9e0};
enum class TimeRuntimeOutcome { returned, transferred, fault, budget_return };
struct TimeRuntimeBoundaryObserver {
    virtual ~TimeRuntimeBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, TimeRuntimeOutcome) = 0;
};
bool try_modern_time_runtime(State&, std::uint32_t);
void reset_time_runtime_diagnostics(State&);
std::string time_runtime_call_summary(const State&);
}
