#include "marking_pressure_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(marking_pressure_diagnostics(s).original_body) {
        marking_pressure_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { marking_pressure_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, MarkingPressureOutcome outcome) {
    auto& d = marking_pressure_diagnostics(s);
    if (!original) {
        if (outcome == MarkingPressureOutcome::returned) ++d.returns[index];
        else if (outcome == MarkingPressureOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == MarkingPressureOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == MarkingPressureOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, marking_pressure::entries[index], original, outcome);
}
}
bool try_modern_marking_pressure(State& s, std::uint32_t entry) {
    const auto found = std::find(marking_pressure::entries.begin(), marking_pressure::entries.end(), entry);
    if (found == marking_pressure::entries.end()) throw std::runtime_error("Unknown marking-pressure entry");
    const auto index = unsigned(found - marking_pressure::entries.begin()); s.pc = entry;
    auto& d = marking_pressure_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original marking-pressure body");
            if (s.pc != caller) run_until(s, caller);
        } else marking_pressure::invoke(s, marking_pressure::Entry(index));
    } catch (const ApplicationExit&) { leave(s, index, original, MarkingPressureOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, MarkingPressureOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? MarkingPressureOutcome::budget_stop : MarkingPressureOutcome::fault); throw;
    } catch (...) { leave(s, index, original, MarkingPressureOutcome::fault); throw; }
    leave(s, index, original, MarkingPressureOutcome::returned); return true;
}
}
