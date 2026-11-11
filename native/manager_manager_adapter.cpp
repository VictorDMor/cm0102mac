#include "manager_manager_adapter.hpp"
#include "manager_manager_blocks.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdexcept>

namespace cm::aot {
// Set while a checker child runs original bodies, and for the whole run with
// CM_MANAGER_MANAGER_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool manager_manager_declining = std::getenv("CM_MANAGER_MANAGER_ORIGINAL") != nullptr;
void (*manager_manager_call_observer)(State&, std::uint32_t, unsigned) = nullptr;
namespace {
using guest::Cpu;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry, end;
    unsigned results;
    void (*hook)(Cpu&);
    bool production;  // MANAGER_MANAGER_ROUTINE: generation hooks it; MANAGER_MANAGER_DRAFT: check tools only
};
#define MANAGER_MANAGER_ROUTINE(entry, end, results, digest, ...) Routine{entry, end, results, __VA_ARGS__, true},
#define MANAGER_MANAGER_DRAFT(entry, end, results, digest, ...) Routine{entry, end, results, __VA_ARGS__, false},
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "manager_manager_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef MANAGER_MANAGER_DRAFT
#undef MANAGER_MANAGER_ROUTINE
constexpr auto routines = sorted_routines();
static_assert(std::ranges::adjacent_find(routines, {}, &Routine::entry) == routines.end(), "a routine is listed twice");
// Every entry is 16-byte aligned (tools/manager_manager_audit.py), and the list
// search is called millions of times a day: a table of slots, not a search.
constexpr std::uint32_t first = 0x10193630, last = 0x101c42f0;
constexpr auto slots = [] {
    std::array<std::int16_t, (last - first + 15) / 16> table{};
    table.fill(-1);
    for (std::size_t n = 0; n < routines.size(); ++n) {
        if (routines[n].entry < first || routines[n].entry >= last || routines[n].entry % 16) throw "not a MANAGER_MANAGER.CPP entry";
        table[(routines[n].entry - first) / 16] = std::int16_t(n);
    }
    return table;
}();

// The entries compared under the native contract (manager_manager_native.inc).
#define MANAGER_MANAGER_NATIVE(entry) entry,
constexpr std::uint32_t native_entries[] = {
#include "manager_manager_native.inc"
    0};
#undef MANAGER_MANAGER_NATIVE
bool native(std::uint32_t entry) { return entry && std::ranges::find(native_entries, entry) != std::end(native_entries); }
// The module's routines whose leftover stack a later reader reads (tools/stack_contract.py module
// MANAGER_MANAGER.CPP, 2026-11-11): their dead stack stays compared; for every other routine it is free.
constexpr std::uint32_t leave_readers[] = {0x10196390, 0x1019d320, 0x101a2eb0, 0x101ad080, 0x101b1620};
bool dead_stack_free(std::uint32_t entry) { return std::ranges::find(leave_readers, entry) == std::end(leave_readers); }
// Sets the checker's comparison for one routine's check and puts it back.
struct NativeContract {
    bool before = routine_check_native_contract, before_stack = routine_check_dead_stack_free;
    explicit NativeContract(std::uint32_t entry) {
        routine_check_native_contract = native(entry);
        routine_check_dead_stack_free = dead_stack_free(entry);
    }
    ~NativeContract() { routine_check_native_contract = before; routine_check_dead_stack_free = before_stack; }
};

const Routine* listed(std::uint32_t entry) {
    const auto offset = entry - first;
    if (offset >= last - first || offset % 16) return nullptr;
    const auto slot = slots[offset / 16];
    return slot < 0 ? nullptr : &routines[std::size_t(slot)];
}
void run_rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    listed(entry)->hook(c);
    c.finish();
}
void run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, manager_manager_declining); }
bool run(State& s, const Routine& routine) {
    if (routine_checker) [[unlikely]] {
        if (manager_manager_call_observer && !routine_check_child()) manager_manager_call_observer(s, routine.entry, routine.results);
        const NativeContract contract(routine.entry);
        routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    }
    Cpu c(s);
    routine.hook(c);
    c.finish();
    return true;
}
}

bool manager_manager_lists(std::uint32_t entry) { return listed(entry) != nullptr; }
bool manager_manager_native(std::uint32_t entry) { return native(entry); }
bool manager_manager_dead_stack_free(std::uint32_t entry) { return dead_stack_free(entry); }
bool manager_manager_native_code(std::uint32_t address) {
    for (const auto& routine : routines)
        if (address >= routine.entry && address < routine.end) return native(routine.entry);
    return false;
}
unsigned manager_manager_results(std::uint32_t entry) {
    const auto* routine = listed(entry);
    return routine ? routine->results : 0;
}
std::vector<std::uint32_t> manager_manager_production_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) if (routine.production) entries.push_back(routine.entry);
    return entries;
}
std::vector<std::uint32_t> manager_manager_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) entries.push_back(routine.entry);
    return entries;
}

bool manager_manager_dispatch_probe(State& s) {
    if (manager_manager_declining) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool try_modern_manager_manager(State& s, std::uint32_t entry) {
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    if (manager_manager_declining || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto* routine = listed(entry);
    // A draft outside a checker: the generated body runs.
    if (!routine || (!routine_checker && !routine->production)) return false;
    return run(s, *routine);
}
namespace manager_manager {
void call(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    const bool through_hook = manager_manager_declining || (routine_checker && (!routine_check_child() || !routine_checker->wants(entry)));
    const auto* routine = through_hook ? nullptr : listed(entry);
    if (!routine || (!routine->production && !routine_checker)) { c.call(entry, back); return; }
    c.link(back);
    routine->hook(c);
}
}
void manager_manager_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void manager_manager_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
}
