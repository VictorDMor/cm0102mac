#include "actor_construction_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(actor_construction_diagnostics(s).original_body) {
        actor_construction_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { actor_construction_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ActorConstructionOutcome outcome) {
    auto& d = actor_construction_diagnostics(s);
    if (!original) {
        if (outcome == ActorConstructionOutcome::returned) ++d.returns[index];
        else if (outcome == ActorConstructionOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ActorConstructionOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ActorConstructionOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, actor_construction::entries[index], original, outcome);
}
}
bool try_modern_actor_construction(State& s, std::uint32_t entry) {
    const auto found = std::find(actor_construction::entries.begin(), actor_construction::entries.end(), entry);
    if (found == actor_construction::entries.end()) throw std::runtime_error("Unknown actor-construction entry");
    const auto index = unsigned(found - actor_construction::entries.begin()); s.pc = entry;
    auto& d = actor_construction_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original actor-construction body");
            if (s.pc != caller) run_until(s, caller);
        } else if (index == 0) actor_construction::prepare_actor(s);
        else actor_construction::initialize_actor(s);
    } catch (const ApplicationExit&) { leave(s, index, original, ActorConstructionOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ActorConstructionOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ActorConstructionOutcome::budget_stop : ActorConstructionOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ActorConstructionOutcome::fault); throw; }
    leave(s, index, original, ActorConstructionOutcome::returned); return true;
}
}
