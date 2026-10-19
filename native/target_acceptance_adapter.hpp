#pragma once
#include "target_acceptance.hpp"

namespace cm::aot {
enum class TargetAcceptanceOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct TargetAcceptanceBoundaryObserver : SemanticGuestObserver {
    virtual ~TargetAcceptanceBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, TargetAcceptanceOutcome) = 0;
};
struct TargetAcceptanceDiagnostics {
    bool original = false;
    std::array<bool, target_acceptance::entries.size()> entry_original{};
    std::array<std::uint64_t, target_acceptance::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    TargetAcceptanceBoundaryObserver* observer = nullptr;
};
TargetAcceptanceDiagnostics& target_acceptance_diagnostics(State&);
const TargetAcceptanceDiagnostics& target_acceptance_diagnostics(const State&);
bool try_modern_target_acceptance(State&, std::uint32_t);
}
