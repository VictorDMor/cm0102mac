#pragma once
#include "pitch_distance.hpp"

namespace cm::aot {
enum class PitchDistanceOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct PitchDistanceBoundaryObserver : SemanticGuestObserver {
    virtual ~PitchDistanceBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, PitchDistanceOutcome) = 0;
};
struct PitchDistanceDiagnostics {
    bool original = false;
    std::array<bool, 3> entry_original{};
    std::array<std::uint64_t, 3> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    PitchDistanceBoundaryObserver* observer = nullptr;
};
PitchDistanceDiagnostics& pitch_distance_diagnostics(State&);
const PitchDistanceDiagnostics& pitch_distance_diagnostics(const State&);
bool try_modern_pitch_distance(State&, std::uint32_t);
}
