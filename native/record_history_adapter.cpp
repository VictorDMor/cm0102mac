#include "record_history_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(record_history_diagnostics(s).original_body) {
        record_history_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { record_history_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, RecordHistoryOutcome outcome) {
    auto& d = record_history_diagnostics(s);
    if (!original) {
        if (outcome == RecordHistoryOutcome::returned) ++d.returns[index];
        else if (outcome == RecordHistoryOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == RecordHistoryOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == RecordHistoryOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, record_history::entries[index], original, outcome);
}
}
bool try_modern_record_history(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto index = record_history::entry_index(entry);
    auto& d = record_history_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original record-history body");
            if (s.pc != caller) run_until(s, caller);
        } else record_history::invoke(s, index);
    } catch (const ApplicationExit&) {
        leave(s, index, original, RecordHistoryOutcome::terminated); throw;
    } catch (const GuestContextTransfer&) {
        leave(s, index, original, RecordHistoryOutcome::context_transfer); throw;
    } catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted"
            ? RecordHistoryOutcome::budget_stop : RecordHistoryOutcome::fault); throw;
    } catch (...) { leave(s, index, original, RecordHistoryOutcome::fault); throw; }
    leave(s, index, original, RecordHistoryOutcome::returned); return true;
}
void reset_record_history_diagnostics(State& s) { record_history_diagnostics(s) = RecordHistoryDiagnostics{}; }
std::string record_history_call_summary(const State& s) {
    const auto& d = record_history_diagnostics(s); std::ostringstream out;
    for (unsigned n = 0; n < record_history::entries.size(); ++n)
        out << "record_history.entry=" << hex_address(record_history::entries[n]) << " modern=" << d.calls[n]
            << " original=" << d.original_calls[n] << " returns=" << d.returns[n] << " faults=" << d.faults[n]
            << " budget_stops=" << d.budget_stops[n] << " transfers=" << d.transfers[n] << " terminations=" << d.terminations[n]
            << " profile_original=" << d.profile_original_calls[n] << '\n';
    out << "record_history.dispatch_escapes=" << d.dispatch_escapes << '\n'; return out.str();
}
}
