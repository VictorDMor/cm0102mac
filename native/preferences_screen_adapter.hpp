#pragma once
#include "preferences_screen.hpp"

namespace cm::aot {
enum class PreferencesScreenOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct PreferencesScreenBoundaryObserver : SemanticGuestObserver {
    virtual ~PreferencesScreenBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, PreferencesScreenOutcome) = 0;
};
struct PreferencesScreenDiagnostics {
    bool original = false;
    std::array<bool, preferences_screen::entries.size()> entry_original{};
    std::array<std::uint64_t, preferences_screen::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    PreferencesScreenBoundaryObserver* observer = nullptr;
};
PreferencesScreenDiagnostics& preferences_screen_diagnostics(State&);
const PreferencesScreenDiagnostics& preferences_screen_diagnostics(const State&);
bool try_modern_preferences_screen(State&, std::uint32_t);
}
