#include "actor_policy_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(actor_policy_diagnostics(s).original_body) {
        actor_policy_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { actor_policy_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ActorPolicyOutcome outcome) {
    auto& d = actor_policy_diagnostics(s);
    if (!original) {
        if (outcome == ActorPolicyOutcome::returned) ++d.returns[index];
        else if (outcome == ActorPolicyOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ActorPolicyOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ActorPolicyOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, actor_policy::entries[index], original, outcome);
}
}
bool try_modern_actor_policy(State& s, std::uint32_t entry) {
    const auto found = std::find(actor_policy::entries.begin(), actor_policy::entries.end(), entry);
    if (found == actor_policy::entries.end()) throw std::runtime_error("Unknown actor-policy entry");
    const auto index = unsigned(found - actor_policy::entries.begin()); s.pc = entry;
    auto& d = actor_policy_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original actor-policy body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{actor_policy::context_control,actor_policy::column_excess,actor_policy::phase_marker,actor_policy::adjust_action_bias,actor_policy::find_linked_opponent};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ActorPolicyOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ActorPolicyOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ActorPolicyOutcome::budget_stop : ActorPolicyOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ActorPolicyOutcome::fault); throw; }
    leave(s, index, original, ActorPolicyOutcome::returned); return true;
}
}
