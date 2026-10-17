#include "injury_progression_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ProgressionDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(injury_progression_diagnostics(s).original_body) {
        injury_progression_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { injury_progression_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, InjuryProgressionOutcome outcome) {
    auto& d = injury_progression_diagnostics(s);
    if (!original) {
        if (outcome == InjuryProgressionOutcome::returned) ++d.returns[index];
        else if (outcome == InjuryProgressionOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == InjuryProgressionOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == InjuryProgressionOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, injury_progression::entries[index], original, outcome);
}
}
bool try_modern_injury_progression(State& s, std::uint32_t entry) {
    const auto found = std::find(injury_progression::entries.begin(), injury_progression::entries.end(), entry);
    if (found == injury_progression::entries.end()) throw std::runtime_error("Unknown injury-progression entry");
    const auto index = unsigned(found - injury_progression::entries.begin()); s.pc = entry;
    auto& d = injury_progression_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original injury-progression body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{injury_progression::process_daily_injuries,injury_progression::prepare_progress_notice,injury_progression::compare_queued_end,injury_progression::compare_queued_start};
            ProgressionDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, InjuryProgressionOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, InjuryProgressionOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? InjuryProgressionOutcome::budget_stop : InjuryProgressionOutcome::fault); throw;
    } catch (...) { leave(s, index, original, InjuryProgressionOutcome::fault); throw; }
    leave(s, index, original, InjuryProgressionOutcome::returned); return true;
}
}
