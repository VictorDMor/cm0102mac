#include "injury_reporting_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ReportingDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(injury_reporting_diagnostics(s).original_body) {
        injury_reporting_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { injury_reporting_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, InjuryReportingOutcome outcome) {
    auto& d = injury_reporting_diagnostics(s);
    if (!original) {
        if (outcome == InjuryReportingOutcome::returned) ++d.returns[index];
        else if (outcome == InjuryReportingOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == InjuryReportingOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == InjuryReportingOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, injury_reporting::entries[index], original, outcome);
}
}
bool try_modern_injury_reporting(State& s, std::uint32_t entry) {
    const auto found = std::find(injury_reporting::entries.begin(), injury_reporting::entries.end(), entry);
    if (found == injury_reporting::entries.end()) throw std::runtime_error("Unknown injury-reporting entry");
    const auto index = unsigned(found - injury_reporting::entries.begin()); s.pc = entry;
    auto& d = injury_reporting_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original injury-reporting body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{injury_reporting::has_special_injury_type,injury_reporting::compose_injury_notice,injury_reporting::describe_injury_match,injury_reporting::clear_injury_history};
            ReportingDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, InjuryReportingOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, InjuryReportingOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? InjuryReportingOutcome::budget_stop : InjuryReportingOutcome::fault); throw;
    } catch (...) { leave(s, index, original, InjuryReportingOutcome::fault); throw; }
    leave(s, index, original, InjuryReportingOutcome::returned); return true;
}
}
