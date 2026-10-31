// Returned comparisons against private hook-free originals. Pure runtime
// routines (strings, path parts, RNG) run for real; files, the clock, dialogs,
// Internet Config and formatted output (which reaches an import here) are
// scripted. Every call a check dispatches, real or scripted, puts its target,
// return address, arguments, non-volatile registers and CR into compared memory;
// leaf routines run before the probe, and neither their order nor their return
// address reaches anything the game can read.
#include "utils_adapter.hpp"
#include "guest_values.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <ranges>
#include <set>
#include <unistd.h>
#include <vector>

#if defined(CM_UTILS_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif
using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool utils_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, text_a = data + 0x1000, text_b = data + 0x2000, output = data + 0x3000,
    heap = data + 0x40000, trace_at = data + 0x60000, stack = 0x71010000, sentinel = 0xfffffff0, toc_base = 0x108bba90,
    input_stream = data + 0x100, output_stream = data + 0x200, command_line = 0x1091eddc;
constexpr std::uint32_t swap_memory = 0x10745e00, trim = 0x10745fc0, environment_directory = 0x107460c0, data_path = 0x10746280,
    folder_path = 0x10746300, usable_path = 0x10746340, path_with_extension = 0x10746470, read_marker = 0x107464d0,
    write_marker = 0x107465a0, compare_text = 0x107465f0, to_upper = 0x10746760, to_lower = 0x10746780, ordinal = 0x107467a0,
    milliseconds = 0x10746a30, number_text = 0x10746a70, break_line = 0x10746e80, read_line = 0x10746f80, flush_input = 0x10747080,
    copy_file = 0x107470b0, open_url = 0x107471d0, delete_temporary = 0x10747260, short_name = 0x107473c0, initialize_random = 0x107474a0;
constexpr std::array entries{swap_memory, trim, environment_directory, data_path, folder_path, usable_path, path_with_extension,
    read_marker, write_marker, compare_text, to_upper, to_lower, ordinal, milliseconds, number_text, break_line, read_line,
    flush_input, copy_file, open_url, delete_temporary, short_name, initialize_random};
constexpr std::uint32_t error_dialog = 0x10769b60, get_env = 0x1077f6d0, folder_location = 0x107390e0, find_first = 0x10739690,
    find_next = 0x107397d0, remove_file = 0x10776690, open_file = 0x10739de0, close_file = 0x107766f0, file_position = 0x10777cc0,
    seek_file = 0x10777f10, read_file = 0x10775fe0, write_file = 0x10776360, read_character = 0x10775c10, allocate = 0x10775130,
    release = 0x10775170, localized_text = 0x10727890, format_text = 0x1077b720, time_of_day = 0x1077f900, draw_random = 0x1077b770, microseconds = 0x10780600,
    flush_events = 0x10781308, internet_start = 0x10781290, internet_launch = 0x107812a8, internet_stop = 0x107812c0;
const std::set<std::uint32_t> scripted{error_dialog, get_env, folder_location, find_first, find_next, remove_file, open_file,
    close_file, file_position, seek_file, read_file, write_file, read_character, allocate, release, localized_text, format_text, time_of_day,
    microseconds, flush_events, internet_start, internet_launch, internet_stop};
const std::set<std::uint32_t> glue{microseconds, flush_events, internet_start, internet_launch, internet_stop};

struct Case {
    std::uint32_t entry = trim;
    std::string label{};
    std::array<std::uint32_t, 4> arguments{};
    std::string a{}, b{}, line{};           // text at text_a and text_b; the command line
    int language = 0;
    std::uint32_t environment = 0;          // what getenv returns
    std::vector<std::uint8_t> stream{};     // what fgetc returns
    std::uint32_t read_items = 1, marker = 0, file_size = 0x20, read_limit = 0xffffffff;
    unsigned open_failures = 0;             // bit 0: source, bit 1: destination
    unsigned allocation_failures = 0;
    bool allocates_local = false;
    unsigned files = 0;
    std::uint32_t find_found = 0, find_end = 1;
    std::uint32_t internet_status = 0, clock_high = 0, clock_low = 0, time_value = 0x12345678;
    std::vector<std::uint32_t> draws{};     // scripted rand results; empty: the real one
    std::uint32_t cursor_after_draws = 0;   // written to the cursor after the second draw
    std::function<void(Cpu&)> prepare{};
};
Case active;
bool original_mode = false, production_mode = false, ownership_mode = false, wrong_argument = false, wrong_return = false, clobbers = false;
unsigned calls = 0, next_stream = 0, allocation_failures_left = 0, files_left = 0, listed = 0, next_draw = 0;
std::uint32_t file_left = 0;

// Instructions a production run may spend outside the probe between calls, each
// below the nine the original's first run charges: the RNG core's hook counts
// one, and leaf routines run before the probe.
std::uint64_t slack(std::uint32_t entry) {
    return entry == initialize_random || entry == delete_temporary ? 1 : entry == usable_path ? 4 : 0;
}
void trace(Cpu& c, std::uint32_t target) {
    if (++calls > 12000) throw std::runtime_error("fixture call limit");
    auto index = c.u32(trace_at);
    const auto add = [&](std::uint32_t word) { c.put32(trace_at + 4 + index++ * 4, word); };
    add(target); add(c.lr() ^ (wrong_return && !original_mode ? 4 : 0));
    for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned f = 0; f < 8; ++f) add(c.cr(f));
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
void write_name(Cpu& c, std::uint32_t record) {
    write_text(c, record, "file" + std::to_string(listed++));
}
std::uint32_t script(Cpu& c, std::uint32_t target) {
    const auto arg3 = c.r(3), arg4 = c.r(4), arg5 = c.r(5);
    switch (target) {
    case get_env: return active.environment;
    case folder_location:
        copy_text(c, arg3, arg4);
        write_text(c, arg3 + std::uint32_t(c.u8(arg4) ? 1 : 0), "F");
        return 0;
    case find_first: case find_next: {
        const auto record = target == find_first ? arg4 : arg3;
        if (!files_left) return active.find_end;
        --files_left;
        write_name(c, record);
        return active.find_found;
    }
    case open_file: {
        const bool source = arg4 == c.toc(-0x3e34) + 0x254;
        if (active.open_failures & (source ? 1u : 2u)) return 0;
        return source ? input_stream : output_stream;
    }
    case file_position: return active.file_size;
    case read_file:
        if (arg4 == 4 && arg5 == 1) {
            if (active.read_items == 1) c.put32(arg3, active.marker);
            return active.read_items;
        } else {
            const auto got = std::min({arg5, file_left, active.read_limit});
            file_left -= got;
            if (got) c.put8(arg3, 0x40 + got % 26);
            return got;
        }
    case write_file: return arg5;
    case read_character:
        return next_stream < active.stream.size() ? active.stream[next_stream++] : 0xffffffffu;
    case allocate:
        if (allocation_failures_left) { --allocation_failures_left; return 0; }
        return active.allocates_local ? c.sp() + 0x38 : heap;
    case localized_text:
        write_text(c, arg3, "localized");
        return 1;
    case format_text:
        copy_text(c, arg3, arg4);
        return 1;
    case time_of_day: return active.time_value;
    case microseconds:
        c.put32(arg3, active.clock_high);
        c.put32(arg3 + 4, active.clock_low);
        return 0;
    case internet_start:
        c.put32(arg3, data + 0x7000);
        return active.internet_status;
    default: return 0;
    }
}
bool probe(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    if (target >= 0x10745e00 && target < 0x107475b0) {
        if (original_mode) return utils_reference(s);
        if (production_mode) return false;
        utils_run_rewrite(s, target);
        return true;
    }
    if (target == draw_random && !active.draws.empty()) {
        trace(c, target);
        const auto index = next_draw++;
        if (index == 1 && active.cursor_after_draws) c.put32(c.toc(-0x7b84), active.cursor_after_draws);
        c.set(3, active.draws[index % active.draws.size()]);
        s.pc = c.lr();
        return true;
    }
    if (!scripted.contains(target)) {
        if (ownership_mode) {
            // Dependencies may run; the module must return with no instruction
            // budget of its own, before and after this call.
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
    if (glue.contains(target)) c.put32(c.sp() + 0x14, c.r(2));
    const auto answer = script(c, target);
    if (clobbers) {
        const auto mark = 0xc0000000u ^ (target << 4) ^ calls;
        for (unsigned r : {0u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u}) c.set(r, mark + r);
        c.set_cr(0, std::uint8_t(calls & 0xe));
        c.set_cr(1, std::uint8_t((calls >> 1) & 0xe));
        if (glue.contains(target)) c.set(2, mark);
    }
    c.set(3, answer);
    s.pc = c.lr();
    return true;
}
void initialize(Cpu& c) {
    c.s.memory.map(data, 0x200000);
    c.s.memory.map(0x71000000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x60000, 0xa5));
    c.s.memory.copy_in(0x71000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, std::uint8_t(f + 1));
    c.s.wr<1>(reg::xer_so, clobbers);  // every compare copies it into CR
    c.set(1, stack);
    c.set(2, toc_base);
    c.set_lr(sentinel);
    c.put8(toc_base - 0x35ac, std::uint32_t(active.language));
    write_text(c, command_line, active.line);
    write_text(c, text_a, active.a);
    write_text(c, text_b, active.b);
    c.put32(trace_at, 0);
    for (unsigned n = 0; n < 4; ++n) c.set(3 + n, active.arguments[n]);
    if (active.prepare) active.prepare(c);
    c.s.budget = c.s.executed + 5000000;
    calls = next_stream = next_draw = listed = 0;
    allocation_failures_left = active.allocation_failures;
    files_left = active.files;
    file_left = active.file_size;
}
void reset_script() {
    calls = next_stream = next_draw = listed = 0;
    allocation_failures_left = active.allocation_failures;
    files_left = active.files;
    file_left = active.file_size;
}
void flush_coverage() {
#if defined(CM_UTILS_COVERAGE)
    // LLVM expands %p before fork: name each child's profile explicitly.
    const auto profile = std::string(CM_UTILS_PROFILE_DIR) + "/check-" + std::to_string(getpid()) + ".profraw";
    __llvm_profile_set_filename(profile.c_str());
    if (__llvm_profile_write_file() != 0) throw std::runtime_error("Cannot flush UTILS child coverage");
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
        utils_run_rewrite(s, entry);
    }
    flush_coverage();
}
void corrupt_memory(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.put8(text_a, c.u8(text_a) ^ 1); }
void corrupt_register(State& s, std::uint32_t entry) { rewrite(s, entry); s.wr<4>(reg::gpr(5), s.r<4>(reg::gpr(5)) ^ 1); }
void corrupt_cr(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.set_cr(0, c.cr(0) ^ 2); }
void corrupt_stack(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.put8(stack - 0x60, c.u8(stack - 0x60) ^ 1); }
struct Observed final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
};
std::vector<std::uint8_t> bytes(std::string_view text) { return {text.begin(), text.end()}; }

std::vector<Case> all_cases() {
    std::vector<Case> cases;
    const auto add = [&](Case test) { cases.push_back(std::move(test)); };
    // swap_memory
    add({.entry = swap_memory, .label = "null first", .arguments = {0, text_b, 4}});
    add({.entry = swap_memory, .label = "null second", .arguments = {text_a, 0, 4}});
    const std::string long_a(70, 'a'), long_b(70, 'b');
    for (std::uint32_t size : {0u, 1u, 2u, 7u, 8u, 9u, 15u, 16u, 17u, 24u, 63u, 64u}) {
        add({.entry = swap_memory, .label = "swap " + std::to_string(size), .arguments = {text_a, text_b, size},
             .a = "abcdefghijklmnopqrstuvwxyz0123456789" + long_a, .b = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" + long_b});
        for (std::uint32_t distance : {0u, 1u, 3u, 8u})
            add({.entry = swap_memory, .label = "overlap", .arguments = {text_a, text_a + distance, size},
                 .a = "abcdefghijklmnopqrstuvwxyz0123456789" + long_a});
        add({.entry = swap_memory, .label = "overlap down", .arguments = {text_a + 5, text_a, size},
             .a = "abcdefghijklmnopqrstuvwxyz0123456789" + long_a});
    }
    // trim
    add({.entry = trim, .label = "null", .arguments = {0}});
    for (const char* text : {"", "abc", " abc ", "\x01\x02" "abc", "abc\r\n", "\t\nabc \x1f", "\x01\x02", "\x1f", " ", "\x7f" "a\x80",
                             "\xff" "b\xff", "\x01 x \x01", "a\x01" "b"})
        add({.entry = trim, .label = std::string("trim ") + text, .arguments = {text_a}, .a = text});
    // environment_directory
    const std::array variables{"SI_DATA", "CM3_DATA", "CM3_TEMP", "CM3_DEBUG", "CM3_PREFS", "CM3_TACTICS", "CM3_TRAINING",
                               "CM3_HISTORY", "CM3_PICTURES", "CM3_SOUNDS", "CM3_HELP", "CM3_QSTART", "cm3_help", "OTHER", ""};
    for (const auto* variable : variables) {
        add({.entry = environment_directory, .label = variable, .arguments = {text_a}, .a = variable});
        add({.entry = environment_directory, .label = "set", .arguments = {text_a}, .a = variable, .b = "env", .environment = text_b});
        add({.entry = environment_directory, .label = "ignored", .arguments = {text_a}, .a = variable, .b = "env",
             .line = "game -ignoreenv x", .environment = text_b});
    }
    // data_path and short_name
    const std::array names{"", "a", "name.dat", "file with spaces.dat", "123456789012345678901234567890",
                           "1234567890123456789012345678901", "12345678901234567890 12345678901", "   123456789012345678901234567890  ",
                           "123456789012345678901234567890 ", "1234567890123456789012345678901234567890.dat",
                           "                                   ", "a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9"};
    for (const auto* name : names) {
        add({.entry = short_name, .label = name, .arguments = {text_a, output}, .a = name});
        add({.entry = data_path, .label = name, .arguments = {text_a, text_b}, .a = "CM3_DATA", .b = name});
        add({.entry = data_path, .label = "unknown", .arguments = {text_a, text_b}, .a = "OTHER", .b = name});
        add({.entry = data_path, .label = "ignored", .arguments = {text_a, text_b}, .a = "CM3_TEMP", .b = name, .line = "-ignoreenv"});
    }
    // folder_path, usable_path, path_with_extension
    add({.entry = folder_path, .label = "folder", .arguments = {text_a, output}, .a = "prefs"});
    add({.entry = folder_path, .label = "empty", .arguments = {text_a, output}, .a = ""});
    add({.entry = usable_path, .label = "null", .arguments = {0}});
    add({.entry = usable_path, .label = "empty", .arguments = {text_a}, .a = ""});
    add({.entry = usable_path, .label = "path", .arguments = {text_a}, .a = "folder:file"});
    for (const char* path : {"name", "name.dat", "folder:name.dat", "a:b:c.d.e", "", ":x", "folder\\file.txt", "folder/file.txt"})
        for (const char* extension : {".out", "", "x"})
            add({.entry = path_with_extension, .label = path, .arguments = {text_a, output, text_b}, .a = path, .b = extension});
    // File markers
    for (std::uint32_t items : {0u, 1u, 2u, 0xffffffffu})
        for (std::uint32_t marker : {0xf1fffff1u, 0xf2fffff2u, 0xf3fffff3u, 0xf1fffff0u, 0xf1fffff2u, 0xf2fffff1u, 0xf2fffff3u,
                                     0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu})
            add({.entry = read_marker, .label = "marker", .arguments = {input_stream}, .read_items = items, .marker = marker, .file_size = 0x1234});
    for (std::uint32_t marker : {0xf1fffff1u, 0u, 0xffffffffu}) {
        add({.entry = write_marker, .label = "write", .arguments = {output_stream, marker}});
    }
    // compare_text
    const std::array<std::pair<const char*, const char*>, 16> pairs{{
        {"", ""}, {"a", ""}, {"", "a"}, {"abc", "abc"}, {"abc", "abd"}, {"abd", "abc"}, {"abc", "ABC"}, {"ABC", "abc"},
        {"abc", "abcd"}, {"abcd", "abc"}, {"\xe9t\xe9", "ete"}, {"ete", "\xc9TE"}, {"\xff", "y"}, {"\x01", "\x02"},
        {"a\x80", "a\x81"}, {"abcdef", "abcxyz"}}};
    for (const auto& [first, second] : pairs)
        for (int language : {0, 2, 10})
            for (std::uint32_t exact : {0u, 1u, 0x80u, 0x100u})
                for (std::uint32_t limit : {0u, 1u, 2u, 3u, 4u, 0xffffffffu, 0x80000000u})
                    add({.entry = compare_text, .label = std::string("compare ") + first + "/" + second,
                         .arguments = {text_a, text_b, exact, limit}, .a = first, .b = second, .language = language});
    for (int language : {0, 10}) {
        add({.entry = compare_text, .label = "both null", .arguments = {0, 0, 0, 0}, .language = language});
        add({.entry = compare_text, .label = "first null", .arguments = {0, text_b, 0, 0}, .b = "x", .language = language});
        add({.entry = compare_text, .label = "second null", .arguments = {text_a, 0, 0, 0}, .a = "x", .language = language});
    }
    // to_upper, to_lower
    for (std::uint32_t code = 0; code < 256; ++code) {
        add({.entry = to_upper, .label = "upper", .arguments = {code}});
        add({.entry = to_lower, .label = "lower", .arguments = {code}});
    }
    for (std::uint32_t code : {0x141u, 0xffffff41u, 0x11fu, 0xffffff1fu, 0x80000020u}) {
        add({.entry = to_upper, .label = "upper high", .arguments = {code}});
        add({.entry = to_lower, .label = "lower high", .arguments = {code}});
    }
    // ordinal_text
    for (std::uint32_t number : {0u, 1u, 2u, 3u, 4u, 9u, 10u, 11u, 12u, 13u, 14u, 20u, 21u, 22u, 23u, 24u, 100u, 101u, 102u, 103u,
                                 109u, 110u, 111u, 112u, 113u, 120u, 121u, 1011u, 0x7fffffffu, 0xffffffffu})
        for (std::uint32_t gender : {0u, 1u, 2u, 3u, 0x101u, 0xffu})
            for (int language : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0x82})
                add({.entry = ordinal, .label = "ordinal " + std::to_string(number),
                     .arguments = {number, gender}, .language = language});
    // milliseconds, flush_input
    for (const auto& [high, low] : std::array<std::pair<std::uint32_t, std::uint32_t>, 7>{{{0, 0}, {0, 999}, {0, 1000}, {0, 0xffffffff},
            {1, 0}, {0xffffffff, 0xffffffff}, {0x12345, 0x6789abcd}}})
        add({.entry = milliseconds, .label = "clock", .clock_high = high, .clock_low = low});
    add({.entry = flush_input, .label = "flush"});
    // number_text
    add({.entry = number_text, .label = "null", .arguments = {3, 0, 0x32, 1}});
    add({.entry = number_text, .label = "null and small", .arguments = {3, 0, 0, 0}});
    for (std::uint32_t number = 0; number <= 22; ++number)
        for (std::uint32_t lowercase : {0u, 1u, 0x80u, 0x100u})
            add({.entry = number_text, .label = "number " + std::to_string(number), .arguments = {number, output, 0x32, lowercase}});
    for (std::uint32_t capacity : {0u, 0x31u, 0x32u, 0x33u, 0x7fffu, 0x8000u, 0xffffu, 0x10032u, 0x10031u})
        for (std::uint32_t number : {5u, 21u, 0xffffffffu})
            add({.entry = number_text, .label = "capacity " + std::to_string(capacity), .arguments = {number, output, capacity, 1}});
    add({.entry = number_text, .label = "large", .arguments = {21, output, 0x32, 0}});
    // break_line
    const std::array lines{"", "a", " ", "ab", "a b", "ab cd", "abc def", "abcd ef", "ab cdef", " abcdef", "abcdef ", "abcdefgh",
                           "a b c d e f", "   ", "    ", "long line with several words in it"};
    for (const auto* line : lines)
        for (std::uint32_t before : {0u, 1u, 0x100u})
            for (std::uint32_t after : {0u, 1u, 0x80u})
                add({.entry = break_line, .label = line, .arguments = {text_a, before, after}, .a = line});
    add({.entry = break_line, .label = "null", .arguments = {0, 1, 1}});
    // read_line
    for (const std::string_view stream : {std::string_view(""), std::string_view("\n"), std::string_view("abc"), std::string_view("abc\n"),
                                          std::string_view("  abc\n"), std::string_view("#comment\nabc\n"), std::string_view("#comment"),
                                          std::string_view("\n\nabc"), std::string_view("\tabc\r\n"), std::string_view("a b\tc\rd\n"),
                                          std::string_view("a#b\nc\n"), std::string_view("a#b"), std::string_view("\x01\x08\x0b\x0c\x0e\x22\x24\x7f\n"),
                                          std::string_view("\xff" "abc"), std::string_view("ab\xff" "c"), std::string_view("\x80\xfe\n"),
                                          std::string_view("#\xff" "abc\n"), std::string_view("##\n#\n \n\t\n\r\nend"), std::string_view("   ")})
        add({.entry = read_line, .label = "line", .arguments = {input_stream, output}, .stream = bytes(stream)});
    // copy_file
    add({.entry = copy_file, .label = "no source", .arguments = {text_a, text_b}, .open_failures = 1});
    add({.entry = copy_file, .label = "no destination", .arguments = {text_a, text_b}, .open_failures = 2});
    add({.entry = copy_file, .label = "neither", .arguments = {text_a, text_b}, .open_failures = 3});
    for (std::uint32_t size : {0u, 1u, 0x20u, 0xffu, 0x100u, 0x101u, 0x1000u, 0x12345u, 0xffffffffu})
        for (unsigned failures : {0u, 1u, 2u, 3u, 20u}) {
            // Twenty failed allocations leave 4,095-byte chunks: a million reads.
            if (size == 0xffffffffu && failures == 20) continue;
            add({.entry = copy_file, .label = "copy", .arguments = {text_a, text_b}, .file_size = size, .allocation_failures = failures});
            if (size <= 0x1000)
                add({.entry = copy_file, .label = "copy limited", .arguments = {text_a, text_b}, .file_size = size,
                     .read_limit = 0x40, .allocation_failures = failures});
        }
    add({.entry = copy_file, .label = "stack buffer", .arguments = {text_a, text_b}, .file_size = 0x10, .allocates_local = true});
    // open_url
    for (std::uint32_t status : {0u, 1u, 0xffff0000u, 0x10000u, 0xffffffffu, 0x8000u})
        for (const char* url : {"", "http://x", "http://www.championshipmanager.co.uk/"})
            add({.entry = open_url, .label = url, .arguments = {text_a}, .a = url, .internet_status = status,
                 .prepare = [](Cpu& c) { c.put32(c.toc(-0x5bbc) + 0x14, 0x434d3032); }});
    // delete_temporary_files
    for (unsigned files : {0u, 1u, 2u, 3u, 99u, 100u, 101u, 199u, 200u, 205u})
        for (const char* pattern : {"*.tmp", ""})
            add({.entry = delete_temporary, .label = "delete " + std::to_string(files), .arguments = {text_a}, .a = pattern, .files = files});
    for (std::uint32_t found : {0x100u, 0xffffff00u})
        add({.entry = delete_temporary, .label = "low byte found", .arguments = {text_a}, .a = "*", .files = 2, .find_found = found});
    for (std::uint32_t end : {1u, 0xffu, 0x101u, 0x80u})
        add({.entry = delete_temporary, .label = "end value", .arguments = {text_a}, .a = "*", .files = 2, .find_end = end});
    add({.entry = delete_temporary, .label = "ignored env", .arguments = {text_a}, .a = "*", .line = "-ignoreenv", .files = 1});
    add({.entry = delete_temporary, .label = "env folder", .arguments = {text_a}, .a = "*", .b = "tmpdir", .environment = text_b, .files = 1});
    // initialize_random
    for (std::uint32_t seed : {0u, 1u, 2u, 65535u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
        add({.entry = initialize_random, .label = "seed " + std::to_string(seed), .arguments = {seed}});
        add({.entry = initialize_random, .label = "seed, time 0", .arguments = {seed}, .time_value = 0});
    }
    for (const auto& draws : std::array<std::vector<std::uint32_t>, 10>{{{0, 0}, {50999, 65534}, {51000, 65535}, {51001, 65536}, {32767, 32767},
            {0x7fffffff, 0x7fffffff}, {0x80000000, 0x80000000}, {0xffffffff, 0xffffffff}, {0xfffe1234, 0xfffe1234},
            // Negative numerators with no bits shifted out: the shifts leave the carry clear.
            {0xffff38ce, 0xffff0002}}})
        add({.entry = initialize_random, .label = "draws", .arguments = {7}, .draws = draws});
    add({.entry = initialize_random, .label = "cursor past end", .arguments = {7}, .draws = {5, 5}, .cursor_after_draws = 0xfffffff0});
    add({.entry = initialize_random, .label = "cursor before pool", .arguments = {7}, .draws = {5, 5}, .cursor_after_draws = 4});
    return cases;
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        auto base = std::make_unique<State>();
        load_program(*base, argv[1]);
        dispatch_probe = probe;
        const auto cases = all_cases();
        unsigned completed = 0, failures = 0;
        std::map<std::uint32_t, std::array<unsigned, 2>> counts;
        for (bool production : {false, true}) {
            production_mode = production;
            for (const auto& test : cases) for (bool clobber : {false, true}) {
                if (failures >= 8) break;
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
                          << " clobber=" << clobber << " args=" << hex_address(test.arguments[0]) << ',' << hex_address(test.arguments[1])
                          << ',' << hex_address(test.arguments[2]) << ',' << hex_address(test.arguments[3]) << " language=" << test.language << '\n';
                for (const auto& difference : differences) std::cout << "  " << difference << '\n';
                ++failures;
            }
        }
        clobbers = false;
        production_mode = false;
        // The comparison must notice wrong memory, registers, CR, stack and call arguments.
        Case control{.entry = trim, .label = "control", .arguments = {text_a}, .a = "\x01" "abc\n"};
        for (const auto& [name, runner] : std::array<std::pair<const char*, RoutineRunner>, 4>{{
                 {"memory", corrupt_memory}, {"register", corrupt_register}, {"condition register", corrupt_cr}, {"stack", corrupt_stack}}}) {
            active = control;
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s);
            initialize(c);
            if (compare_routine_call(*s, trim, original, runner, RoutineChecker::result_r3).empty()) {
                ++failures;
                std::cout << "FAIL " << name << " corruption control\n";
            } else std::cout << "ok deliberate " << name << " corruption detected\n";
        }
        {
            active = {.entry = usable_path, .label = "control", .arguments = {0}};
            wrong_argument = true;
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s);
            initialize(c);
            if (compare_routine_call(*s, usable_path, original, rewrite, RoutineChecker::result_r3).empty()) {
                ++failures;
                std::cout << "FAIL argument control\n";
            } else std::cout << "ok deliberate call-argument corruption detected\n";
            wrong_argument = false;
        }
        {
            active = {.entry = trim, .label = "control", .arguments = {text_a}, .a = "abc"};
            wrong_return = true;
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s);
            initialize(c);
            if (compare_routine_call(*s, trim, original, rewrite, RoutineChecker::result_r3).empty()) {
                ++failures;
                std::cout << "FAIL return address control\n";
            } else std::cout << "ok deliberate return-address corruption detected\n";
            wrong_return = false;
        }
        // Production routing: every entry returns with no instruction budget, so
        // no generated body of this module runs; the original exhausts it.
        unsigned ownership = 0;
        for (const auto entry : entries) {
            // The last case of each entry: none takes an assertion path, whose
            // version-text leaf would need budget.
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
                rejected = std::string(e.what()) == "Native instruction budget exhausted" && reference->pc >= 0x10745e00 && reference->pc < 0x107475b0;
            }
            if (!rejected) { ++failures; std::cout << "FAIL original budget control " << hex_address(entry) << '\n'; }
        }
        for (const auto& [entry, n] : counts) std::cout << hex_address(entry) << " returned " << n[0] << " controlled + " << n[1] << " production\n";
        std::cout << ownership << "/23 production controls; " << completed << " returned comparisons; " << failures << " failures\n";
        if (counts.size() != entries.size()) { std::cout << "FAIL an entry has no returned comparison\n"; return 1; }
        return failures ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
