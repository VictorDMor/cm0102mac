#include "transfer_manager_native_adapter.hpp"
#include "transfer_manager_native.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iterator>
#include <stdexcept>

namespace cm::aot {
// Set while a checker child runs original bodies, and for the whole run with
// CM_TRANSFER_MANAGER_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool transfer_manager_native_declining = std::getenv("CM_TRANSFER_MANAGER_ORIGINAL") != nullptr;
namespace {
using guest::Cpu;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry, end;
    unsigned results;
    void (*hook)(Cpu&);
    bool production;  // TRANSFER_MANAGER_NATIVE; a TRANSFER_MANAGER_NATIVE_DRAFT runs in check tools only
};
#define TRANSFER_MANAGER_NATIVE(entry, end, results, digest, ...) Routine{entry, end, results, __VA_ARGS__, true},
#define TRANSFER_MANAGER_NATIVE_DRAFT(entry, end, results, digest, ...) Routine{entry, end, results, __VA_ARGS__, false},
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "transfer_manager_native_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef TRANSFER_MANAGER_NATIVE_DRAFT
#undef TRANSFER_MANAGER_NATIVE
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
void run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, transfer_manager_native_declining); }
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

bool transfer_manager_native_lists(std::uint32_t entry) { return listed(entry) != nullptr; }
bool transfer_manager_native_code(std::uint32_t address) {
    const auto after = std::ranges::upper_bound(routines, address, {}, &Routine::entry);
    return after != routines.begin() && address < std::prev(after)->end;
}
unsigned transfer_manager_native_results(std::uint32_t entry) {
    const auto* routine = listed(entry);
    return routine ? routine->results : 0;
}
std::vector<std::uint32_t> transfer_manager_native_production_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) if (routine.production) entries.push_back(routine.entry);
    return entries;
}
std::vector<std::uint32_t> transfer_manager_native_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) entries.push_back(routine.entry);
    return entries;
}

bool transfer_manager_native_dispatch_probe(State& s) {
    if (transfer_manager_native_declining) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool try_modern_transfer_manager_native(State& s, std::uint32_t entry) {
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    if (transfer_manager_native_declining || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto* routine = listed(entry);
    // A draft outside a checker, or an entry not written yet: the generated body runs.
    if (!routine || (!routine_checker && !routine->production)) return false;
    return run(s, *routine);
}
namespace transfer_manager_native {
void call(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    const bool through_hook = transfer_manager_native_declining || (routine_checker && (!routine_check_child() || !routine_checker->wants(entry)));
    const auto* routine = through_hook ? nullptr : listed(entry);
    if (!routine || (!routine->production && !routine_checker)) { c.call(entry, back); return; }
    c.link(back);
    routine->hook(c);
}
}
void transfer_manager_native_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void transfer_manager_native_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
}
