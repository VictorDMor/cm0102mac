#include "movement_choice_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(movement_choice_diagnostics(s).original_body) {
        movement_choice_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { movement_choice_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, MovementChoiceOutcome outcome) {
    auto& d = movement_choice_diagnostics(s);
    if (!original) {
        if (outcome == MovementChoiceOutcome::returned) ++d.returns[index];
        else if (outcome == MovementChoiceOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == MovementChoiceOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == MovementChoiceOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, movement_choice::entries[index], original, outcome);
}
}
bool try_modern_movement_choice(State& s, std::uint32_t entry) {
    const auto found = std::find(movement_choice::entries.begin(), movement_choice::entries.end(), entry);
    if (found == movement_choice::entries.end()) throw std::runtime_error("Unknown movement-choice entry");
    const auto index = unsigned(found - movement_choice::entries.begin()); s.pc = entry;
    auto& d = movement_choice_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original movement-choice body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{movement_choice::choose_cell, movement_choice::adjustment_byte, movement_choice::outside_central_rows, movement_choice::inside_end_area};
            methods[index](s, nullptr, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, MovementChoiceOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, MovementChoiceOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? MovementChoiceOutcome::budget_stop : MovementChoiceOutcome::fault); throw;
    } catch (...) { leave(s, index, original, MovementChoiceOutcome::fault); throw; }
    leave(s, index, original, MovementChoiceOutcome::returned); return true;
}
}
