#include "match_selection_support_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(match_selection_support_diagnostics(s).original_body) {
        match_selection_support_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { match_selection_support_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, MatchSelectionSupportOutcome outcome) {
    auto& d = match_selection_support_diagnostics(s);
    if (!original) {
        if (outcome == MatchSelectionSupportOutcome::returned) ++d.returns[index];
        else if (outcome == MatchSelectionSupportOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == MatchSelectionSupportOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == MatchSelectionSupportOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, match_selection_support::entries[index], original, outcome);
}
}
bool try_modern_match_selection_support(State& s, std::uint32_t entry) {
    const auto found = std::find(match_selection_support::entries.begin(), match_selection_support::entries.end(), entry);
    if (found == match_selection_support::entries.end()) throw std::runtime_error("Unknown match-selection-support entry");
    const auto index = unsigned(found - match_selection_support::entries.begin()); s.pc = entry;
    auto& d = match_selection_support_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original match-selection-support body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{match_selection_support::select_context_weighted_actor,match_selection_support::select_score_weighted_actor,match_selection_support::actor_selection_score};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, MatchSelectionSupportOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, MatchSelectionSupportOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? MatchSelectionSupportOutcome::budget_stop : MatchSelectionSupportOutcome::fault); throw;
    } catch (...) { leave(s, index, original, MatchSelectionSupportOutcome::fault); throw; }
    leave(s, index, original, MatchSelectionSupportOutcome::returned); return true;
}
}
