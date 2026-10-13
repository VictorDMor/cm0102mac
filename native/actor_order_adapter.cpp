#include "actor_order_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(actor_order_diagnostics(s).original_body) {
        actor_order_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { actor_order_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ActorOrderOutcome outcome) {
    auto& d = actor_order_diagnostics(s);
    if (!original) {
        if (outcome == ActorOrderOutcome::returned) ++d.returns[index];
        else if (outcome == ActorOrderOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ActorOrderOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ActorOrderOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, actor_order::entries[index], original, outcome);
}
}
bool try_modern_actor_order(State& s, std::uint32_t entry) {
    const auto found = std::find(actor_order::entries.begin(), actor_order::entries.end(), entry);
    if (found == actor_order::entries.end()) throw std::runtime_error("Unknown actor-order entry");
    const auto index = unsigned(found - actor_order::entries.begin()); s.pc = entry;
    auto& d = actor_order_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original actor-order body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{actor_order::secondary_rating, actor_order::primary_rating, actor_order::build_queue, actor_order::compare_actors, actor_order::current_fitness};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ActorOrderOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ActorOrderOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ActorOrderOutcome::budget_stop : ActorOrderOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ActorOrderOutcome::fault); throw; }
    leave(s, index, original, ActorOrderOutcome::returned); return true;
}
}
