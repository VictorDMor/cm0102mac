#include "actor_action_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(actor_action_diagnostics(s).original_body) {
        actor_action_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { actor_action_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ActorActionOutcome outcome) {
    auto& d = actor_action_diagnostics(s);
    if (!original) {
        if (outcome == ActorActionOutcome::returned) ++d.returns[index];
        else if (outcome == ActorActionOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ActorActionOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ActorActionOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, actor_action::entries[index], original, outcome);
}
}
bool try_modern_actor_action(State& s, std::uint32_t entry) {
    const auto found = std::find(actor_action::entries.begin(), actor_action::entries.end(), entry);
    if (found == actor_action::entries.end()) throw std::runtime_error("Unknown actor-action entry");
    const auto index = unsigned(found - actor_action::entries.begin()); s.pc = entry;
    auto& d = actor_action_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original actor-action body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{actor_action::route_actor_action,actor_action::select_primary_action,actor_action::can_reach_destination,actor_action::choose_turn_side,actor_action::select_secondary_action};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ActorActionOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ActorActionOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ActorActionOutcome::budget_stop : ActorActionOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ActorActionOutcome::fault); throw; }
    leave(s, index, original, ActorActionOutcome::returned); return true;
}
}
