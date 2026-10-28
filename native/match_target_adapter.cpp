#include "match_target_adapter.hpp"
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
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(match_target_diagnostics(s).original_body) {
        match_target_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { match_target_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, MatchTargetOutcome outcome) {
    auto& d = match_target_diagnostics(s);
    if (!original) {
        if (outcome == MatchTargetOutcome::returned) ++d.returns[index];
        else if (outcome == MatchTargetOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == MatchTargetOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == MatchTargetOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, match_target::entries[index], original, outcome);
}
}
bool try_modern_match_target(State& s, std::uint32_t entry) {
    const auto found = std::find(match_target::entries.begin(), match_target::entries.end(), entry);
    if (found == match_target::entries.end()) throw std::runtime_error("Unknown match-target entry");
    const auto index = unsigned(found - match_target::entries.begin()); s.pc = entry;
    auto& d = match_target_diagnostics(s);
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
            constexpr Function methods[]{match_target::target_advance_enabled,match_target::resolve_target_cell,match_target::side_target_control,match_target::normalized_actor_side,match_target::is_nearest_candidate,match_target::controlled_target_override,match_target::uncontrolled_target_override,match_target::strategy_flags};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, MatchTargetOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, MatchTargetOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? MatchTargetOutcome::budget_stop : MatchTargetOutcome::fault); throw;
    } catch (...) { leave(s, index, original, MatchTargetOutcome::fault); throw; }
    leave(s, index, original, MatchTargetOutcome::returned); return true;
}
}
