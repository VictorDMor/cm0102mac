#pragma once
#include "actor_encounter.hpp"

namespace cm::aot {
enum class ActorEncounterOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ActorEncounterBoundaryObserver : SemanticGuestObserver {
    virtual ~ActorEncounterBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ActorEncounterOutcome) = 0;
};
struct ActorEncounterDiagnostics {
    bool original = false;
    std::array<bool, actor_encounter::entries.size()> entry_original{};
    std::array<std::uint64_t, actor_encounter::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ActorEncounterBoundaryObserver* observer = nullptr;
};
ActorEncounterDiagnostics& actor_encounter_diagnostics(State&);
const ActorEncounterDiagnostics& actor_encounter_diagnostics(const State&);
bool try_modern_actor_encounter(State&, std::uint32_t);
}
