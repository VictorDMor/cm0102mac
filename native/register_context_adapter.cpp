#include "register_context_adapter.hpp"
#include "register_context.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
constexpr std::uint32_t body(unsigned index) {
    return index < 14 ? 0x107737e0 : index < 28 ? 0x10773830 : register_context_entries[index];
}
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, unsigned index) : state(s), previous(s.register_context_original_body) {
        state.register_context_original_body = body(index);
    }
    ~OriginalBody() { state.register_context_original_body = previous; }
};
void leave(State& s, unsigned index, bool original, RegisterContextOutcome outcome) {
    if (!original) {
        if (outcome == RegisterContextOutcome::returned) ++s.register_context_returns[index];
        else if (outcome == RegisterContextOutcome::transferred) ++s.register_context_transfers[index];
        else ++s.register_context_faults[index];
    }
    if (s.register_context_observer)
        s.register_context_observer->leave(s, register_context_entries[index], original, outcome);
}
}
bool try_modern_register_context(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto found = std::find(register_context_entries.begin(), register_context_entries.end(), entry);
    if (found == register_context_entries.end()) throw std::runtime_error("Unknown register-context entry");
    const auto index = unsigned(found - register_context_entries.begin());
    // Shared suffixes fall through subsequent callable labels. Keep the whole
    // selected original family suppressed until its return or stopping fault.
    if (s.register_context_original_body == body(index)) return false;
    bool original = s.register_context_original || s.register_context_entry_original[index];
    if (original) ++s.register_context_original_calls[index];
    else if (match_pc_profile_enabled()) { ++s.register_context_profile_original_calls[index]; original = true; }
    else {
        const std::uint64_t required = index < 28 ? 15 - index % 14 : index == 28 ? 29 : 32;
        if (s.executed > s.budget || required > s.budget - s.executed) {
            ++s.register_context_budget_original_calls[index]; original = true;
        } else ++s.register_context_calls[index];
    }
    if (s.register_context_observer) s.register_context_observer->enter(s, entry, original);
    GuestScalarContextCapture capture(index == 28 ? &s : nullptr);
    try {
        if (original) {
            OriginalBody selection(s, index);
            if (!dispatch_page(s)) { ++s.register_context_dispatch_escapes; throw std::runtime_error("Missing original register-context body"); }
        } else {
            s.step(entry);
            if (index < 14) register_context::save_floating_suffix(s, index + 18);
            else if (index < 28) register_context::restore_floating_suffix(s, index - 14 + 18);
            else if (index == 28) register_context::capture_nonlocal_context(s);
            else register_context::restore_nonlocal_context(s);
        }
        if (index == 28) capture.commit();
    } catch (...) { leave(s, index, original, RegisterContextOutcome::fault); throw; }
    const auto outcome = index == 29 ? RegisterContextOutcome::transferred : RegisterContextOutcome::returned;
    leave(s, index, original, outcome);
    if (index == 29) complete_scalar_context_transfer(s, s.register_context_unknown_transfers);
    return true;
}
void invoke_register_context(State& s, std::uint32_t entry) {
    if (!try_modern_register_context(s, entry)) throw std::runtime_error("Recursive register-context diagnostic invocation");
}
void reset_register_context_diagnostics(State& s) {
    s.register_context_original = false; s.register_context_entry_original.fill(false);
    s.register_context_calls.fill(0); s.register_context_original_calls.fill(0);
    s.register_context_returns.fill(0); s.register_context_transfers.fill(0); s.register_context_faults.fill(0);
    s.register_context_budget_original_calls.fill(0); s.register_context_profile_original_calls.fill(0);
    s.register_context_original_body = 0; s.register_context_dispatch_escapes = s.register_context_unknown_transfers = 0;
    s.register_context_observer = nullptr;
}
std::string register_context_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < register_context_entries.size(); ++i)
        out << "register_context.entry=" << hex_address(register_context_entries[i]) << " modern=" << s.register_context_calls[i]
            << " original=" << s.register_context_original_calls[i] << " returns=" << s.register_context_returns[i]
            << " transfers=" << s.register_context_transfers[i] << " faults=" << s.register_context_faults[i]
            << " budget_original=" << s.register_context_budget_original_calls[i]
            << " profile_original=" << s.register_context_profile_original_calls[i] << '\n';
    out << "register_context.dispatch_escapes=" << s.register_context_dispatch_escapes
        << " unknown_transfers=" << s.register_context_unknown_transfers << '\n'; return out.str();
}
}
