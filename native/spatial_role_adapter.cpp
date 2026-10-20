#include "spatial_role_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <string_view>

namespace cm::aot {
namespace {
// Direct entry dispatch avoids rescanning the complete owner table for every
// position/role query in the shared match engine.
unsigned entry_index(std::uint32_t entry) {
    using spatial_role::Entry;
    switch (entry) {
    case spatial_role::entries[unsigned(Entry::current_position)]: return unsigned(Entry::current_position);
    case spatial_role::entries[unsigned(Entry::column)]: return unsigned(Entry::column);
    case spatial_role::entries[unsigned(Entry::row)]: return unsigned(Entry::row);
    case spatial_role::entries[unsigned(Entry::marking_target)]: return unsigned(Entry::marking_target);
    case spatial_role::entries[unsigned(Entry::set_marking_target)]: return unsigned(Entry::set_marking_target);
    case spatial_role::entries[unsigned(Entry::clear_marking_list)]: return unsigned(Entry::clear_marking_list);
    case spatial_role::entries[unsigned(Entry::marking_count)]: return unsigned(Entry::marking_count);
    case spatial_role::entries[unsigned(Entry::current_role)]: return unsigned(Entry::current_role);
    case spatial_role::entries[unsigned(Entry::secondary_role)]: return unsigned(Entry::secondary_role);
    case spatial_role::entries[unsigned(Entry::previous_role)]: return unsigned(Entry::previous_role);
    case spatial_role::entries[unsigned(Entry::previous_position)]: return unsigned(Entry::previous_position);
    case spatial_role::entries[unsigned(Entry::register_center_cell)]: return unsigned(Entry::register_center_cell);
    case spatial_role::entries[unsigned(Entry::set_column)]: return unsigned(Entry::set_column);
    case spatial_role::entries[unsigned(Entry::set_row)]: return unsigned(Entry::set_row);
    case spatial_role::entries[unsigned(Entry::remove_from_cell)]: return unsigned(Entry::remove_from_cell);
    case spatial_role::entries[unsigned(Entry::marking_member)]: return unsigned(Entry::marking_member);
    case spatial_role::entries[unsigned(Entry::clear_goalkeeper)]: return unsigned(Entry::clear_goalkeeper);
    case spatial_role::entries[unsigned(Entry::resolve_goalkeeper)]: return unsigned(Entry::resolve_goalkeeper);
    case spatial_role::entries[unsigned(Entry::set_previous_position)]: return unsigned(Entry::set_previous_position);
    case spatial_role::entries[unsigned(Entry::cached_role)]: return unsigned(Entry::cached_role);
    case spatial_role::entries[unsigned(Entry::actor_current_role)]: return unsigned(Entry::actor_current_role);
    case spatial_role::entries[unsigned(Entry::actor_previous_role)]: return unsigned(Entry::actor_previous_role);
    case spatial_role::entries[unsigned(Entry::actor_secondary_role)]: return unsigned(Entry::actor_secondary_role);
    default: throw std::runtime_error("Unknown spatial-role entry");
    }
}
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(spatial_role_diagnostics(s).original_body) {
        spatial_role_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { spatial_role_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, SpatialRoleOutcome outcome) {
    auto& d = spatial_role_diagnostics(s);
    if (!original) {
        if (outcome == SpatialRoleOutcome::returned) ++d.returns[index];
        else if (outcome == SpatialRoleOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == SpatialRoleOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == SpatialRoleOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, spatial_role::entries[index], original, outcome);
}
}
bool try_modern_spatial_role(State& s, std::uint32_t entry) {
    const auto index = entry_index(entry); s.pc = entry;
    auto& d = spatial_role_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original spatial-role body");
            if (s.pc != caller) run_until(s, caller);
        } else spatial_role::invoke(s, spatial_role::Entry(index));
    } catch (const ApplicationExit&) { leave(s, index, original, SpatialRoleOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, SpatialRoleOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? SpatialRoleOutcome::budget_stop : SpatialRoleOutcome::fault); throw;
    } catch (...) { leave(s, index, original, SpatialRoleOutcome::fault); throw; }
    leave(s, index, original, SpatialRoleOutcome::returned); return true;
}
}
