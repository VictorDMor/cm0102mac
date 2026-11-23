#pragma once
// graph_carbon.cpp's checker case lists (tools/graph_carbon_audit.py). A block adds a `graph_carbon_checks_<block>.cpp`
// with GRAPH_CARBON_CASES blocks; the checker runs each case's call as the original and as
// the C++ in forked children from a real state (--world: a UI snapshot of the running game), then the same call with
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

namespace cm::aot::graph_carbon_checks {
// The module's stretch (graph_carbon_audit.SEGMENTS) and the number of routines the audit owns (the static initializer 0x1076ea20 is
// pinned by tools/engine_initialization_audit.py and never routed).
inline constexpr std::uint32_t module_start = 0x10768810, module_end = 0x1076eaa0;
inline constexpr std::size_t module_routines = 54;
inline bool in_module(std::uint32_t address) { return address >= module_start && address < module_end; }
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
    // allocator_entries, cpp_object_storage_entries (new, delete, malloc, free of 0x107735..); any other entry is an error). The switch is undone after the case: never set
    // State::stream_io_entry_original or allocator_entry_original from a case list, which would leak into
    // every other block's cases.
    std::vector<std::uint32_t> original_owners = {};
    // An expectation checked on the final state of both runs (not of the mutated ones): false is a failure of the whole check. For what
    // the two runs can agree on and still get wrong (the bytes of a file against an independent serialization, a round trip).
    std::function<bool(State&)> verify = nullptr;
    // Runs inside every compared run of this case (the original's, the rewrite's and each traced run), on that run's own state, before the checked call: it
    // may build the state the entry needs by running original code (graph_carbon_run_original from the state's registers, the link register left as it
    // is), which is the same on both sides. The mutation of such a case changes values the call reads from the prepared state (applied in each run
    // after the preparation, at addresses a prepared run read).
    std::function<void(State&)> prepare = nullptr;
    // Start with FPSCR cleared (the snapshot's has the sticky inexact bit set, which hides a missing inexact result).
    bool clear_fpscr = false;
    // Answers for imports (Toolbox calls a snapshot has no host for: the Resource Manager, the Memory Manager, FSMakeFSSpec), by the import's
    // name ("CarbonLib::CurResFile"), given on both sides at the import itself, which the generated code and the C++'s dispatched calls both
    // reach: the `nth` call (from 1; 0 for every call) of the import in a run sets r3 to `r3`, applies `writes` and returns.
    struct Import { const char* name; unsigned nth; std::uint32_t r3; std::vector<Write> writes = {}; };
    std::vector<Import> imports = {};
    // The assertion box 0x10769b60 runs for real on both sides instead of being answered (the checker answers it while the module does not list
    // it, which reaches only calls that go through the dispatcher: an assertion on the box's own page (0x10769xxx) is a call in the page, so the
    // original would run the box and the C++ would not). The case must make the box leave quietly (its re-entrancy byte, small data -0x356f, or the
    // "initialising" byte -0x3550 for its printf path).
    bool real_dialog = false;
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
// graph_carbon_checks.cpp: starts the leaver check (the stack contract of the twelve leavers) for the run that begins at this state's stack pointer.
void install_leaver_check(State& s);
using CaseList = std::function<void(const Scenario&, std::vector<Case>&)>;
struct CaseBlock {
    CaseBlock(const char* name, CaseList list);
};
#define GRAPH_CARBON_CASES_JOIN2(a, b) a##b
#define GRAPH_CARBON_CASES_JOIN(a, b) GRAPH_CARBON_CASES_JOIN2(a, b)
// GRAPH_CARBON_CASES("ratings") { if (!scenario.match) cases.push_back({.entry = 0x..., .registers = {{3, ...}}}); }
#define GRAPH_CARBON_CASES(name) \
    static void GRAPH_CARBON_CASES_JOIN(graph_carbon_cases_, __LINE__)(const ::cm::aot::graph_carbon_checks::Scenario&, std::vector<::cm::aot::graph_carbon_checks::Case>&); \
    static ::cm::aot::graph_carbon_checks::CaseBlock GRAPH_CARBON_CASES_JOIN(graph_carbon_block_, __LINE__){name, GRAPH_CARBON_CASES_JOIN(graph_carbon_cases_, __LINE__)}; \
    static void GRAPH_CARBON_CASES_JOIN(graph_carbon_cases_, __LINE__)([[maybe_unused]] const ::cm::aot::graph_carbon_checks::Scenario& scenario, [[maybe_unused]] std::vector<::cm::aot::graph_carbon_checks::Case>& cases)
}
