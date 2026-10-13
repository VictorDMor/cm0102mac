#pragma once
#include "team_spatial.hpp"

namespace cm::aot {
enum class TeamSpatialOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct TeamSpatialBoundaryObserver : SemanticGuestObserver {
    virtual ~TeamSpatialBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, TeamSpatialOutcome) = 0;
};
struct TeamSpatialDiagnostics {
    bool original = false;
    std::array<bool, 4> entry_original{};
    std::array<std::uint64_t, 4> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    TeamSpatialBoundaryObserver* observer = nullptr;
};
TeamSpatialDiagnostics& team_spatial_diagnostics(State&);
const TeamSpatialDiagnostics& team_spatial_diagnostics(const State&);
bool try_modern_team_spatial(State&, std::uint32_t);
}
