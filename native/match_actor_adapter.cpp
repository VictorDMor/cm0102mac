#include "match_actor_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(match_actor_diagnostics(s).original_body) {
        match_actor_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { match_actor_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, MatchActorOutcome outcome) {
    auto& d = match_actor_diagnostics(s);
    if (!original) {
        if (outcome == MatchActorOutcome::returned) ++d.returns[index];
        else if (outcome == MatchActorOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == MatchActorOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == MatchActorOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, match_actor::entries[index], original, outcome);
}
}
bool try_modern_match_actor(State& s, std::uint32_t entry) {
    const auto found = std::find(match_actor::entries.begin(), match_actor::entries.end(), entry);
    if (found == match_actor::entries.end()) throw std::runtime_error("Unknown match-actor entry");
    const auto index = unsigned(found - match_actor::entries.begin()); s.pc = entry;
    auto& d = match_actor_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original match-actor body");
            if (s.pc != caller) run_until(s, caller);
        } else if (index == 0) match_actor::withdraw_injured(s);
        else if (index == 1) match_actor::read_position(s);
        else match_actor::write_position(s);
    } catch (const ApplicationExit&) { leave(s, index, original, MatchActorOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, MatchActorOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? MatchActorOutcome::budget_stop : MatchActorOutcome::fault); throw;
    } catch (...) { leave(s, index, original, MatchActorOutcome::fault); throw; }
    leave(s, index, original, MatchActorOutcome::returned); return true;
}
}
