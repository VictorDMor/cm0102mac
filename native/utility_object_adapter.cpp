#include "utility_object_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(utility_object_diagnostics(s).original_body) {
        utility_object_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { utility_object_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, UtilityObjectOutcome outcome) {
    auto& d = utility_object_diagnostics(s);
    if (!original) {
        if (outcome == UtilityObjectOutcome::returned) ++d.returns[index];
        else if (outcome == UtilityObjectOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == UtilityObjectOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == UtilityObjectOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, utility_object::entries[index], original, outcome);
}
}
bool try_modern_utility_object(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto index = utility_object::entry_index(entry);
    auto& d = utility_object_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original utility-object body");
            if (s.pc != caller) run_until(s, caller);
        } else utility_object::invoke(s, index);
    } catch (const ApplicationExit&) {
        leave(s, index, original, UtilityObjectOutcome::terminated); throw;
    } catch (const GuestContextTransfer&) {
        leave(s, index, original, UtilityObjectOutcome::context_transfer); throw;
    } catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted"
            ? UtilityObjectOutcome::budget_stop : UtilityObjectOutcome::fault); throw;
    } catch (...) { leave(s, index, original, UtilityObjectOutcome::fault); throw; }
    leave(s, index, original, UtilityObjectOutcome::returned); return true;
}
void reset_utility_object_diagnostics(State& s) { utility_object_diagnostics(s) = UtilityObjectDiagnostics{}; }
std::string utility_object_call_summary(const State& s) {
    const auto& d = utility_object_diagnostics(s); std::ostringstream out;
    for (unsigned n = 0; n < utility_object::entries.size(); ++n)
        out << "utility_object.entry=" << hex_address(utility_object::entries[n]) << " modern=" << d.calls[n]
            << " original=" << d.original_calls[n] << " returns=" << d.returns[n] << " faults=" << d.faults[n]
            << " budget_stops=" << d.budget_stops[n] << " transfers=" << d.transfers[n] << " terminations=" << d.terminations[n]
            << " profile_original=" << d.profile_original_calls[n] << '\n';
    out << "utility_object.dispatch_escapes=" << d.dispatch_escapes << '\n'; return out.str();
}
}
