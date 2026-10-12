#pragma once
#include "player_decision.hpp"

namespace cm::aot {
enum class PlayerDecisionOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct PlayerDecisionBoundaryObserver : SemanticGuestObserver {
    virtual ~PlayerDecisionBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, PlayerDecisionOutcome) = 0;
};
struct PlayerDecisionDiagnostics {
    bool original = false;
    std::array<bool, 1> entry_original{};
    std::array<std::uint64_t, 1> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    PlayerDecisionBoundaryObserver* observer = nullptr;
};
PlayerDecisionDiagnostics& player_decision_diagnostics(State&);
const PlayerDecisionDiagnostics& player_decision_diagnostics(const State&);
bool try_modern_player_decision(State&, std::uint32_t);
}
