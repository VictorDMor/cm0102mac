#include "engine_initialization_adapter.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(engine_initialization_diagnostics(s).original_body) {
        engine_initialization_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { engine_initialization_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, EngineInitializationOutcome outcome) {
    if (!original) {
        if (outcome == EngineInitializationOutcome::returned) ++engine_initialization_diagnostics(s).returns[index];
        else if (outcome == EngineInitializationOutcome::budget_stop) ++engine_initialization_diagnostics(s).budget_stops[index];
        else if (outcome == EngineInitializationOutcome::context_transfer) ++engine_initialization_diagnostics(s).transfers[index];
        else ++engine_initialization_diagnostics(s).faults[index];
    }
    if (engine_initialization_diagnostics(s).observer) engine_initialization_diagnostics(s).observer->leave(s, engine_initialization::entries[index], original, outcome);
}
}
bool try_modern_engine_initialization(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto index = engine_initialization::entry_index(entry);
    if (engine_initialization_diagnostics(s).original_body == entry) return false;
    bool original = engine_initialization_diagnostics(s).original || engine_initialization_diagnostics(s).entry_original[index];
    if (original) ++engine_initialization_diagnostics(s).original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++engine_initialization_diagnostics(s).profile_original_calls[index]; }
    else ++engine_initialization_diagnostics(s).calls[index];
    // Semantic checkpoints need no original budget fallback.
    if (engine_initialization_diagnostics(s).observer) engine_initialization_diagnostics(s).observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original engine-initialization body");
            if (s.pc != caller) run_until(s, caller);
        } else engine_initialization::invoke(s, index);
    } catch (const GuestContextTransfer&) {
        leave(s, index, original, EngineInitializationOutcome::context_transfer); throw;
    } catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++engine_initialization_diagnostics(s).dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted"
            ? EngineInitializationOutcome::budget_stop : EngineInitializationOutcome::fault); throw;
    } catch (...) { leave(s, index, original, EngineInitializationOutcome::fault); throw; }
    leave(s, index, original, EngineInitializationOutcome::returned); return true;
}
void reset_engine_initialization_diagnostics(State& s) {
    engine_initialization_diagnostics(s) = EngineInitializationDiagnostics{};
}
std::string engine_initialization_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned n = 0; n < engine_initialization::entries.size(); ++n)
        out << "engine_initialization.entry=" << hex_address(engine_initialization::entries[n]) << " modern=" << engine_initialization_diagnostics(s).calls[n]
            << " original=" << engine_initialization_diagnostics(s).original_calls[n] << " returns=" << engine_initialization_diagnostics(s).returns[n]
            << " faults=" << engine_initialization_diagnostics(s).faults[n] << " budget_stops=" << engine_initialization_diagnostics(s).budget_stops[n]
            << " transfers=" << engine_initialization_diagnostics(s).transfers[n] << " profile_original=" << engine_initialization_diagnostics(s).profile_original_calls[n] << '\n';
    out << "engine_initialization.dispatch_escapes=" << engine_initialization_diagnostics(s).dispatch_escapes << '\n'; return out.str();
}
}
