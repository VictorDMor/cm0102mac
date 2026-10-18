#include "injury_daily_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct DailyDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(injury_daily_diagnostics(s).original_body) {
        injury_daily_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { injury_daily_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, InjuryDailyOutcome outcome) {
    auto& d = injury_daily_diagnostics(s);
    if (!original) {
        if (outcome == InjuryDailyOutcome::returned) ++d.returns[index];
        else if (outcome == InjuryDailyOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == InjuryDailyOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == InjuryDailyOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, injury_daily::entries[index], original, outcome);
}
}
bool try_modern_injury_daily(State& s, std::uint32_t entry) {
    const auto found = std::find(injury_daily::entries.begin(), injury_daily::entries.end(), entry);
    if (found == injury_daily::entries.end()) throw std::runtime_error("Unknown injury-daily entry");
    const auto index = unsigned(found - injury_daily::entries.begin()); s.pc = entry;
    auto& d = injury_daily_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original injury-daily body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{injury_daily::process_daily_injuries};
            DailyDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, InjuryDailyOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, InjuryDailyOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? InjuryDailyOutcome::budget_stop : InjuryDailyOutcome::fault); throw;
    } catch (...) { leave(s, index, original, InjuryDailyOutcome::fault); throw; }
    leave(s, index, original, InjuryDailyOutcome::returned); return true;
}
}
