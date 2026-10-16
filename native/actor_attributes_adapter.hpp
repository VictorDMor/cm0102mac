#pragma once
#include "actor_attributes.hpp"

namespace cm::aot {
enum class ActorAttributesOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ActorAttributesBoundaryObserver : SemanticGuestObserver {
    virtual ~ActorAttributesBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ActorAttributesOutcome) = 0;
};
struct ActorAttributesDiagnostics {
    bool original = false;
    std::array<bool, 2> entry_original{};
    std::array<std::uint64_t, 2> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ActorAttributesBoundaryObserver* observer = nullptr;
};
ActorAttributesDiagnostics& actor_attributes_diagnostics(State&);
const ActorAttributesDiagnostics& actor_attributes_diagnostics(const State&);
bool try_modern_actor_attributes(State&, std::uint32_t);
}
