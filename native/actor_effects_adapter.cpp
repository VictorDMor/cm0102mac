#include "actor_effects_adapter.hpp"
#include "match_pl_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(actor_effects_diagnostics(s).original_body) {
        actor_effects_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { actor_effects_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ActorEffectsOutcome outcome) {
    auto& d = actor_effects_diagnostics(s);
    if (!original) {
        if (outcome == ActorEffectsOutcome::returned) ++d.returns[index];
        else if (outcome == ActorEffectsOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ActorEffectsOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ActorEffectsOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, actor_effects::entries[index], original, outcome);
}
}
bool try_modern_actor_effects(State& s, std::uint32_t entry) {
    const auto found = std::find(actor_effects::entries.begin(), actor_effects::entries.end(), entry);
    if (found == actor_effects::entries.end()) throw std::runtime_error("Unknown actor-effects entry");
    const auto index = unsigned(found - actor_effects::entries.begin()); s.pc = entry;
    auto& d = actor_effects_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            OriginalBody selected(s, entry);
            match_pl_run_original(s, entry);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{actor_effects::update_balance,actor_effects::add_counter,actor_effects::assign_counter};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ActorEffectsOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ActorEffectsOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ActorEffectsOutcome::budget_stop : ActorEffectsOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ActorEffectsOutcome::fault); throw; }
    leave(s, index, original, ActorEffectsOutcome::returned); return true;
}
}
