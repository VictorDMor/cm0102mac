#include "preferences_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct PreferencesDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(preferences_diagnostics(s).original_body) {
        preferences_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { preferences_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, PreferencesOutcome outcome) {
    auto& d = preferences_diagnostics(s);
    if (!original) {
        if (outcome == PreferencesOutcome::returned) ++d.returns[index];
        else if (outcome == PreferencesOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == PreferencesOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == PreferencesOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, preferences::entries[index], original, outcome);
}
}
bool try_modern_preferences(State& s, std::uint32_t entry) {
    const auto found = std::find(preferences::entries.begin(), preferences::entries.end(), entry);
    if (found == preferences::entries.end()) throw std::runtime_error("Unknown preferences entry");
    const auto index = unsigned(found - preferences::entries.begin()); s.pc = entry;
    auto& d = preferences_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original preferences body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{preferences::initialize_preferences,preferences::copy_preferences,preferences::destroy_preferences,preferences::set_defaults,preferences::select_option,preferences::set_language,preferences::set_currency,preferences::currency_for_language,preferences::set_interval,preferences::apply_preferences,preferences::video_mode,preferences::set_video_mode};
            PreferencesDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, PreferencesOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, PreferencesOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? PreferencesOutcome::budget_stop : PreferencesOutcome::fault); throw;
    } catch (...) { leave(s, index, original, PreferencesOutcome::fault); throw; }
    leave(s, index, original, PreferencesOutcome::returned); return true;
}
}
