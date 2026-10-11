#include "cpp_object_storage_adapter.hpp"
#include "cpp_object_storage_internal.hpp"
#include "application_exit.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
bool try_modern_cpp_object_storage(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.cpp_object_storage_original_resume == entry) { s.cpp_object_storage_original_resume = 0; return false; }
    const auto found = std::find(cpp_object_storage_entries.begin(), cpp_object_storage_entries.end(), entry);
    if (found == cpp_object_storage_entries.end()) throw std::runtime_error("Unknown C++ object-storage entry");
    const auto index = std::size_t(found - cpp_object_storage_entries.begin());
    const bool original = s.cpp_object_storage_original || s.cpp_object_storage_entry_original[index];
    ++(original ? s.cpp_object_storage_original_calls[index] : s.cpp_object_storage_calls[index]);
    if (original && !s.cpp_object_storage_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (s.cpp_object_storage_observer) s.cpp_object_storage_observer->enter(s, entry, original);
    try {
        if (original) {
            s.cpp_object_storage_original_resume = entry;
            try { run_until(s, caller); } catch (...) { s.cpp_object_storage_original_resume = 0; throw; }
        } else {
            s.step(entry); cpp_object_storage::Abi a{s, s.cpp_object_storage_dispatch_escapes};
            using namespace cpp_object_storage;
            switch (index) {
            case 0: construct_array(a); break;
            case 1: cleanup_partial_array(a); break;
            case 2: construct_range(a); break;
            case 3: destroy_range(a); break;
            case 4: destroy_array(a); break;
            case 5: allocate_object(a); break;
            case 6: release_object(a); break;
            case 7: allocation_facade(a, false); break;
            case 8: allocation_facade(a, true); break;
            case 9: allocation_failure(a); break;
            case 10: destroy_failure(a); break;
            case 11: failure_type(a); break;
            }
        }
    } catch (const ApplicationExit&) {
        if (!original) ++s.cpp_object_storage_terminations[index];
        if (s.cpp_object_storage_observer) s.cpp_object_storage_observer->leave(s, entry, original, true);
        throw;
    }
    if (!original) ++s.cpp_object_storage_returns[index];
    if (s.cpp_object_storage_observer) s.cpp_object_storage_observer->leave(s, entry, original, false);
    return true;
}
void reset_cpp_object_storage_diagnostics(State& s) {
    s.cpp_object_storage_original = false; s.cpp_object_storage_entry_original.fill(false);
    s.cpp_object_storage_calls.fill(0); s.cpp_object_storage_original_calls.fill(0);
    s.cpp_object_storage_returns.fill(0); s.cpp_object_storage_terminations.fill(0);
    s.cpp_object_storage_original_resume = 0; s.cpp_object_storage_dispatch_escapes = 0;
    s.cpp_object_storage_observer = nullptr;
}
std::string cpp_object_storage_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < cpp_object_storage_entries.size(); ++i)
        out << "cpp_object_storage.entry=" << hex_address(cpp_object_storage_entries[i]) << " mode="
            << (s.cpp_object_storage_original || s.cpp_object_storage_entry_original[i] ? "original" : "modern")
            << " modern=" << s.cpp_object_storage_calls[i] << " original=" << s.cpp_object_storage_original_calls[i]
            << " returns=" << s.cpp_object_storage_returns[i] << " terminations=" << s.cpp_object_storage_terminations[i] << '\n';
    out << "cpp_object_storage.dispatch_escapes=" << s.cpp_object_storage_dispatch_escapes << '\n';
    return out.str();
}
}
