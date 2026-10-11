#pragma once
#include "match_engine.hpp"

namespace cm::aot {
enum class MatchEngineOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MatchEngineBoundaryObserver {
    virtual ~MatchEngineBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MatchEngineOutcome) = 0;
};
struct MatchEngineDiagnostics {
    bool original = false;
    std::array<bool, 3> entry_original{};
    std::array<std::uint64_t, 3> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MatchEngineBoundaryObserver* observer = nullptr;
};
MatchEngineDiagnostics& match_engine_diagnostics(State&);
const MatchEngineDiagnostics& match_engine_diagnostics(const State&);
bool try_modern_match_engine(State&, std::uint32_t);
void reset_match_engine_diagnostics(State&);
std::string match_engine_call_summary(const State&);
}
