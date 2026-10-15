#pragma once
#include "ball_action.hpp"

namespace cm::aot {
enum class BallActionOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct BallActionBoundaryObserver : SemanticGuestObserver {
    virtual ~BallActionBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, BallActionOutcome) = 0;
};
struct BallActionDiagnostics {
    bool original = false;
    std::array<bool,1> entry_original{};
    std::array<std::uint64_t,1> calls{},original_calls{},returns{},faults{},budget_stops{},transfers{},terminations{},profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    BallActionBoundaryObserver* observer = nullptr;
};
BallActionDiagnostics& ball_action_diagnostics(State&);
const BallActionDiagnostics& ball_action_diagnostics(const State&);
bool try_modern_ball_action(State&, std::uint32_t);
}
