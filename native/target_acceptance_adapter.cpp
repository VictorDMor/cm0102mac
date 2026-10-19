#include "target_acceptance_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(target_acceptance_diagnostics(s).original_body) {
        target_acceptance_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { target_acceptance_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, TargetAcceptanceOutcome outcome) {
    auto& d = target_acceptance_diagnostics(s);
    if (!original) {
        if (outcome == TargetAcceptanceOutcome::returned) ++d.returns[index];
        else if (outcome == TargetAcceptanceOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == TargetAcceptanceOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == TargetAcceptanceOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, target_acceptance::entries[index], original, outcome);
}
}
bool try_modern_target_acceptance(State& s, std::uint32_t entry) {
    const auto found = std::find(target_acceptance::entries.begin(), target_acceptance::entries.end(), entry);
    if (found == target_acceptance::entries.end()) throw std::runtime_error("Unknown target-acceptance entry");
    const auto index = unsigned(found - target_acceptance::entries.begin()); s.pc = entry;
    auto& d = target_acceptance_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original target-acceptance body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{target_acceptance::can_face_target,target_acceptance::target_position_allowed,target_acceptance::route_obstruction,target_acceptance::cell_occupants};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, TargetAcceptanceOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, TargetAcceptanceOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? TargetAcceptanceOutcome::budget_stop : TargetAcceptanceOutcome::fault); throw;
    } catch (...) { leave(s, index, original, TargetAcceptanceOutcome::fault); throw; }
    leave(s, index, original, TargetAcceptanceOutcome::returned); return true;
}
}
