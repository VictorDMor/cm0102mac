// Returned comparisons against private hook-free originals, in two passes.
// Controlled: string routines and path parts run for real; the screen manager,
// controls, files, dialogs, the clock and localized or formatted text are scripted,
// so their arguments are compared and their effects are not. Every dispatched call
// puts its target, return address, arguments, non-volatile registers and CR into
// compared memory.
// Real state (run_real_state): from a capture's snapshot the screen manager,
// controls and text run for real; only files, the clock and paths are scripted.
// Calls the module itself makes are recorded the same way, and memory is compared.
#include "file_screens_adapter.hpp"
#include "guest_values.hpp"
#include "match_replay.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <ranges>
#include <set>
#include <unistd.h>
#include <vector>

#if defined(CM_FILE_SCREENS_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif
using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool file_screens_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x78000000, text_a = data + 0x1000, text_b = data + 0x1100, text_c = data + 0x1200, folder_buffer = data + 0x1400,
    copy_buffer = data + 0x1600, name_buffer = data + 0x1800, path_buffer = data + 0x1a00, tm_at = data + 0x1c00, leagues_at = data + 0x2000,
    people_at = data + 0x4000, clubs_at = data + 0x8000, heap = data + 0x40000, trace_at = data + 0x200000, controls = data + 0x100000,
    stack = 0x71010000, sentinel = 0xfffffff0, toc_base = 0x108bba90, screens_slot = toc_base - 0x7bb0;
constexpr std::uint32_t open_dialog = 0x100edf20, render_dialog = 0x100ee220, dialog_action = 0x100ee9d0, open_info = 0x100ef7d0,
    render_info = 0x100ef840, info_action = 0x100f0530, open_message = 0x100f05a0, open_long = 0x100f0720, render_message = 0x100f08c0,
    message_action = 0x100f0ba0, fill_summary = 0x100f0d70, module_low = 0x100edf20, module_high = 0x100f1000;
constexpr std::array entries{open_dialog, render_dialog, dialog_action, open_info, render_info, info_action, open_message, open_long,
                             render_message, message_action, fill_summary};
constexpr std::uint32_t history_value = 0x1073ef70, set_history = 0x1073f1e0, append_history = 0x1073e4b0, localized = 0x10727890,
    format = 0x1077b720, error_dialog = 0x10769b60, copy_limited = 0x1077dc10, allocate = 0x107735f0, release = 0x10773610,
    allocate_object = 0x10773520, usable_path = 0x10746340, create_directory = 0x10739cd0, file_exists = 0x10739d60,
    find_first = 0x10739690, find_next = 0x107397d0, glue_call = 0x10781320, language = 0x10727880,
    open_entry = 0x107475b0, read_entry = 0x10747a80, close_entry = 0x10747900, begin_screen = 0x10712d00, begin_plain = 0x10712e60,
    screen_colours = 0x10744250, screen_title = 0x107442f0, panel = 0x102a1070, add_table = 0x107198c0, add_field = 0x10719740,
    select_row = 0x107145f0, add_cell = 0x107445a0, add_button = 0x107444e0, add_box = 0x10744390, add_frame = 0x103cbb30,
    row_for_item = 0x10743510, default_action = 0x102a54b0, common_action = 0x102a83a0, make_date = 0x10715880,
    date_text = 0x10716eb0, month_name = 0x107189d0, day_name = 0x107187d0, local_time = 0x1077f940, clock_time = 0x1077f900,
    person_name = 0x100caeb0, get_date_time = 0x10780900, microseconds = 0x10780600, text_object = 0x1074b730, init_text = 0x1074b6e0, destroy_text = 0x1074b7c0, set_text = 0x1074b830,
    draw_text = 0x1074bcd0, add_text = 0x1074a350, add_text_object = 0x1074a500;
const std::set<std::uint32_t> scripted{history_value, set_history, append_history, localized, format, error_dialog, copy_limited, allocate,
    release, allocate_object, usable_path, create_directory, file_exists, find_first, find_next, glue_call, language,
    open_entry, read_entry, close_entry, begin_screen, begin_plain, screen_colours, screen_title, panel, add_table, add_field, select_row,
    add_cell, add_button, add_box, add_frame, row_for_item, default_action, common_action, make_date, date_text, month_name,
    day_name, local_time, clock_time, person_name, text_object, init_text, destroy_text, set_text, draw_text, add_text, add_text_object};

// On a real snapshot the screen manager, controls and text run for real; these reach
// files, the clock or (the error dialog) a window this pass can't show.
const std::set<std::uint32_t> real_scripted{open_entry, read_entry, close_entry, clock_time, local_time, find_first, find_next, file_exists,
    create_directory, usable_path, error_dialog, get_date_time, microseconds};
using History = std::map<std::uint32_t, std::uint32_t>;
struct Person { std::uint8_t manager = 0; std::uint32_t club = 0, second = 0; };
struct Case {
    std::uint32_t entry = open_info;
    std::string label{};
    std::array<std::uint32_t, 8> arguments{};
    History history{};
    std::array<std::uint32_t, 4> stack_words{};    // caller's sp+0x38 on: height, parent, three flag words
    std::uint32_t stack_flags = 0;
    std::uint32_t append = 1, folders = 0, created = 0, usable = 1, exists = 1, opened = data + 0x700, version = 0x3e027, bad_read = 0,
        answer = 0, pending = 0, pending_action = 0, action = 0, key = 0, allocate_fails = 0, default_result = 0, common_result = 0,
        spoken = 0, guard = 0, played = 0, saved = 0, wday = 3, table_row = 0x10, find_found = 0, find_end = 1, now = 0x12345, last_time = 0x12000,
        total_time = 0x500, person_count = 16, current_window = 0;
    std::vector<std::uint32_t> filter_flags{};     // what the filter returns, in turn
    std::string background{};
    std::vector<Person> managers{};
    std::uint32_t league_flags = 0;                // flags of the first two leagues, in their low bytes
    std::function<void(Cpu&)> prepare{};
};
Case active;
bool real_mode = false;
std::vector<std::uint8_t> real_summary;  // what the game's own writer wrote, for the reader
bool original_mode = false, production_mode = false, ownership_mode = false, wrong_argument = false, wrong_return = false, clobbers = false;
unsigned calls = 0, next_filter = 0, files_left = 0, tables = 0;
std::uint32_t next_heap = 0;
History history;

// Instructions a production run may spend outside the probe between calls, each
// below what the original's first run charges: the leaf routines (the find handle's
// close, the colour maker, the version text and abs) run before the probe.
std::uint64_t slack(std::uint32_t entry) {
    return entry == render_dialog ? 2 : entry == render_info ? 8 : entry == fill_summary ? 8 : entry == dialog_action ? 4 : 0;
}
void trace(Cpu& c, std::uint32_t target) {
    if (++calls > 10000) throw std::runtime_error("fixture call limit");
    static const bool show_calls = std::getenv("FILE_SCREENS_TRACE") != nullptr;
    if (show_calls) std::cout << (original_mode ? "O " : "C ") << hex_address(target) << " back " << hex_address(c.lr())
        << " r3 " << hex_address(c.r(3)) << " r4 " << hex_address(c.r(4)) << std::endl;
    auto index = c.u32(trace_at);
    const auto add = [&](std::uint32_t word) { c.put32(trace_at + 4 + index++ * 4, word); };
    add(target); add(c.lr() ^ (wrong_return && !original_mode ? 4 : 0));
    for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned f = 0; f < 8; ++f) add(c.cr(f));
    for (unsigned word = 0x38; word <= 0x60; word += 4) add(c.u32(c.sp() + word));
    c.put32(trace_at, index);
}
void write_text(Cpu& c, std::uint32_t at, std::string_view text) {
    for (const auto character : text) c.put8(at++, std::uint8_t(character));
    c.put8(at, 0);
}
void copy_text(Cpu& c, std::uint32_t to, std::uint32_t from) {
    while (const auto character = c.u8(from++)) c.put8(to++, character);
    c.put8(to, 0);
}
std::uint32_t summary_size(std::uint32_t size) { return size == 0xf70 ? 1 : 2; }
std::uint32_t script(Cpu& c, std::uint32_t target) {
    const auto arg3 = c.r(3), arg4 = c.r(4), arg5 = c.r(5);
    switch (target) {
    case history_value: {
        const auto found = history.find(arg4);
        return found == history.end() ? 0x77000000 + arg4 : found->second;
    }
    case set_history: history[arg4] = arg5; return arg3 + arg4;
    case append_history: return active.append;
    case localized: case format:
        copy_text(c, arg3, arg4);
        return arg3;
    case copy_limited:
        if (arg4) for (std::uint32_t n = 0; n < arg5; ++n) {
            const auto character = c.u8(arg4 + n);
            c.put8(arg3 + n, character);
            if (!character) break;
        }
        return arg3;
    case allocate: {
        const auto at = next_heap;
        next_heap += (arg3 + 15) & ~15u;
        return at;
    }
    case allocate_object:
        if (active.allocate_fails) return 0;
        next_heap += 16;
        return next_heap - 16;
    case usable_path: return active.usable;
    case create_directory: return active.created;
    case file_exists: return active.exists;
    case find_first: case find_next: {
        const auto record = target == find_first ? arg4 : arg3;
        if (!files_left) return active.find_end;
        write_text(c, record, "file" + std::to_string(--files_left) + ".sav");
        return active.find_found;
    }
    case glue_call: return next_filter < active.filter_flags.size() ? active.filter_flags[next_filter++] : 1;
    case language: return active.spoken;
    case open_entry: return active.opened;
    case read_entry:
        if (arg4 == 4) {
            if (active.bad_read & 1) return 0;
            c.put32(arg3, active.version);
            return 1;
        }
        if (active.bad_read & summary_size(arg4) << 1) return 0;
        if (real_mode) {
            for (std::uint32_t at = 0; at < arg4; ++at) c.put8(arg3 + at, at < real_summary.size() ? real_summary[at] : 0);
            c.put32(arg3 + 0xf6c, active.played);
            if (arg4 == 0xf74) c.put32(arg3 + 0xf70, active.saved);
            return 1;
        }
        for (std::uint32_t at = 0; at < arg4; at += 4) c.put32(arg3 + at, 0x5a5a5a5a);
        write_text(c, arg3, "Premier");
        write_text(c, arg3 + 0x3e8, active.background);
        write_text(c, arg3 + 0x7d0, "3.9.68");
        for (std::uint32_t at = 0; at < 16; ++at) {
            c.put8(arg3 + 0x7e4 + at * 0x78, 0);
            c.put8(arg3 + 0x820 + at * 0x78, 0);
        }
        for (std::size_t at = 0; at < active.managers.size(); ++at) {
            const auto record = arg3 + 0x7e4 + std::uint32_t(at) * 0x78;
            if (active.managers[at].manager) write_text(c, record, "Manager");
            if (active.managers[at].club) write_text(c, record + 0x3c, "Club");
        }
        c.put32(arg3 + 0xf6c, active.played);
        if (arg4 == 0xf74) c.put32(arg3 + 0xf70, active.saved);
        return 1;
    case add_table: return ++tables == active.table_row ? 0xffff : 0x100 + tables;
    case row_for_item: return arg4 + 2;
    case default_action: return active.default_result;
    case common_action: return active.common_result;
    case month_name: return text_b;
    case day_name: return text_c;
    case local_time:
        c.put32(tm_at + 4, 7); c.put32(tm_at + 8, 21); c.put32(tm_at + 0xc, 17); c.put32(tm_at + 0x10, 7);
        c.put32(tm_at + 0x14, 101); c.put32(tm_at + 0x18, active.wday);
        return tm_at;
    case clock_time:
        if (arg3) c.put32(arg3, active.now);
        return active.now + (arg3 ? 0 : 7);
    case person_name: write_text(c, arg4, "Person"); return 1;
    case microseconds:
        c.put32(arg3, 0);
        c.put32(arg3 + 4, active.now);
        return 0;
    case get_date_time:
        c.put32(arg3, active.now);
        return 0;
    default: return arg3 + 1;
    }
}
bool probe(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    if (target >= module_low && target < module_high) {
        if (original_mode) return file_screens_reference(s);
        if (production_mode) return false;
        file_screens_run_rewrite(s, target);
        return true;
    }
    // The dialog's filter has no real routine behind a fixture's vector.
    const bool stubbed_filter = real_mode && target == glue_call && c.lr() == 0x100ee558;
    if (!stubbed_filter && !(real_mode ? real_scripted : scripted).contains(target)) {
        if (real_mode) {
            // Only the module's own calls: the real routines make over a million more.
            if (target != c.lr() && c.lr() >= module_low && c.lr() < module_high && !is_leaf_entry(target)) trace(c, target);
            return false;
        }
        if (ownership_mode) {
            // Runtime routines may run; the module has no instruction budget of its own.
            const auto saved = dispatch_probe;
            dispatch_probe = nullptr;
            s.budget = s.executed + 1000000;
            const auto back = c.lr();
            s.pc = target;
            try { run_until(s, back); }
            catch (...) { dispatch_probe = saved; throw; }
            dispatch_probe = saved;
            s.budget = s.executed + slack(active.entry);
            return true;
        }
        // A real runtime call: on the pc == lr dispatches a caller merely continues.
        if (target != c.lr() && !is_leaf_entry(target)) trace(c, target);
        return false;
    }
    if (ownership_mode) s.budget = s.executed + slack(active.entry);
    if (wrong_argument && !original_mode) c.set(3, c.r(3) ^ 1);
    trace(c, target);
    if (target == glue_call || target == get_date_time || target == microseconds) c.put32(c.sp() + 0x14, c.r(2));
    const auto answer = script(c, target);
    if (clobbers) {
        const auto mark = 0xc0000000u ^ (target << 4) ^ calls;
        for (unsigned r : {0u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u}) c.set(r, mark + r);
        c.set_cr(0, std::uint8_t(calls & 0xe));
        c.set_cr(1, std::uint8_t((calls >> 1) & 0xe));
        if (target == glue_call) c.set(2, mark);
    }
    c.set(3, answer);
    s.pc = c.lr();
    return true;
}
void reset_script() {
    calls = next_filter = tables = 0;
    next_heap = heap;
    files_left = active.folders;
    history = active.history;
}
void initialize(Cpu& c) {
    c.s.memory.map(data, 0x400000);
    c.s.memory.map(0x71000000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x60000, 0xa5));
    c.s.memory.copy_in(0x71000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, std::uint8_t(f + 1));
    c.s.wr<1>(reg::xer_so, clobbers);
    c.set(1, stack);
    c.set(2, toc_base);
    c.set_lr(sentinel);
    c.put32(trace_at, 0);
    write_text(c, text_a, "alpha");
    write_text(c, text_b, "bravo");
    write_text(c, text_c, "charlie");
    write_text(c, folder_buffer, "saves");
    write_text(c, copy_buffer, "backup");
    write_text(c, name_buffer, "career.sav");
    write_text(c, path_buffer, "");
    const auto screens = c.u32(screens_slot);
    c.put16(screens + 0x2c58, active.current_window);
    c.put32(screens + 0x24 + active.current_window * 0x2c0, controls);
    c.put8(screens + 0x2c5f, active.pending);
    c.put32(screens + 0x260000 - 0x7e60, 0x7777);
    c.put32(screens + 0x260000 - 0x7e5c, active.answer);
    c.put16(screens + 0x260000 - 0x7e62, active.pending_action);
    c.put8(c.toc(-0x74b0), active.key);
    for (const std::uint32_t id : {0u, 1u, 7u, 0xffffu - 1u})
        c.put32(controls + 0xc0000 + std::uint32_t(std::int16_t(id)) * 0x17c - 0x5794, active.action);
    c.put8(toc_base - 0x39b6, active.guard);
    c.put32(c.toc(-0x74b8), active.last_time);
    c.put32(c.toc(-0x74b4), active.total_time);
    // Leagues: names first, flags in the first two records' low bytes.
    for (std::uint32_t at = 0; at < 0x22; ++at) c.put32(c.toc(-0x7b44) + at * 0x4c, 0);
    for (std::uint32_t at = 0; at < 3; ++at) {
        const auto league = leagues_at + at * 0x200;
        write_text(c, league + 0xe1, "League" + std::to_string(at));
        c.put8(league + 0x120, (active.league_flags >> (8 * at)) & 0xff);
        c.put32(c.toc(-0x7b44) + (at + 1) * 0x4c, active.league_flags >> (8 * at) & 0xff ? league : 0);
    }
    c.put32(c.toc(-0x7be4), active.person_count);
    c.put32(c.toc(-0x7be0), people_at);
    for (std::uint32_t at = 0; at < 16; ++at) c.put8(people_at + at * 0x70 + 0x6b, 0);
    for (std::size_t at = 0; at < active.managers.size(); ++at) {
        // The summary lists the people from the end backwards.
        const auto person = people_at + (active.person_count - std::uint32_t(at) - 1) * 0x70;
        const auto& manager = active.managers[at];
        c.put8(person + 0x6b, manager.manager);
        c.put32(person + 0x24, manager.club ? clubs_at : 0);
        c.put32(person + 0x20, manager.second ? clubs_at + 0x400 : 0);
    }
    write_text(c, clubs_at + 0x221, "First");
    write_text(c, clubs_at + 0x400 + 0x221, "Second");
    for (unsigned n = 0; n < 8; ++n) c.set(3 + n, active.arguments[n]);
    // The caller's outgoing arguments.
    for (unsigned n = 0; n < active.stack_words.size(); ++n) c.put32(stack + 0x38 + n * 4, active.stack_words[n]);
    c.put8(stack + 0x4b, active.stack_flags & 0xff);
    c.put8(stack + 0x4f, active.stack_flags >> 8);
    c.put8(stack + 0x43, active.stack_flags >> 16);
    c.put8(stack + 0x47, active.stack_flags >> 24);
    if (active.prepare) active.prepare(c);
    c.s.budget = c.s.executed + 5000000;
    reset_script();
}
void flush_coverage() {
#if defined(CM_FILE_SCREENS_COVERAGE)
    // LLVM expands %p before fork: name each child's profile explicitly.
    const auto profile = std::string(CM_FILE_SCREENS_PROFILE_DIR) + "/check-" + std::to_string(getpid()) + ".profraw";
    __llvm_profile_set_filename(profile.c_str());
    if (__llvm_profile_write_file() != 0) throw std::runtime_error("Cannot flush FILE_SCREENS child coverage");
#endif
}
void original(State& s, std::uint32_t entry) {
    original_mode = true;
    reset_script();
    s.pc = entry;
    run_until(s, sentinel);
    flush_coverage();
}
void rewrite(State& s, std::uint32_t entry) {
    original_mode = false;
    reset_script();
    if (production_mode) {
        s.pc = entry;
        run_until(s, sentinel);
    } else {
        file_screens_run_rewrite(s, entry);
    }
    flush_coverage();
}
void corrupt_memory(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.put8(text_a, c.u8(text_a) ^ 1); }
void corrupt_register(State& s, std::uint32_t entry) { rewrite(s, entry); s.wr<4>(reg::gpr(5), s.r<4>(reg::gpr(5)) ^ 1); }
void corrupt_result(State& s, std::uint32_t entry) { rewrite(s, entry); s.wr<4>(reg::gpr(3), s.r<4>(reg::gpr(3)) ^ 1); }
void corrupt_cr(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.set_cr(0, c.cr(0) ^ 2); }
void corrupt_stack(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.put8(stack - 0x60, c.u8(stack - 0x60) ^ 1); }
void corrupt_fpscr(State& s, std::uint32_t entry) { rewrite(s, entry); s.wr<1>(1296, s.r<1>(1296) ^ 1); }
struct Observed final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
};

History dialog_history(std::uint32_t mode, std::uint32_t question = 0) {
    return {{0, text_a}, {1, text_b}, {2, mode}, {3, folder_buffer}, {4, text_c}, {5, 0}, {6, copy_buffer}, {7, name_buffer}, {8, path_buffer},
            {9, question}, {10, 0x1234}, {11, 0xfedc}, {12, 0xabcd}, {13, 0}, {14, 3}, {15, 2}, {16, 1}, {17, 0}, {18, 0}};
}
// Every entry from a real capture's state. Each case first opens the screen it
// needs with the original, then compares the entry; the screen manager's real
// history, controls and text sit behind it.
unsigned run_real_state(const std::filesystem::path& program, const std::filesystem::path& snapshot) {
    State base;
    load_program(base, program);
    load_match_snapshot(base, snapshot);
    // The snapshot stops at the end of an import, inside its glue: the game's
    // TOC is the one the glue saved at sp+0x14.
    base.wr<4>(reg::gpr(2), base.memory.read<4>(std::uint32_t(base.r<4>(reg::gpr(1))) + 0x14));
    if (base.r<4>(reg::gpr(2)) != toc_base) throw std::runtime_error("The snapshot does not hold the game's TOC at sp+0x14");
    base.memory.map(data, 0x400000);
    base.memory.copy_in(data, std::vector<std::uint8_t>(0x60000, 0xa5));
    real_mode = true;
    // The capture starts between screens, with no group current; by its end group 0
    // is current, and it is the group with a window.
    const auto manager = std::uint32_t(base.memory.read<4>(screens_slot));
    base.memory.write<2>(manager + 0x2c58, 0);
    if (!base.memory.read<4>(manager + 0x24) || base.memory.read<4>(toc_base - 0x7be4) == 0 ||
        base.memory.read<4>(std::uint32_t(base.memory.read<4>(toc_base - 0x7be4))) < 8 ||
        !base.memory.read<4>(std::uint32_t(base.memory.read<4>(toc_base - 0x7be8))))
        throw std::runtime_error("The snapshot lacks the window, people or clubs this pass uses");
    const auto frame = (std::uint32_t(base.r<4>(reg::gpr(1))) - 0x10000) & ~0xfu;
    const auto path = data + 0x1000, summary = data + 0x10000, heading = data + 0x1100, body = data + 0x1200, folder = data + 0x1400, name = data + 0x1500,
               extension = data + 0x1540, object_text = data + 0x1580, blank = data + 0x15c0;
    const auto write_texts = [=](Cpu& c) {
        write_text(c, path, "Mac HD:Saves:career.sav");
        write_text(c, heading, "Save Game");
        write_text(c, body, "A real message");
        write_text(c, folder, "Mac HD:Saves");
        write_text(c, name, "career.sav");
        write_text(c, extension, ".sav");
        c.put8(blank, 0);
        c.put32(object_text, body);
        c.put32(object_text + 4, 0);
    };
    // Runs an entry with the original to prepare the state a later one reads.
    const auto prepare = [=](Cpu& c, std::uint32_t entry, std::initializer_list<std::uint32_t> arguments) {
        unsigned n = 3;
        for (const auto value : arguments) c.set(n++, value);
        c.set(1, frame);
        c.set_lr(sentinel);
        original_mode = true;
        c.s.pc = entry;
        run_until(c.s, sentinel);
    };
    // Real leagues in both lists, and managers with every mix of clubs.
    const auto enrich = [=](Cpu& c) {
        const auto leagues = c.toc(-0x7b44);
        unsigned seen = 0;
        for (std::uint32_t at = 0; at < 0x22 && seen < 5; ++at) {
            const auto league = c.u32(leagues + at * 0x4c);
            if (league) c.put8(league + 0x120, ++seen % 3 + 1);
        }
        const auto clubs = c.u32(c.toc(-0x7be8)), people = c.u32(c.toc(-0x7be0)), count = c.u32(c.toc(-0x7be4));
        // Only the last person has a name here: the others borrow its record.
        const auto named = people + (count - 1) * 0x70;
        for (std::uint32_t at = 0; at < 8; ++at) {
            const auto person = people + (count - at - 1) * 0x70;
            if (at) for (std::uint32_t byte = 0; byte < 0x70; ++byte) c.put8(person + byte, c.u8(named + byte));
            c.put8(person + 0x6b, at == 7 ? 0 : 1);
            c.put32(person + 0x24, at % 4 < 2 ? clubs + at * 0x248 : 0);
            c.put32(person + 0x20, at % 2 ? clubs + (at + 1) * 0x248 : 0);
        }
    };
    struct DialogOptions { std::uint32_t mode = 0, folders_only = 0, plain_cancel = 0, info_button = 1; bool blank_name = false; std::uint32_t extension_text = extension, filter = 0; bool rendered = true; };
    const auto open_dialog_screen = [=](Cpu& c, const DialogOptions& o) {
        c.put32(frame + 0x38, 0xfedc); c.put32(frame + 0x3c, 0x1234);
        c.put8(frame + 0x43, o.folders_only); c.put8(frame + 0x47, 0); c.put8(frame + 0x4b, o.plain_cancel); c.put8(frame + 0x4f, o.info_button);
        prepare(c, open_dialog, {heading, body, o.mode, folder, o.extension_text, name, o.filter, 0x1234});
        // The typed name, as the dialog's field holds it.
        c.set(3, c.u32(screens_slot));
        c.set(4, 7);
        c.set(1, frame);
        c.call(history_value, sentinel);
        if (o.blank_name) c.put8(c.r(3), 0);
        else write_text(c, c.r(3), "career.sav");
        // As the game does before any action: this builds the list the action reads.
        if (o.rendered) prepare(c, render_dialog, {});
    };
    const auto pending_action = [](Cpu& c, std::uint32_t action) { c.put16(c.u32(screens_slot) + 0x260000 - 0x7e62, action); };
    const auto question_pending = [](Cpu& c, std::uint32_t question, std::uint32_t answer) {
        const auto screens = c.u32(screens_slot);
        c.set(3, screens); c.set(4, 9); c.set(5, question); c.set(6, 0);
        c.call(set_history, sentinel);
        c.put8(screens + 0x2c5f, 1);
        c.put32(screens + 0x260000 - 0x7e5c, answer);
    };
    struct RealCase {
        std::string label;
        std::uint32_t entry;
        Case data;
        std::function<void(Cpu&)> setup;
        std::array<std::uint32_t, 8> arguments{};
        std::array<std::uint8_t, 4> caller_flags{};  // bytes at the caller's sp+0x43, +0x47, +0x4b and +0x4f
    };
    std::vector<RealCase> cases;
    const auto add = [&](RealCase test) { cases.push_back(std::move(test)); };
    add({"summary", fill_summary, {}, {}, {summary}});
    add({"summary, leagues and managers", fill_summary, {}, enrich, {summary}});
    add({"open info", open_info, {}, {}, {path}});
    const auto info_screen = [=](Cpu& c) { prepare(c, open_info, {path}); };
    // The rating rows exist only in language 0.
    const auto info_in_english = [=](Cpu& c) { info_screen(c); c.put8(toc_base - 0x35ac, 0); };
    for (const std::uint32_t version : {0x3e027u, 0x3e026u, 0xfffc1fd9u})
        for (const std::uint32_t played : {0u, 3725u, 3 * 86400u + 7265u, 11 * 86400u + 13 * 3600u}) {
            add({"info screen", render_info, {.version = version, .played = played, .saved = 0x3b000000}, info_screen});
            add({"info screen, rated", render_info, {.version = version, .played = played, .saved = 0x3b000000}, info_in_english});
        }
    for (const std::uint32_t days : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u})
        add({"rating", render_info, {.played = days * 86400 + 60, .saved = 0x3b000000}, info_in_english});
    add({"rating, day and a half", render_info, {.played = 86400 + 13 * 3600, .saved = 0x3b000000}, info_in_english});
    add({"rating, day and under half", render_info, {.played = 86400 + 12 * 3600, .saved = 0x3b000000}, info_in_english});
    add({"info no entry", render_info, {.opened = 0}, info_screen});
    add({"info no version", render_info, {.bad_read = 1}, info_screen});
    add({"info no summary", render_info, {.bad_read = 4}, info_screen});
    add({"info no old summary", render_info, {.version = 0x3e026, .bad_read = 2}, info_screen});
    for (const std::uint32_t action : {0u, 1u, 2u})
        add({"info action", info_action, {}, [=](Cpu& c) { info_screen(c); pending_action(c, action); }, {0xffff}});
    for (const std::uint32_t confirm : {0u, 1u}) {
        const auto message_screen = [=](Cpu& c) { prepare(c, open_message, {heading, body, body, confirm, 0x1234, 0x7654, 0x55}); };
        add({"open message", open_message, {}, {}, {heading, body, body, confirm, 0x1234, 0x7654, 0x55}});
        add({"message screen", render_message, {}, message_screen});
        const auto long_screen = [=](Cpu& c) { prepare(c, open_long, {heading, body, object_text, confirm, 0x1234, 0x7654, 0x55}); };
        add({"open long message", open_long, {}, {}, {heading, body, object_text, confirm, 0x1234, 0x7654, 0x55}});
        add({"long message screen", render_message, {}, long_screen});
        for (const std::uint32_t key : {std::uint32_t('Y'), std::uint32_t('N'), std::uint32_t('\r'), 0u})
            for (const auto& screen : {std::function<void(Cpu&)>(message_screen), std::function<void(Cpu&)>(long_screen)})
                add({"message key", message_action, {},
                     [=](Cpu& c) { screen(c); prepare(c, render_message, {}); c.put8(c.toc(-0x74b0), key); }, {0xffff}});
        for (const std::uint32_t action : {1u, 2u, 3u})
            add({"message action", message_action, {}, [=](Cpu& c) { long_screen(c); prepare(c, render_message, {}); pending_action(c, action); }, {0xffff}});
    }
    add({"message default", message_action, {}, [=](Cpu& c) {
             prepare(c, open_message, {heading, body, body, 0, 0x1234, 0x7654, 0x55}); prepare(c, render_message, {}); pending_action(c, 6); }, {0xffff}});
    for (const std::array<std::uint8_t, 4> flags : {std::array<std::uint8_t, 4>{0, 0, 0, 0}, {1, 0, 1, 1}, {0x80, 0xff, 0x80, 0x80}})
        for (const std::uint32_t mode : {0u, 1u, 2u, 0x101u})
            add({"open dialog", open_dialog, {}, {}, {heading, body, mode, folder, extension, name, 0, 0x1234}, flags});
    for (const std::uint32_t mode : {0u, 1u, 2u}) {
        const DialogOptions options{.mode = mode};
        const auto screen = [=](Cpu& c) { open_dialog_screen(c, options); };
        for (const std::uint32_t files : {0u, 3u, 8u})
            add({"dialog screen", render_dialog, {.folders = files}, [=](Cpu& c) { open_dialog_screen(c, {.mode = mode, .rendered = false}); }});
        for (const std::uint32_t action : {1u, 3u, 4u, 5u})
            for (const std::uint32_t exists : {0u, 1u})
                add({"dialog action", dialog_action, {.usable = exists, .exists = exists}, [=](Cpu& c) { screen(c); pending_action(c, action); }, {0xffff}});
        for (const std::uint32_t key : {std::uint32_t('Y'), std::uint32_t('N')})
            add({"dialog key", dialog_action, {}, [=](Cpu& c) { screen(c); c.put8(c.toc(-0x74b0), key); }, {0xffff}});
        add({"ok, folders only", dialog_action, {}, [=](Cpu& c) { open_dialog_screen(c, {.mode = mode, .folders_only = 1}); pending_action(c, 4); }, {0xffff}});
        add({"ok, blank folder", dialog_action, {}, [=](Cpu& c) {
                 open_dialog_screen(c, {.mode = mode, .folders_only = 1, .blank_name = true}); pending_action(c, 4); }, {0xffff}});
        add({"ok, blank name", dialog_action, {}, [=](Cpu& c) { open_dialog_screen(c, {.mode = mode, .blank_name = true}); pending_action(c, 4); }, {0xffff}});
        add({"info, blank name", dialog_action, {}, [=](Cpu& c) { open_dialog_screen(c, {.mode = mode, .blank_name = true}); pending_action(c, 5); }, {0xffff}});
        add({"ok, no extension", dialog_action, {}, [=](Cpu& c) { open_dialog_screen(c, {.mode = mode, .extension_text = 0}); pending_action(c, 4); }, {0xffff}});
        // The three questions and each answer.
        for (const std::uint32_t question : {1u, 2u, 3u, 4u})
            for (const std::uint32_t answer : {0u, 1u, 2u})
                for (const std::uint32_t created : {0u, 1u})
                    add({"question", dialog_action, {.created = created}, [=](Cpu& c) { screen(c); question_pending(c, question, answer); }, {0xffff}});
        // A control picks a name from the list.
        const auto with_control = [=](Cpu& c, std::uint32_t action) {
            screen(c);
            const auto controls_at = c.u32(c.u32(screens_slot) + 0x24) + 0xc0000 + 3 * 0x17c;
            c.put32(controls_at - 0x5794, action);
            write_text(c, controls_at - 0x5724, "chosen.sav");
        };
        add({"pick", dialog_action, {}, [=](Cpu& c) { with_control(c, 2); }, {3}});
        add({"control without action", dialog_action, {.default_result = 0x77}, [=](Cpu& c) { with_control(c, 0); }, {3}});
    }
    add({"common action", dialog_action, {}, [=](Cpu& c) { open_dialog_screen(c, {}); pending_action(c, 6); }, {0xffff}});
    add({"info, no entry", dialog_action, {.opened = 0}, [=](Cpu& c) { open_dialog_screen(c, {}); pending_action(c, 5); }, {0xffff}});
    add({"info, no version", dialog_action, {.bad_read = 1}, [=](Cpu& c) { open_dialog_screen(c, {}); pending_action(c, 5); }, {0xffff}});
    add({"info, old save", dialog_action, {.version = 0x3e022}, [=](Cpu& c) { open_dialog_screen(c, {}); pending_action(c, 5); }, {0xffff}});
    add({"info, opens", dialog_action, {.version = 0x3e023}, [=](Cpu& c) { open_dialog_screen(c, {}); pending_action(c, 5); }, {0xffff}});
    for (const std::array<std::uint8_t, 2> layout : {std::array<std::uint8_t, 2>{0, 0}, {1, 0}, {0, 1}, {1, 1}})
        for (const std::uint32_t only : {0u, 1u})
            add({"dialog layout", render_dialog, {.folders = 2}, [=](Cpu& c) {
                     open_dialog_screen(c, {.folders_only = only, .plain_cancel = layout[0], .info_button = layout[1], .rendered = false}); }});

    // A screen already in the history refuses to open again, in the real manager.
    add({"open info refused", open_info, {}, info_screen, {path}});
    add({"open message refused", open_message, {}, [=](Cpu& c) { prepare(c, open_message, {heading, body, body, 0, 0x1234, 0x7654, 0x55}); },
         {heading, body, body, 0, 0x1234, 0x7654, 0x55}});
    add({"open long message refused", open_long, {}, [=](Cpu& c) { prepare(c, open_long, {heading, body, object_text, 0, 0x1234, 0x7654, 0x55}); },
         {heading, body, object_text, 0, 0x1234, 0x7654, 0x55}});
    add({"open dialog refused", open_dialog, {}, [=](Cpu& c) { open_dialog_screen(c, {.rendered = false}); },
         {heading, body, 0, folder, extension, name, 0, 0x1234}});
    // A filter chooses rows and their colours.
    add({"filter", render_dialog, {.folders = 6, .filter_flags = {1, 5, 9, 13, 0, 2}}, [=](Cpu& c) { open_dialog_screen(c, {.filter = 0x10a00000, .rendered = false}); }});
    // Null arguments reach the assertion.
    for (const auto& [title, prompt, target] : std::array<std::array<std::uint32_t, 3>, 3>{{{0, body, folder}, {heading, 0, folder}, {heading, body, 0}}}) {
        add({"assertion", open_dialog, {}, {}, {title, prompt, 0, target, extension, name, 0, 0x1234}});
        for (const auto entry : {open_message, open_long})
            add({"assertion", entry, {}, {}, {title, prompt, target, 0, 0x1234, 0x7654, 0x55}});
    }

    // A state ready for a case, with the script reset.
    const auto fresh = [&](const Case& data, std::unique_ptr<State>& s) {
        active = data;
        s = cm::semantic_io_checks::clone(base);
        s->budget = s->executed + 200000000;
        Cpu c(*s);
        c.put32(trace_at, 0);
        write_texts(c);
        reset_script();
    };
    // The reader gets what the game's writer wrote, with both lists and every kind of row.
    {
        std::unique_ptr<State> s;
        fresh({}, s);
        Cpu c(*s);
        enrich(c);
        prepare(c, fill_summary, {summary});
        real_summary.clear();
        for (std::uint32_t at = 0; at < 0xf74; ++at) real_summary.push_back(c.u8(summary + at));
    }
    const auto ready = [&](const RealCase& test, std::unique_ptr<State>& s) {
        fresh(test.data, s);
        Cpu c(*s);
        for (unsigned n = 0; n < test.caller_flags.size(); ++n) c.put8(frame + 0x43 + n * 4, test.caller_flags[n]);
        if (test.entry == open_dialog) { c.put32(frame + 0x38, 0xfedc); c.put32(frame + 0x3c, 0x1234); }
        if (test.setup) test.setup(c);
        for (unsigned n = 0; n < 8; ++n) c.set(3 + n, test.arguments[n]);
        c.set(1, frame);
        c.set_lr(sentinel);
    };
    unsigned completed = 0, failures = 0;
    std::set<std::uint32_t> reached;
    Observed observer;
    for (const bool production : {false, true}) {
        production_mode = production;
        for (const auto& test : cases) {
            std::unique_ptr<State> s;
            std::vector<std::string> differences;
            try {
                ready(test, s);
                if (production) s->memory.observer = &observer;
                differences = compare_routine_call(*s, test.entry, original, rewrite, RoutineChecker::result_r3);
                s->memory.observer = nullptr;
            } catch (const std::exception& e) { differences.push_back(e.what()); }
            if (differences.empty()) { ++completed; reached.insert(test.entry); continue; }
            ++failures;
            if (failures < 8) {
                std::cout << "REAL DIFFERS " << hex_address(test.entry) << " '" << test.label << "' production=" << production << '\n';
                for (const auto& difference : differences) std::cout << "  " << difference << '\n';
            }
        }
    }
    // The comparison must notice a wrong byte the real routines wrote, a wrong result
    // and a wrong argument to a scripted call, through production too. A wrong return
    // address on the message screen, which calls only real routines, shows the module's
    // calls to them are recorded: nothing but the record differs.
    const RealCase info{"control", render_info, {.played = 3725, .saved = 0x3b000000}, info_screen};
    const RealCase message{"control", render_message, {}, [=](Cpu& c) { prepare(c, open_message, {heading, body, body, 1, 0x1234, 0x7654, 0x55}); }};
    for (const bool production : {false, true}) {
        production_mode = production;
        const auto detected = [&](const RealCase& test, RoutineRunner runner, bool corrupt = false) {
            std::unique_ptr<State> s;
            ready(test, s);
            if (production) s->memory.observer = &observer;
            const bool found = !compare_routine_call(*s, test.entry, original, runner, RoutineChecker::result_r3, corrupt).empty();
            s->memory.observer = nullptr;
            return found;
        };
        const bool byte = detected(info, rewrite, true);
        const bool register_value = detected(info, corrupt_register);
        const bool result = detected(info, corrupt_result);
        wrong_argument = true;
        const bool argument = detected(info, rewrite);
        wrong_argument = false;
        wrong_return = true;
        const bool recorded = detected(message, rewrite);
        wrong_return = false;
        if (byte && register_value && result && argument && recorded) std::cout << "ok real-state corruptions detected, production=" << production << '\n';
        else {
            ++failures;
            std::cout << "FAIL real-state corruption control, production=" << production << " byte=" << byte << " register=" << register_value
                      << " result=" << result << " argument=" << argument << " recorded=" << recorded << '\n';
        }
    }
    production_mode = false;
    real_mode = false;
    std::cout << "real state: " << completed << " returned comparisons, " << reached.size() << "/" << entries.size() << " entries, " << failures << " failures\n";
    if (reached.size() != entries.size()) ++failures;
    return failures;
}

std::vector<Case> all_cases() {
    std::vector<Case> cases;
    const auto add = [&](Case test) { cases.push_back(std::move(test)); };

    // open_file_dialog
    const auto dialog = [&](std::string label, std::uint32_t title, std::uint32_t prompt, std::uint32_t mode, std::uint32_t folder,
                            std::uint32_t extension, std::uint32_t preset, std::uint32_t append, std::uint32_t flags) {
        add({.entry = open_dialog, .label = std::move(label),
             .arguments = {title, prompt, mode, folder, extension, preset, 0x51515151, 0xfffe1234},
             .stack_words = {0x0000fedc, 0x7654321}, .stack_flags = flags, .append = append});
    };
    dialog("null title", 0, text_b, 0, folder_buffer, 0, 0, 1, 0);
    dialog("null prompt", text_a, 0, 0, folder_buffer, 0, 0, 1, 0);
    dialog("null folder", text_a, text_b, 0, 0, 0, 0, 1, 0);
    dialog("not appended", text_a, text_b, 0, folder_buffer, 0, 0, 0, 0);
    dialog("not appended, high bits", text_a, text_b, 0, folder_buffer, 0, 0, 0x100, 0);
    for (const std::uint32_t mode : {0u, 1u, 2u, 3u, 0x101u, 0x100u})
        for (const std::uint32_t preset : {0u, name_buffer})
            for (const std::uint32_t extension : {0u, text_c})
                for (const std::uint32_t flags : {0u, 0x01010101u, 0x80ff8001u})
                    dialog("open", text_a, text_b, mode, folder_buffer, extension, preset, 1, flags);

    // render_file_dialog
    for (const std::uint32_t folders : {0u, 1u, 3u, 5u, 6u, 7u, 10u})
        for (const std::uint32_t only : {0u, 1u, 0x180u})
            for (const std::uint32_t info : {0u, 1u})
                for (const std::uint32_t extra : {0u, 1u}) {
                    auto values = dialog_history(0);
                    values[13] = only; values[18] = info; values[17] = extra;
                    add({.entry = render_dialog, .label = "render", .history = values, .folders = folders});
                }
    {
        auto filtered = dialog_history(0);
        filtered[5] = 0x10a00000;
        add({.entry = render_dialog, .label = "filter", .history = filtered, .folders = 7,
             .filter_flags = {1, 5, 9, 13, 0, 2, 0xffff0001}});
        add({.entry = render_dialog, .label = "filter skip all", .history = filtered, .folders = 2, .filter_flags = {0, 4}});
    }
    add({.entry = render_dialog, .label = "no list", .history = dialog_history(0), .folders = 1, .table_row = 2});
    // Only the results' low bytes count.
    add({.entry = render_dialog, .label = "find result bytes", .history = dialog_history(0), .folders = 2, .find_found = 0x100, .find_end = 0x101});
    add({.entry = render_dialog, .label = "find end byte", .history = dialog_history(0), .folders = 0, .find_end = 0xff});

    // file_dialog_action: answers to the three questions.
    for (const std::uint32_t question : {1u, 2u, 3u, 4u, 0u})
        for (const std::uint32_t answer : {0u, 1u, 2u, 3u})
            for (const std::uint32_t created : {0u, 1u, 0x100u})
                for (const std::uint32_t pending : {1u, 0xffu})
                    add({.entry = dialog_action, .label = "question", .arguments = {0xffff}, .history = dialog_history(0, question),
                         .created = created, .answer = answer, .pending = pending});
    // Controls, the pending action and the key.
    for (const std::uint32_t control : {0xffffu, 0u, 1u, 7u, 0x7ffffu, 0xfffffffeu})
        for (const std::uint32_t action : {0u, 6u, 9u})
            for (const std::uint32_t pending_action : {0u, 6u})
                for (const std::uint32_t result : {0u, 0x123u})
                    add({.entry = dialog_action, .label = "control", .arguments = {control}, .history = dialog_history(0),
                         .pending_action = pending_action, .action = action, .default_result = result, .common_result = 0x55});
    for (const char key : {'N', 'n', 'Y', 'y', '\r', '\n', '\x1b', 'a', '\0', 'M', 'O', 'X', 'Z', 'm', 'o', 'x', 'z', '\x7f', '\x0c', '\x0e', '\x1a', '\x1c', '\xff'}) {
        add({.entry = dialog_action, .label = "key", .arguments = {0xffff}, .history = dialog_history(1), .key = std::uint32_t(std::uint8_t(key))});
        add({.entry = message_action, .label = "key", .arguments = {0xffff}, .history = {{3, 0}}, .key = std::uint32_t(std::uint8_t(key))});
        add({.entry = message_action, .label = "key yes/no", .arguments = {0xffff}, .history = {{3, 0x1101}, {6, 0}, {7, 9}}, .key = std::uint32_t(std::uint8_t(key))});
    }
    // up_action
    for (const std::uint32_t mode : {0u, 1u, 2u})
        for (const std::uint32_t usable : {0u, 1u, 0x100u}) {
            add({.entry = dialog_action, .label = "up", .arguments = {7}, .history = dialog_history(mode), .usable = usable, .action = 1});
            auto blank = dialog_history(mode);
            blank[3] = data + 0x1f00;
            add({.entry = dialog_action, .label = "up blank", .arguments = {7}, .history = blank, .usable = usable, .action = 1,
                 .prepare = [](Cpu& c) { c.put8(data + 0x1f00, 0); }});
        }
    // pick_action: from a control, and from none, whose pending action picks nothing.
    add({.entry = dialog_action, .label = "pick", .arguments = {7}, .history = dialog_history(0), .action = 2,
         .prepare = [](Cpu& c) { write_text(c, controls + 0xc0000 + 7 * 0x17c - 0x5724, "chosen name"); }});
    add({.entry = dialog_action, .label = "pick long", .arguments = {7}, .history = dialog_history(0), .action = 2,
         .prepare = [](Cpu& c) { write_text(c, controls + 0xc0000 + 7 * 0x17c - 0x5724, "a name longer than thirty one characters"); }});
    add({.entry = dialog_action, .label = "pick none", .arguments = {0xffff}, .history = dialog_history(0), .pending_action = 2});
    // cancel_action
    add({.entry = dialog_action, .label = "cancel", .arguments = {7}, .history = dialog_history(2), .action = 3});
    // info_action
    const auto info = [&](std::string label, Case test, std::uint32_t name = name_buffer, std::uint32_t extension = text_c) {
        test.entry = dialog_action;
        test.label = std::move(label);
        test.arguments = {7};
        test.action = 5;
        test.history = dialog_history(0);
        test.history[7] = name;
        test.history[4] = extension;
        add(std::move(test));
    };
    const auto blank_name = [](Cpu& c) { c.put8(data + 0x1f00, 0); };
    const auto matching_name = [](Cpu& c) { write_text(c, name_buffer, "career.alpha"); write_text(c, text_c, ".alpha"); };
    info("info blank", {.prepare = blank_name}, data + 0x1f00);
    info("info no extension", {}, name_buffer, 0);
    info("info other extension", {});
    info("info same extension", {.prepare = matching_name});
    info("info missing", {.exists = 0});
    info("info low-byte exists", {.exists = 0x100});
    info("info no entry", {.opened = 0});
    info("info no version", {.bad_read = 1});
    for (const std::uint32_t version : {0u, 0x3e022u, 0x3e023u, 0x3e024u, 0xfffc1fddu, 0xfffc1fdeu, 0x7fffffffu, 0x80000000u})
        info("info version", {.version = version});
    // ok_action
    for (const std::uint32_t mode : {0u, 1u, 2u, 3u, 0xffu})
        for (const std::uint32_t only : {0u, 1u, 0x180u, 0x100u})
            for (const std::uint32_t exists : {0u, 1u, 0x100u})
                for (const bool blank : {false, true}) {
                    auto values = dialog_history(mode);
                    values[13] = only;
                    Case test{.entry = dialog_action, .label = "ok", .arguments = {7}, .history = values, .exists = exists, .action = 4,
                              .default_result = mode > 2 ? 0x77u : 0u, .common_result = 0x88};
                    if (blank) { test.history[7] = data + 0x1f00; test.prepare = blank_name; }
                    add(test);
                }
    for (const std::uint32_t mode : {0u, 1u, 2u}) {
        auto values = dialog_history(mode);
        values[4] = 0;
        add({.entry = dialog_action, .label = "ok no extension", .arguments = {7}, .history = values, .action = 4});
        add({.entry = dialog_action, .label = "ok same extension", .arguments = {7}, .history = dialog_history(mode), .action = 4, .prepare = matching_name});
    }

    // open_save_info
    for (const std::uint32_t append : {0u, 1u, 0x100u, 0xffu})
        add({.entry = open_info, .label = "open info", .arguments = {path_buffer}, .append = append});

    // render_save_info
    const auto save = [&](std::string label, Case test) {
        test.entry = render_info;
        test.label = std::move(label);
        test.history = {{0, path_buffer}};
        add(std::move(test));
    };
    save("info no entry", {.opened = 0});
    save("info no version", {.bad_read = 1});
    save("info old summary fails", {.version = 0x3e026, .bad_read = 2});
    save("info new summary fails", {.version = 0x3e027, .bad_read = 4});
    for (const std::uint32_t version : {0x3e026u, 0x3e027u, 0xfffc1fd9u, 0xfffc1fdau, 0x7fffffffu, 0x80000000u})
        save("info version", {.version = version, .played = 100, .saved = 200});
    save("guard already set", {.guard = 1});
    save("guard other byte", {.guard = 0x80});
    for (const std::uint32_t wday : {0u, 1u, 3u, 6u, 7u, 0xffffffffu, 0x100u})
        save("weekday", {.saved = 0x1234, .wday = wday});
    for (const std::uint32_t days : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 24000u})
        for (const std::uint32_t hours : {0u, 1u, 12u, 13u, 23u})
            for (const std::uint32_t minutes : {0u, 59u})
                for (const std::uint32_t spoken : {0u, 1u, 0x100u})
                    save("played", {.spoken = spoken, .played = days * 86400 + hours * 3600 + minutes * 60 + 37});
    for (const std::uint32_t played : {0u, 1u, 59u, 60u, 3599u, 3600u, 86399u, 86400u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 0xfffeae80u, 0xfffeae7fu, 0xffffc4f0u})
        save("played edge", {.played = played});
    save("background", {.background = "Serie A"});
    {
        std::vector<Person> managers;
        for (std::uint8_t at = 0; at < 16; ++at) managers.push_back({std::uint8_t(at % 3 ? 1 : 0), at % 2u, 0});
        save("managers", {.managers = managers, .background = "Serie A"});
    }

    // save_info_action
    for (const std::uint32_t control : {0xffffu, 7u, 0x10007u})
        for (const std::uint32_t action : {0u, 1u, 2u, 0x10001u})
            for (const std::uint32_t pending : {0u, 1u, 2u, 0xffffu})
                add({.entry = info_action, .label = "info action", .arguments = {control}, .pending_action = pending, .action = action});

    // The message boxes.
    const auto message = [&](std::string label, std::uint32_t entry, std::array<std::uint32_t, 8> arguments, std::uint32_t append = 1, std::uint32_t fails = 0) {
        add({.entry = entry, .label = std::move(label), .arguments = arguments, .append = append, .allocate_fails = fails});
    };
    for (const auto entry : {open_message, open_long}) {
        message("null colours", entry, {0, text_a, text_b, 0, 1, 2, 3});
        message("null title", entry, {text_c, 0, text_b, 0, 1, 2, 3});
        message("null body", entry, {text_c, text_a, 0, 0, 1, 2, 3});
        message("not appended", entry, {text_c, text_a, text_b, 0, 1, 2, 3}, 0);
        message("not appended, high bits", entry, {text_c, text_a, text_b, 0, 1, 2, 3}, 0x100);
        for (const std::uint32_t confirm : {0u, 1u, 0x101u})
            for (const std::uint32_t width : {0u, 0xffffu, 0x18000u})
                message("message", entry, {text_c, text_a, text_b, confirm, width, 0x12345, 0x777});
    }
    message("no object", open_long, {text_c, text_a, text_b, 0, 1, 2, 3}, 1, 1);
    message("no object, not appended", open_long, {text_c, text_a, text_b, 0, 1, 2, 3}, 0, 1);
    for (const std::uint32_t body : {0u, text_b})
        for (const std::uint32_t confirm : {0u, 1u, 2u, 0x100u, 0x101u})
            for (const std::uint32_t width : {0x1234u, 0xffffu})
                add({.entry = render_message, .label = "render message",
                     .history = {{0, text_c}, {1, text_a}, {2, body}, {3, confirm}, {4, width}, {5, 0x8000}, {6, data + 0x700}, {7, 0x1234}}});
    for (const std::uint32_t action : {0u, 1u, 2u, 3u, 4u, 0x10001u})
        for (const std::uint32_t pending : {0u, 2u})
            for (const std::uint32_t object : {0u, data + 0x700})
                for (const std::uint32_t result : {0u, 0x99u}) {
                    add({.entry = message_action, .label = "message action", .arguments = {7}, .history = {{3, 1}, {6, object}, {7, 0x333}},
                         .pending_action = pending, .action = action, .default_result = result, .common_result = 0x44});
                    add({.entry = message_action, .label = "message action no control", .arguments = {0xffff}, .history = {{3, 1}, {6, object}, {7, 0x333}},
                         .pending_action = pending, .action = action, .default_result = result, .common_result = 0x44});
                }

    // Another window is current.
    add({.entry = render_dialog, .label = "second window", .history = dialog_history(0), .folders = 2, .current_window = 1});
    add({.entry = render_info, .label = "second window", .history = {{0, path_buffer}}, .current_window = 2});
    add({.entry = render_message, .label = "second window", .history = {{3, 0}}, .current_window = 1});
    add({.entry = render_message, .label = "second window, question", .history = {{3, 1}}, .current_window = 1});
    add({.entry = dialog_action, .label = "second window", .arguments = {7}, .history = dialog_history(0), .action = 2, .current_window = 1,
         .prepare = [](Cpu& c) { write_text(c, controls + 0xc0000 + 7 * 0x17c - 0x5724, "chosen name"); }});
    add({.entry = info_action, .label = "second window", .arguments = {7}, .action = 1, .current_window = 3});
    add({.entry = message_action, .label = "second window", .arguments = {7}, .history = {{6, 0}}, .action = 1, .current_window = 1});
    add({.entry = dialog_action, .label = "mode high bits", .arguments = {7}, .history = dialog_history(0x101), .usable = 0, .action = 1});
    add({.entry = dialog_action, .label = "mode high bits ok", .arguments = {7}, .history = dialog_history(0x100), .action = 4});

    // fill_save_summary
    for (const std::uint32_t flags : {0u, 0x010101u, 0x020202u, 0x030303u, 0x010203u, 0x000302u, 0xfefdfcu})
        add({.entry = fill_summary, .label = "summary", .arguments = {data + 0x10000}, .league_flags = flags});
    {
        std::vector<Person> all;
        for (std::uint32_t at = 0; at < 16; ++at) all.push_back({std::uint8_t(at % 5 == 4 ? 0 : 1), at % 2, (at >> 1) % 2});
        add({.entry = fill_summary, .label = "people", .arguments = {data + 0x10000}, .managers = all, .league_flags = 0x030303});
        for (const std::uint32_t manager : {0u, 2u, 0x101u})
            add({.entry = fill_summary, .label = "byte", .arguments = {data + 0x10000}, .managers = {{std::uint8_t(manager), 1, 1}}, .person_count = 20});
        add({.entry = fill_summary, .label = "clock wraps", .arguments = {data + 0x10000}, .now = 5, .last_time = 0xfffffff0, .total_time = 0x7fffffff});
    }
    return cases;
}
}
int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: cm-file-screens-checks <export> [<a session capture's initial.snapshot>]\n";
        return 2;
    }
    try {
        auto base = std::make_unique<State>();
        load_program(*base, argv[1]);
        dispatch_probe = probe;
        // A leaf runs before the probe from C++ and after it from a page: it can't be scripted.
        for (const auto& list : {scripted, real_scripted})
            for (const auto target : list)
                if (is_leaf_entry(target)) throw std::runtime_error("Scripted leaf " + hex_address(target));
        const auto cases = all_cases();
        unsigned completed = 0, failures = 0;
        std::map<std::uint32_t, std::array<unsigned, 2>> counts;
        for (bool production : {false, true}) {
            production_mode = production;
            for (const auto& test : cases) for (bool clobber : {false, true}) {
                if (failures >= 8) break;
                if (const auto* only = std::getenv("FILE_SCREENS_ONLY"); only && test.label != only) continue;
                active = test;
                clobbers = clobber;
                auto s = cm::semantic_io_checks::clone(*base);
                Cpu c(*s);
                initialize(c);
                Observed observer;
                if (production) s->memory.observer = &observer;
                const auto differences = compare_routine_call(*s, test.entry, original, rewrite, RoutineChecker::result_r3);
                if (differences.empty()) { ++completed; ++counts[test.entry][production ? 1 : 0]; continue; }
                std::cout << "DIFFERS " << hex_address(test.entry) << " '" << test.label << "' production=" << production
                          << " clobber=" << clobber << '\n';
                for (const auto& difference : differences) std::cout << "  " << difference << '\n';
                ++failures;
            }
        }
        clobbers = false;
        production_mode = false;
        // The comparison must notice wrong memory, registers, CR, stack, FPSCR and call arguments.
        const Case control{.entry = render_info, .label = "control", .history = {{0, path_buffer}}, .played = 100};
        for (const auto& [name, runner] : std::array<std::pair<const char*, RoutineRunner>, 5>{{
                 {"memory", corrupt_memory}, {"register", corrupt_register}, {"condition register", corrupt_cr}, {"stack", corrupt_stack}, {"FPSCR", corrupt_fpscr}}}) {
            active = control;
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s);
            initialize(c);
            if (compare_routine_call(*s, render_info, original, runner, RoutineChecker::result_r3).empty()) {
                ++failures;
                std::cout << "FAIL " << name << " corruption control\n";
            } else std::cout << "ok deliberate " << name << " corruption detected\n";
        }
        for (auto* flag : {&wrong_argument, &wrong_return}) {
            active = control;
            *flag = true;
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s);
            initialize(c);
            if (compare_routine_call(*s, render_info, original, rewrite, RoutineChecker::result_r3).empty()) {
                ++failures;
                std::cout << "FAIL call corruption control\n";
            } else std::cout << "ok deliberate call corruption detected\n";
            *flag = false;
        }
        // Production routing: every entry returns with no instruction budget, so
        // no generated body of this module runs; the original exhausts it.
        unsigned ownership = 0;
        for (const auto entry : entries) {
            const auto found = std::ranges::find(cases | std::views::reverse, entry, &Case::entry);
            active = *found;
            auto normal = cm::semantic_io_checks::clone(*base);
            Cpu nc(*normal);
            initialize(nc);
            normal->budget = normal->executed + slack(entry);
            Observed observer;
            normal->memory.observer = &observer;
            production_mode = ownership_mode = true;
            try {
                rewrite(*normal, entry);
                ++ownership;
            } catch (const std::exception& e) {
                ++failures;
                std::cout << "FAIL production ownership " << hex_address(entry) << ' ' << e.what() << '\n';
            }
            production_mode = ownership_mode = false;
            auto reference = cm::semantic_io_checks::clone(*base);
            Cpu rc(*reference);
            initialize(rc);
            reference->budget = reference->executed + slack(entry);
            bool rejected = false;
            try { original(*reference, entry); }
            catch (const std::exception& e) {
                rejected = std::string(e.what()) == "Native instruction budget exhausted" && reference->pc >= module_low && reference->pc < module_high;
            }
            if (!rejected) { ++failures; std::cout << "FAIL original budget control " << hex_address(entry) << '\n'; }
        }
        for (const auto& [entry, n] : counts) std::cout << hex_address(entry) << " returned " << n[0] << " controlled + " << n[1] << " production\n";
        std::cout << ownership << "/" << entries.size() << " production controls; " << completed << " returned comparisons; " << failures << " failures\n";
        if (counts.size() != entries.size()) { std::cout << "FAIL an entry has no returned comparison\n"; return 1; }
        if (argc == 3) failures += run_real_state(argv[1], argv[2]);
        else std::cout << "real state: not run, no snapshot\n";
        return failures ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
