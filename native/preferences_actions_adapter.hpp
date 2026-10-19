#pragma once
#include "preferences_actions.hpp"

namespace cm::aot {
enum class PreferencesActionsOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct PreferencesActionsBoundaryObserver : SemanticGuestObserver {
    virtual ~PreferencesActionsBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, PreferencesActionsOutcome) = 0;
};
struct PreferencesActionsDiagnostics {
    bool original = false;
    std::array<bool, preferences_actions::entries.size()> entry_original{};
    std::array<std::uint64_t, preferences_actions::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    PreferencesActionsBoundaryObserver* observer = nullptr;
};
PreferencesActionsDiagnostics& preferences_actions_diagnostics(State&);
const PreferencesActionsDiagnostics& preferences_actions_diagnostics(const State&);
bool try_modern_preferences_actions(State&, std::uint32_t);
}
