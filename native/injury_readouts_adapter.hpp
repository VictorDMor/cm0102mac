#pragma once
#include "injury_readouts.hpp"

namespace cm::aot {
enum class InjuryReadoutsOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct InjuryReadoutsBoundaryObserver : SemanticGuestObserver {
    virtual ~InjuryReadoutsBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, InjuryReadoutsOutcome) = 0;
};
struct InjuryReadoutsDiagnostics {
    bool original = false;
    std::array<bool, injury_readouts::entries.size()> entry_original{};
    std::array<std::uint64_t, injury_readouts::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    InjuryReadoutsBoundaryObserver* observer = nullptr;
};
InjuryReadoutsDiagnostics& injury_readouts_diagnostics(State&);
const InjuryReadoutsDiagnostics& injury_readouts_diagnostics(const State&);
bool try_modern_injury_readouts(State&, std::uint32_t);
}
