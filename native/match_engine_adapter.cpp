#include "match_engine_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(match_engine_diagnostics(s).original_body) {
        match_engine_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { match_engine_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, MatchEngineOutcome outcome) {
    auto& d = match_engine_diagnostics(s);
    if (!original) {
        if (outcome == MatchEngineOutcome::returned) ++d.returns[index];
        else if (outcome == MatchEngineOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == MatchEngineOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == MatchEngineOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, match_engine::entries[index], original, outcome);
}
}
bool try_modern_match_engine(State& s, std::uint32_t entry) {
    const auto found = std::find(match_engine::entries.begin(), match_engine::entries.end(), entry);
    if (found == match_engine::entries.end()) throw std::runtime_error("Unknown match-engine entry");
    const auto index = unsigned(found - match_engine::entries.begin()); s.pc = entry;
    auto& d = match_engine_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original match-engine body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            if (index == 0) match_engine::advance_watched(s);
            else if (index == 1) match_engine::advance_background(s);
            else match_engine::advance_shared(s);
        }
    } catch (const ApplicationExit&) {
        leave(s, index, original, MatchEngineOutcome::terminated); throw;
    } catch (const GuestContextTransfer&) {
        leave(s, index, original, MatchEngineOutcome::context_transfer); throw;
    } catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted"
            ? MatchEngineOutcome::budget_stop : MatchEngineOutcome::fault); throw;
    } catch (...) { leave(s, index, original, MatchEngineOutcome::fault); throw; }
    leave(s, index, original, MatchEngineOutcome::returned); return true;
}
void reset_match_engine_diagnostics(State& s) { match_engine_diagnostics(s) = MatchEngineDiagnostics{}; }
std::string match_engine_call_summary(const State& s) {
    const auto& d = match_engine_diagnostics(s); std::ostringstream out;
    for (unsigned n = 0; n < match_engine::entries.size(); ++n)
        out << "match_engine.entry=" << hex_address(match_engine::entries[n]) << " modern=" << d.calls[n]
            << " original=" << d.original_calls[n] << " returns=" << d.returns[n] << " faults=" << d.faults[n]
            << " budget_stops=" << d.budget_stops[n] << " transfers=" << d.transfers[n] << " terminations=" << d.terminations[n]
            << " profile_original=" << d.profile_original_calls[n] << '\n';
    out << "match_engine.dispatch_escapes=" << d.dispatch_escapes << '\n'; return out.str();
}
}
