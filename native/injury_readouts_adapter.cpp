#include "injury_readouts_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ReadoutsDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(injury_readouts_diagnostics(s).original_body) {
        injury_readouts_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { injury_readouts_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, InjuryReadoutsOutcome outcome) {
    auto& d = injury_readouts_diagnostics(s);
    if (!original) {
        if (outcome == InjuryReadoutsOutcome::returned) ++d.returns[index];
        else if (outcome == InjuryReadoutsOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == InjuryReadoutsOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == InjuryReadoutsOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, injury_readouts::entries[index], original, outcome);
}
}
bool try_modern_injury_readouts(State& s, std::uint32_t entry) {
    const auto found = std::find(injury_readouts::entries.begin(), injury_readouts::entries.end(), entry);
    if (found == injury_readouts::entries.end()) throw std::runtime_error("Unknown injury-readouts entry");
    const auto index = unsigned(found - injury_readouts::entries.begin()); s.pc = entry;
    auto& d = injury_readouts_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original injury-readouts body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{injury_readouts::find_history_record,injury_readouts::risk_multiplier,injury_readouts::risk_type,injury_readouts::describe_current_injury,injury_readouts::describe_injury_type,injury_readouts::injury_type_form,injury_readouts::describe_remaining_time,injury_readouts::describe_injury_category};
            ReadoutsDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, InjuryReadoutsOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, InjuryReadoutsOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? InjuryReadoutsOutcome::budget_stop : InjuryReadoutsOutcome::fault); throw;
    } catch (...) { leave(s, index, original, InjuryReadoutsOutcome::fault); throw; }
    leave(s, index, original, InjuryReadoutsOutcome::returned); return true;
}
}
