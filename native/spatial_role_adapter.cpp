#include "spatial_role_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
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
    const auto found = std::find(spatial_role::entries.begin(), spatial_role::entries.end(), entry);
    if (found == spatial_role::entries.end()) throw std::runtime_error("Unknown spatial-role entry");
    const auto index = unsigned(found - spatial_role::entries.begin()); s.pc = entry;
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
