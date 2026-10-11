#pragma once
#include "utility_object.hpp"

namespace cm::aot {
enum class UtilityObjectOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct UtilityObjectBoundaryObserver {
    virtual ~UtilityObjectBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, UtilityObjectOutcome) = 0;
};
struct UtilityObjectDiagnostics {
    bool original = false;
    std::array<bool, 11> entry_original{};
    std::array<std::uint64_t, 11> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    UtilityObjectBoundaryObserver* observer = nullptr;
};
UtilityObjectDiagnostics& utility_object_diagnostics(State&);
const UtilityObjectDiagnostics& utility_object_diagnostics(const State&);
bool try_modern_utility_object(State&, std::uint32_t);
void reset_utility_object_diagnostics(State&);
std::string utility_object_call_summary(const State&);
}
