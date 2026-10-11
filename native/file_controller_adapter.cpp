#include "file_controller_adapter.hpp"
#include "file_controller_internal.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
bool try_modern_file_controller(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.file_controller_original_resume == entry) { s.file_controller_original_resume = 0; return false; }
    const auto found = std::find(file_controller_entries.begin(), file_controller_entries.end(), entry);
    if (found == file_controller_entries.end()) throw std::runtime_error("Unknown Mac file-controller entry");
    const auto index = std::size_t(found - file_controller_entries.begin());
    const bool original = s.file_controller_original || s.file_controller_entry_original[index];
    ++(original ? s.file_controller_original_calls[index] : s.file_controller_calls[index]);
    if (original && !s.file_controller_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (s.file_controller_observer) s.file_controller_observer->enter(s, entry, original);
    if (original) {
        s.file_controller_original_resume = entry;
        try { run_until(s, caller); } catch (...) { s.file_controller_original_resume = 0; throw; }
    } else {
        s.step(entry); file_controller::Abi a{s, s.file_controller_dispatch_escapes};
        using namespace file_controller;
        switch (index) {
        case 0: filename_facade(a, false); break;
        case 1: filename_facade(a, true); break;
        case 2: open_stream(a); break;
        case 3: reopen_stream(a); break;
        case 4: parse_mode(a); break;
        case 5: fork_capability(a); break;
        case 6: set_classic_finder(a); break;
        case 7: find_file_record(a); break;
        case 8: alias_policy(a); break;
        case 9: metadata_code(a, true); break;
        case 10: metadata_code(a, false); break;
        case 11: open_file(a); break;
        case 12: transfer_file(a, false); break;
        case 13: transfer_file(a, true); break;
        case 14: seek_file(a); break;
        case 15: close_file(a); break;
        case 16: remove_file(a); break;
        case 17: rename_file(a); break;
        case 18: tell_position(a); break;
        case 19: tell_facade(a); break;
        case 20: get_position(a); break;
        case 21: seek_position(a); break;
        case 22: seek_facade(a); break;
        case 23: set_position(a); break;
        case 24: clear_status(a); break;
        case 25: initialize_stdio(a); break;
        }
    }
    if (s.file_controller_observer) s.file_controller_observer->leave(s, entry, original);
    return true;
}
void reset_file_controller_diagnostics(State& s) {
    s.file_controller_original = false; s.file_controller_entry_original.fill(false);
    s.file_controller_calls.fill(0); s.file_controller_original_calls.fill(0);
    s.file_controller_original_resume = 0; s.file_controller_dispatch_escapes = 0;
    s.file_controller_observer = nullptr;
}
std::string file_controller_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < file_controller_entries.size(); ++i)
        out << "file_controller.entry=" << hex_address(file_controller_entries[i]) << " mode="
            << (s.file_controller_original || s.file_controller_entry_original[i] ? "original" : "modern")
            << " modern=" << s.file_controller_calls[i] << " original=" << s.file_controller_original_calls[i] << '\n';
    out << "file_controller.dispatch_escapes=" << s.file_controller_dispatch_escapes << '\n';
    return out.str();
}
}
