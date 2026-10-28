#include "pitch_distance_adapter.hpp"
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
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(pitch_distance_diagnostics(s).original_body) {
        pitch_distance_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { pitch_distance_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, PitchDistanceOutcome outcome) {
    auto& d = pitch_distance_diagnostics(s);
    if (!original) {
        if (outcome == PitchDistanceOutcome::returned) ++d.returns[index];
        else if (outcome == PitchDistanceOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == PitchDistanceOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == PitchDistanceOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, pitch_distance::entries[index], original, outcome);
}
}
bool try_modern_pitch_distance(State& s, std::uint32_t entry) {
    const auto found = std::find(pitch_distance::entries.begin(), pitch_distance::entries.end(), entry);
    if (found == pitch_distance::entries.end()) throw std::runtime_error("Unknown pitch-distance entry");
    const auto index = unsigned(found - pitch_distance::entries.begin()); s.pc = entry;
    auto& d = pitch_distance_diagnostics(s);
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
            constexpr Function methods[]{pitch_distance::point_to_point, pitch_distance::actor_to_cell, pitch_distance::actor_to_attacking_goal};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, PitchDistanceOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, PitchDistanceOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? PitchDistanceOutcome::budget_stop : PitchDistanceOutcome::fault); throw;
    } catch (...) { leave(s, index, original, PitchDistanceOutcome::fault); throw; }
    leave(s, index, original, PitchDistanceOutcome::returned); return true;
}
}
