#include "time_runtime_adapter.hpp"
#include "time_runtime.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(s.time_runtime_original_body) {
        state.time_runtime_original_body = entry;
    }
    ~OriginalBody() { state.time_runtime_original_body = previous; }
};
void resume_time(State& s, std::uint32_t caller, std::uint32_t entry) {
    OriginalBody selected(s, entry); run_until(s, caller);
}
void leave(State& s, unsigned index, bool original, TimeRuntimeOutcome outcome) {
    if (!original) {
        if (outcome == TimeRuntimeOutcome::returned) ++s.time_runtime_returns[index];
        else if (outcome == TimeRuntimeOutcome::transferred) ++s.time_runtime_transfers[index];
        else if (outcome == TimeRuntimeOutcome::budget_return) ++s.time_runtime_budget_returns[index];
        else ++s.time_runtime_faults[index];
    }
    if (s.time_runtime_observer) s.time_runtime_observer->leave(s, time_runtime_entries[index], original, outcome);
}
}
bool try_modern_time_runtime(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto found = std::find(time_runtime_entries.begin(), time_runtime_entries.end(), entry);
    if (found == time_runtime_entries.end()) throw std::runtime_error("Unknown time-runtime entry");
    const auto index = unsigned(found - time_runtime_entries.begin());
    if (s.time_runtime_original_body == entry) return false;
    bool original = s.time_runtime_original || s.time_runtime_entry_original[index];
    if (original) ++s.time_runtime_original_calls[index];
    else if (match_pc_profile_enabled()) { ++s.time_runtime_profile_original_calls[index]; original = true; }
    else {
        constexpr std::array<unsigned, 8> first_phase{2, 9, 9, 13, 6, 7, 5, 5};
        const std::uint64_t required = first_phase[index];
        if (s.executed > s.budget || required > s.budget - s.executed) {
            ++s.time_runtime_budget_original_calls[index]; original = true;
        } else ++s.time_runtime_calls[index];
    }
    if (s.time_runtime_observer) s.time_runtime_observer->enter(s, entry, original);
    time_runtime::Execution policy{0, resume_time, nullptr};
    const auto record_resumes = [&] { s.time_runtime_budget_resumes[index] += policy.budget_resumes; };
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original time-runtime body");
            if (s.pc != caller) run_until(s, caller);
        } else time_runtime::invoke(s, index, &policy);
    } catch (const GuestContextTransfer&) {
        record_resumes(); leave(s, index, original, TimeRuntimeOutcome::transferred); throw;
    } catch (const std::runtime_error& error) {
        if (std::string_view(error.what()).starts_with("Uncompiled original address:")) ++s.time_runtime_dispatch_escapes;
        record_resumes(); leave(s, index, original, TimeRuntimeOutcome::fault); throw;
    } catch (...) { record_resumes(); leave(s, index, original, TimeRuntimeOutcome::fault); throw; }
    record_resumes();
    leave(s, index, original, policy.budget_resumes ? TimeRuntimeOutcome::budget_return : TimeRuntimeOutcome::returned);
    return true;
}
void reset_time_runtime_diagnostics(State& s) {
    s.time_runtime_original = false; s.time_runtime_entry_original.fill(false);
    s.time_runtime_calls.fill(0); s.time_runtime_original_calls.fill(0);
    s.time_runtime_returns.fill(0); s.time_runtime_transfers.fill(0); s.time_runtime_faults.fill(0);
    s.time_runtime_budget_original_calls.fill(0); s.time_runtime_profile_original_calls.fill(0);
    s.time_runtime_budget_resumes.fill(0); s.time_runtime_budget_returns.fill(0);
    s.time_runtime_original_body = 0; s.time_runtime_dispatch_escapes = 0; s.time_runtime_observer = nullptr;
}
std::string time_runtime_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned n = 0; n < time_runtime_entries.size(); ++n)
        out << "time_runtime.entry=" << hex_address(time_runtime_entries[n]) << " modern=" << s.time_runtime_calls[n]
            << " original=" << s.time_runtime_original_calls[n] << " returns=" << s.time_runtime_returns[n]
            << " transfers=" << s.time_runtime_transfers[n] << " faults=" << s.time_runtime_faults[n]
            << " budget_original=" << s.time_runtime_budget_original_calls[n]
            << " profile_original=" << s.time_runtime_profile_original_calls[n]
            << " budget_resumes=" << s.time_runtime_budget_resumes[n] << " budget_returns=" << s.time_runtime_budget_returns[n] << '\n';
    out << "time_runtime.dispatch_escapes=" << s.time_runtime_dispatch_escapes << '\n'; return out.str();
}
}
