#include "contract_draft_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ContractDraftDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(contract_draft_diagnostics(s).original_body) {
        contract_draft_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { contract_draft_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ContractDraftOutcome outcome) {
    auto& d = contract_draft_diagnostics(s);
    if (!original) {
        if (outcome == ContractDraftOutcome::returned) ++d.returns[index];
        else if (outcome == ContractDraftOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ContractDraftOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ContractDraftOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, contract_draft::entries[index], original, outcome);
}
}
bool try_modern_contract_draft(State& s, std::uint32_t entry) {
    const auto found = std::find(contract_draft::entries.begin(), contract_draft::entries.end(), entry);
    if (found == contract_draft::entries.end()) throw std::runtime_error("Unknown contract-draft entry");
    const auto index = unsigned(found - contract_draft::entries.begin()); s.pc = entry;
    auto& d = contract_draft_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original contract-draft body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{contract_draft::initialize_draft,contract_draft::prepare_new_contract,contract_draft::prepare_from_primary_contract};
            ContractDraftDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ContractDraftOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ContractDraftOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ContractDraftOutcome::budget_stop : ContractDraftOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ContractDraftOutcome::fault); throw; }
    leave(s, index, original, ContractDraftOutcome::returned); return true;
}
}
