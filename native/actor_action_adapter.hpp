#pragma once
#include "actor_action.hpp"

namespace cm::aot {
enum class ActorActionOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ActorActionBoundaryObserver : SemanticGuestObserver {
    virtual ~ActorActionBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ActorActionOutcome) = 0;
};
struct ActorActionDiagnostics {
    bool original = false;
    std::array<bool, actor_action::entries.size()> entry_original{};
    std::array<std::uint64_t, actor_action::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ActorActionBoundaryObserver* observer = nullptr;
};
ActorActionDiagnostics& actor_action_diagnostics(State&);
const ActorActionDiagnostics& actor_action_diagnostics(const State&);
bool try_modern_actor_action(State&, std::uint32_t);
}
