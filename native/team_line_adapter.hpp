#pragma once
#include "team_line.hpp"

namespace cm::aot {
enum class TeamLineOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct TeamLineBoundaryObserver : SemanticGuestObserver {
    virtual ~TeamLineBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, TeamLineOutcome) = 0;
};
struct TeamLineDiagnostics {
    bool original = false;
    std::array<bool, 5> entry_original{};
    std::array<std::uint64_t, 5> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    TeamLineBoundaryObserver* observer = nullptr;
};
TeamLineDiagnostics& team_line_diagnostics(State&);
const TeamLineDiagnostics& team_line_diagnostics(const State&);
bool try_modern_team_line(State&, std::uint32_t);
}
