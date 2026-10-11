#pragma once
#include "aggregate_object.hpp"

namespace cm::aot {
enum class AggregateObjectOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct AggregateObjectBoundaryObserver {
    virtual ~AggregateObjectBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, AggregateObjectOutcome) = 0;
};
struct AggregateObjectDiagnostics {
    bool original = false;
    std::array<bool, 6> entry_original{};
    std::array<std::uint64_t, 6> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    AggregateObjectBoundaryObserver* observer = nullptr;
};
AggregateObjectDiagnostics& aggregate_object_diagnostics(State&);
const AggregateObjectDiagnostics& aggregate_object_diagnostics(const State&);
bool try_modern_aggregate_object(State&, std::uint32_t);
void reset_aggregate_object_diagnostics(State&);
std::string aggregate_object_call_summary(const State&);
}
