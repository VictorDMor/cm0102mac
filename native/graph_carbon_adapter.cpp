#include "graph_carbon_adapter.hpp"
#include "graph_carbon.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace cm::aot {
// Set while a checker child runs original bodies, and for the whole run with
// CM_GRAPH_CARBON_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool graph_carbon_declining = std::getenv("CM_GRAPH_CARBON_ORIGINAL") != nullptr;
void (*graph_carbon_direct_call_probe)(State&, std::uint32_t) = nullptr;
namespace {
using guest::Cpu;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, R4 = RoutineChecker::result_r4, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry, end;
    unsigned results;
    void (*hook)(Cpu&);
    bool production;  // GRAPH_CARBON_ROUTINE; a GRAPH_CARBON_DRAFT runs in check tools only
};
#define GRAPH_CARBON_ROUTINE(entry, end, results, digest, ...) Routine{entry, end, results, __VA_ARGS__, true},
#define GRAPH_CARBON_DRAFT(entry, end, results, digest, ...) Routine{entry, end, results, __VA_ARGS__, false},
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "graph_carbon_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef GRAPH_CARBON_DRAFT
#undef GRAPH_CARBON_ROUTINE
constexpr auto routines = sorted_routines();
static_assert(std::ranges::adjacent_find(routines, {}, &Routine::entry) == routines.end(), "a routine is listed twice");

// CM_GRAPH_CARBON_ONLY=0xENTRY,0xENTRY,...: only these routed entries run their C++ (a replay that isolates one block); the rest
// run generated. Unset: every routed entry.
bool selected(std::uint32_t entry) {
    static const auto only = [] {
        std::vector<std::uint32_t> entries;
        if (const char* text = std::getenv("CM_GRAPH_CARBON_ONLY"))
            for (const char* at = text; *at;) {
                char* end = nullptr;
                const auto value = std::strtoul(at, &end, 16);
                if (end == at) break;
                entries.push_back(std::uint32_t(value));
                at = *end ? end + 1 : end;
            }
        return entries;
    }();
    return only.empty() || std::ranges::find(only, entry) != only.end();
}
const Routine* listed(std::uint32_t entry) {
    const auto found = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    return found != routines.end() && found->entry == entry ? &*found : nullptr;
}
void run_rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    listed(entry)->hook(c);
    c.finish();
}
void run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, graph_carbon_declining); }
// Every routine is native inside (CLAUDE.md's per-reader contract): a check compares the results, the
// non-volatile registers, FPSCR, the heap and memory outside the dead stack (routine_check.hpp).
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

bool graph_carbon_lists(std::uint32_t entry) { return listed(entry) != nullptr; }
bool graph_carbon_code(std::uint32_t address) {
    return std::ranges::any_of(routines, [&](const Routine& r) { return address >= r.entry && address < r.end; });
}
unsigned graph_carbon_results(std::uint32_t entry) {
    const auto* routine = listed(entry);
    return routine ? routine->results : 0;
}
std::vector<std::uint32_t> graph_carbon_production_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) if (routine.production) entries.push_back(routine.entry);
    return entries;
}
std::vector<std::uint32_t> graph_carbon_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) entries.push_back(routine.entry);
    return entries;
}

bool graph_carbon_dispatch_probe(State& s) {
    if (graph_carbon_declining || !selected(s.pc)) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

// The checker's leaver check (graph_carbon_checks.cpp): called for the original's entry into a leaver (the hook is reached at every
// entry, through the dispatcher or from the same page, and for a call from another page twice: the checker de-duplicates).
void (*graph_carbon_leaver_probe)(State&, std::uint32_t) = nullptr;
bool try_modern_graph_carbon(State& s, std::uint32_t entry) {
    if (graph_carbon_leaver_probe && graph_carbon_declining) [[unlikely]] graph_carbon_leaver_probe(s, entry);
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    if (graph_carbon_declining || !selected(entry) || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto* routine = listed(entry);
    // A draft outside a checker, or an entry not written yet: the generated body runs.
    if (!routine || (!routine_checker && !routine->production)) return false;
    return run(s, *routine);
}
void graph_carbon_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
// A checker runs the rewrite outside any dispatcher; the production dispatcher a longjmp's transfer is addressed to is stood in for by a
// scope (a routine that took a setjmp catches its own longjmp, and any other transfer leaves the run).
void graph_carbon_run_rewrite(State& s, std::uint32_t entry) {
    GuestDispatchScope scope(s);
    run_rewrite(s, entry);
}
}
