#include "shortlist_quotation_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ShortlistQuotationDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(shortlist_quotation_diagnostics(s).original_body) {
        shortlist_quotation_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { shortlist_quotation_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ShortlistQuotationOutcome outcome) {
    auto& d = shortlist_quotation_diagnostics(s);
    if (!original) {
        if (outcome == ShortlistQuotationOutcome::returned) ++d.returns[index];
        else if (outcome == ShortlistQuotationOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ShortlistQuotationOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ShortlistQuotationOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, shortlist_quotation::entries[index], original, outcome);
}
}
bool try_modern_shortlist_quotation(State& s, std::uint32_t entry) {
    const auto found = std::find(shortlist_quotation::entries.begin(), shortlist_quotation::entries.end(), entry);
    if (found == shortlist_quotation::entries.end()) throw std::runtime_error("Unknown shortlist-quotation entry");
    const auto index = unsigned(found - shortlist_quotation::entries.begin()); s.pc = entry;
    auto& d = shortlist_quotation_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original shortlist-quotation body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{shortlist_quotation::evaluate_candidate_context,shortlist_quotation::calculate_reference_quote};
            ShortlistQuotationDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ShortlistQuotationOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ShortlistQuotationOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ShortlistQuotationOutcome::budget_stop : ShortlistQuotationOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ShortlistQuotationOutcome::fault); throw; }
    leave(s, index, original, ShortlistQuotationOutcome::returned); return true;
}
}
