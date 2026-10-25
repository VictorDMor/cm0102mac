// Fixture checks of the C++ MAIN.CPP routines (main_program_routines.inc)
// against their generated originals, for the paths session captures cannot
// reach: the open-documents handler and its callbacks (the host never sends
// that event), the debugging script, speech (the host has none), the About
// box, the exit path of a fatal error, and every branch of the event pump,
// the menus and the Carbon handlers.
//
// Each case starts from a real game state (a session capture's initial
// snapshot), sets the arguments and the memory the branch needs, and runs the
// routine twice in forked children: the original body and the C++
// (routine_check.hpp compare_routine_call, strict: r0, r11, r12, XER and CTR
// too). The Toolbox imports are stand-ins (the memory ones run live), and so
// are a few guest library routines whose own effects are not under test.
// Both runs see the same stand-ins, scripted per case. A last case spoils the
// rewrite's result and must differ.
#include "main_program_adapter.hpp"
#include "main_program_detail.hpp"
#include "application_exit.hpp"
#include "runtime_lifecycle_adapter.hpp"
#include "match_replay.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <cctype>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace cm::aot;
using guest::Cpu;
namespace slot = main_program::detail::slot;
namespace global = main_program::global;
namespace event_record = main_program::detail::event_record;

namespace {
// The stand-ins, each with the values a case scripts for its successive calls
// (0 once they run out). A Toolbox call is replaced at its import, where both
// runs arrive whatever calls the glue (generated code calls glues in place);
// it is named by its glue's address. A guest routine is replaced at its entry
// through the dispatcher, so only routines both runs reach there (not owners,
// not on the caller's page).
struct Fake {
    bool import = true;
    std::function<void(Cpu&, std::uint32_t)> effect;  // gets the next scripted value; null: r3 = it
};
std::map<std::uint32_t, Fake> fakes;
std::map<std::uint32_t, std::deque<std::uint32_t>> script;
std::map<std::string, std::uint32_t, std::less<>> glues;  // import name -> its glue
std::uint32_t next(std::uint32_t id) {
    auto& values = script[id];
    if (values.empty()) return 0;
    const auto value = values.front();
    values.pop_front();
    return value;
}
void apply(Cpu& c, std::uint32_t id, const Fake& fake) {
    const auto value = next(id);
    if (fake.effect) fake.effect(c, value);
    else c.set(3, value);
    c.s.pc = c.lr();
}
bool fixture_probe(State& s) {
    const auto found = fakes.find(s.pc);
    if (found == fakes.end() || found->second.import) return false;
    Cpu c(s);
    apply(c, s.pc, found->second);
    return true;
}
// Every other import answers 0, except the memory ones, which run live.
// Each call folds its name and r3-r10 into a word of guest memory, so the
// runs must also make the same calls with the same arguments.
std::uint32_t import_digest = 0;  // its address
// A run that keeps calling the Toolbox (a loop waiting for a dialog item the
// script never gives) stops here instead of hanging.
unsigned import_calls = 0;
struct ImportFixture final : RuntimeLifecycleBoundaryObserver {
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        const std::string_view import(name);
        if (++import_calls > 20000) throw std::runtime_error("the run does not end (20000 Toolbox calls)");
        auto digest = std::uint32_t(s.memory.read<4>(import_digest));
        const auto mix = [&](std::uint32_t word) { digest = (digest ^ word) * 0x01000193u; };
        for (const char ch : import) mix(std::uint8_t(ch));
        for (unsigned n = 3; n <= 10; ++n) mix(std::uint32_t(s.r<4>(n * 4)));
        s.memory.write<4>(import_digest, digest);
        if (import == "CarbonLib::ExitToShell") throw ApplicationExit{};
        const auto glue = glues.find(import);
        const auto found = glue != glues.end() ? fakes.find(glue->second) : fakes.end();
        if (found == fakes.end()) {
            for (const std::string_view live : {"CarbonLib::BlockMoveData", "CarbonLib::NewPtr",
                                                "CarbonLib::NewPtrClear", "CarbonLib::DisposePtr"})
                if (import == live) return false;
            Cpu c(s);
            c.set(3, 0);
            s.pc = c.lr();
            return true;
        }
        Cpu c(s);
        apply(c, glue->second, found->second);
        return true;
    }
} import_fixture;
// The import a glue reaches: `lwz r12,offset(r2)` gives its transition vector.
void name_glue(const State& s, std::uint32_t glue) {
    const auto load = std::uint32_t(s.memory.read<4>(glue));
    if (load >> 16 != 0x8182) throw std::runtime_error("Not an import glue: " + hex_address(glue));
    const auto vector = std::uint32_t(s.memory.read<4>(0x108bba90 + std::uint32_t(std::int32_t(std::int16_t(load)))));
    const char* name = import_name(std::uint32_t(s.memory.read<4>(vector)));
    if (!name) throw std::runtime_error("No import behind glue " + hex_address(glue));
    glues[name] = glue;
}
void put_pascal(Cpu& c, std::uint32_t at, std::string_view text) {
    c.put8(at, std::uint32_t(text.size()));
    for (std::size_t i = 0; i < text.size(); ++i) c.put8(at + 1 + std::uint32_t(i), std::uint8_t(text[i]));
}
void put_c(Cpu& c, std::uint32_t at, std::string_view text) {
    for (std::size_t i = 0; i < text.size(); ++i) c.put8(at + std::uint32_t(i), std::uint8_t(text[i]));
    c.put8(at + std::uint32_t(text.size()), 0);
}

// A TOC word of the game (inside an import r2 is the import's own TOC).
std::uint32_t game_toc(const Cpu& c, std::int32_t offset) { return c.u32(0x108bba90 + std::uint32_t(offset)); }

// Guest scratch for a case's data (0x4000 bytes above the routine's stack).
std::uint32_t scratch = 0;
std::uint32_t path_text = 0;  // the text FSpGetFullPath's stand-in returns (a handle's block)
std::uint16_t path_length = 0;

void install_fakes() {
    const auto value_r3 = [](Cpu& c, std::uint32_t v) { c.set(3, v); };
    // Toolbox glues.
    for (const std::uint32_t glue : {0x1077fb80u, 0x107800c0u, 0x10780648u, 0x10781008u, 0x10781020u, 0x10780678u,
                                     0x10780690u, 0x107806a8u, 0x107806d8u, 0x10780708u, 0x107807b0u, 0x107807c8u,
                                     0x107807f8u, 0x107807e0u, 0x10780660u, 0x10780810u, 0x10780738u, 0x10780750u,
                                     0x10780768u, 0x10780780u, 0x10780798u, 0x10780828u, 0x107805a0u, 0x107804f8u,
                                     0x10780528u, 0x10780630u, 0x10780eb8u, 0x10780ed0u, 0x107812d8u, 0x1077fb08u,
                                     0x1077fb20u, 0x1077fb38u, 0x1077fb68u, 0x10780108u, 0x10780120u, 0x107801b0u,
                                     0x107801c8u, 0x107801f8u, 0x10780210u, 0x10780228u, 0x10780270u, 0x10780240u,
                                     0x1077fee0u, 0x107802b8u, 0x107802d0u, 0x107802e8u, 0x1077ffa0u, 0x10780cc0u,
                                     0x10780cd8u, 0x10780cf0u})
        fakes[glue] = Fake{true, value_r3};
    fakes[0x1077fb50] = Fake{true, [](Cpu&, std::uint32_t) {}};  // NewAEEventHandlerUPP: the TVector itself
    fakes[0x1077fc40] = Fake{true, [](Cpu& c, std::uint32_t) {     // EqualString, ignoring case
        const auto a = c.r(3), b = c.r(4);
        bool equal = c.u8(a) == c.u8(b);
        for (std::uint32_t i = 1; equal && i <= c.u8(a); ++i)
            equal = std::tolower(c.u8(a + i)) == std::tolower(c.u8(b + i));
        c.set(3, equal ? 1 : 0);
    }};
    fakes[0x10780ff0] = Fake{true, [](Cpu& c, std::uint32_t v) {  // NewSpeechChannel
        if (!v) c.put32(c.r(4), 0x5000);
        c.set(3, v);
    }};
    fakes[0x107806c0] = Fake{true, [](Cpu& c, std::uint32_t v) { c.put16(c.r(4), v); }};  // ModalDialog: the item
    fakes[0x10780720] = Fake{true, [](Cpu& c, std::uint32_t v) {  // FindWindow: part << 16 | which window
        const auto window = v & 0xffff;
        c.put32(c.r(4), window == 1 ? c.u32(game_toc(c, slot::game_window))
                        : window == 2 ? c.u32(game_toc(c, slot::other_window)) : 0x7700);
        c.set(3, v >> 16);
    }};
    fakes[0x107803f0] = Fake{true, [](Cpu& c, std::uint32_t) { c.put32(c.r(3), 0x7200); }};  // GetPort
    fakes[0x10780510] = Fake{true, [](Cpu& c, std::uint32_t v) {  // WaitNextEvent: the event kind in the high half, 0xffffffff for none
        if (v == 0xffffffff) { c.set(3, 0); return; }
        const auto event = c.r(4);
        c.put16(event + event_record::what, v >> 16);
        c.put32(event + event_record::message, next(0xe0000000));
        c.put32(event + event_record::when, 1234);
        c.put32(event + event_record::where, 0x00400080);
        c.put16(event + event_record::modifiers, next(0xe0000001));
        c.set(3, 1);
    }};
    fakes[0x107800f0] = Fake{true, [](Cpu& c, std::uint32_t) {  // BlockMove
        const auto bytes = c.s.memory.copy_out(c.r(3), c.r(5));
        c.s.memory.copy_in(c.r(4), bytes);
    }};
    fakes[0x10780618] = Fake{true, [](Cpu& c, std::uint32_t v) { c.put32(c.r(3), v); }};  // GetMouse
    fakes[0x10780ee8] = Fake{true, [](Cpu& c, std::uint32_t v) {  // GetEventParameter
        if (c.r(7) == 2) c.put16(c.r(9), v); else c.put32(c.r(9), v);
        c.set(3, 0);
    }};
    fakes[0x1077fad8] = Fake{true, [](Cpu& c, std::uint32_t v) {  // Gestalt: error, then the response
        const auto response = next(0x1077fad9);
        if (!v) c.put32(c.r(4), response);
        c.set(3, v);
    }};
    fakes[0x10780258] = Fake{true, [](Cpu& c, std::uint32_t v) {  // GetMenuItemText
        put_pascal(c, c.r(5), "Item " + std::to_string(c.r(4)) + "." + std::to_string(v));
    }};
    fakes[0x107801e0] = Fake{true, [](Cpu& c, std::uint32_t v) {  // GetIndMenuItemWithCommandID
        c.put32(c.r(6), 0x7800);
        c.put16(c.r(7), 7);
        c.set(3, v);
    }};
    fakes[0x10780288] = Fake{true, [](Cpu& c, std::uint32_t v) {  // FindFolder
        c.put16(c.r(6), 0xffff);
        c.put32(c.r(7), 0x55);
        c.set(3, v);
    }};
    fakes[0x107802a0] = Fake{true, [](Cpu& c, std::uint32_t) {  // GetIndString
        put_pascal(c, c.r(3), "Name " + std::to_string(c.r(5)));
    }};
    fakes[0x10780000] = Fake{true, [](Cpu& c, std::uint32_t v) {  // PBHGetVInfoSync: volume, or nsvErr
        if (v) { c.set(3, std::uint32_t(-35)); return; }
        c.put16(c.r(3) + 0x16, next(0x10780001));
        put_pascal(c, c.u32(c.r(3) + 0x12), "Disk");
        c.set(3, 0);
    }};
    // Guest library routines.
    fakes[0x107681d0] = Fake{false, [](Cpu& c, std::uint32_t v) {  // the list's first FSSpec
        if (v >> 16) { c.set(3, v >> 16); return; }
        const char* names[]{"ab", "abc", "game.sav", "quick.cfg", "world.rgn", "notes.txt", "x.SAV"};
        c.put16(c.r(4), 0xffff);
        c.put32(c.r(4) + 2, 0x55);
        put_pascal(c, c.r(4) + 6, names[v % std::size(names)]);
        c.set(3, 0);
    }};
    fakes[0x1076eaf0] = Fake{false, [](Cpu& c, std::uint32_t v) {  // FSpGetFullPath(spec, &length, &handle)
        if (v) { c.set(3, v); return; }
        c.put16(c.r(4), path_length);
        c.put32(scratch + 0x3ff0, path_text);  // the handle's master pointer
        c.put32(c.r(5), scratch + 0x3ff0);
        c.set(3, 0);
    }};
    fakes[0x1074c6e0] = Fake{false, [](Cpu& c, std::uint32_t v) {  // the script's next line
        if (v) put_c(c, c.r(4), "line " + std::to_string(v));
        c.set(3, v ? 1 : 0);
    }};
    fakes[0x10746a30] = Fake{false, [](Cpu& c, std::uint32_t) { c.set(3, 4321); }};      // the game's tick count
    // fopen, fclose, fprintf (owners), exit (an owner) and the Finder
    // comment's reader (on find_folders's page) run for real against the
    // import stand-ins.
    for (const std::uint32_t routine : {0x10768080u, 0x1076b2f0u, 0x1076b7e0u, 0x1074cc30u, 0x10743590u, 0x10769b50u,
                                        0x107692f0u, 0x10118fe0u, 0x1074ca50u, 0x1074dbf0u, 0x1074d770u, 0x1074d4c0u})
        fakes[routine] = Fake{false, value_r3};
}

struct Case {
    std::string name;
    std::uint32_t entry;
    unsigned results;
    std::function<void(Cpu&)> setup;
    std::map<std::uint32_t, std::deque<std::uint32_t>> script{};
};

int failures = 0, cases = 0;
void run_case(const State& base, const Case& test, bool corrupt = false) {
    auto s = cm::semantic_io_checks::clone(base);
    s->budget = s->executed + 200000000;
    import_calls = 0;
    // A fresh frame well below the snapshot's, the case's data above it.
    const auto sp = (std::uint32_t(s->r<4>(4)) - 0x10000) & ~0xfu;
    scratch = sp + 0x100;
    import_digest = scratch + 0x3ffc;
    path_text = scratch + 0x3000;
    path_length = 12;
    s->runtime_lifecycle_observer = &import_fixture;
    Cpu c(*s);
    s->wr<4>(4, sp);
    s->wr<4>(4128, return_sentinel);
    s->pc = test.entry;
    script = test.script;
    put_c(c, path_text, "Disk:CM:a.sav");
    test.setup(c);
    // The setup may have called the game's allocator.
    s->wr<4>(4128, return_sentinel);
    s->pc = test.entry;
    ++cases;
    std::vector<std::string> found;
    try {
        found = compare_routine_call(*s, test.entry, main_program_run_original, main_program_run_rewrite, test.results,
                                     corrupt);
    } catch (const std::exception& error) { found.push_back(error.what()); }
    if (corrupt) {
        std::cout << (found.empty() ? "FAIL (a spoiled result went unnoticed): " : "ok (spoiled result detected): ")
                  << test.name << '\n';
        if (found.empty()) ++failures;
        return;
    }
    std::cout << (found.empty() ? "ok: " : "DIFFERS: ") << test.name << '\n';
    for (const auto& line : found) std::cout << "  " << line << '\n';
    if (!found.empty()) ++failures;
}

std::uint32_t global_at(Cpu& c, std::int32_t offset) { return c.r(2) + std::uint32_t(offset); }
void set_global32(Cpu& c, std::int32_t offset, std::uint32_t v) { c.put32(global_at(c, offset), v); }
void set_global8(Cpu& c, std::int32_t offset, std::uint32_t v) { c.put8(global_at(c, offset), v); }
// The speech text is freed with free() and the setup text with DisposePtr(),
// so the cases take their blocks from malloc() and NewPtr() (run in the
// parent before the children fork).
std::uint32_t allocate(Cpu& c, std::uint32_t size) {
    c.set(3, size);
    c.call(0x10775130, return_sentinel);  // malloc
    return c.r(3);
}
std::uint32_t new_pointer(Cpu& c, std::uint32_t size) {
    c.set(3, size);
    c.call(0x1077fc70, return_sentinel);  // NewPtr, through its glue
    c.set(2, 0x108bba90);
    return c.r(3);
}

std::vector<Case> all_cases() {
    std::vector<Case> list;
    const auto add = [&](std::string name, std::uint32_t entry, unsigned results, std::function<void(Cpu&)> setup,
                         std::map<std::uint32_t, std::deque<std::uint32_t>> script = {}) {
        list.push_back({std::move(name), entry, results, std::move(setup), std::move(script)});
    };
    constexpr unsigned R3 = RoutineChecker::result_r3, NONE = 0;
    const auto args = [](std::initializer_list<std::pair<unsigned, std::uint32_t>> list) {
        return [values = std::vector(list)](Cpu& c) {
            for (const auto& [n, value] : values) c.set(n, value);
        };
    };

    // The open-documents handler: each suffix, each failure.
    for (std::uint32_t name = 0; name < 7; ++name)
        for (std::uint32_t command_line : {0u, 1u})
            for (std::uint32_t full_path : {0u, 0xffffffd5u})
                add("open_documents name " + std::to_string(name) + " line " + std::to_string(command_line) +
                        " path " + std::to_string(full_path),
                    0x10770ac0, R3,
                    [=](Cpu& c) {
                        c.set(3, 0x6000);
                        if (!command_line) set_global32(c, global::command_line, 0);
                        else {
                            put_c(c, scratch + 0x1000, "-x");
                            set_global32(c, global::command_line, scratch + 0x1000);
                        }
                    },
                    {{0x107681d0, {name}}, {0x1076eaf0, {full_path}}, {0x107768f0, {name == 3 ? 0x6200u : 0u}}});
    add("open_documents no list", 0x10770ac0, R3, args({{3, 0x6000}}), {{0x1077fb80, {0xfffff95cu}}});
    add("open_documents no spec", 0x10770ac0, R3, args({{3, 0x6000}}), {{0x107681d0, {0xffd50000u}}});
    add("open_documents cfg opened", 0x10770ac0, R3, args({{3, 0x6000}}), {{0x107681d0, {3}}, {0x107768f0, {0x6200}}});

    // Its callbacks.
    // A FILE the game's own fopen() returns (its imports answer as stand-ins).
    const auto file = [](Cpu& c) {
        put_c(c, scratch + 0x2000, "quick_start.cfg");
        put_c(c, scratch + 0x2010, "w");
        c.set({{3, scratch + 0x2000}, {4, scratch + 0x2010}});
        c.call(0x107768f0, return_sentinel);
        if (!c.r(3)) throw std::runtime_error("fopen failed in the fixture");
        return c.r(3);
    };
    for (std::uint16_t length : {std::uint16_t(12), std::uint16_t(0xff), std::uint16_t(0x100), std::uint16_t(0x120)})
        add("write_quick_start length " + std::to_string(length), 0x10770f60, R3,
            [=](Cpu& c) { path_length = length; c.set(4, file(c)); c.set(3, scratch); });
    add("write_quick_start no file", 0x10770f60, R3, [](Cpu& c) { c.set({{3, scratch}, {4, 0}}); });
    add("write_quick_start no spec", 0x10770f60, R3, [](Cpu& c) { c.set({{3, 0}, {4, 0x6200}}); });
    add("write_quick_start no path", 0x10770f60, R3, [](Cpu& c) { c.set({{3, scratch}, {4, 0x6200}}); },
        {{0x1076eaf0, {0xffffffd5u}}});
    add("add_setup_line first", 0x10771010, R3, [](Cpu& c) { set_global32(c, global::setup_text, 0); c.set(3, scratch); });
    add("add_setup_line more", 0x10771010, R3, [](Cpu& c) {
        const auto text = new_pointer(c, 32);
        put_c(c, text, "AUTO=1\r");
        set_global32(c, global::setup_text, text);
        c.set(3, scratch);
    });
    add("add_setup_line no spec", 0x10771010, R3, args({{3, 0}}));
    add("add_setup_line no path", 0x10771010, R3, [](Cpu& c) { c.set(3, scratch); }, {{0x1076eaf0, {0xffffffd5u}}});
    add("add_setup_line no memory", 0x10771010, R3, [](Cpu& c) {
        set_global32(c, global::setup_text, 0);
        c.set(3, scratch);
        fakes[0x1077fc70] = Fake{true, nullptr};  // NewPtr fails
    });
    add("run_script", 0x10770cc0, NONE, [](Cpu& c) {
        put_c(c, scratch, "a\rb\r");
        c.set(3, scratch);
    }, {{0x1074c6e0, {1, 2, 3, 0}}, {0x1076b2f0, {0x6400, 0, 0x6400}}});
    add("print_documents", 0x10770c90, R3, args({}));
    add("open_application", 0x10770ab0, R3, args({}));
    add("quit_application", 0x10770ca0, R3, args({}));
    add("high_level_event", 0x10770a80, NONE, [](Cpu& c) { c.set(3, scratch); });

    // Speech.
    const auto speech = [](std::uint32_t available, std::uint32_t wanted, std::uint32_t channel, bool text) {
        return [=](Cpu& c) {
            set_global8(c, global::speech_available, available);
            c.put8(c.toc(slot::speech_wanted), wanted);
            set_global32(c, global::speech_channel, channel);
            set_global32(c, global::spoken_text, text ? allocate(c, 8) : 0);
            put_c(c, scratch, "Goal");
            c.set(3, scratch);
        };
    };
    for (std::uint32_t available : {0u, 1u}) for (std::uint32_t wanted : {0u, 1u}) for (std::uint32_t channel : {0u, 0x5000u})
        for (bool text : {false, true})
            add("speak available " + std::to_string(available) + " wanted " + std::to_string(wanted) + " channel " +
                    std::to_string(channel) + " text " + std::to_string(text),
                0x10770d20, NONE, speech(available, wanted, channel, text));
    add("speak no channel", 0x10770d20, NONE, speech(1, 1, 0, false), {{0x10780ff0, {0xffffff94u}}});
    add("speak while speaking", 0x10770d20, NONE, [=](Cpu& c) {
        speech(1, 1, 0x5000, true)(c);
        c.put32(c.toc(slot::idle_wait), 3);  // the pump only counts down meanwhile
    }, {{0x10781008, {1, 2, 0}}});
    for (std::uint32_t channel : {0u, 0x5000u}) for (bool text : {false, true}) {
        add("stop_speech channel " + std::to_string(channel) + " text " + std::to_string(text), 0x1076f780, NONE,
            speech(1, 1, channel, text));
        for (std::uint32_t wanted : {0u, 1u}) for (std::uint32_t key : {0x73u, 0x53u, 0x173u})
            add("speech_key " + std::to_string(key) + " wanted " + std::to_string(wanted) + " channel " +
                    std::to_string(channel) + " text " + std::to_string(text),
                0x107704e0, R3, [=](Cpu& c) { speech(1, wanted, channel, text)(c); c.set(3, key); });
    }

    // The fatal exit, game started or not; shut-down.
    for (std::uint32_t started : {0u, 1u}) for (std::uint32_t region : {0u, 0x7400u}) {
        add("fatal_exit started " + std::to_string(started) + " region " + std::to_string(region), 0x1076f7d0, NONE,
            [=](Cpu& c) {
                c.put8(c.toc(slot::game_started), started);
                set_global32(c, global::mouse_region, region);
                c.set(3, 7);
            });
        add("shut_down started " + std::to_string(started) + " region " + std::to_string(region), 0x1076f730, NONE,
            [=](Cpu& c) {
                c.put8(c.toc(slot::game_started), started);
                set_global32(c, global::mouse_region, region);
            });
    }

    // The About box and the menus.
    add("about_box", 0x10770e40, NONE, args({}), {{0x10780678, {0x6000}}, {0x107806c0, {5, 0, 3, 7, 2}}});
    add("about_box first item", 0x10770e40, NONE, args({}), {{0x10780678, {0x6000}}, {0x107806c0, {1}}});
    add("about_box no dialog", 0x10770e40, NONE, args({}), {{0x10780678, {0}}, {0x107806c0, {1}}});
    for (std::uint32_t item : {1u, 2u, 3u, 4u, 5u, 8u, 0xffffu}) {
        add("file_menu " + std::to_string(item), 0x10770970, NONE, args({{3, 0x6000}, {4, item}}));
        add("apple_menu " + std::to_string(item), 0x10770950, NONE, args({{3, 0x6000}, {4, item}}));
        add("edit_menu " + std::to_string(item), 0x107709e0, NONE, args({{3, 0x6000}, {4, item}}),
            {{0x10780678, {0x6000}}, {0x107806c0, {1}}});
    }
    for (std::uint32_t menu : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 0xffffu})
        for (std::uint32_t item : {1u, 3u, 5u, 8u})
            add("menu_command " + std::to_string(menu) + "/" + std::to_string(item), 0x107708c0, NONE,
                args({{3, 0x6000}, {4, menu << 16 | item}}), {{0x10780678, {0x6000}}, {0x107806c0, {1}}});
    add("menu_command nothing", 0x107708c0, NONE, args({{3, 0x6000}, {4, 0}}));
    add("menu_command negative", 0x107708c0, NONE, args({{3, 0x6000}, {4, 0xfffe0003u}}));
    for (std::uint16_t request : {std::uint16_t(0), std::uint16_t(0x402), std::uint16_t(0x3fe)})
        for (std::uint32_t file : {0u, 0x7002u}) for (std::uint32_t debug : {0u, 0x7004u})
            add("adjust_menus request " + std::to_string(request) + " file " + std::to_string(file) + " debug " +
                    std::to_string(debug),
                0x10770240, NONE,
                [=](Cpu& c) { c.put16(c.toc(slot::game) + main_program::detail::screen_request, request); c.set(3, scratch); },
                {{0x107807b0, {file, debug}}});

    // Keys.
    for (std::uint32_t code = 0x70; code < 0x80; ++code)
        for (std::uint32_t modifiers : {0u, 0x100u, 0x800u, 0x1000u})
            add("key_down code " + std::to_string(code) + " modifiers " + std::to_string(modifiers), 0x10770380, NONE,
                [=](Cpu& c) {
                    c.put16(scratch + event_record::what, 3);
                    c.put32(scratch + event_record::message, code << 8 | (code == 0x71 ? 0x73 : 0x61));
                    c.put16(scratch + event_record::modifiers, modifiers);
                    c.set(3, scratch);
                },
                {{0x10780660, {code & 1 ? 0x00020005u : 0u}}});

    // Mouse.
    for (std::uint32_t aqua : {0u, 2u}) for (std::uint16_t state : {std::uint16_t(0), std::uint16_t(1), std::uint16_t(2),
                                                                     std::uint16_t(3), std::uint16_t(4)})
        for (std::uint32_t modifiers : {0u, 0x800u, 0x1000u})
            add("content_click aqua " + std::to_string(aqua) + " state " + std::to_string(state) + " modifiers " +
                    std::to_string(modifiers),
                0x10770570, NONE, [=](Cpu& c) {
                    set_global8(c, global::menu_checked, 1);
                    set_global32(c, global::menu_features, aqua);
                    c.put16(c.toc(slot::button_state), state);
                    c.put16(scratch + event_record::modifiers, modifiers);
                    c.set(3, scratch);
                });
    add("mouse_up", 0x10770650, NONE, args({}));
    for (std::uint32_t point : {0x00100020u, 0xffff0010u, 0x0010ffffu, 0x80000000u, 0xfffffffeu})
        add("track_mouse " + std::to_string(point), 0x107701d0, NONE, args({}), {{0x10780630, {point}}});
    for (std::uint32_t message : {0x01000000u, 0x01000001u, 0xfa000000u, 0x00000001u, 0x81000001u})
        add("os_event " + std::to_string(message), 0x10770a30, NONE, [=](Cpu& c) {
            c.put32(scratch + event_record::message, message);
            c.set(3, scratch);
        });
    add("activate_event", 0x10770a20, NONE, args({{3, 0}}));
    add("debug_menu", 0x10770a10, NONE, args({}));
    add("nothing", 0x1076f880, NONE, args({}));

    // Carbon handlers.
    for (std::uint32_t event_class : {0x77696e64u, 0x6d6f7573u, 0x6b657962u, 0x7a7a7a7au, 0x00000001u})
        for (std::uint32_t kind : {2u, 10u, 0x55u, 1u, 0x56u})
            add("window_event class " + std::to_string(event_class) + " kind " + std::to_string(kind), 0x10770670, R3,
                args({{3, 0x6600}, {4, 0x6700}, {5, 0x6800}}),
                {{0x10780eb8, {event_class}}, {0x10780ed0, {kind, kind}}, {0x10780ee8, {1, 0x1000}}});
    for (std::uint32_t delta : {3u, 0xfffffffdu})
        add("wheel_event " + std::to_string(delta), 0x10770760, R3, args({{3, 0x6600}, {4, 0x6700}, {5, 0x6800}}),
            {{0x10780ee8, {delta}}});
    for (std::uint32_t button : {1u, 2u, 3u}) for (std::uint32_t modifiers : {0u, 0x800u, 0x1000u, 0x200u})
        add("click_event button " + std::to_string(button) + " modifiers " + std::to_string(modifiers), 0x107707c0, R3,
            args({{3, 0x6600}, {4, 0x6700}, {5, 0x6800}}), {{0x10780ee8, {button, modifiers}}});
    add("bounds_event", 0x10770880, R3, args({{3, 0x6600}, {4, 0x6700}, {5, 0x6800}}));

    // The event pump: the countdown, the wheel, and each event kind.
    for (std::uint32_t wait : {1u, 2u})
        add("pump_events waiting " + std::to_string(wait), 0x1076fe80, NONE,
            [=](Cpu& c) { c.put32(c.toc(slot::idle_wait), wait); });
    for (std::uint32_t scroll : {2u, 0xfffffffeu}) for (std::uint32_t control : {0u, 1u})
        add("pump_events scroll " + std::to_string(scroll) + " control " + std::to_string(control), 0x1076fe80, NONE,
            [=](Cpu& c) {
                c.put32(c.toc(slot::idle_wait), 0);
                c.put32(c.toc(slot::pending_scroll), scroll);
                c.put8(c.toc(slot::control_down), control);
            });
    const auto pump = [](Cpu& c) {
        c.put32(c.toc(slot::idle_wait), 0);
        c.put32(c.toc(slot::pending_scroll), 0);
    };
    for (std::uint32_t what = 0; what < 26; ++what)
        add("pump_events event " + std::to_string(what), 0x1076fe80, NONE, pump,
            {{0x10780510, {what << 16}}, {0xe0000000, {what == 15 ? 0x01000001u : 0x00000001u}}, {0xe0000001, {0}}});
    for (std::uint32_t part = 0; part < 8; ++part) for (std::uint32_t window : {0u, 1u, 2u})
        for (std::uint32_t track : {0u, 1u})
            add("pump_events mouse down part " + std::to_string(part) + " window " + std::to_string(window) +
                    " track " + std::to_string(track),
                0x1076fe80, NONE, pump,
                {{0x10780510, {1u << 16}}, {0x10780720, {part << 16 | window}}, {0x10780798, {track}},
                 {0x10780810, {0x00030008u}}, {0x10780678, {0x6000}}, {0x107806c0, {1}}, {0x107807b0, {0x7002, 0x7004}}});
    for (std::uint32_t left : {0u, 1u}) for (std::uint32_t held : {0u, 1u})
        for (std::uint16_t state : {std::uint16_t(1), std::uint16_t(2)}) for (std::uint32_t button : {0u, 1u})
            add("pump_events idle left " + std::to_string(left) + " held " + std::to_string(held) + " state " +
                    std::to_string(state) + " button " + std::to_string(button),
                0x1076fe80, NONE, [=](Cpu& c) {
                    pump(c);
                    c.put8(c.toc(slot::left_held), left);
                    c.put8(c.toc(slot::button_down), held);
                    c.put16(c.toc(slot::button_state), state);
                },
                {{0x10780510, {0xffffffffu}}, {0x10780828, {button}}});
    for (std::uint32_t what : {0u, 23u, 1u})
        add("poll_high_level_events " + std::to_string(what), 0x10770170, NONE, args({}),
            {{0x10780510, {what ? what << 16 : 0xffffffffu}}});

    // Start-up.
    for (std::uint32_t failing : {1u, 2u, 3u, 0u})
        add("process_information failing " + std::to_string(failing), 0x1076f440, R3, args({}),
            {{0x1077fb08, {failing == 1 ? 0xffffffffu : 0u}}, {0x1077fb20, {failing == 2 ? 0xffffffffu : 0u}},
             {0x1077fad8, {failing == 3 ? 0xffffffffu : 0u}}, {0x1077fad9, {3}}});
    add("install_handlers", 0x1076f4f0, NONE, args({}));
    for (std::uint32_t pointer : {0u, 1u})
        add("initialize pointer " + std::to_string(pointer), 0x1076f600, NONE,
            [=](Cpu&) { if (!pointer) fakes[0x1077fc70] = Fake{true, nullptr}; },
            {{0x10780000, {0, 0, 0, 1}}, {0x10780001, {5, 6, 0xffff}}, {0x1074dbf0, {1}}});
    add("initialize volume not found", 0x1076f600, NONE, args({}), {{0x10780000, {0, 0, 1}}, {0x10780001, {5, 6}}});
    for (std::uint32_t bar : {0u, 0x7500u}) for (std::uint32_t aqua : {0u, 2u}) for (std::uint32_t found : {0u, 1u})
        for (std::uint32_t apple : {0u, 0x7001u})
            add("set_up_menus bar " + std::to_string(bar) + " aqua " + std::to_string(aqua) + " found " +
                    std::to_string(found) + " apple " + std::to_string(apple),
                0x1076f890, NONE, [=](Cpu& c) {
                    set_global8(c, global::menu_checked, 1);
                    set_global32(c, global::menu_features, aqua);
                },
                {{0x107801b0, {bar}}, {0x107801e0, {found ? 0u : 0xffffff00u}}, {0x10780210, {apple, 0x7002, 0x7003, 0}},
                 {0x10780240, {3, 2}}});
    add("remember_menu_texts many", 0x1076f960, NONE, args({}),
        {{0x10780210, {0x7002, 0x7003, 0x7004}}, {0x10780240, {30, 30, 30}}});
    for (std::uint32_t empty_at : {0u, 3u, 100u})
        add("localize_menus empty at " + std::to_string(empty_at), 0x1076fa20, NONE, [=](Cpu& c) {
            const auto texts = c.toc(slot::menu_texts);
            for (std::uint32_t i = 0; i < 50; ++i) c.put32(texts + 4 * i, i == empty_at || i >= 8 ? 0 : texts);
        }, {{0x10780210, {0x7002, 0x7003, 0}}, {0x10780240, {5, 6}}});
    for (std::uint32_t folder : {0u, 1u}) for (std::uint32_t prefs : {0u, 0xffffffd5u, 0xffffffffu})
        for (std::uint32_t data : {0u, 0xffffffd5u, 0xffffffffu}) for (std::uint32_t made : {0u, 1u})
            for (std::uint32_t alias : {0u, 1u}) for (std::uint32_t comment : {0u, 1u})
                add("find_folders folder " + std::to_string(folder) + " prefs " + std::to_string(prefs) + " data " +
                        std::to_string(data) + " made " + std::to_string(made) + " alias " + std::to_string(alias) +
                        " comment " + std::to_string(comment),
                    0x1076faf0, NONE, args({}),
                    {{0x10780288, {folder ? 0xffffffffu : 0u}},
                     {0x1077fee0, {prefs, prefs == 0xffffffd5u ? 0u : prefs, data, made ? 0u : 0xffffffffu}},
                     {0x107802e8, {made ? 0u : 0xffffffffu}}, {0x1077ffa0, {alias ? 0xffffffffu : 0u}},
                     {0x1074d4c0, {0}}, {0x1076f080, {comment ? 0xffffffffu : 0u}}});
    for (std::uint32_t aqua : {0u, 2u})
        add("install_window_handlers aqua " + std::to_string(aqua), 0x1076fd90, NONE, [=](Cpu& c) {
            set_global8(c, global::menu_checked, 1);
            set_global32(c, global::menu_features, aqua);
        });
    for (std::uint32_t checked : {0u, 1u}) for (std::uint32_t error : {0u, 1u}) for (std::uint32_t features : {0u, 2u, 0xffffffffu})
        add("aqua_menus checked " + std::to_string(checked) + " error " + std::to_string(error) + " features " +
                std::to_string(features),
            0x10770ee0, R3, [=](Cpu& c) {
                set_global8(c, global::menu_checked, checked);
                set_global32(c, global::menu_features, features);
            },
            {{0x1077fad8, {error}}, {0x1077fad9, {6}}});
    return list;
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("Usage: cm-main-program-checks <a session capture's initial.snapshot>");
        State base;
        load_match_snapshot(base, argv[1]);
        // The snapshot stops at the end of an import, inside its glue: the
        // game's TOC is the one the glue saved at sp+0x14.
        base.wr<4>(8, base.memory.read<4>(std::uint32_t(base.r<4>(4)) + 0x14));
        if (base.r<4>(8) != 0x108bba90) throw std::runtime_error("The snapshot does not hold the game's TOC at sp+0x14");
        install_fakes();
        for (const auto& [id, fake] : fakes)
            if (fake.import) name_glue(base, id);
        name_glue(base, 0x1077fc70);  // NewPtr: live, failing in some cases
        dispatch_probe = fixture_probe;
        const auto defaults = fakes;
        auto list = all_cases();
        for (const auto& test : list) {
            fakes = defaults;
            run_case(base, test);
        }
        fakes = defaults;
        run_case(base, list.front(), true);
        std::cout << cases << " cases, " << failures << " failed\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
