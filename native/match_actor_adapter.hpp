#pragma once
#include "match_actor.hpp"

namespace cm::aot {
enum class MatchActorOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MatchActorBoundaryObserver {
    virtual ~MatchActorBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MatchActorOutcome) = 0;
};
struct MatchActorDiagnostics {
    bool original = false;
    std::array<bool, 3> entry_original{};
    std::array<std::uint64_t, 3> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MatchActorBoundaryObserver* observer = nullptr;
};
MatchActorDiagnostics& match_actor_diagnostics(State&);
const MatchActorDiagnostics& match_actor_diagnostics(const State&);
bool try_modern_match_actor(State&, std::uint32_t);
}
