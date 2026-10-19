#include "match_event_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(match_event_diagnostics(s).original_body) {
        match_event_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { match_event_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, MatchEventOutcome outcome) {
    auto& d = match_event_diagnostics(s);
    if (!original) {
        if (outcome == MatchEventOutcome::returned) ++d.returns[index];
        else if (outcome == MatchEventOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == MatchEventOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == MatchEventOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, match_event::entries[index], original, outcome);
}
}
bool try_modern_match_event(State& s, std::uint32_t entry) {
    const auto found = std::find(match_event::entries.begin(), match_event::entries.end(), entry);
    if (found == match_event::entries.end()) throw std::runtime_error("Unknown match-event entry");
    const auto index = unsigned(found - match_event::entries.begin()); s.pc = entry;
    auto& d = match_event_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original match-event body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{match_event::actor_address,match_event::ball_cell_side,match_event::qualify_event,match_event::sample_event_jitter,match_event::select_transition,match_event::same_actor_pair,match_event::compatible_actor_context,match_event::emit_event,match_event::classify_event};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, MatchEventOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, MatchEventOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? MatchEventOutcome::budget_stop : MatchEventOutcome::fault); throw;
    } catch (...) { leave(s, index, original, MatchEventOutcome::fault); throw; }
    leave(s, index, original, MatchEventOutcome::returned); return true;
}
}
