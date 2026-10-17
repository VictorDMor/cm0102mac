#include "injury_record_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct InjuryDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(injury_record_diagnostics(s).original_body) {
        injury_record_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { injury_record_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, InjuryRecordOutcome outcome) {
    auto& d = injury_record_diagnostics(s);
    if (!original) {
        if (outcome == InjuryRecordOutcome::returned) ++d.returns[index];
        else if (outcome == InjuryRecordOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == InjuryRecordOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == InjuryRecordOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, injury_record::entries[index], original, outcome);
}
}
bool try_modern_injury_record(State& s, std::uint32_t entry) {
    const auto found = std::find(injury_record::entries.begin(), injury_record::entries.end(), entry);
    if (found == injury_record::entries.end()) throw std::runtime_error("Unknown injury-record entry");
    const auto index = unsigned(found - injury_record::entries.begin()); s.pc = entry;
    auto& d = injury_record_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original injury-record body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{injury_record::register_injury,injury_record::prepare_injury_notice,injury_record::propagate_injury_risk};
            InjuryDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, InjuryRecordOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, InjuryRecordOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? InjuryRecordOutcome::budget_stop : InjuryRecordOutcome::fault); throw;
    } catch (...) { leave(s, index, original, InjuryRecordOutcome::fault); throw; }
    leave(s, index, original, InjuryRecordOutcome::returned); return true;
}
}
