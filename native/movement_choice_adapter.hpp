#pragma once
#include "movement_choice.hpp"

namespace cm::aot {
enum class MovementChoiceOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MovementChoiceBoundaryObserver : SemanticGuestObserver {
    virtual ~MovementChoiceBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MovementChoiceOutcome) = 0;
};
struct MovementChoiceDiagnostics {
    bool original = false;
    std::array<bool, 4> entry_original{};
    std::array<std::uint64_t, 4> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MovementChoiceBoundaryObserver* observer = nullptr;
};
MovementChoiceDiagnostics& movement_choice_diagnostics(State&);
const MovementChoiceDiagnostics& movement_choice_diagnostics(const State&);
bool try_modern_movement_choice(State&, std::uint32_t);
}
