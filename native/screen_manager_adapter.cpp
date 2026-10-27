#include "screen_manager_adapter.hpp"
#include "screen_manager.hpp"
#include "screen_manager_packets.hpp"
#include "screen_manager_sound.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
// Set while a checker child runs original bodies, and for the whole run with
// CM_SCREEN_MANAGER_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool declining = std::getenv("CM_SCREEN_MANAGER_ORIGINAL") != nullptr;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry;
    unsigned results;
    void (*hook)(Cpu&);
};
#define SCREEN_MANAGER_ROUTINE(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__},
#define SCREEN_MANAGER_DRAFT SCREEN_MANAGER_ROUTINE
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "screen_manager_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef SCREEN_MANAGER_DRAFT
#undef SCREEN_MANAGER_ROUTINE
constexpr auto routines = sorted_routines();
static_assert(std::ranges::adjacent_find(routines, {}, &Routine::entry) == routines.end(), "a routine is listed twice");

// Every audited entry is 16-byte aligned. A bitmap and its prefix counts
// select the sorted routine directly, without searching or a sparse byte table.
constexpr std::uint32_t entry_alignment = 16;
constexpr auto first_entry = routines.front().entry;
constexpr auto last_entry = routines.back().entry;
consteval auto entry_index() {
    static_assert(routines.size() <= 255);
    struct Index {
        std::array<std::uint64_t, (last_entry - first_entry) / entry_alignment / 64 + 1> bits{};
        std::array<std::uint8_t, (last_entry - first_entry) / entry_alignment / 64 + 1> first{};
    } index;
    for (std::size_t n = 0; n < routines.size(); ++n) {
        const auto offset = routines[n].entry - first_entry;
        const auto slot = offset / entry_alignment;
        const auto bit = std::uint64_t(1) << (slot % 64);
        if (routines[n].entry % entry_alignment || (index.bits[slot / 64] & bit))
            throw "SCRMAN entry is unaligned or duplicated";
        index.bits[slot / 64] |= bit;
    }
    unsigned first = 0;
    for (std::size_t n = 0; n < index.bits.size(); ++n) {
        index.first[n] = std::uint8_t(first);
        first += std::popcount(index.bits[n]);
    }
    return index;
}
constexpr auto routine_index = entry_index();

const Routine* listed(std::uint32_t entry) {
    const auto offset = entry - first_entry;
    if (offset > last_entry - first_entry || offset % entry_alignment) return nullptr;
    const auto slot = offset / entry_alignment;
    const auto bit = std::uint64_t(1) << (slot % 64);
    const auto bits = routine_index.bits[slot / 64];
    if (!(bits & bit)) return nullptr;
    const auto index = routine_index.first[slot / 64] + std::popcount(bits & (bit - 1));
    return &routines[index];
}
void run_rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    listed(entry)->hook(c);
    c.finish();
}
void run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, declining); }
bool run(State& s, const Routine& routine) {
    if (routine_checker) routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    const bool initializing = routine.entry == 0x10744640;
    if (initializing) ++screen_manager_diagnostics(s).initialization_calls;
    Cpu c(s);
    routine.hook(c);
    c.finish();
    if (initializing) ++screen_manager_diagnostics(s).initialization_returns;
    return true;
}
}

bool screen_manager_lists(std::uint32_t entry) { return listed(entry) != nullptr; }

bool screen_manager_dispatch_probe(State& s) {
    if (declining) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool try_modern_screen_manager(State& s, std::uint32_t entry) {
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto* routine = listed(entry);
    if (!routine) throw std::runtime_error("Unknown SCREEN_MANAGER.CPP routine " + hex_address(entry));
    return run(s, *routine);
}
void screen_manager_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void screen_manager_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
}
