#include "file_path_adapter.hpp"
#include "file_path_internal.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
bool try_modern_file_path(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.file_path_original_resume == entry) { s.file_path_original_resume = 0; return false; }
    const auto found = std::find(file_path_entries.begin(), file_path_entries.end(), entry);
    if (found == file_path_entries.end()) throw std::runtime_error("Unknown Mac file-path entry");
    const auto index = std::size_t(found - file_path_entries.begin());
    const bool original = s.file_path_original || s.file_path_entry_original[index];
    ++(original ? s.file_path_original_calls[index] : s.file_path_calls[index]);
    if (original && !s.file_path_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (s.file_path_observer) s.file_path_observer->enter(s, entry, original);
    if (original) {
        s.file_path_original_resume = entry;
        try { run_until(s, caller); } catch (...) { s.file_path_original_resume = 0; throw; }
    } else {
        s.step(entry); file_path::Abi a{s, s.file_path_dispatch_escapes};
        using namespace file_path;
        switch (index) {
        case 0: copy_pascal(a); break;
        case 1: classic_spec(a); break;
        case 2: compatible_spec(a); break;
        case 3: script_encoding(a); break;
        case 4: reference_from_path(a); break;
        case 5: unicode_name(a); break;
        case 6: parent_and_leaf(a); break;
        }
    }
    if (s.file_path_observer) s.file_path_observer->leave(s, entry, original);
    return true;
}
void reset_file_path_diagnostics(State& s) {
    s.file_path_original = false; s.file_path_entry_original.fill(false);
    s.file_path_calls.fill(0); s.file_path_original_calls.fill(0);
    s.file_path_original_resume = 0; s.file_path_dispatch_escapes = 0;
    s.file_path_observer = nullptr;
}
std::string file_path_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < file_path_entries.size(); ++i)
        out << "file_path.entry=" << hex_address(file_path_entries[i]) << " mode="
            << (s.file_path_original || s.file_path_entry_original[i] ? "original" : "modern")
            << " modern=" << s.file_path_calls[i] << " original=" << s.file_path_original_calls[i] << '\n';
    out << "file_path.dispatch_escapes=" << s.file_path_dispatch_escapes << '\n';
    return out.str();
}
}
