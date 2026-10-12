#include "player_decision_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(player_decision_diagnostics(s).original_body) {
        player_decision_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { player_decision_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, PlayerDecisionOutcome outcome) {
    auto& d = player_decision_diagnostics(s);
    if (!original) {
        if (outcome == PlayerDecisionOutcome::returned) ++d.returns[index];
        else if (outcome == PlayerDecisionOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == PlayerDecisionOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == PlayerDecisionOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, player_decision::entries[index], original, outcome);
}
}
bool try_modern_player_decision(State& s, std::uint32_t entry) {
    const auto found = std::find(player_decision::entries.begin(), player_decision::entries.end(), entry);
    if (found == player_decision::entries.end()) throw std::runtime_error("Unknown player-decision entry");
    const auto index = unsigned(found - player_decision::entries.begin()); s.pc = entry;
    auto& d = player_decision_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original player-decision body");
            if (s.pc != caller) run_until(s, caller);
        } else player_decision::decide(s, nullptr, d.observer);
    } catch (const ApplicationExit&) { leave(s, index, original, PlayerDecisionOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, PlayerDecisionOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? PlayerDecisionOutcome::budget_stop : PlayerDecisionOutcome::fault); throw;
    } catch (...) { leave(s, index, original, PlayerDecisionOutcome::fault); throw; }
    leave(s, index, original, PlayerDecisionOutcome::returned); return true;
}
}
