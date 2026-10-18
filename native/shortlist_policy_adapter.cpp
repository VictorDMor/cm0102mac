#include "shortlist_policy_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ShortlistPolicyDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(shortlist_policy_diagnostics(s).original_body) {
        shortlist_policy_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { shortlist_policy_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ShortlistPolicyOutcome outcome) {
    auto& d = shortlist_policy_diagnostics(s);
    if (!original) {
        if (outcome == ShortlistPolicyOutcome::returned) ++d.returns[index];
        else if (outcome == ShortlistPolicyOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ShortlistPolicyOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ShortlistPolicyOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, shortlist_policy::entries[index], original, outcome);
}
}
bool try_modern_shortlist_policy(State& s, std::uint32_t entry) {
    const auto found = std::find(shortlist_policy::entries.begin(), shortlist_policy::entries.end(), entry);
    if (found == shortlist_policy::entries.end()) throw std::runtime_error("Unknown shortlist-policy entry");
    const auto index = unsigned(found - shortlist_policy::entries.begin()); s.pc = entry;
    auto& d = shortlist_policy_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original shortlist-policy body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{shortlist_policy::classify_contract_offer,shortlist_policy::check_candidate_country_policy,shortlist_policy::evaluate_row_contract_gate};
            ShortlistPolicyDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ShortlistPolicyOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ShortlistPolicyOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ShortlistPolicyOutcome::budget_stop : ShortlistPolicyOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ShortlistPolicyOutcome::fault); throw; }
    leave(s, index, original, ShortlistPolicyOutcome::returned); return true;
}
}
