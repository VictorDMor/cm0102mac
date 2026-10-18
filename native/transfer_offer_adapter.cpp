#include "transfer_offer_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct TransferOfferDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(transfer_offer_diagnostics(s).original_body) {
        transfer_offer_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { transfer_offer_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, TransferOfferOutcome outcome) {
    auto& d = transfer_offer_diagnostics(s);
    if (!original) {
        if (outcome == TransferOfferOutcome::returned) ++d.returns[index];
        else if (outcome == TransferOfferOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == TransferOfferOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == TransferOfferOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, transfer_offer_queries::entries[index], original, outcome);
}
}
bool try_modern_transfer_offer(State& s, std::uint32_t entry) {
    const auto found = std::find(transfer_offer_queries::entries.begin(), transfer_offer_queries::entries.end(), entry);
    if (found == transfer_offer_queries::entries.end()) throw std::runtime_error("Unknown transfer-offer entry");
    const auto index = unsigned(found - transfer_offer_queries::entries.begin()); s.pc = entry;
    auto& d = transfer_offer_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original transfer-offer body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{transfer_offer_queries::append_staff_entry,transfer_offer_queries::count_club_staff_entries,transfer_offer_queries::contains_staff_entry,transfer_offer_queries::set_staff_contract_flag,transfer_offer_queries::clear_staff_contract_flag,transfer_offer_queries::query_staff_contract_flag,transfer_offer_queries::rejection_reason_from_squad_status,transfer_offer_queries::rejection_reason_allowed,transfer_offer_queries::describe_rejection_reason,transfer_offer_queries::squad_status_for_rejection_reason,transfer_offer_queries::collect_staff_offer_indices,transfer_offer_queries::collect_club_staff_offer_indices,transfer_offer_queries::collect_secondary_staff_offer_indices,transfer_offer_queries::collect_club_offer_indices,transfer_offer_queries::primary_offer_slot,transfer_offer_queries::secondary_offer_slot,transfer_offer_queries::best_qualifying_offer,transfer_offer_queries::query_special_staff_offer,transfer_offer_queries::offer_is_filtered};
            TransferOfferDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, TransferOfferOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, TransferOfferOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? TransferOfferOutcome::budget_stop : TransferOfferOutcome::fault); throw;
    } catch (...) { leave(s, index, original, TransferOfferOutcome::fault); throw; }
    leave(s, index, original, TransferOfferOutcome::returned); return true;
}
}
