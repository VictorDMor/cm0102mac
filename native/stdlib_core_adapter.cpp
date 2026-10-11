#include "stdlib_core_adapter.hpp"
#include "stdlib_core.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(s.stdlib_core_original_body) {
        state.stdlib_core_original_body = entry;
    }
    ~OriginalBody() { state.stdlib_core_original_body = previous; }
};
void resume_search(State& s, std::uint32_t caller) {
    OriginalBody selected(s, stdlib_core_entries[3]); run_until(s, caller);
}
void leave(State& s, unsigned index, bool original, StdlibCoreOutcome outcome) {
    if (!original) {
        if (outcome == StdlibCoreOutcome::returned) ++s.stdlib_core_returns[index];
        else if (outcome == StdlibCoreOutcome::transferred) ++s.stdlib_core_transfers[index];
        else if (outcome == StdlibCoreOutcome::budget_return) ++s.stdlib_core_budget_returns[index];
        else ++s.stdlib_core_faults[index];
    }
    if (s.stdlib_core_observer) s.stdlib_core_observer->leave(s, stdlib_core_entries[index], original, outcome);
}
}
bool try_modern_stdlib_core(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto found = std::find(stdlib_core_entries.begin(), stdlib_core_entries.end(), entry);
    if (found == stdlib_core_entries.end()) throw std::runtime_error("Unknown stdlib-core entry");
    const auto index = unsigned(found - stdlib_core_entries.begin());
    if (s.stdlib_core_original_body == entry) return false;
    bool original = s.stdlib_core_original || s.stdlib_core_entry_original[index];
    if (original) ++s.stdlib_core_original_calls[index];
    else if (match_pc_profile_enabled()) { ++s.stdlib_core_profile_original_calls[index]; original = true; }
    else {
        const std::uint64_t required = index < 2 ? 4 : index == 2 ? 23 : 10;
        if (s.executed > s.budget || required > s.budget - s.executed) {
            ++s.stdlib_core_budget_original_calls[index]; original = true;
        } else ++s.stdlib_core_calls[index];
    }
    if (s.stdlib_core_observer) s.stdlib_core_observer->enter(s, entry, original);
    stdlib_core::SearchExecution policy{0, resume_search};
    const auto record_resumes = [&] { s.stdlib_core_budget_resumes[index] += policy.budget_resumes; };
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original stdlib-core body");
            if (s.pc != caller) run_until(s, caller);
        } else if (index < 2) {
            s.step(entry); s.executed += 3;
            if (index == 0) stdlib_core::absolute_word(s); else stdlib_core::absolute_long_word(s);
        } else if (index == 2) stdlib_core::floor_remainder(s, true);
        else stdlib_core::binary_search(s, &policy);
    } catch (const GuestContextTransfer&) {
        record_resumes(); leave(s, index, original, StdlibCoreOutcome::transferred); throw;
    } catch (const std::runtime_error& error) {
        if (std::string_view(error.what()).starts_with("Uncompiled original address:")) ++s.stdlib_core_dispatch_escapes;
        record_resumes(); leave(s, index, original, StdlibCoreOutcome::fault); throw;
    } catch (...) { record_resumes(); leave(s, index, original, StdlibCoreOutcome::fault); throw; }
    record_resumes();
    leave(s, index, original, policy.budget_resumes ? StdlibCoreOutcome::budget_return : StdlibCoreOutcome::returned);
    return true;
}
void reset_stdlib_core_diagnostics(State& s) {
    s.stdlib_core_original = false; s.stdlib_core_entry_original.fill(false);
    s.stdlib_core_calls.fill(0); s.stdlib_core_original_calls.fill(0);
    s.stdlib_core_returns.fill(0); s.stdlib_core_transfers.fill(0); s.stdlib_core_faults.fill(0);
    s.stdlib_core_budget_original_calls.fill(0); s.stdlib_core_profile_original_calls.fill(0);
    s.stdlib_core_budget_resumes.fill(0); s.stdlib_core_budget_returns.fill(0);
    s.stdlib_core_original_body = 0; s.stdlib_core_dispatch_escapes = 0; s.stdlib_core_observer = nullptr;
}
std::string stdlib_core_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned n = 0; n < stdlib_core_entries.size(); ++n)
        out << "stdlib_core.entry=" << hex_address(stdlib_core_entries[n]) << " modern=" << s.stdlib_core_calls[n]
            << " original=" << s.stdlib_core_original_calls[n] << " returns=" << s.stdlib_core_returns[n]
            << " transfers=" << s.stdlib_core_transfers[n] << " faults=" << s.stdlib_core_faults[n]
            << " budget_original=" << s.stdlib_core_budget_original_calls[n]
            << " profile_original=" << s.stdlib_core_profile_original_calls[n]
            << " budget_resumes=" << s.stdlib_core_budget_resumes[n] << " budget_returns=" << s.stdlib_core_budget_returns[n] << '\n';
    out << "stdlib_core.dispatch_escapes=" << s.stdlib_core_dispatch_escapes << '\n'; return out.str();
}
}
