#include "contract_query_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ContractQueryDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(contract_query_diagnostics(s).original_body) {
        contract_query_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { contract_query_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ContractQueryOutcome outcome) {
    auto& d = contract_query_diagnostics(s);
    if (!original) {
        if (outcome == ContractQueryOutcome::returned) ++d.returns[index];
        else if (outcome == ContractQueryOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ContractQueryOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ContractQueryOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, contract_queries::entries[index], original, outcome);
}
}
bool try_modern_contract_query(State& s, std::uint32_t entry) {
    const auto found = std::find(contract_queries::entries.begin(), contract_queries::entries.end(), entry);
    if (found == contract_queries::entries.end()) throw std::runtime_error("Unknown contract-query entry");
    const auto index = unsigned(found - contract_queries::entries.begin()); s.pc = entry;
    auto& d = contract_query_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else {
        ++d.calls[index];
        if (!d.observer) {
            if (const auto run = contract_queries::run_native(s, entry); run.ran) {
                ++d.returns[index];
                if (run.nested_secondary) { ++d.calls[2]; ++d.returns[2]; }   // as through its hook
                return true;
            }
        }
    }
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original contract-query body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{contract_queries::find_primary,contract_queries::find_for_club,contract_queries::find_secondary};
            ContractQueryDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ContractQueryOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ContractQueryOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ContractQueryOutcome::budget_stop : ContractQueryOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ContractQueryOutcome::fault); throw; }
    leave(s, index, original, ContractQueryOutcome::returned); return true;
}
}
