#include "tactics_adapter.hpp"
#include "formation.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3,
                                    R3_R4 = R3 | RoutineChecker::result_r4, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry;
    unsigned results;
    void (*hook)(Cpu&);
};
#define TACTICS_ROUTINE(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__},
#define TACTICS_DRAFT TACTICS_ROUTINE
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "tactics_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef TACTICS_DRAFT
#undef TACTICS_ROUTINE
constexpr auto routines = sorted_routines();
static_assert(std::ranges::adjacent_find(routines, {}, &Routine::entry) == routines.end(), "a routine is listed twice");

// Set while a checker child runs original bodies, and for the whole run with
// CM_TACTICS_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool declining = std::getenv("CM_TACTICS_ORIGINAL") != nullptr;

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
bool run(State& s, const Routine& routine) {
    if (routine_checker) routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    Cpu c(s);
    routine.hook(c);
    c.finish();
    return true;
}
}

bool tactics_dispatch_probe(State& s) {
    if (declining) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool try_modern_tactics(State& s, std::uint32_t entry) {
    if (declining) return false;
    const auto* routine = listed(entry);
    if (!routine) throw std::runtime_error("Unknown tactics routine " + hex_address(entry));
    return run(s, *routine);
}
}
