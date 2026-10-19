#pragma once
#include "actor_flags.hpp"

namespace cm::aot {
enum class ActorFlagsOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ActorFlagsBoundaryObserver : SemanticGuestObserver {
    virtual ~ActorFlagsBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ActorFlagsOutcome) = 0;
};
struct ActorFlagsDiagnostics {
    bool original = false;
    std::array<bool, actor_flags::entries.size()> entry_original{};
    std::array<std::uint64_t, actor_flags::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ActorFlagsBoundaryObserver* observer = nullptr;
};
ActorFlagsDiagnostics& actor_flags_diagnostics(State&);
const ActorFlagsDiagnostics& actor_flags_diagnostics(const State&);
bool try_modern_actor_flags(State&, std::uint32_t);
}
