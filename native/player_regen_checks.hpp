#pragma once
// The PLAYER_REGEN.CPP checker's case lists. A block adds a `player_regen_checks_<block>.cpp`
// with PLAYER_REGEN_CASES blocks; the checker runs each case's call as the original and as
// the C++ in forked children from a real state (a career with --world), then the same call with
// mutated inputs (values the original actually read), and compares all the game can observe.
// Pointers and states come from the real game, never fabricated.
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot::player_regen_checks {
inline constexpr std::uint32_t module_start = 0x102d8230, module_end = 0x102eb6f8;
inline constexpr std::size_t module_routines = 47;
// A mapped area outside the game's memory for rows a case builds.
inline constexpr std::uint32_t scratch_base = 0x60000000, scratch_size = 0x100000;
// The fake archive's state (Case::Archive): +0 calls, +4 opens, +8 writes, +0xc reads, +0x10 closes, +0x14 position, +0x18 length, +0x1c mode,
// +0x20..+0x33 the open call's r3-r7, the call log at +0x100 (16 bytes a call), the file's bytes at archive_data.
inline constexpr std::uint32_t archive_state = scratch_base + 0x80000, archive_data = archive_state + 0x2000, archive_data_size = 0x3c000,
                               archive_handle = archive_state + 0x40;
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
    // An in-memory stand-in for the zip archive's entry functions (ZIPDIR.CPP: 0x107475b0 open, 0x10747cc0 write, 0x10747a80 read,
    // 0x10747900 close), answered on both sides when `enabled`: the file's bytes, its position and length, the counts of each call and a
    // log of every call (kind, size, count, result) live in scratch memory (archive_state), where the comparison of memory sees them, so the
    // bytes a routine writes, and the calls it makes in order, are compared. A read case gives the file's bytes and length through `memory`
    // (archive_data, archive_state + 0x18). Failures are injected by call number (from 1, per kind): `fail_*` returns 0 items, `short_*` one
    // item fewer than asked; `fail_open` makes the open return 0; `keep_handle` leaves the handle variable alone at close (else it is zeroed).
    struct Archive {
        bool enabled = false, fail_open = false, keep_handle = false;
        unsigned fail_write = 0, short_write = 0, fail_read = 0, short_read = 0;
    };
    Archive archive = {};
    // An expectation checked on the final state of both runs (not of the mutated ones): false is a failure of the whole check. For what
    // the two runs can agree on and still get wrong (the bytes of a file against an independent serialization, a round trip).
    std::function<bool(State&)> verify = nullptr;
    // Start with FPSCR cleared (the snapshot's has the sticky inexact bit set, which hides a missing inexact result).
    bool clear_fpscr = false;
    // The system's file lookup (FSMakeFSSpec, an import the generated code and the C++ both reach through the glue 0x1077fee0): the answers of
    // its calls in order (the OSErr in r3); calls beyond the list find the file (0); an empty list leaves the import unanswered. Counted in scratch memory (fs_lookup_state), so each run
    // starts from the case's count of 0.
    std::vector<std::uint32_t> lookups = {};
};
inline constexpr std::uint32_t fs_lookup_state = scratch_base + 0x7f000;
struct Scenario {
    State& s;
    std::uint32_t r32(std::uint32_t at) const { return std::uint32_t(s.memory.read<4>(at)); }
    std::uint32_t u16(std::uint32_t at) const { return std::uint32_t(s.memory.read<2>(at)); }
    std::uint32_t u8(std::uint32_t at) const { return std::uint32_t(s.memory.read<1>(at)); }
    std::uint32_t toc(int slot) const { return r32(0x108bba90 + std::uint32_t(slot)); }
    std::uint32_t global(int slot) const { return r32(toc(slot)); }
    // The world's records: club (0x248) and competition (0x6c) tables, and the people
    // (0x70 each, `people()` of them; database_init_detail.hpp's person).
    std::uint32_t club(unsigned index) const { return global(-0x7be8) + index * 0x248; }
    std::uint32_t competition(unsigned index) const { return global(-0x7bbc) + index * 0x6c; }
    std::uint32_t person(unsigned index) const { return global(-0x7be0) + index * 0x70; }
    std::uint32_t people() const { return global(-0x7be4); }
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
using CaseList = std::function<void(const Scenario&, std::vector<Case>&)>;
struct CaseBlock {
    CaseBlock(const char* name, CaseList list);
};
#define PLAYER_REGEN_CASES_JOIN2(a, b) a##b
#define PLAYER_REGEN_CASES_JOIN(a, b) PLAYER_REGEN_CASES_JOIN2(a, b)
// PLAYER_REGEN_CASES("ratings") { if (!scenario.match) cases.push_back({.entry = 0x..., .registers = {{3, ...}}}); }
#define PLAYER_REGEN_CASES(name) \
    static void PLAYER_REGEN_CASES_JOIN(player_regen_cases_, __LINE__)(const ::cm::aot::player_regen_checks::Scenario&, std::vector<::cm::aot::player_regen_checks::Case>&); \
    static ::cm::aot::player_regen_checks::CaseBlock PLAYER_REGEN_CASES_JOIN(player_regen_block_, __LINE__){name, PLAYER_REGEN_CASES_JOIN(player_regen_cases_, __LINE__)}; \
    static void PLAYER_REGEN_CASES_JOIN(player_regen_cases_, __LINE__)([[maybe_unused]] const ::cm::aot::player_regen_checks::Scenario& scenario, [[maybe_unused]] std::vector<::cm::aot::player_regen_checks::Case>& cases)
}
