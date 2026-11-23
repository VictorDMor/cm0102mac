#pragma once
// The font engine checker's case lists. A block adds a `font_engine_checks_<block>.cpp`
// with FONT_ENGINE_CASES blocks; the checker runs each case's call as the original and as
// the C++ in forked children from a real state (--world: a UI snapshot with the fonts loaded), then the same call with
// mutated inputs (values the original actually read), and compares all the game can observe.
// Pointers and states come from the real game, never fabricated.
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <cstdio>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot::font_engine_checks {
inline constexpr std::uint32_t module_start = 0x10751350, module_end = 0x10768074;
inline constexpr std::size_t module_routines = 263;
// A mapped area outside the game's memory for rows a case builds.
inline constexpr std::uint32_t scratch_base = 0x60000000, scratch_size = 0x100000;
struct Write { unsigned size; std::uint32_t address, value; };
struct Case {
    std::uint32_t entry = 0;
    std::vector<std::pair<unsigned, std::uint32_t>> registers;  // r3...
    std::vector<Write> memory;                                  // applied to both sides, undone afterwards
    std::optional<double> f1;
    std::string label = {};
    // Another module's routines answered on both sides (entry, r3) when dispatched, with no
    // body run: for paths that depend on what the snapshot can't produce.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {};
    // Memory written on both sides each time a routine the checked one calls is dispatched, before
    // it runs (or is answered): for outcomes that need a record to change in the middle of a call,
    // which no starting state produces. The comparison is of the two bodies from that same event.
    // The routine must reach the dispatcher on both sides (not a routine under check, a leaf run in
    // place or a tactics owner); with --call-trace a poke that fires on one side more than on the
    // other is a finding, and the ones that fire on neither are counted (pokes_unfired).
    struct Poke { std::uint32_t entry; std::vector<Write> writes; };
    std::vector<Poke> pokes = {};
    // The bytes a routine the checked one calls is handed, observed: at each dispatch of `entry` (answered or not)
    // `size` bytes at the address in register `reg` are copied to `to` on both sides, where the memory comparison sees
    // them (a record built in the dead stack and given to an answered routine is otherwise invisible).
    struct Capture { std::uint32_t entry; unsigned reg; std::uint32_t size, to; };
    std::vector<Capture> captures = {};
    // An answer for one call only: the `nth` dispatch (from 1) of `entry` in a run is answered with
    // `answer` and the others run. For a failure at a later call of the same routine (the third
    // file read of a record).
    struct NthAnswer { std::uint32_t entry; unsigned nth; std::uint32_t answer; };
    std::vector<NthAnswer> nth_answers = {};
    // Run-time owners whose generated callers run their C++ in place, without the dispatcher (fread 0x10775fe0
    // and the other stream I/O entries, the allocator's entries that generated pages call in place): an answer
    // or nth answer for one reaches the rewrite only. Listed here, the owner runs its original body for this
    // case alone, which is dispatched on both sides, so the answers apply alike (stream_io_entries,
    // allocator_entries; any other entry is an error). The switch is undone after the case: never set
    // State::stream_io_entry_original or allocator_entry_original from a case list, which would leak into
    // every other block's cases.
    std::vector<std::uint32_t> original_owners = {};
    // An expectation checked on the final state of both runs (not of the mutated ones): false is a failure of the whole check. For what
    // the two runs can agree on and still get wrong (the bytes of a file against an independent serialization, a round trip).
    std::function<bool(State&)> verify = nullptr;
    // Runs inside every compared run of this case (the original's, the rewrite's and each traced run), on that run's own state, before the checked call: it
    // may build the state the entry needs by running original code (font_engine_run_original from the state's registers, the link register left as it
    // is), which is the same on both sides. The mutation of such a case changes values the call reads from the prepared state (applied in each run
    // after the preparation, at addresses a prepared run read).
    std::function<void(State&)> prepare = nullptr;
    // Start with FPSCR cleared (the snapshot's has the sticky inexact bit set, which hides a missing inexact result).
    bool clear_fpscr = false;
};
struct Scenario {
    State& s;
    std::uint32_t r32(std::uint32_t at) const { return std::uint32_t(s.memory.read<4>(at)); }
    std::uint32_t u16(std::uint32_t at) const { return std::uint32_t(s.memory.read<2>(at)); }
    std::uint32_t u8(std::uint32_t at) const { return std::uint32_t(s.memory.read<1>(at)); }
    std::uint32_t toc(int slot) const { return r32(0x108bba90 + std::uint32_t(slot)); }
    std::uint32_t global(int slot) const { return r32(toc(slot)); }
    // The stack pointer a case starts from (a frame's locals sit at fixed addresses below it).
    std::uint32_t sp() const { return std::uint32_t(s.r<4>(guest::reg::gpr(1))); }
};
// Writes for a case's memory, and a label's hex.
inline Write w8(std::uint32_t at, std::uint32_t value) { return {1, at, value & 0xff}; }
inline Write w16(std::uint32_t at, std::uint32_t value) { return {2, at, value & 0xffff}; }
inline Write w32(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }
inline void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }
inline std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
// font_engine_checks_e2e.cpp: the end-to-end comparison (see there).
// font_engine_checks.cpp: starts the leaver check (the stack contract of the five leavers) for the run that begins at this state's stack pointer.
void install_leaver_check(State& s);
// font_engine_checks_fnt_e2e.cpp: the interpreter's synthetic programs on a hint font (called from the end-to-end sweep); the values to log.
std::vector<std::uint32_t> run_interpreter_programs(State& s, std::uint32_t hint);
int run_end_to_end(State& s, bool list_only, bool exhaustive, bool corrupt, const std::vector<std::filesystem::path>& files, bool hinted = false);
using CaseList = std::function<void(const Scenario&, std::vector<Case>&)>;
struct CaseBlock {
    CaseBlock(const char* name, CaseList list);
};
#define FONT_ENGINE_CASES_JOIN2(a, b) a##b
#define FONT_ENGINE_CASES_JOIN(a, b) FONT_ENGINE_CASES_JOIN2(a, b)
// FONT_ENGINE_CASES("ratings") { if (!scenario.match) cases.push_back({.entry = 0x..., .registers = {{3, ...}}}); }
#define FONT_ENGINE_CASES(name) \
    static void FONT_ENGINE_CASES_JOIN(font_engine_cases_, __LINE__)(const ::cm::aot::font_engine_checks::Scenario&, std::vector<::cm::aot::font_engine_checks::Case>&); \
    static ::cm::aot::font_engine_checks::CaseBlock FONT_ENGINE_CASES_JOIN(font_engine_block_, __LINE__){name, FONT_ENGINE_CASES_JOIN(font_engine_cases_, __LINE__)}; \
    static void FONT_ENGINE_CASES_JOIN(font_engine_cases_, __LINE__)([[maybe_unused]] const ::cm::aot::font_engine_checks::Scenario& scenario, [[maybe_unused]] std::vector<::cm::aot::font_engine_checks::Case>& cases)
}
