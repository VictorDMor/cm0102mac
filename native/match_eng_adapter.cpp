#include "match_eng_adapter.hpp"
#include "match_eng_blocks.hpp"
#include "match_eng_leaves.hpp"
#include "match_pl_adapter.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
// Set while a checker child runs original bodies, and for the whole run with
// CM_MATCH_ENG_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool match_eng_declining = std::getenv("CM_MATCH_ENG_ORIGINAL") != nullptr;
void (*match_eng_call_observer)(State&, std::uint32_t, unsigned) = nullptr;
namespace {
using guest::Cpu;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, R4 = RoutineChecker::result_r4,
                                    F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry;
    unsigned results;
    void (*hook)(Cpu&);
    bool production;
};
#define MATCH_ENG_ROUTINE(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__, true},
#define MATCH_ENG_DRAFT(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__, false},
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "match_eng_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef MATCH_ENG_DRAFT
#undef MATCH_ENG_ROUTINE
constexpr auto routines = sorted_routines();
static_assert(std::ranges::adjacent_find(routines, {}, &Routine::entry) == routines.end(), "a routine is listed twice");
// Every entry is 16-byte aligned (tools/match_eng_audit.py), and this routes the
// engine's hottest calls: a table of the module's slots, not a search.
constexpr std::uint32_t first = 0x101d16a0, last = 0x101fef60;
constexpr auto slots = [] {
    std::array<std::int16_t, (last - first) / 16> table{};
    table.fill(-1);
    for (std::size_t n = 0; n < routines.size(); ++n) table[(routines[n].entry - first) / 16] = std::int16_t(n);
    return table;
}();

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
void run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, match_eng_declining); }
bool run(State& s, const Routine& routine) {
    if (routine_checker) [[unlikely]] {
        if (match_eng_call_observer && !routine_check_child()) match_eng_call_observer(s, routine.entry, routine.results);
        routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    }
    Cpu c(s);
    routine.hook(c);
    c.finish();
    return true;
}
}

bool match_eng_lists(std::uint32_t entry) { return listed(entry) != nullptr; }
std::vector<std::uint32_t> match_eng_production_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) if (routine.production) entries.push_back(routine.entry);
    return entries;
}

bool match_eng_dispatch_probe(State& s) {
    if (match_eng_declining) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool try_modern_match_eng(State& s, std::uint32_t entry) {
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    if (match_eng_declining || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto* routine = listed(entry);
    if (!routine || (!routine->production && !routine_checker)) return false;
    return run(s, *routine);
}
namespace match_eng {
void call(guest::Cpu& c, std::uint32_t entry, std::uint32_t back) {
    // A checker's parent goes through the hook, so nested calls are checked too.
    // --only runs the routines left out as their originals here too.
    const bool through_hook = match_eng_declining || (routine_checker && (!routine_check_child() || !routine_checker->wants(entry)));
    const auto* routine = through_hook ? nullptr : listed(entry);
    if (!routine || (!routine->production && !routine_checker)) { c.call(entry, back); return; }
    c.link(back);
    routine->hook(c);
}
}
void match_eng_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void match_eng_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
void match_family_run_original(State& s, std::uint32_t entry) {
    struct Restore { bool value; ~Restore() { match_eng_declining = value; } } restore{match_eng_declining};
    match_eng_declining = true;
    match_pl_run_original(s, entry);
}
}
