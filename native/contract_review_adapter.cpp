#include "contract_review_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ContractReviewDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(contract_review_diagnostics(s).original_body) {
        contract_review_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { contract_review_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ContractReviewOutcome outcome) {
    auto& d = contract_review_diagnostics(s);
    if (!original) {
        if (outcome == ContractReviewOutcome::returned) ++d.returns[index];
        else if (outcome == ContractReviewOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ContractReviewOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ContractReviewOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, contract_review::entries[index], original, outcome);
}
}
bool try_modern_contract_review(State& s, std::uint32_t entry) {
    const auto found = std::find(contract_review::entries.begin(), contract_review::entries.end(), entry);
    if (found == contract_review::entries.end()) throw std::runtime_error("Unknown contract-review entry");
    const auto index = unsigned(found - contract_review::entries.begin()); s.pc = entry;
    auto& d = contract_review_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original contract-review body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{contract_review::reset_review_state,contract_review::has_review_trigger,contract_review::refresh_review_conditions,contract_review::clear_review_condition,contract_review::clear_review_conditions,contract_review::add_status_bits,contract_review::clear_status_bits,contract_review::reset_status_bits,contract_review::status_bits,contract_review::apply_review_conditions,contract_review::describe_squad_status,contract_review::describe_release_summary,contract_review::query_start_date_window};
            ContractReviewDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ContractReviewOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ContractReviewOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ContractReviewOutcome::budget_stop : ContractReviewOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ContractReviewOutcome::fault); throw; }
    leave(s, index, original, ContractReviewOutcome::returned); return true;
}
}
