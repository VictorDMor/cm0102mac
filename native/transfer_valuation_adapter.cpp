#include "transfer_valuation_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct TransferValuationDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(transfer_valuation_diagnostics(s).original_body) {
        transfer_valuation_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { transfer_valuation_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, TransferValuationOutcome outcome) {
    auto& d = transfer_valuation_diagnostics(s);
    if (!original) {
        if (outcome == TransferValuationOutcome::returned) ++d.returns[index];
        else if (outcome == TransferValuationOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == TransferValuationOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == TransferValuationOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, transfer_valuation::entries[index], original, outcome);
}
}
bool try_modern_transfer_valuation(State& s, std::uint32_t entry) {
    const auto found = std::find(transfer_valuation::entries.begin(), transfer_valuation::entries.end(), entry);
    if (found == transfer_valuation::entries.end()) throw std::runtime_error("Unknown transfer-valuation entry");
    const auto index = unsigned(found - transfer_valuation::entries.begin()); s.pc = entry;
    auto& d = transfer_valuation_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original transfer-valuation body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{transfer_valuation::sample_offer_range,transfer_valuation::adjusted_offer_value,transfer_valuation::positive_installment_total,transfer_valuation::staff_offer_status,transfer_valuation::equivalent_offer_terms,transfer_valuation::remove_exchange_staff,transfer_valuation::find_exchange_staff,transfer_valuation::rebalance_offer_terms};
            TransferValuationDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, TransferValuationOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, TransferValuationOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? TransferValuationOutcome::budget_stop : TransferValuationOutcome::fault); throw;
    } catch (...) { leave(s, index, original, TransferValuationOutcome::fault); throw; }
    leave(s, index, original, TransferValuationOutcome::returned); return true;
}
}
