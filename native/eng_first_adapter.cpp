#include "eng_first_adapter.hpp"
#include "eng_first.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
// Set while a checker child runs original bodies, and for the whole run with
// CM_ENG_FIRST_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool declining = std::getenv("CM_ENG_FIRST_ORIGINAL") != nullptr;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry;
    unsigned results;
    void (*hook)(Cpu&);
};
#define ENG_FIRST_ROUTINE(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__},
#define ENG_FIRST_DRAFT ENG_FIRST_ROUTINE
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "eng_first_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef ENG_FIRST_DRAFT
#undef ENG_FIRST_ROUTINE
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
bool run(State& s, const Routine& routine) {
    if (routine_checker) routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    Cpu c(s);
    routine.hook(c);
    c.finish();
    return true;
}
}

bool eng_first_lists(std::uint32_t entry) { return listed(entry) != nullptr; }

bool eng_first_dispatch_probe(State& s) {
    if (declining) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool try_modern_eng_first(State& s, std::uint32_t entry) {
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto* routine = listed(entry);
    if (!routine) throw std::runtime_error("Unknown ENG_FIRST.CPP routine " + hex_address(entry));
    return run(s, *routine);
}
void eng_first_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void eng_first_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
namespace eng_first {
void call(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    const bool through_hook = declining || (routine_checker &&
        (!routine_check_child() || !routine_checker->wants(entry)));
    const auto* routine = through_hook ? nullptr : listed(entry);
    if (!routine) { c.call(entry, back); return; }
    c.link(back);
    routine->hook(c);
}
}
}
