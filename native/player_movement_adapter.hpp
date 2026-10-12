#pragma once
#include "player_movement.hpp"

namespace cm::aot {
enum class PlayerMovementOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct PlayerMovementBoundaryObserver : SemanticGuestObserver {
    virtual ~PlayerMovementBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, PlayerMovementOutcome) = 0;
};
struct PlayerMovementDiagnostics {
    bool original = false;
    std::array<bool,1> entry_original{};
    std::array<std::uint64_t,1> calls{},original_calls{},returns{},faults{},budget_stops{},transfers{},terminations{},profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    PlayerMovementBoundaryObserver* observer = nullptr;
};
PlayerMovementDiagnostics& player_movement_diagnostics(State&);
const PlayerMovementDiagnostics& player_movement_diagnostics(const State&);
bool try_modern_player_movement(State&, std::uint32_t);
}
