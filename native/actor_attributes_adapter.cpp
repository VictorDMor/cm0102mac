#include "actor_attributes_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct AttributeDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t target, std::uint32_t continuation) override {
        // The maximum leaf has no dependency or memory callbacks. Keep the
        // general dispatcher for observed, original, profiled or budgeted calls.
        const auto& d = actor_attributes_diagnostics(state);
        if (target == actor_attributes::maximum_entry && !d.observer &&
            !d.original && !d.entry_original[1] && !d.original_body &&
            !match_pc_profile_enabled() && state.executed <= state.budget &&
            state.budget - state.executed >= 5) {
            if (try_modern_actor_attributes(state, target)) return;
        }
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(actor_attributes_diagnostics(s).original_body) {
        actor_attributes_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { actor_attributes_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ActorAttributesOutcome outcome) {
    auto& d = actor_attributes_diagnostics(s);
    if (!original) {
        if (outcome == ActorAttributesOutcome::returned) ++d.returns[index];
        else if (outcome == ActorAttributesOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ActorAttributesOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ActorAttributesOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, actor_attributes::entries[index], original, outcome);
}
}
bool try_modern_actor_attributes(State& s, std::uint32_t entry) {
    const auto found = std::find(actor_attributes::entries.begin(), actor_attributes::entries.end(), entry);
    if (found == actor_attributes::entries.end()) throw std::runtime_error("Unknown actor-attributes entry");
    const auto index = unsigned(found - actor_attributes::entries.begin()); s.pc = entry;
    auto& d = actor_attributes_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original actor-attributes body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{actor_attributes::refresh, actor_attributes::signed_maximum, actor_attributes::recover_condition};
            AttributeDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ActorAttributesOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ActorAttributesOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ActorAttributesOutcome::budget_stop : ActorAttributesOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ActorAttributesOutcome::fault); throw; }
    leave(s, index, original, ActorAttributesOutcome::returned); return true;
}
}
