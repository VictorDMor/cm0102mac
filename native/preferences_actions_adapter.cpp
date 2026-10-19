#include "preferences_actions_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct PreferencesActionsDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(preferences_actions_diagnostics(s).original_body) {
        preferences_actions_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { preferences_actions_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, PreferencesActionsOutcome outcome) {
    auto& d = preferences_actions_diagnostics(s);
    if (!original) {
        if (outcome == PreferencesActionsOutcome::returned) ++d.returns[index];
        else if (outcome == PreferencesActionsOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == PreferencesActionsOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == PreferencesActionsOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, preferences_actions::entries[index], original, outcome);
}
}
bool try_modern_preferences_actions(State& s, std::uint32_t entry) {
    const auto found = std::find(preferences_actions::entries.begin(), preferences_actions::entries.end(), entry);
    if (found == preferences_actions::entries.end()) throw std::runtime_error("Unknown preferences_actions entry");
    const auto index = unsigned(found - preferences_actions::entries.begin()); s.pc = entry;
    auto& d = preferences_actions_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original preferences_actions body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{preferences_actions::open_with_mode,preferences_actions::open_from_game_state,preferences_actions::dispatch_action};
            PreferencesActionsDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, PreferencesActionsOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, PreferencesActionsOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? PreferencesActionsOutcome::budget_stop : PreferencesActionsOutcome::fault); throw;
    } catch (...) { leave(s, index, original, PreferencesActionsOutcome::fault); throw; }
    leave(s, index, original, PreferencesActionsOutcome::returned); return true;
}
}
