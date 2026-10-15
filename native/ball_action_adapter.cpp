#include "ball_action_adapter.hpp"
#include "aot_runtime.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(ball_action_diagnostics(s).original_body) {
        ball_action_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { ball_action_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, BallActionOutcome outcome) {
    auto& d = ball_action_diagnostics(s);
    if (!original) {
        if (outcome == BallActionOutcome::returned) ++d.returns[index];
        else if (outcome == BallActionOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == BallActionOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == BallActionOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, ball_action::entries[index], original, outcome);
}
}
bool try_modern_ball_action(State& s, std::uint32_t entry) {
    const auto found = std::find(ball_action::entries.begin(), ball_action::entries.end(), entry);
    if (found == ball_action::entries.end()) throw std::runtime_error("Unknown ball-action entry");
    const auto index = unsigned(found - ball_action::entries.begin()); s.pc = entry;
    auto& d = ball_action_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original ball-action body");
            if (s.pc != caller) run_until(s, caller);
        } else ball_action::act(s, nullptr, d.observer);
    } catch (const ApplicationExit&) { leave(s, index, original, BallActionOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, BallActionOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? BallActionOutcome::budget_stop : BallActionOutcome::fault); throw;
    } catch (...) { leave(s, index, original, BallActionOutcome::fault); throw; }
    leave(s, index, original, BallActionOutcome::returned); return true;
}
}
