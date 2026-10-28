#include "ball_contact_adapter.hpp"
#include "match_pl_adapter.hpp"
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
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(ball_contact_diagnostics(s).original_body) {
        ball_contact_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { ball_contact_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, BallContactOutcome outcome) {
    auto& d = ball_contact_diagnostics(s);
    if (!original) {
        if (outcome == BallContactOutcome::returned) ++d.returns[index];
        else if (outcome == BallContactOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == BallContactOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == BallContactOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, ball_contact::replacement_entries[index], original, outcome);
}
}
bool try_modern_ball_contact(State& s, std::uint32_t entry) {
    const auto found = std::find(ball_contact::replacement_entries.begin(), ball_contact::replacement_entries.end(), entry);
    if (found == ball_contact::replacement_entries.end()) throw std::runtime_error("Unknown ball-contact entry");
    const auto index = unsigned(found - ball_contact::replacement_entries.begin()); s.pc = entry;
    auto& d = ball_contact_diagnostics(s);
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
            switch(index) {
            case 0:ball_contact::screen(s,nullptr,d.observer);break;
            case 1:ball_contact::attempt(s,nullptr,d.observer);break;
            case 2:ball_contact::describe(s,nullptr,d.observer);break;
            case 3:ball_contact::react_to_failure(s,nullptr,d.observer);break;
            }
        }
    } catch (const ApplicationExit&) { leave(s, index, original, BallContactOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, BallContactOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? BallContactOutcome::budget_stop : BallContactOutcome::fault); throw;
    } catch (...) { leave(s, index, original, BallContactOutcome::fault); throw; }
    leave(s, index, original, BallContactOutcome::returned); return true;
}
}
