#include "discipline_adapter.hpp"
#include "discipline_blocks.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdexcept>

namespace cm::aot {
// Set while a checker child runs original bodies, and for the whole run with
// CM_DISCIPLINE_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool discipline_declining = std::getenv("CM_DISCIPLINE_ORIGINAL") != nullptr;
void (*discipline_call_observer)(State&, std::uint32_t, unsigned) = nullptr;
namespace {
using guest::Cpu;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry, end;
    unsigned results;
    void (*hook)(Cpu&);
    bool production;  // DISCIPLINE_ROUTINE: generation hooks it; DISCIPLINE_DRAFT: check tools only
};
#define DISCIPLINE_ROUTINE(entry, end, results, digest, ...) Routine{entry, end, results, __VA_ARGS__, true},
#define DISCIPLINE_DRAFT(entry, end, results, digest, ...) Routine{entry, end, results, __VA_ARGS__, false},
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "discipline_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef DISCIPLINE_DRAFT
#undef DISCIPLINE_ROUTINE
constexpr auto routines = sorted_routines();
static_assert(std::ranges::adjacent_find(routines, {}, &Routine::entry) == routines.end(), "a routine is listed twice");
// Every entry is 16-byte aligned (tools/discipline_audit.py), and the list
// search is called millions of times a day: a table of slots, not a search.
constexpr std::uint32_t first = 0x100de410, last = 0x100e92d0;
constexpr auto slots = [] {
    std::array<std::int16_t, (last - first + 15) / 16> table{};
    table.fill(-1);
    for (std::size_t n = 0; n < routines.size(); ++n) {
        if (routines[n].entry < first || routines[n].entry >= last || routines[n].entry % 16) throw "not a DISCIPLINE.CPP entry";
        table[(routines[n].entry - first) / 16] = std::int16_t(n);
    }
    return table;
}();

// The entries compared under the native contract (discipline_native.inc).
#define DISCIPLINE_NATIVE(entry) entry,
constexpr std::uint32_t native_entries[] = {
#include "discipline_native.inc"
    0};
#undef DISCIPLINE_NATIVE
bool native(std::uint32_t entry) { return entry && std::ranges::find(native_entries, entry) != std::end(native_entries); }
// The module's routines whose leftover stack a later reader reads (tools/stack_contract.py module
// DISCIPLINE.CPP, 2026-11-12: the BlockMoveData reader 0x10bf7bbc). Every entry is native-first, and
// the native contract never compares the dead stack (routine_check.cpp), so this list does not change
// the case checker's comparison: the reader's bytes are checked by the live checks and paired replays
// on the captures that reach them (season-day, same-save-continue), where a ruling body copies them
// into a ban record. The routines on that path keep the original's non-volatile registers at their calls.
constexpr std::uint32_t leave_readers[] = {0x100e0250, 0x100e8780};
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
void run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, discipline_declining); }
bool run(State& s, const Routine& routine) {
    if (routine_checker) [[unlikely]] {
        if (discipline_call_observer && !routine_check_child()) discipline_call_observer(s, routine.entry, routine.results);
        const NativeContract contract(routine.entry);
        routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    }
    Cpu c(s);
    routine.hook(c);
    c.finish();
    return true;
}
}

bool discipline_lists(std::uint32_t entry) { return listed(entry) != nullptr; }
bool discipline_native(std::uint32_t entry) { return native(entry); }
bool discipline_dead_stack_free(std::uint32_t entry) { return dead_stack_free(entry); }
bool discipline_native_code(std::uint32_t address) {
    for (const auto& routine : routines)
        if (address >= routine.entry && address < routine.end) return native(routine.entry);
    return false;
}
unsigned discipline_results(std::uint32_t entry) {
    const auto* routine = listed(entry);
    return routine ? routine->results : 0;
}
std::vector<std::uint32_t> discipline_production_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) if (routine.production) entries.push_back(routine.entry);
    return entries;
}
std::vector<std::uint32_t> discipline_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) entries.push_back(routine.entry);
    return entries;
}

bool discipline_dispatch_probe(State& s) {
    if (discipline_declining) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool try_modern_discipline(State& s, std::uint32_t entry) {
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    if (discipline_declining || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto* routine = listed(entry);
    // A draft outside a checker: the generated body runs.
    if (!routine || (!routine_checker && !routine->production)) return false;
    return run(s, *routine);
}
namespace discipline {
void call(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    const bool through_hook = discipline_declining || (routine_checker && (!routine_check_child() || !routine_checker->wants(entry)));
    const auto* routine = through_hook ? nullptr : listed(entry);
    if (!routine || (!routine->production && !routine_checker)) { c.call(entry, back); return; }
    c.link(back);
    routine->hook(c);
}
}
void discipline_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void discipline_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
}
