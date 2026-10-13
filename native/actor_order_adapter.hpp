#pragma once
#include "actor_order.hpp"

namespace cm::aot {
enum class ActorOrderOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ActorOrderBoundaryObserver : SemanticGuestObserver {
    virtual ~ActorOrderBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ActorOrderOutcome) = 0;
};
struct ActorOrderDiagnostics {
    bool original = false;
    std::array<bool, 5> entry_original{};
    std::array<std::uint64_t, 5> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ActorOrderBoundaryObserver* observer = nullptr;
};
ActorOrderDiagnostics& actor_order_diagnostics(State&);
const ActorOrderDiagnostics& actor_order_diagnostics(const State&);
bool try_modern_actor_order(State&, std::uint32_t);
}
