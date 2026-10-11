#pragma once
#include "engine_initialization.hpp"

namespace cm::aot {
enum class EngineInitializationOutcome { returned, fault, budget_stop, context_transfer };
struct EngineInitializationBoundaryObserver {
    virtual ~EngineInitializationBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, EngineInitializationOutcome) = 0;
};
struct EngineInitializationDiagnostics {
    bool original = false;
    std::array<bool, 20> entry_original{};
    std::array<std::uint64_t, 20> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    EngineInitializationBoundaryObserver* observer = nullptr;
};
EngineInitializationDiagnostics& engine_initialization_diagnostics(State&);
const EngineInitializationDiagnostics& engine_initialization_diagnostics(const State&);
bool try_modern_engine_initialization(State&, std::uint32_t);
void reset_engine_initialization_diagnostics(State&);
std::string engine_initialization_call_summary(const State&);
}
