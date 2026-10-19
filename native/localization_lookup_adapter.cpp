#include "localization_lookup_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct LocalizationLookupDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(localization_lookup_diagnostics(s).original_body) {
        localization_lookup_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { localization_lookup_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, LocalizationLookupOutcome outcome) {
    auto& d = localization_lookup_diagnostics(s);
    if (!original) {
        if (outcome == LocalizationLookupOutcome::returned) ++d.returns[index];
        else if (outcome == LocalizationLookupOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == LocalizationLookupOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == LocalizationLookupOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, localization_lookup::entries[index], original, outcome);
}
}
bool try_modern_localization_lookup(State& s, std::uint32_t entry) {
    const auto found = std::find(localization_lookup::entries.begin(), localization_lookup::entries.end(), entry);
    if (found == localization_lookup::entries.end()) throw std::runtime_error("Unknown localization_lookup entry");
    const auto index = unsigned(found - localization_lookup::entries.begin()); s.pc = entry;
    auto& d = localization_lookup_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original localization_lookup body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{localization_lookup::initialize_record,localization_lookup::current_language,localization_lookup::lookup};
            LocalizationLookupDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, LocalizationLookupOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, LocalizationLookupOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? LocalizationLookupOutcome::budget_stop : LocalizationLookupOutcome::fault); throw;
    } catch (...) { leave(s, index, original, LocalizationLookupOutcome::fault); throw; }
    leave(s, index, original, LocalizationLookupOutcome::returned); return true;
}
}
