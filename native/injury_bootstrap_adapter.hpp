#pragma once
#include "injury_bootstrap.hpp"

namespace cm::aot {
enum class InjuryBootstrapOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct InjuryBootstrapBoundaryObserver : SemanticGuestObserver {
    virtual ~InjuryBootstrapBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, InjuryBootstrapOutcome) = 0;
};
struct InjuryBootstrapDiagnostics {
    bool original = false;
    std::array<bool, injury_bootstrap::entries.size()> entry_original{};
    std::array<std::uint64_t, injury_bootstrap::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    InjuryBootstrapBoundaryObserver* observer = nullptr;
};
InjuryBootstrapDiagnostics& injury_bootstrap_diagnostics(State&);
const InjuryBootstrapDiagnostics& injury_bootstrap_diagnostics(const State&);
bool try_modern_injury_bootstrap(State&, std::uint32_t);
}
