#pragma once
#include "injury_daily.hpp"

namespace cm::aot {
enum class InjuryDailyOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct InjuryDailyBoundaryObserver : SemanticGuestObserver {
    virtual ~InjuryDailyBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, InjuryDailyOutcome) = 0;
};
struct InjuryDailyDiagnostics {
    bool original = false;
    std::array<bool, injury_daily::entries.size()> entry_original{};
    std::array<std::uint64_t, injury_daily::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    InjuryDailyBoundaryObserver* observer = nullptr;
};
InjuryDailyDiagnostics& injury_daily_diagnostics(State&);
const InjuryDailyDiagnostics& injury_daily_diagnostics(const State&);
bool try_modern_injury_daily(State&, std::uint32_t);
}
