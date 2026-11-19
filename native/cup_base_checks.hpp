#pragma once
// The CUP.CPP checker's case lists. A block adds a `cup_base_checks_<block>.cpp`
// with CUP_CASES blocks; the checker runs each case's call as the original and as
// the C++ in forked children from a real state (a career with --world, a match with
// --match), then the same call with mutated inputs (values the original actually read),
// and compares all the game can observe. Pointers and states come from the real game,
// never fabricated. A case list sees both states in turn and adds the cases that apply
// (scenario.match is 0 on the career).
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot::cup_base_checks {
inline constexpr std::uint32_t module_start = 0x10478f20, module_end = 0x104866f0;
inline constexpr std::size_t module_routines = 47;
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
};
struct Scenario {
    State& s;
    std::uint32_t match;  // the match object on a match snapshot, 0 on the career
    std::uint32_t r32(std::uint32_t at) const { return std::uint32_t(s.memory.read<4>(at)); }
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
    // A match's two sides (0xd8 each at +0x7aac) and their 20 player records.
    std::uint32_t side(unsigned index) const { return match + 0x7aac + index * 0xd8; }
    std::uint32_t player(unsigned side, unsigned index) const { return match + 0x311c + side * 0x2350 + index * 0x1c4; }
};
using CaseList = std::function<void(const Scenario&, std::vector<Case>&)>;
struct CaseBlock {
    CaseBlock(const char* name, CaseList list);
};
#define CUP_CASES_JOIN2(a, b) a##b
#define CUP_CASES_JOIN(a, b) CUP_CASES_JOIN2(a, b)
// CUP_CASES("ratings") { if (!scenario.match) cases.push_back({.entry = 0x..., .registers = {{3, ...}}}); }
#define CUP_CASES(name) \
    static void CUP_CASES_JOIN(cup_cases_, __LINE__)(const ::cm::aot::cup_base_checks::Scenario&, std::vector<::cm::aot::cup_base_checks::Case>&); \
    static ::cm::aot::cup_base_checks::CaseBlock CUP_CASES_JOIN(cup_block_, __LINE__){name, CUP_CASES_JOIN(cup_cases_, __LINE__)}; \
    static void CUP_CASES_JOIN(cup_cases_, __LINE__)([[maybe_unused]] const ::cm::aot::cup_base_checks::Scenario& scenario, [[maybe_unused]] std::vector<::cm::aot::cup_base_checks::Case>& cases)
}
