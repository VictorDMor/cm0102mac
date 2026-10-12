#pragma once
#include "actor_construction.hpp"

namespace cm::aot {
enum class ActorConstructionOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ActorConstructionBoundaryObserver {
    virtual ~ActorConstructionBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ActorConstructionOutcome) = 0;
};
struct ActorConstructionDiagnostics {
    bool original = false;
    std::array<bool, 2> entry_original{};
    std::array<std::uint64_t, 2> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ActorConstructionBoundaryObserver* observer = nullptr;
};
ActorConstructionDiagnostics& actor_construction_diagnostics(State&);
const ActorConstructionDiagnostics& actor_construction_diagnostics(const State&);
bool try_modern_actor_construction(State&, std::uint32_t);
}
