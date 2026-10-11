#include "native_service_adapter.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(s.native_service_original_body) {
        state.native_service_original_body = entry;
    }
    ~OriginalBody() { state.native_service_original_body = previous; }
};
void leave(State& s, unsigned index, bool original, NativeServiceOutcome outcome) {
    if (!original) {
        if (outcome == NativeServiceOutcome::returned) ++s.native_service_returns[index];
        else if (outcome == NativeServiceOutcome::budget_stop) ++s.native_service_budget_stops[index];
        else if (outcome == NativeServiceOutcome::context_transfer) ++s.native_service_transfers[index];
        else ++s.native_service_faults[index];
    }
    if (s.native_service_observer) s.native_service_observer->leave(s, native_service::entries[index], original, outcome);
}
}
bool try_modern_native_service(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto index = native_service::entry_index(entry);
    if (s.native_service_original_body == entry) return false;
    bool original = s.native_service_original || s.native_service_entry_original[index];
    if (original) ++s.native_service_original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++s.native_service_profile_original_calls[index]; }
    else ++s.native_service_calls[index];
    // The nine individually checked operations need no original budget fallback.
    if (s.native_service_observer) s.native_service_observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original native-service body");
            if (s.pc != caller) run_until(s, caller);
        } else native_service::invoke(s, native_service::Service(index));
    } catch (const GuestContextTransfer&) {
        leave(s, index, original, NativeServiceOutcome::context_transfer); throw;
    } catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++s.native_service_dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted"
            ? NativeServiceOutcome::budget_stop : NativeServiceOutcome::fault); throw;
    } catch (...) { leave(s, index, original, NativeServiceOutcome::fault); throw; }
    leave(s, index, original, NativeServiceOutcome::returned); return true;
}
void reset_native_service_diagnostics(State& s) {
    s.native_service_original = false; s.native_service_entry_original.fill(false);
    s.native_service_calls.fill(0); s.native_service_original_calls.fill(0);
    s.native_service_returns.fill(0); s.native_service_faults.fill(0);
    s.native_service_budget_stops.fill(0); s.native_service_transfers.fill(0);
    s.native_service_profile_original_calls.fill(0); s.native_service_original_body = 0;
    s.native_service_dispatch_escapes = 0; s.native_service_observer = nullptr;
}
std::string native_service_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned n = 0; n < native_service::entries.size(); ++n)
        out << "native_service.entry=" << hex_address(native_service::entries[n]) << " modern=" << s.native_service_calls[n]
            << " original=" << s.native_service_original_calls[n] << " returns=" << s.native_service_returns[n]
            << " faults=" << s.native_service_faults[n] << " budget_stops=" << s.native_service_budget_stops[n]
            << " transfers=" << s.native_service_transfers[n] << " profile_original=" << s.native_service_profile_original_calls[n] << '\n';
    out << "native_service.dispatch_escapes=" << s.native_service_dispatch_escapes << '\n'; return out.str();
}
}
