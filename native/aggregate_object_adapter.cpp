#include "aggregate_object_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(aggregate_object_diagnostics(s).original_body) {
        aggregate_object_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { aggregate_object_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, AggregateObjectOutcome outcome) {
    auto& d = aggregate_object_diagnostics(s);
    if (!original) {
        if (outcome == AggregateObjectOutcome::returned) ++d.returns[index];
        else if (outcome == AggregateObjectOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == AggregateObjectOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == AggregateObjectOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, aggregate_object::entries[index], original, outcome);
}
}
bool try_modern_aggregate_object(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto index = aggregate_object::entry_index(entry);
    auto& d = aggregate_object_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original aggregate-object body");
            if (s.pc != caller) run_until(s, caller);
        } else aggregate_object::invoke(s, index);
    } catch (const ApplicationExit&) {
        leave(s, index, original, AggregateObjectOutcome::terminated); throw;
    } catch (const GuestContextTransfer&) {
        leave(s, index, original, AggregateObjectOutcome::context_transfer); throw;
    } catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted"
            ? AggregateObjectOutcome::budget_stop : AggregateObjectOutcome::fault); throw;
    } catch (...) { leave(s, index, original, AggregateObjectOutcome::fault); throw; }
    leave(s, index, original, AggregateObjectOutcome::returned); return true;
}
void reset_aggregate_object_diagnostics(State& s) { aggregate_object_diagnostics(s) = AggregateObjectDiagnostics{}; }
std::string aggregate_object_call_summary(const State& s) {
    const auto& d = aggregate_object_diagnostics(s); std::ostringstream out;
    for (unsigned n = 0; n < aggregate_object::entries.size(); ++n)
        out << "aggregate_object.entry=" << hex_address(aggregate_object::entries[n]) << " modern=" << d.calls[n]
            << " original=" << d.original_calls[n] << " returns=" << d.returns[n] << " faults=" << d.faults[n]
            << " budget_stops=" << d.budget_stops[n] << " transfers=" << d.transfers[n] << " terminations=" << d.terminations[n]
            << " profile_original=" << d.profile_original_calls[n] << '\n';
    out << "aggregate_object.dispatch_escapes=" << d.dispatch_escapes << '\n'; return out.str();
}
}
