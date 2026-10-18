#include "transfer_synthesis_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct TransferSynthesisDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(transfer_synthesis_diagnostics(s).original_body) {
        transfer_synthesis_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { transfer_synthesis_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, TransferSynthesisOutcome outcome) {
    auto& d = transfer_synthesis_diagnostics(s);
    if (!original) {
        if (outcome == TransferSynthesisOutcome::returned) ++d.returns[index];
        else if (outcome == TransferSynthesisOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == TransferSynthesisOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == TransferSynthesisOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, transfer_synthesis::entries[index], original, outcome);
}
}
bool try_modern_transfer_synthesis(State& s, std::uint32_t entry) {
    const auto found = std::find(transfer_synthesis::entries.begin(), transfer_synthesis::entries.end(), entry);
    if (found == transfer_synthesis::entries.end()) throw std::runtime_error("Unknown transfer-synthesis entry");
    const auto index = unsigned(found - transfer_synthesis::entries.begin()); s.pc = entry;
    auto& d = transfer_synthesis_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original transfer-synthesis body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{transfer_synthesis::synthesize_offer};
            TransferSynthesisDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, TransferSynthesisOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, TransferSynthesisOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? TransferSynthesisOutcome::budget_stop : TransferSynthesisOutcome::fault); throw;
    } catch (...) { leave(s, index, original, TransferSynthesisOutcome::fault); throw; }
    leave(s, index, original, TransferSynthesisOutcome::returned); return true;
}
}
