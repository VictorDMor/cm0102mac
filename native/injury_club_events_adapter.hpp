#pragma once
#include "injury_club_events.hpp"

namespace cm::aot {
enum class InjuryClubEventsOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct InjuryClubEventsBoundaryObserver : SemanticGuestObserver {
    virtual ~InjuryClubEventsBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, InjuryClubEventsOutcome) = 0;
};
struct InjuryClubEventsDiagnostics {
    bool original = false;
    std::array<bool, injury_club_events::entries.size()> entry_original{};
    std::array<std::uint64_t, injury_club_events::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    InjuryClubEventsBoundaryObserver* observer = nullptr;
};
InjuryClubEventsDiagnostics& injury_club_events_diagnostics(State&);
const InjuryClubEventsDiagnostics& injury_club_events_diagnostics(const State&);
bool try_modern_injury_club_events(State&, std::uint32_t);
}
