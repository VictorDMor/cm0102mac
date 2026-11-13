#include "game_config_adapter.hpp"
#include "game_config.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
// Set while a checker child runs original bodies, and for the whole run with
// CM_GAME_CONFIG_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool declining = std::getenv("CM_GAME_CONFIG_ORIGINAL") != nullptr;
constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3;
struct Routine {
    std::uint32_t entry;
    unsigned results;
    void (*hook)(Cpu&);
};
#define GAME_CONFIG_ROUTINE(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__},
#define GAME_CONFIG_DRAFT GAME_CONFIG_ROUTINE
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "game_config_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef GAME_CONFIG_DRAFT
#undef GAME_CONFIG_ROUTINE
constexpr auto routines = sorted_routines();
static_assert(std::ranges::adjacent_find(routines, {}, &Routine::entry) == routines.end(), "a routine is listed twice");

const Routine* listed(std::uint32_t entry) {
    const auto found = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    return found != routines.end() && found->entry == entry ? &*found : nullptr;
}
void run_rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    listed(entry)->hook(c);
    c.finish();
}
void run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, declining); }
// Every routine is native inside (CLAUDE.md's per-reader contract): a check compares the results, the
// non-volatile registers, the heap and memory outside the dead stack (routine_check.hpp).
struct NativeContract {
    bool before = routine_check_native_contract;
    NativeContract() { routine_check_native_contract = true; }
    ~NativeContract() { routine_check_native_contract = before; }
};
bool run(State& s, const Routine& routine) {
    if (routine_checker) [[unlikely]] {
        const NativeContract contract;
        routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    }
    Cpu c(s);
    routine.hook(c);
    c.finish();
    return true;
}
}

bool game_config_lists(std::uint32_t entry) { return listed(entry) != nullptr; }
std::vector<std::uint32_t> game_config_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) entries.push_back(routine.entry);
    return entries;
}
unsigned game_config_results(std::uint32_t entry) {
    const auto* routine = listed(entry);
    return routine ? routine->results : 0;
}

bool game_config_dispatch_probe(State& s) {
    if (declining) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool try_modern_game_config(State& s, std::uint32_t entry) {
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto* routine = listed(entry);
    if (!routine) throw std::runtime_error("Unknown GAME_CONFIG.CPP routine " + hex_address(entry));
    return run(s, *routine);
}
void game_config_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void game_config_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
}
