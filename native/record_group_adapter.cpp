#include "record_group_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(record_group_diagnostics(s).original_body) {
        record_group_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { record_group_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, RecordGroupOutcome outcome) {
    auto& d = record_group_diagnostics(s);
    if (!original) {
        if (outcome == RecordGroupOutcome::returned) ++d.returns[index];
        else if (outcome == RecordGroupOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == RecordGroupOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == RecordGroupOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, record_group::entries[index], original, outcome);
}
}
bool try_modern_record_group(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto index = record_group::entry_index(entry);
    auto& d = record_group_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original record-group body");
            if (s.pc != caller) run_until(s, caller);
        } else record_group::invoke(s, index);
    } catch (const ApplicationExit&) {
        leave(s, index, original, RecordGroupOutcome::terminated); throw;
    } catch (const GuestContextTransfer&) {
        leave(s, index, original, RecordGroupOutcome::context_transfer); throw;
    } catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted"
            ? RecordGroupOutcome::budget_stop : RecordGroupOutcome::fault); throw;
    } catch (...) { leave(s, index, original, RecordGroupOutcome::fault); throw; }
    leave(s, index, original, RecordGroupOutcome::returned); return true;
}
void reset_record_group_diagnostics(State& s) { record_group_diagnostics(s) = RecordGroupDiagnostics{}; }
std::string record_group_call_summary(const State& s) {
    const auto& d = record_group_diagnostics(s); std::ostringstream out;
    for (unsigned n = 0; n < record_group::entries.size(); ++n)
        out << "record_group.entry=" << hex_address(record_group::entries[n]) << " modern=" << d.calls[n]
            << " original=" << d.original_calls[n] << " returns=" << d.returns[n] << " faults=" << d.faults[n]
            << " budget_stops=" << d.budget_stops[n] << " transfers=" << d.transfers[n] << " terminations=" << d.terminations[n]
            << " profile_original=" << d.profile_original_calls[n] << '\n';
    out << "record_group.dispatch_escapes=" << d.dispatch_escapes << '\n'; return out.str();
}
}
