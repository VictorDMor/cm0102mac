#include "exception_runtime_adapter.hpp"
#include "exception_runtime_context.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include <algorithm>
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
using Operation = void (*)(State&);
constexpr std::array<Operation, 21> operations{
    exception_runtime::advance_action, exception_runtime::copy_saved_registers,
    exception_runtime::cleanup_actions, exception_runtime::matches_specification,
    exception_runtime::handle_unexpected, exception_runtime::destroy_derived_exception,
    exception_runtime::destroy_base_exception, exception_runtime::restore_context,
    exception_runtime::install_catch, exception_runtime::search_throw,
    exception_runtime::throw_dispatch, exception_runtime::throw_vector,
    exception_runtime::throw_scalar, exception_runtime::cleanup_exception_object,
    exception_runtime::derived_exception_type, exception_runtime::base_exception_type,
    exception_runtime::abort_facade, exception_runtime::termination_facade,
    exception_runtime::invoke_termination, exception_runtime::invoke_unexpected,
    exception_runtime::match_types};
constexpr std::uint32_t landing = 0x107724f4;
void leave(State& s, unsigned index, bool original, bool continuation, ExceptionRuntimeOutcome outcome) {
    if (!original && !continuation) {
        auto& counts = outcome == ExceptionRuntimeOutcome::returned ? s.exception_runtime_returns :
            outcome == ExceptionRuntimeOutcome::transferred ? s.exception_runtime_transfers : s.exception_runtime_terminations;
        ++counts[index];
    }
    if (!original && continuation) {
        if (outcome == ExceptionRuntimeOutcome::returned) ++s.exception_runtime_landing_returns;
        else if (outcome == ExceptionRuntimeOutcome::transferred) ++s.exception_runtime_landing_transfers;
        else ++s.exception_runtime_landing_terminations;
    }
    if (s.exception_runtime_observer) {
        if (continuation) s.exception_runtime_observer->landing_leave(s, original, outcome);
        else s.exception_runtime_observer->leave(s, exception_runtime_entries[index], original, outcome);
    }
}
void execute(State& s, unsigned index, bool original, bool continuation) {
    const auto entry = continuation ? landing : exception_runtime_entries[index];
    const auto caller = std::uint32_t(s.r<4>(4128));
    bool continuation_returned = false;
    if (s.exception_runtime_observer) {
        if (continuation) s.exception_runtime_observer->landing_enter(s, original);
        else s.exception_runtime_observer->enter(s, entry, original);
    }
    try {
        if (original) {
            s.exception_runtime_original_resume = entry;
            try { run_until(s, caller); }
            catch (...) { s.exception_runtime_original_resume = 0; throw; }
            s.exception_runtime_original_resume = 0;
        } else {
            s.step(entry);
            if (continuation) continuation_returned = exception_runtime::resume_unexpected_continuation(s);
            else operations[index](s);
        }
    } catch (const GuestContextTransfer&) {
        leave(s, index, original, continuation, ExceptionRuntimeOutcome::transferred);
        throw;
    } catch (const ApplicationExit&) {
        leave(s, index, original, continuation, ExceptionRuntimeOutcome::terminated);
        throw;
    }
    const bool returned = continuation ? continuation_returned : s.pc == caller;
    leave(s, index, original, continuation, returned ? ExceptionRuntimeOutcome::returned : ExceptionRuntimeOutcome::transferred);
}
}
bool try_modern_exception_runtime(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.exception_runtime_original_resume == entry) { s.exception_runtime_original_resume = 0; return false; }
    const auto found = std::find(exception_runtime_entries.begin(), exception_runtime_entries.end(), entry);
    if (found == exception_runtime_entries.end()) throw std::runtime_error("Unknown exception-runtime entry");
    const auto index = unsigned(found - exception_runtime_entries.begin());
    const bool original = s.exception_runtime_original || s.exception_runtime_entry_original[index];
    ++(original ? s.exception_runtime_original_calls[index] : s.exception_runtime_calls[index]);
    if (original && !s.exception_runtime_observer) return false;
    execute(s, index, original, false);
    return true;
}
void invoke_exception_runtime(State& s, std::uint32_t entry) {
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (try_modern_exception_runtime(s, entry)) return;
    s.exception_runtime_original_resume = entry;
    try { run_until(s, caller); }
    catch (...) { s.exception_runtime_original_resume = 0; throw; }
    s.exception_runtime_original_resume = 0;
}
bool try_modern_exception_unexpected_landing(State& s) {
    s.pc = landing;
    if (s.exception_runtime_original_resume == landing) { s.exception_runtime_original_resume = 0; return false; }
    const bool original = s.exception_runtime_original || s.exception_runtime_entry_original[4];
    ++(original ? s.exception_runtime_original_landings : s.exception_runtime_landings);
    // The original continuation is already in its generated function. Avoid
    // nesting a new engine using the throw's scratch LR as a return boundary.
    if (original) return false;
    execute(s, 4, false, true);
    return true;
}
void resume_exception_unknown_transfer(State& s, std::uint32_t target, std::uint32_t caller) {
    ++s.exception_runtime_unknown_transfers;
    s.pc = target;
    try { run_until(s, caller); }
    catch (const std::runtime_error& error) {
        if (std::string_view(error.what()).starts_with("Uncompiled original address:")) ++s.exception_runtime_dispatch_escapes;
        throw;
    }
}
void reset_exception_runtime_diagnostics(State& s) {
    s.exception_runtime_original = false; s.exception_runtime_entry_original.fill(false);
    s.exception_runtime_calls.fill(0); s.exception_runtime_original_calls.fill(0);
    s.exception_runtime_returns.fill(0); s.exception_runtime_transfers.fill(0); s.exception_runtime_terminations.fill(0);
    s.exception_runtime_original_resume = 0; s.exception_runtime_dispatch_escapes = 0; s.exception_runtime_unknown_transfers = 0;
    s.exception_runtime_landings = s.exception_runtime_original_landings = 0;
    s.exception_runtime_landing_returns = s.exception_runtime_landing_transfers = s.exception_runtime_landing_terminations = 0;
    s.exception_runtime_observer = nullptr;
}
std::string exception_runtime_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < exception_runtime_entries.size(); ++i)
        out << "exception_runtime.entry=" << hex_address(exception_runtime_entries[i]) << " mode="
            << (s.exception_runtime_original || s.exception_runtime_entry_original[i] ? "original" : "modern")
            << " modern=" << s.exception_runtime_calls[i] << " original=" << s.exception_runtime_original_calls[i]
            << " returns=" << s.exception_runtime_returns[i] << " transfers=" << s.exception_runtime_transfers[i]
            << " terminations=" << s.exception_runtime_terminations[i] << '\n';
    out << "exception_runtime.landing=" << hex_address(landing) << " modern=" << s.exception_runtime_landings
        << " original=" << s.exception_runtime_original_landings << " returns=" << s.exception_runtime_landing_returns
        << " transfers=" << s.exception_runtime_landing_transfers << " terminations=" << s.exception_runtime_landing_terminations << '\n';
    out << "exception_runtime.dispatch_escapes=" << s.exception_runtime_dispatch_escapes << '\n'
        << "exception_runtime.unknown_transfers=" << s.exception_runtime_unknown_transfers << '\n';
    return out.str();
}
}
