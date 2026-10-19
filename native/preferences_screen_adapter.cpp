#include "preferences_screen_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct PreferencesScreenDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(preferences_screen_diagnostics(s).original_body) {
        preferences_screen_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { preferences_screen_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, PreferencesScreenOutcome outcome) {
    auto& d = preferences_screen_diagnostics(s);
    if (!original) {
        if (outcome == PreferencesScreenOutcome::returned) ++d.returns[index];
        else if (outcome == PreferencesScreenOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == PreferencesScreenOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == PreferencesScreenOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, preferences_screen::entries[index], original, outcome);
}
}
bool try_modern_preferences_screen(State& s, std::uint32_t entry) {
    const auto found = std::find(preferences_screen::entries.begin(), preferences_screen::entries.end(), entry);
    if (found == preferences_screen::entries.end()) throw std::runtime_error("Unknown preferences_screen entry");
    const auto index = unsigned(found - preferences_screen::entries.begin()); s.pc = entry;
    auto& d = preferences_screen_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original preferences_screen body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{preferences_screen::build_screen};
            PreferencesScreenDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, PreferencesScreenOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, PreferencesScreenOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? PreferencesScreenOutcome::budget_stop : PreferencesScreenOutcome::fault); throw;
    } catch (...) { leave(s, index, original, PreferencesScreenOutcome::fault); throw; }
    leave(s, index, original, PreferencesScreenOutcome::returned); return true;
}
}
