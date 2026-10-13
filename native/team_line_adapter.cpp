#include "team_line_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(team_line_diagnostics(s).original_body) {
        team_line_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { team_line_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, TeamLineOutcome outcome) {
    auto& d = team_line_diagnostics(s);
    if (!original) {
        if (outcome == TeamLineOutcome::returned) ++d.returns[index];
        else if (outcome == TeamLineOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == TeamLineOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == TeamLineOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, team_line::entries[index], original, outcome);
}
}
bool try_modern_team_line(State& s, std::uint32_t entry) {
    const auto found = std::find(team_line::entries.begin(), team_line::entries.end(), entry);
    if (found == team_line::entries.end()) throw std::runtime_error("Unknown team-line entry");
    const auto index = unsigned(found - team_line::entries.begin()); s.pc = entry;
    auto& d = team_line_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original team-line body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{team_line::tactical_flags, team_line::line_adjustment_enabled, team_line::actor_column_excess, team_line::column_excess, team_line::row_lane_clear};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, TeamLineOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, TeamLineOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? TeamLineOutcome::budget_stop : TeamLineOutcome::fault); throw;
    } catch (...) { leave(s, index, original, TeamLineOutcome::fault); throw; }
    leave(s, index, original, TeamLineOutcome::returned); return true;
}
}
