#include "localization_format_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct LocalizationFormatDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(localization_format_diagnostics(s).original_body) {
        localization_format_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { localization_format_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, LocalizationFormatOutcome outcome) {
    auto& d = localization_format_diagnostics(s);
    if (!original) {
        if (outcome == LocalizationFormatOutcome::returned) ++d.returns[index];
        else if (outcome == LocalizationFormatOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == LocalizationFormatOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == LocalizationFormatOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, localization_format::entries[index], original, outcome);
}
}
bool try_modern_localization_format(State& s, std::uint32_t entry) {
    const auto found = std::find(localization_format::entries.begin(), localization_format::entries.end(), entry);
    if (found == localization_format::entries.end()) throw std::runtime_error("Unknown localization_format entry");
    const auto index = unsigned(found - localization_format::entries.begin()); s.pc = entry;
    auto& d = localization_format_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original localization_format body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{localization_format::format_phrase};
            LocalizationFormatDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, LocalizationFormatOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, LocalizationFormatOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? LocalizationFormatOutcome::budget_stop : LocalizationFormatOutcome::fault); throw;
    } catch (...) { leave(s, index, original, LocalizationFormatOutcome::fault); throw; }
    leave(s, index, original, LocalizationFormatOutcome::returned); return true;
}
}
