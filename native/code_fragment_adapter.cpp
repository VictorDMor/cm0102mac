#include "code_fragment_adapter.hpp"
#include "code_fragment_internal.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
bool try_modern_code_fragment(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.code_fragment_original_resume == entry) { s.code_fragment_original_resume = 0; return false; }
    const auto found = std::find(code_fragment_entries.begin(), code_fragment_entries.end(), entry);
    if (found == code_fragment_entries.end()) throw std::runtime_error("Unknown code-fragment entry");
    const auto index = std::size_t(found - code_fragment_entries.begin());
    const bool original = s.code_fragment_original || s.code_fragment_entry_original[index];
    ++(original ? s.code_fragment_original_calls[index] : s.code_fragment_calls[index]);
    if (original && !s.code_fragment_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (s.code_fragment_observer) s.code_fragment_observer->enter(s, entry, original);
    if (original) {
        s.code_fragment_original_resume = entry;
        try { run_until(s, caller); } catch (...) { s.code_fragment_original_resume = 0; throw; }
    } else {
        s.step(entry); code_fragment::Abi a{s, s.code_fragment_dispatch_escapes};
        using namespace code_fragment;
        switch (index) {
        case 0: register_fragment(a); break;
        case 1: find_fragment(a); break;
        case 2: resolve_metadata(a); break;
        case 3: saved_frame_word(a); break;
        case 4: action_kind(a); break;
        }
    }
    if (s.code_fragment_observer) s.code_fragment_observer->leave(s, entry, original);
    return true;
}
void reset_code_fragment_diagnostics(State& s) {
    s.code_fragment_original = false; s.code_fragment_entry_original.fill(false);
    s.code_fragment_calls.fill(0); s.code_fragment_original_calls.fill(0);
    s.code_fragment_original_resume = 0; s.code_fragment_dispatch_escapes = 0;
    s.code_fragment_observer = nullptr;
}
std::string code_fragment_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < code_fragment_entries.size(); ++i)
        out << "code_fragment.entry=" << hex_address(code_fragment_entries[i]) << " mode="
            << (s.code_fragment_original || s.code_fragment_entry_original[i] ? "original" : "modern")
            << " modern=" << s.code_fragment_calls[i] << " original=" << s.code_fragment_original_calls[i] << '\n';
    out << "code_fragment.dispatch_escapes=" << s.code_fragment_dispatch_escapes << '\n';
    return out.str();
}
}
