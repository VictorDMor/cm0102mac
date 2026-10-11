#include "runtime_lifecycle_adapter.hpp"
#include "runtime_lifecycle_internal.hpp"
#include "application_exit.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
bool try_modern_runtime_lifecycle(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.runtime_lifecycle_original_resume == entry) { s.runtime_lifecycle_original_resume = 0; return false; }
    const auto found = std::find(runtime_lifecycle_entries.begin(), runtime_lifecycle_entries.end(), entry);
    if (found == runtime_lifecycle_entries.end()) throw std::runtime_error("Unknown runtime lifecycle entry");
    const auto index = std::size_t(found - runtime_lifecycle_entries.begin());
    const bool original = s.runtime_lifecycle_original || s.runtime_lifecycle_entry_original[index];
    ++(original ? s.runtime_lifecycle_original_calls[index] : s.runtime_lifecycle_calls[index]);
    if (original && !s.runtime_lifecycle_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (s.runtime_lifecycle_observer) s.runtime_lifecycle_observer->enter(s, entry, original);
    try {
        if (original) {
            s.runtime_lifecycle_original_resume = entry;
            try { run_until(s, caller); } catch (...) { s.runtime_lifecycle_original_resume = 0; throw; }
        } else {
            s.step(entry); runtime_lifecycle::Abi a{s, s.runtime_lifecycle_dispatch_escapes};
            using namespace runtime_lifecycle;
            switch (index) {
            case 0: register_destructor(a); break;
            case 1: drain_destructors(a); break;
            case 2: abort_runtime(a); break;
            case 3: exit_runtime(a); break;
            case 4: terminate_runtime(a); break;
            case 5: raise_signal(a); break;
            }
        }
    } catch (const ApplicationExit&) {
        if (!original) ++s.runtime_lifecycle_terminations[index];
        if (s.runtime_lifecycle_observer) s.runtime_lifecycle_observer->leave(s, entry, original, true);
        throw;
    }
    if (!original) ++s.runtime_lifecycle_returns[index];
    if (s.runtime_lifecycle_observer) s.runtime_lifecycle_observer->leave(s, entry, original, false);
    return true;
}
void reset_runtime_lifecycle_diagnostics(State& s) {
    s.runtime_lifecycle_original = false; s.runtime_lifecycle_entry_original.fill(false);
    s.runtime_lifecycle_calls.fill(0); s.runtime_lifecycle_original_calls.fill(0);
    s.runtime_lifecycle_returns.fill(0); s.runtime_lifecycle_terminations.fill(0);
    s.runtime_lifecycle_original_resume = 0; s.runtime_lifecycle_dispatch_escapes = 0;
    s.runtime_lifecycle_observer = nullptr;
}
std::string runtime_lifecycle_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < runtime_lifecycle_entries.size(); ++i)
        out << "runtime_lifecycle.entry=" << hex_address(runtime_lifecycle_entries[i]) << " mode="
            << (s.runtime_lifecycle_original || s.runtime_lifecycle_entry_original[i] ? "original" : "modern")
            << " modern=" << s.runtime_lifecycle_calls[i] << " original=" << s.runtime_lifecycle_original_calls[i]
            << " returns=" << s.runtime_lifecycle_returns[i] << " terminations=" << s.runtime_lifecycle_terminations[i] << '\n';
    out << "runtime_lifecycle.dispatch_escapes=" << s.runtime_lifecycle_dispatch_escapes << '\n';
    return out.str();
}
}
