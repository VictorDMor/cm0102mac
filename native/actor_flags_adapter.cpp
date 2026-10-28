#include "actor_flags_adapter.hpp"
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
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(actor_flags_diagnostics(s).original_body) {
        actor_flags_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { actor_flags_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ActorFlagsOutcome outcome) {
    auto& d = actor_flags_diagnostics(s);
    if (!original) {
        if (outcome == ActorFlagsOutcome::returned) ++d.returns[index];
        else if (outcome == ActorFlagsOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ActorFlagsOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ActorFlagsOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, actor_flags::entries[index], original, outcome);
}
}
bool try_modern_actor_flags(State& s, std::uint32_t entry) {
    const auto found = std::find(actor_flags::entries.begin(), actor_flags::entries.end(), entry);
    if (found == actor_flags::entries.end()) throw std::runtime_error("Unknown actor-flags entry");
    const auto index = unsigned(found - actor_flags::entries.begin()); s.pc = entry;
    auto& d = actor_flags_diagnostics(s);
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
            constexpr Function methods[]{actor_flags::test_bit21,actor_flags::test_bit25,actor_flags::test_bit29,actor_flags::test_bit27,actor_flags::read_flags,actor_flags::read_flags_alternate,actor_flags::team_or_slot_status};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ActorFlagsOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ActorFlagsOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ActorFlagsOutcome::budget_stop : ActorFlagsOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ActorFlagsOutcome::fault); throw; }
    leave(s, index, original, ActorFlagsOutcome::returned); return true;
}
}
