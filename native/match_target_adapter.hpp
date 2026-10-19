#pragma once
#include "match_target.hpp"

namespace cm::aot {
enum class MatchTargetOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MatchTargetBoundaryObserver : SemanticGuestObserver {
    virtual ~MatchTargetBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MatchTargetOutcome) = 0;
};
struct MatchTargetDiagnostics {
    bool original = false;
    std::array<bool, match_target::entries.size()> entry_original{};
    std::array<std::uint64_t, match_target::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MatchTargetBoundaryObserver* observer = nullptr;
};
MatchTargetDiagnostics& match_target_diagnostics(State&);
const MatchTargetDiagnostics& match_target_diagnostics(const State&);
bool try_modern_match_target(State&, std::uint32_t);
}
