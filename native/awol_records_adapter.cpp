#include "awol_records_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct AwolDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(awol_records_diagnostics(s).original_body) {
        awol_records_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { awol_records_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, AwolRecordsOutcome outcome) {
    auto& d = awol_records_diagnostics(s);
    if (!original) {
        if (outcome == AwolRecordsOutcome::returned) ++d.returns[index];
        else if (outcome == AwolRecordsOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == AwolRecordsOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == AwolRecordsOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, awol::record_entries[index], original, outcome);
}
}
bool try_modern_awol_records(State& s, std::uint32_t entry) {
    const auto found = std::find(awol::record_entries.begin(), awol::record_entries.end(), entry);
    if (found == awol::record_entries.end()) throw std::runtime_error("Unknown awol-records entry");
    const auto index = unsigned(found - awol::record_entries.begin()); s.pc = entry;
    auto& d = awol_records_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original awol-records body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{awol::construct_records,awol::destroy_records,awol::load_records,awol::save_records,awol::primary_absence_status,awol::secondary_absence_status,awol::has_pending_record,awol::remove_primary_by_staff,awol::remove_primary_item,awol::remove_secondary_item,awol::remove_secondary_by_staff,awol::notify_absence_change,awol::sample_absence_range,awol::process_daily_absences,awol::register_primary_absence,awol::notify_training_absence,awol::describe_primary_absence,awol::describe_training_absence,awol::describe_fitness_concern,awol::process_training_absence,awol::process_fitness_concern};
            AwolDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, AwolRecordsOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, AwolRecordsOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? AwolRecordsOutcome::budget_stop : AwolRecordsOutcome::fault); throw;
    } catch (...) { leave(s, index, original, AwolRecordsOutcome::fault); throw; }
    leave(s, index, original, AwolRecordsOutcome::returned); return true;
}
}
