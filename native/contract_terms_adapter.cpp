#include "contract_terms_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ContractTermsDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(contract_terms_diagnostics(s).original_body) {
        contract_terms_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { contract_terms_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ContractTermsOutcome outcome) {
    auto& d = contract_terms_diagnostics(s);
    if (!original) {
        if (outcome == ContractTermsOutcome::returned) ++d.returns[index];
        else if (outcome == ContractTermsOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ContractTermsOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ContractTermsOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, contract_terms::entries[index], original, outcome);
}
}
bool try_modern_contract_terms(State& s, std::uint32_t entry) {
    const auto found = std::find(contract_terms::entries.begin(), contract_terms::entries.end(), entry);
    if (found == contract_terms::entries.end()) throw std::runtime_error("Unknown contract-terms entry");
    const auto index = unsigned(found - contract_terms::entries.begin()); s.pc = entry;
    auto& d = contract_terms_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original contract-terms body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{contract_terms::sample_contract_range,contract_terms::enable_option,contract_terms::set_bonus,contract_terms::clear_option,contract_terms::clear_bonus,contract_terms::reset_terms,contract_terms::describe_bonus,contract_terms::describe_option,contract_terms::is_expired,contract_terms::end_date_delta,contract_terms::is_rolling,contract_terms::describe_end,contract_terms::bonus_name,contract_terms::option_name,contract_terms::supports_bonus,contract_terms::supports_option,contract_terms::has_release_terms,contract_terms::adjust_value_primary,contract_terms::adjust_value_alternate,contract_terms::set_release_amount,contract_terms::expiry_band};
            ContractTermsDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ContractTermsOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ContractTermsOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ContractTermsOutcome::budget_stop : ContractTermsOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ContractTermsOutcome::fault); throw; }
    leave(s, index, original, ContractTermsOutcome::returned); return true;
}
}
