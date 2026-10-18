#include "transfer_record_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct TransferRecordDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(transfer_record_diagnostics(s).original_body) {
        transfer_record_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { transfer_record_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, TransferRecordOutcome outcome) {
    auto& d = transfer_record_diagnostics(s);
    if (!original) {
        if (outcome == TransferRecordOutcome::returned) ++d.returns[index];
        else if (outcome == TransferRecordOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == TransferRecordOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == TransferRecordOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, transfer_records::entries[index], original, outcome);
}
}
bool try_modern_transfer_record(State& s, std::uint32_t entry) {
    const auto found = std::find(transfer_records::entries.begin(), transfer_records::entries.end(), entry);
    if (found == transfer_records::entries.end()) throw std::runtime_error("Unknown transfer-record entry");
    const auto index = unsigned(found - transfer_records::entries.begin()); s.pc = entry;
    auto& d = transfer_record_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original transfer-record body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{transfer_records::construct_empty_offer,transfer_records::construct_generated_offer,transfer_records::destroy_offer,transfer_records::read_offer,transfer_records::write_offer,transfer_records::initialize_zero_fee_offer,transfer_records::initialize_priced_offer,transfer_records::offer_value};
            TransferRecordDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, TransferRecordOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, TransferRecordOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? TransferRecordOutcome::budget_stop : TransferRecordOutcome::fault); throw;
    } catch (...) { leave(s, index, original, TransferRecordOutcome::fault); throw; }
    leave(s, index, original, TransferRecordOutcome::returned); return true;
}
}
