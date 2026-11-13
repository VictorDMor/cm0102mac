// GAME_CONFIG.CPP: each routine's C++ against an independent copy of its generated original, from
// the same fixture state. Every routine outside the unit is scripted here and traced (target, return
// address, the arguments it reads, the non-volatile registers), except the four C string routines,
// which run. No display, sound device, preferences file or career is opened.
#include "game_config_adapter.hpp"
#include "game_config_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <unistd.h>

#if defined(CM_GAME_CONFIG_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif
using namespace cm::aot;
using namespace cm::aot::guest;
namespace field = cm::aot::game_config::field;
namespace callee = cm::aot::game_config::callee;
namespace cm::aot { bool game_config_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, object = data + 0x1000, other = data + 0x1800, globals = data + 0x3000,
                        path_text = data + 0x9000, player = data + 0xa000, file = data + 0xb000,
                        trace_at = data + 0x30000, stack = 0x71010000, sentinel = 0xfffffff0;
// Globals the fixture points the table of contents at.
constexpr std::uint32_t message = globals, success = globals + 0x400, sound_cell = globals + 0x404,
                        command_line = globals + 0x500, screens = globals + 0x600, database_cell = globals + 0x700,
                        font_cell = globals + 0x704, printer = globals + 0x708, display_mode = globals + 0x70c;
constexpr std::int32_t display_mode_toc = -0x5ae8;  // read by the leaf 0x10769b50
constexpr std::array entries{0x1013f030u, 0x1013f410u, 0x1013f4d0u, 0x1013f570u, 0x1013f680u, 0x1013fb60u,
                             0x1013fc50u, 0x1013fd00u, 0x1013fe00u, 0x1013feb0u, 0x101406e0u, 0x101406f0u};
constexpr std::uint32_t module_end = 0x10140724;
enum : unsigned { construct, copy, destroy, defaults, select_one, language, currency, language_currency, interval, apply, font, set_font };

// Which of r3-r10 each routine the unit calls reads (tools/call_arguments.py 0x1013f030 0x10140724;
// bit 0 is r3). The four that run are not traced.
struct Mask { std::uint32_t target; unsigned mask; };
constexpr Mask masks[] = {
    {0x1000a440, 0x01}, {0x1000b150, 0x00}, {0x100b6050, 0x03}, {0x100b8b70, 0x00}, {0x10711b40, 0x00},
    {0x10712c50, 0xf0}, {0x10712cc0, 0x01}, {0x10712ce0, 0x00}, {0x10726be0, 0x03}, {0x10726cc0, 0xfc},
    {0x10726da0, 0x01}, {0x10726e90, 0xff}, {0x10727880, 0x00}, {0x10739890, 0x1f}, {0x10739de0, 0x03},
    {0x1073af50, 0x00}, {0x1073b060, 0x00}, {0x10742ec0, 0x07}, {0x10744670, 0x01}, {0x107446b0, 0x03},
    {0x10744700, 0x01}, {0x10744860, 0x0f}, {0x10746280, 0x03}, {0x10746300, 0xf3}, {0x1074cc20, 0x01},
    {0x10750b60, 0xff}, {0x10768820, 0x3f}, {0x10768990, 0x00}, {0x10769b60, 0xff}, {0x1076f7d0, 0x01},
    {0x10773520, 0x01}, {0x107735a0, 0x01}, {0x10775fe0, 0x0f}, {0x10776360, 0x0f}, {0x10776690, 0x01},
    {0x107766f0, 0xf1}, {0x1077b720, 0xff}};
constexpr std::uint32_t running[] = {callee::copy_text, callee::copy_bounded, callee::compare_text, callee::find_text};

struct Case {
    unsigned routine = apply;
    std::uint32_t argument = 0;  // r4
    // The record, and for a working copy the record it links to.
    std::uint32_t options = 0x212552a9, font = 0xfffffffe;
    std::uint8_t language = 0, currency = 0, stored = 0;
    std::uint16_t interval = 30;
    bool linked = false, null_record = false;
    std::string folder = "Macintosh HD:games", font_name = "Geneva";
    // The world outside.
    bool player_exists = false, database_loaded = true, display_full_screen = false, backgrounds = true;
    std::uint32_t live_language = 0, database_language = 0, live_currency = 0, display_font = 0xfffffffe;
    std::uint32_t processor = 0, memory = 128, printing = 1, languages_load = 1, language_available = 1;
    bool allocation_fails = false;
    std::uint8_t player_started = 1;
    unsigned display_opens = 3;  // bit n: the nth open succeeds
    std::string command = "";
    // The file.
    bool opens = true;
    std::uint32_t magic = game_config::file_magic, file_options = 0x21255329, file_font = 7;
    std::uint8_t file_language = 2, file_currency = 9;
    std::uint16_t file_interval = 90;
    std::string file_font_name = "Chicago", system_font_name = "Chicago";
    unsigned short_read = 99, short_write = 99;  // the nth read or write moves nothing
    std::uint32_t short_answer = 0;
    bool clobber = false;
};
Case active;
bool original_mode = false, production_mode = false, ownership_mode = false;
unsigned calls = 0, reads = 0, writes = 0, opened = 0;
std::size_t position = 0;

std::vector<std::uint8_t> file_image() {
    std::vector<std::uint8_t> image;
    const auto word = [&](std::uint32_t value, unsigned size) {
        for (unsigned n = size; n-- > 0;) image.push_back(std::uint8_t(value >> (8 * n)));
    };
    word(active.magic, 4); word(active.file_options, 4); word(active.file_language, 1); word(active.file_currency, 1);
    word(active.file_interval, 2);
    for (unsigned n = 0; n < 0x100; ++n) image.push_back(std::uint8_t(n * 7 + 3));
    word(active.file_font, 4);
    for (unsigned n = 0; n < 0x1f; ++n) image.push_back(n < active.file_font_name.size() ? std::uint8_t(active.file_font_name[n]) : 0);
    return image;
}
void put_text(Cpu& c, std::uint32_t at, const std::string& text) {
    for (std::size_t n = 0; n < text.size(); ++n) c.put8(at + std::uint32_t(n), std::uint8_t(text[n]));
    c.put8(at + std::uint32_t(text.size()), 0);
}
void trace(Cpu& c, std::uint32_t target, unsigned mask) {
    if (++calls > 400) throw std::runtime_error("fixture call bound");
    auto size = c.u32(trace_at);
    const auto add = [&](std::uint32_t value) { c.put32(trace_at + 4 + size++ * 4, value); };
    add(target); add(c.lr());
    for (unsigned n = 0; n < 8; ++n) if (mask >> n & 1) add(c.r(3 + n));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    c.put32(trace_at, size);
}
bool probe(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    if (target >= entries.front() && target < module_end) {
        if (original_mode) return game_config_reference(s);
        if (production_mode) return false;
        if (game_config_lists(target)) { game_config_run_rewrite(s, target); return true; }
        return false;
    }
    // Cpu::call lowers leaves before it consults the probe; the original must run them the same way.
    if (run_leaf(s)) return true;
    if (std::ranges::find(running, target) != std::end(running)) {
        if (!ownership_mode) return false;
        // A dependency's instructions may run; the unit's own must not (its budget stays exhausted).
        const auto saved = dispatch_probe;
        dispatch_probe = nullptr;
        s.budget = s.executed + 1000000;
        try { c.call(target, c.lr()); } catch (...) { dispatch_probe = saved; throw; }
        dispatch_probe = saved;
        s.budget = s.executed;
        return true;
    }
    const auto known = std::ranges::find(masks, target, &Mask::target);
    if (known == std::end(masks)) return false;
    trace(c, target, known->mask);
    const auto a = c.r(3), b = c.r(4), d = c.r(5), e = c.r(6), g = c.r(7);
    std::uint32_t answer = 0x5a5a0000 + calls;  // routines with no result leave something in r3
    switch (target) {
    case callee::make_path: c.put32(path_text, a ^ (b << 1)); answer = path_text; break;
    case callee::folder_path: put_text(c, b, active.folder); break;
    case callee::load_languages: answer = active.languages_load; break;
    case callee::language_available: answer = active.language_available; break;
    case callee::current_language: answer = active.live_language; break;
    case callee::database_language: answer = active.database_language; break;
    case callee::current_currency: answer = active.live_currency; break;
    case callee::sound_construct: c.put8(a + game_config::sound_player_started, active.player_started); break;
    case callee::font_name: put_text(c, b, active.system_font_name.substr(0, d ? d - 1 : 0)); break;
    case callee::open_display: answer = (active.display_opens >> opened++ & 1) ? 0xffffff01 : 0xabcdef00; break;
    case callee::backgrounds_enabled: answer = active.backgrounds ? 0x7f01 : 0x1100; break;
    case callee::processor_class: answer = active.processor; break;
    case callee::memory_megabytes: answer = active.memory; break;
    case callee::printing_available: answer = active.printing; break;
    case callee::allocate_object: answer = active.allocation_fails ? 0 : player; break;
    case callee::file_open: answer = active.opens ? file : 0; break;
    case callee::file_read: {
        const auto image = file_image();
        if (++reads == active.short_read) { answer = active.short_answer; break; }
        for (std::uint32_t n = 0; n < b * d && position < image.size(); ++n) c.put8(a + n, image[position++]);
        answer = d;
        break;
    }
    case callee::file_write: {
        // What is written is part of the trace.
        auto size = c.u32(trace_at);
        for (std::uint32_t n = 0; n < b * d; ++n) c.put32(trace_at + 4 + size++ * 4, c.u8(a + n));
        c.put32(trace_at, size);
        answer = ++writes == active.short_write ? active.short_answer : d;
        break;
    }
    case 0x10739890: c.put32(e, 0x47414d00); c.put32(g, 0x43505000); break;
    case 0x1077b720: c.put32(a, c.r(8)); break;
    default: break;
    }
    if (active.clobber) {
        for (unsigned r = 4; r <= 10; ++r) c.set(r, 0x41000000 + calls * 0x100 + r);
        c.set(0, 0x40000000 + calls); c.set(11, 0x4b000000 + calls); c.set(12, 0x4c000000 + calls);
        c.compare(0, 17, 29);
    }
    c.set(3, answer);
    s.pc = c.lr();
    return true;
}
void fill_record(Cpu& c, std::uint32_t at, std::uint32_t link, unsigned salt) {
    for (unsigned n = 0; n < 0x150; ++n) c.put8(at + n, std::uint8_t(0x30 + (n * 5 + salt) % 71));  // no zero bytes
    c.put32(at + field::source, link);
    c.put32(at + field::options, active.options ^ (salt ? 0x00a50f00 : 0));
    c.put8(at + field::language, active.language + salt);
    c.put8(at + field::currency, active.currency + salt);
    c.put16(at + field::interval, active.interval + salt);
    put_text(c, at + field::save_folder, active.folder + (salt ? "!" : ""));
    put_text(c, at + field::unsaved_text, salt ? "other" : "text");
    c.put8(at + field::stored, active.stored);
    c.put32(at + field::font, active.font + salt);
    put_text(c, at + field::font_name, active.font_name + (salt ? "2" : ""));
}
void initialize(Cpu& c) {
    c.s.memory.map(data, 0x50000);
    c.s.memory.map(0x71000000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x50000, 0xa5));
    c.s.memory.copy_in(0x71000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, f + 1);
    c.set(1, stack); c.set(2, 0x108bba90); c.set_lr(sentinel);
    namespace toc = game_config::toc;
    for (const auto [offset, at] : {std::pair{toc::message, message}, {toc::success, success}, {toc::sound, sound_cell},
                                    {toc::command_line, command_line}, {toc::screens, screens},
                                    {toc::database, database_cell}, {toc::current_font, font_cell},
                                    {toc::printer, printer}, {display_mode_toc, display_mode}})
        c.put32(c.r(2) + std::uint32_t(offset), at);
    c.put8(success, 1);
    c.put32(sound_cell, active.player_exists ? player + 0x100 : 0);
    put_text(c, command_line, active.command);
    c.put32(database_cell, active.database_loaded ? data + 0x4000 : 0);
    c.put32(font_cell, active.display_font);
    c.put8(display_mode, active.display_full_screen ? 1 : 0);
    const bool takes_source = active.routine == copy;
    fill_record(c, object, active.linked && !takes_source ? other : 0, 0);
    fill_record(c, other, 0, 1);
    c.put32(trace_at, 0);
    c.set(3, active.null_record ? 0 : object);
    c.set(4, takes_source ? other : active.argument);
    c.s.budget = c.s.executed + 1000000;
}
void reset() { calls = reads = writes = opened = 0; position = 0; }
void original(State& s, std::uint32_t entry) {
    original_mode = true;
    reset();
    s.pc = entry;
    run_until(s, sentinel);
}
void rewrite(State& s, std::uint32_t entry) {
    original_mode = false;
    reset();
    if (production_mode) { s.pc = entry; run_until(s, sentinel); }
    else game_config_run_rewrite(s, entry);
#if defined(CM_GAME_CONFIG_COVERAGE)
    // LLVM expands %p before the fork: name the child's own profile.
    const auto profile = std::string(CM_GAME_CONFIG_PROFILE_DIR) + "/check-" + std::to_string(getpid()) + ".profraw";
    __llvm_profile_set_filename(profile.c_str());
    if (__llvm_profile_write_file() != 0) throw std::runtime_error("Cannot flush the child's coverage");
#endif
}
void corrupt(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    c.put8(object + field::stored, c.u8(object + field::stored) ^ 1);
}
struct Observed final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
};

std::vector<Case> cases() {
    std::vector<Case> all;
    std::mt19937 random(20261113);
    const auto pick = [&](std::initializer_list<std::uint32_t> values) {
        return *(values.begin() + random() % values.size());
    };
    const auto rnd = [&] { return std::uint32_t(random()); };
    const auto chance = [&](unsigned in) { return random() % in == 0; };
    // The world outside, drawn at random around the values the routines test.
    const auto world = [&](Case t) {
        t.options = pick({0x212552a9, 0x21255329, rnd(), rnd(), rnd() | 0x80, rnd() & ~0x180u, 0, 0xffffffff});
        t.language = std::uint8_t(pick({0, 1, 2, 5, 10, 11, 0x7f, 0x80, 0xff}));
        t.currency = std::uint8_t(pick({0, 9, 24, 25, 0x80, 0xff}));
        t.font = pick({0xfffffffe, 7, 0, 0x80000000, 0x7fffffff});
        t.display_font = pick({0xfffffffe, 7, 7, 0, 0x80000001});
        t.player_exists = chance(2);
        t.database_loaded = !chance(3);
        t.display_full_screen = chance(2);
        t.backgrounds = chance(2);
        t.live_language = pick({0, 1, 2, 5, 0x80, 0xff, 0xabcd0002, 0x100});
        t.database_language = pick({0, 2, 5, 0xff, 0x12340005});
        t.live_currency = pick({0, 9, 24, 0xff, 0x5500});
        t.processor = pick({0, 1, 2, 0x102, 0xff, 0x7700});
        t.memory = pick({0, 62, 63, 64, 128, 0x80000000, 0xffffffff});
        t.printing = pick({0, 1, 0x100, 0x80});
        t.languages_load = pick({1, 1, 1, 0, 0x100});
        t.language_available = pick({1, 1, 0, 0x100, 0xff});
        t.allocation_fails = chance(5);
        t.player_started = std::uint8_t(pick({1, 1, 0, 0x80}));
        t.display_opens = pick({3, 3, 3, 2, 1, 0});
        t.command = std::string(chance(3) ? "CM -windowed" : "") + (chance(3) ? " -nosound" : "") + (chance(3) ? " -defaultfont" : "") +
                    (chance(6) ? " -window" : "");
        t.opens = !chance(5);
        t.magic = pick({game_config::file_magic, game_config::file_magic, game_config::file_magic, 0x1553117d, 0x1552117c, 0});
        t.file_options = pick({0x21255329, rnd(), rnd() | 0x88, rnd() | 0x110});
        t.file_language = std::uint8_t(pick({0, 2, 10, 11, 0x80}));
        t.file_currency = std::uint8_t(pick({0, 9, 24, 25, 0x7f, 0x80, 0xff}));
        t.file_font = pick({7, 0xfffffffe, 0});
        t.system_font_name = chance(3) ? "Geneva" : chance(4) ? "" : "Chicago";
        t.file_font_name = chance(5) ? "A font name of thirty characters" : "Chicago";
        t.short_read = chance(2) ? 99 : 1 + random() % 9;
        t.short_write = chance(2) ? 99 : 1 + random() % 9;
        t.short_answer = pick({0, 0, 2, 0xffffffff});
        return t;
    };
    const auto add = [&](unsigned routine, std::uint32_t argument, unsigned count, bool varied = true) {
        for (unsigned n = 0; n < count; ++n) {
            Case t;
            t.routine = routine;
            t.argument = argument;
            all.push_back(varied || n ? world(t) : t);
        }
    };
    add(construct, 0, 1, false); add(construct, 0, 2500);
    add(copy, 0, 60);
    for (const auto deleting : {0u, 1u, 0x7fffu, 0x8000u, 0xffffu, 0x10000u, 0x10001u, 0xffff0000u}) {
        add(destroy, deleting, 12);
        for (unsigned n = 0; n < 6; ++n) {
            auto t = world(Case{.routine = destroy, .argument = deleting});
            t.linked = n & 1; t.null_record = n >= 4;
            all.push_back(t);
        }
    }
    add(defaults, 0, 400);
    for (unsigned bit = 0; bit < 32; ++bit) add(select_one, 1u << bit, bit == 7 || bit == 8 ? 250 : 8);
    for (const auto option : {0u, 3u, 5u, 6u, 7u, 0x18u, 0x180u, 0x81u, 0x8001u, 0x18000u, 0xffffffffu, 0x80000001u, 0x7fffffffu, 0xc0u})
        add(select_one, option, 6);
    for (unsigned n = 0; n < 60; ++n) add(select_one, rnd(), 1);
    for (const auto value : {0u, 1u, 2u, 10u, 11u, 0x7fu, 0x80u, 0xffu, 0x100u, 0x12345605u, 0xffffff80u}) add(language, value, 12);
    for (const auto value : {0u, 1u, 24u, 25u, 26u, 0x7fu, 0x80u, 0xffu, 0x100u, 0x118u, 0x119u, 0xffffff18u}) add(currency, value, 4);
    for (unsigned value = 0; value < 256; ++value) {
        auto t = world(Case{.routine = language_currency});
        t.language = std::uint8_t(value);
        all.push_back(t);
    }
    for (const auto value : {0u, 6u, 7u, 8u, 30u, 364u, 365u, 366u, 0x7fffu, 0x8000u, 0xffffu, 0x10007u, 0x1016du, 0x1016eu, 0xffff0006u})
        add(interval, value, 3);
    add(apply, 0, 1, false); add(apply, 0, 4000);
    for (unsigned n = 0; n < 600; ++n) {
        auto t = world(Case{.routine = apply});
        t.linked = true;
        all.push_back(t);
    }
    // Each write failing in turn, with everything else in order.
    for (unsigned n = 1; n <= 9; ++n) for (const auto answer : {0u, 2u}) {
        Case t;
        t.short_write = n; t.short_answer = answer;
        all.push_back(t);
        t.routine = construct; t.short_write = 99; t.short_read = n;
        all.push_back(t);
    }
    add(font, 0, 12);
    for (const auto value : {0u, 7u, 0xfffffffeu, 0x80000000u}) add(set_font, value, 6);
    return all;
}
std::string describe(const Case& t, bool production) {
    return hex_address(entries[t.routine]) + " production=" + std::to_string(production) + " argument=" + hex_address(t.argument) +
           " options=" + hex_address(t.options) + " linked=" + std::to_string(t.linked) + " player=" + std::to_string(t.player_exists) +
           " opens=" + std::to_string(t.opens) + " short_read=" + std::to_string(t.short_read) + " short_write=" +
           std::to_string(t.short_write) + " display=" + std::to_string(t.display_opens) + " clobber=" + std::to_string(t.clobber);
}
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) return 2;
    try {
        auto base = std::make_unique<State>();
        load_program(*base, argv[1]);
        dispatch_probe = probe;
        routine_check_native_contract = true;
        if (argc == 3 && std::string(argv[2]) == "--original-routing") {
            // With CM_GAME_CONFIG_ORIGINAL=1 the hooks decline: with no budget, the original stops at its entry.
            unsigned declined = 0;
            production_mode = true;
            for (unsigned routine = 0; routine < entries.size(); ++routine) {
                active = Case{.routine = routine};
                auto s = cm::semantic_io_checks::clone(*base);
                Cpu c(*s);
                initialize(c);
                s->budget = s->executed;
                try { rewrite(*s, entries[routine]); }
                catch (const std::exception& e) {
                    declined += std::string(e.what()) == "Native instruction budget exhausted" && s->pc == entries[routine];
                }
            }
            std::cout << declined << "/12 entries ran their original body\n";
            return declined == entries.size() ? 0 : 1;
        }
        if (argc == 3) return 2;
        const auto all = cases();
        unsigned completed = 0, failures = 0;
        std::map<std::uint32_t, std::array<unsigned, 2>> counts;
        for (const bool production : {false, true}) {
            production_mode = production;
            for (const auto& test : all) for (const bool clobber : {false, true}) {
                if (failures >= 6) break;
                active = test;
                active.clobber = clobber;
                auto s = cm::semantic_io_checks::clone(*base);
                Cpu c(*s);
                initialize(c);
                Observed observer;
                if (production) s->memory.observer = &observer;
                const auto entry = entries[test.routine];
                const auto differences = compare_routine_call(*s, entry, original, rewrite, game_config_results(entry));
                if (!differences.empty()) {
                    std::cout << "DIFFERS " << describe(active, production) << '\n';
                    for (const auto& difference : differences) std::cout << "  " << difference << '\n';
                    ++failures;
                } else {
                    ++completed;
                    ++counts[entry][production ? 1 : 0];
                }
            }
            if (failures) break;
        }
        // The comparison must fail when the C++ leaves another byte in the record.
        production_mode = false;
        active = Case{};
        {
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s);
            initialize(c);
            if (compare_routine_call(*s, entries[apply], original, corrupt, RoutineChecker::result_r3).empty()) {
                ++failures;
                std::cout << "FAIL corruption control\n";
            } else std::cout << "ok deliberate memory corruption detected\n";
        }
        // Production ownership: with no instruction budget of its own, each entry still returns through its
        // hook, and the original body does not.
        // The cases avoid the two leaves the runtime lowers (0x10768810, 0x10769b50): their instructions are
        // charged to the same budget.
        unsigned ownership = 0;
        for (unsigned routine = 0; routine < entries.size(); ++routine) {
            const Case owned{.routine = routine,
                             .argument = routine == select_one ? 1u : routine == language ? 2u : routine == currency ? 9u : 30u,
                             .options = 0x212552a1};
            active = owned;
            auto normal = cm::semantic_io_checks::clone(*base);
            Cpu nc(*normal);
            initialize(nc);
            normal->budget = normal->executed;
            Observed observer;
            normal->memory.observer = &observer;
            production_mode = ownership_mode = true;
            try { rewrite(*normal, entries[routine]); ++ownership; }
            catch (const std::exception& e) { ++failures; std::cout << "FAIL production ownership " << hex_address(entries[routine]) << ' ' << e.what() << '\n'; }
            production_mode = ownership_mode = false;
            active = owned;
            auto reference = cm::semantic_io_checks::clone(*base);
            Cpu rc(*reference);
            initialize(rc);
            reference->budget = reference->executed;
            bool rejected = false;
            try { original(*reference, entries[routine]); }
            catch (const std::exception& e) { rejected = std::string(e.what()) == "Native instruction budget exhausted" && reference->pc == entries[routine]; }
            if (!rejected) { ++failures; std::cout << "FAIL original budget control " << hex_address(entries[routine]) << '\n'; }
        }
        for (const auto& [entry, n] : counts) std::cout << hex_address(entry) << " returned " << n[0] << " controlled + " << n[1] << " production\n";
        std::cout << ownership << "/12 production ownership controls; " << completed << " returned comparisons; " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
