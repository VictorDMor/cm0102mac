#include "localization_catalog_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct LocalizationCatalogDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(localization_catalog_diagnostics(s).original_body) {
        localization_catalog_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { localization_catalog_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, LocalizationCatalogOutcome outcome) {
    auto& d = localization_catalog_diagnostics(s);
    if (!original) {
        if (outcome == LocalizationCatalogOutcome::returned) ++d.returns[index];
        else if (outcome == LocalizationCatalogOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == LocalizationCatalogOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == LocalizationCatalogOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, localization_catalog::entries[index], original, outcome);
}
}
bool try_modern_localization_catalog(State& s, std::uint32_t entry) {
    const auto found = std::find(localization_catalog::entries.begin(), localization_catalog::entries.end(), entry);
    if (found == localization_catalog::entries.end()) throw std::runtime_error("Unknown localization_catalog entry");
    const auto index = unsigned(found - localization_catalog::entries.begin()); s.pc = entry;
    auto& d = localization_catalog_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original localization_catalog body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{localization_catalog::release_catalog,localization_catalog::destroy_record,localization_catalog::language_availability,localization_catalog::initialize_catalog};
            LocalizationCatalogDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, LocalizationCatalogOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, LocalizationCatalogOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? LocalizationCatalogOutcome::budget_stop : LocalizationCatalogOutcome::fault); throw;
    } catch (...) { leave(s, index, original, LocalizationCatalogOutcome::fault); throw; }
    leave(s, index, original, LocalizationCatalogOutcome::returned); return true;
}
}
