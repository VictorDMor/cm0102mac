#include "actor_motion_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(actor_motion_diagnostics(s).original_body) {
        actor_motion_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { actor_motion_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ActorMotionOutcome outcome) {
    auto& d = actor_motion_diagnostics(s);
    if (!original) {
        if (outcome == ActorMotionOutcome::returned) ++d.returns[index];
        else if (outcome == ActorMotionOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ActorMotionOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ActorMotionOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, actor_motion::entries[index], original, outcome);
}
}
bool try_modern_actor_motion(State& s, std::uint32_t entry) {
    const auto found = std::find(actor_motion::entries.begin(), actor_motion::entries.end(), entry);
    if (found == actor_motion::entries.end()) throw std::runtime_error("Unknown actor-motion entry");
    const auto index = unsigned(found - actor_motion::entries.begin()); s.pc = entry;
    auto& d = actor_motion_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original actor-motion body");
            if (s.pc != caller) run_until(s, caller);
        } else actor_motion::invoke(s, actor_motion::Entry(index), nullptr, d.observer);
    } catch (const ApplicationExit&) { leave(s, index, original, ActorMotionOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ActorMotionOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ActorMotionOutcome::budget_stop : ActorMotionOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ActorMotionOutcome::fault); throw; }
    leave(s, index, original, ActorMotionOutcome::returned); return true;
}
}
