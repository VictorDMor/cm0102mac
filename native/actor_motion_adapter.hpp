#pragma once
#include "actor_motion.hpp"

namespace cm::aot {
enum class ActorMotionOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ActorMotionBoundaryObserver : SemanticGuestObserver {
    virtual ~ActorMotionBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ActorMotionOutcome) = 0;
};
struct ActorMotionDiagnostics {
    bool original = false;
    std::array<bool, 5> entry_original{};
    std::array<std::uint64_t, 5> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ActorMotionBoundaryObserver* observer = nullptr;
};
ActorMotionDiagnostics& actor_motion_diagnostics(State&);
const ActorMotionDiagnostics& actor_motion_diagnostics(const State&);
bool try_modern_actor_motion(State&, std::uint32_t);
}
