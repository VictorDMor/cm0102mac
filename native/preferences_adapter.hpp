#pragma once
#include "preferences.hpp"

namespace cm::aot {
enum class PreferencesOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct PreferencesBoundaryObserver : SemanticGuestObserver {
    virtual ~PreferencesBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, PreferencesOutcome) = 0;
};
struct PreferencesDiagnostics {
    bool original = false;
    std::array<bool, preferences::entries.size()> entry_original{};
    std::array<std::uint64_t, preferences::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    PreferencesBoundaryObserver* observer = nullptr;
};
PreferencesDiagnostics& preferences_diagnostics(State&);
const PreferencesDiagnostics& preferences_diagnostics(const State&);
bool try_modern_preferences(State&, std::uint32_t);
}
