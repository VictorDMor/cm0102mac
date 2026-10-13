#include "team_spatial_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(team_spatial_diagnostics(s).original_body) {
        team_spatial_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { team_spatial_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, TeamSpatialOutcome outcome) {
    auto& d = team_spatial_diagnostics(s);
    if (!original) {
        if (outcome == TeamSpatialOutcome::returned) ++d.returns[index];
        else if (outcome == TeamSpatialOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == TeamSpatialOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == TeamSpatialOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, team_spatial::entries[index], original, outcome);
}
}
bool try_modern_team_spatial(State& s, std::uint32_t entry) {
    const auto found = std::find(team_spatial::entries.begin(), team_spatial::entries.end(), entry);
    if (found == team_spatial::entries.end()) throw std::runtime_error("Unknown team-spatial entry");
    const auto index = unsigned(found - team_spatial::entries.begin()); s.pc = entry;
    auto& d = team_spatial_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original team-spatial body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{team_spatial::nearest_actor, team_spatial::nearest_distance, team_spatial::nearby_weight, team_spatial::refresh_column_limits};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, TeamSpatialOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, TeamSpatialOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? TeamSpatialOutcome::budget_stop : TeamSpatialOutcome::fault); throw;
    } catch (...) { leave(s, index, original, TeamSpatialOutcome::fault); throw; }
    leave(s, index, original, TeamSpatialOutcome::returned); return true;
}
}
