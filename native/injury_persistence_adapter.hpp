#pragma once
#include "injury_persistence.hpp"

namespace cm::aot {
enum class InjuryPersistenceOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct InjuryPersistenceBoundaryObserver : SemanticGuestObserver {
    virtual ~InjuryPersistenceBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, InjuryPersistenceOutcome) = 0;
};
struct InjuryPersistenceDiagnostics {
    bool original = false;
    std::array<bool, injury_persistence::entries.size()> entry_original{};
    std::array<std::uint64_t, injury_persistence::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    InjuryPersistenceBoundaryObserver* observer = nullptr;
};
InjuryPersistenceDiagnostics& injury_persistence_diagnostics(State&);
const InjuryPersistenceDiagnostics& injury_persistence_diagnostics(const State&);
bool try_modern_injury_persistence(State&, std::uint32_t);
}
