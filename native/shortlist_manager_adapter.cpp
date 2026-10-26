#include "shortlist_manager_adapter.hpp"
#include "shortlist_manager_detail.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdexcept>

namespace cm::aot {
namespace {
using guest::Cpu;
// Set while a checker child runs original bodies, and for the whole run with
// CM_SHORTLIST_MANAGER_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool declining = std::getenv("CM_SHORTLIST_MANAGER_ORIGINAL") != nullptr;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry;
    bool verified;  // SHORTLIST_MANAGER_ROUTINE (the game runs it), not a draft
    unsigned results;
    void (*hook)(Cpu&);
};
#define SHORTLIST_MANAGER_ROUTINE(entry, end, results, digest, ...) Routine{entry, true, results, __VA_ARGS__},
#define SHORTLIST_MANAGER_DRAFT(entry, end, results, digest, ...) Routine{entry, false, results, __VA_ARGS__},
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "shortlist_manager_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef SHORTLIST_MANAGER_DRAFT
#undef SHORTLIST_MANAGER_ROUTINE
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
// Whether this run takes the routine's C++: verified ones always (a check
// run with --only runs the others' originals, so each routine is checked at
// every call from the original's own callers), drafts only under a checker.
bool runs(const Routine* routine) {
    if (declining || !routine) return false;
    return routine_checker ? routine_checker->wants(routine->entry) : routine->verified;
}
}

bool shortlist_manager_lists(std::uint32_t entry) { return listed(entry) != nullptr; }
unsigned shortlist_manager_results(std::uint32_t entry) {
    const auto* routine = listed(entry);
    return routine ? routine->results : 0;
}
ShortlistManagerCallHook* shortlist_manager_call_hook = nullptr;

// Program::call into another routine of the module (shortlist_manager_detail.hpp):
// a verified routine (or, in check tools, a draft) runs its C++ in place, so
// a checked routine's callees run as C++ inside its check.
bool shortlist_manager::detail::call_rewritten(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    if (auto* hook = shortlist_manager_call_hook) [[unlikely]] {
        const auto stack = c.sp();
        c.link(back);
        c.s.pc = entry;
        if (!hook->call(c.s)) return false;  // the caller makes the guest call
        if (c.s.pc != back || c.sp() != stack)
            throw std::logic_error("Hooked call returned elsewhere: " + hex_address(entry));
        return true;
    }
    const auto* routine = listed(entry);
    if (declining || !routine || !(routine->verified || routine_checker)) return false;
    c.link(back);
    routine->hook(c);
    c.s.pc = back;
    return true;
}

bool shortlist_manager_dispatch_probe(State& s) {
    const auto* routine = listed(s.pc);
    return runs(routine) && run(s, *routine);
}

bool try_modern_shortlist_manager(State& s, std::uint32_t entry) {
    const auto* routine = listed(entry);
    return runs(routine) && run(s, *routine);
}
void shortlist_manager_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void shortlist_manager_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
}
