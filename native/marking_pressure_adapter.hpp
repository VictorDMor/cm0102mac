#pragma once
#include "marking_pressure.hpp"

namespace cm::aot {
enum class MarkingPressureOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MarkingPressureBoundaryObserver {
    virtual ~MarkingPressureBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MarkingPressureOutcome) = 0;
};
struct MarkingPressureDiagnostics {
    bool original = false;
    std::array<bool, 13> entry_original{};
    std::array<std::uint64_t, 13> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MarkingPressureBoundaryObserver* observer = nullptr;
};
MarkingPressureDiagnostics& marking_pressure_diagnostics(State&);
const MarkingPressureDiagnostics& marking_pressure_diagnostics(const State&);
bool try_modern_marking_pressure(State&, std::uint32_t);
}
