#include "actor_encounter_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(actor_encounter_diagnostics(s).original_body) {
        actor_encounter_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { actor_encounter_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ActorEncounterOutcome outcome) {
    auto& d = actor_encounter_diagnostics(s);
    if (!original) {
        if (outcome == ActorEncounterOutcome::returned) ++d.returns[index];
        else if (outcome == ActorEncounterOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ActorEncounterOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ActorEncounterOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, actor_encounter::entries[index], original, outcome);
}
}
bool try_modern_actor_encounter(State& s, std::uint32_t entry) {
    const auto found = std::find(actor_encounter::entries.begin(), actor_encounter::entries.end(), entry);
    if (found == actor_encounter::entries.end()) throw std::runtime_error("Unknown actor-encounter entry");
    const auto index = unsigned(found - actor_encounter::entries.begin()); s.pc = entry;
    auto& d = actor_encounter_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original actor-encounter body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{actor_encounter::scale_team_scores,actor_encounter::scale_actor_score,actor_encounter::accept_response,actor_encounter::resolve_peer};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ActorEncounterOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ActorEncounterOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ActorEncounterOutcome::budget_stop : ActorEncounterOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ActorEncounterOutcome::fault); throw; }
    leave(s, index, original, ActorEncounterOutcome::returned); return true;
}
}
