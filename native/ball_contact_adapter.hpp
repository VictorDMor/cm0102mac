#pragma once
#include "ball_contact.hpp"

namespace cm::aot {
enum class BallContactOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct BallContactBoundaryObserver : SemanticGuestObserver {
    virtual ~BallContactBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, BallContactOutcome) = 0;
};
struct BallContactDiagnostics {
    bool original = false;
    std::array<bool,4> entry_original{};
    std::array<std::uint64_t,4> calls{},original_calls{},returns{},faults{},budget_stops{},transfers{},terminations{},profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    BallContactBoundaryObserver* observer = nullptr;
};
BallContactDiagnostics& ball_contact_diagnostics(State&);
const BallContactDiagnostics& ball_contact_diagnostics(const State&);
bool try_modern_ball_contact(State&, std::uint32_t);
}
