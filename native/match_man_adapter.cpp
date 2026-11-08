#include "match_man_adapter.hpp"
#include "match_man_blocks.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdexcept>

namespace cm::aot {
// Set while a checker child runs original bodies, and for the whole run with
// CM_MATCH_MAN_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool match_man_declining = std::getenv("CM_MATCH_MAN_ORIGINAL") != nullptr;
void (*match_man_call_observer)(State&, std::uint32_t, unsigned) = nullptr;
namespace {
using guest::Cpu;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry;
    unsigned results;
    void (*hook)(Cpu&);
    bool production;  // MATCH_MAN_ROUTINE: generation hooks it; MATCH_MAN_DRAFT: check tools only
};
#define MATCH_MAN_ROUTINE(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__, true},
#define MATCH_MAN_DRAFT(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__, false},
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "match_man_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef MATCH_MAN_DRAFT
#undef MATCH_MAN_ROUTINE
constexpr auto routines = sorted_routines();
static_assert(std::ranges::adjacent_find(routines, {}, &Routine::entry) == routines.end(), "a routine is listed twice");
// Every entry is 16-byte aligned (tools/match_man_audit.py), and the rating
// routines are called millions of times a day: a table of slots, not a search.
constexpr std::uint32_t first = 0x102065e0, last = 0x1021b010;
constexpr auto slots = [] {
    std::array<std::int16_t, (last - first + 15) / 16> table{};
    table.fill(-1);
    for (std::size_t n = 0; n < routines.size(); ++n) {
        if (routines[n].entry < first || routines[n].entry >= last || routines[n].entry % 16) throw "not a MATCH_MAN.CPP entry";
        table[(routines[n].entry - first) / 16] = std::int16_t(n);
    }
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
void run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, match_man_declining); }
bool run(State& s, const Routine& routine) {
    if (routine_checker) [[unlikely]] {
        if (match_man_call_observer && !routine_check_child()) match_man_call_observer(s, routine.entry, routine.results);
        routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    }
    Cpu c(s);
    routine.hook(c);
    c.finish();
    return true;
}
}

bool match_man_lists(std::uint32_t entry) { return listed(entry) != nullptr; }
std::vector<std::uint32_t> match_man_production_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) if (routine.production) entries.push_back(routine.entry);
    return entries;
}
std::vector<std::uint32_t> match_man_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) entries.push_back(routine.entry);
    return entries;
}

bool match_man_dispatch_probe(State& s) {
    if (match_man_declining) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool try_modern_match_man(State& s, std::uint32_t entry) {
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    if (match_man_declining || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto* routine = listed(entry);
    // A draft outside a checker: the generated body runs.
    if (!routine || (!routine_checker && !routine->production)) return false;
    return run(s, *routine);
}
namespace match_man {
void call(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    const bool through_hook = match_man_declining || (routine_checker && (!routine_check_child() || !routine_checker->wants(entry)));
    const auto* routine = through_hook ? nullptr : listed(entry);
    if (!routine || (!routine->production && !routine_checker)) { c.call(entry, back); return; }
    c.link(back);
    routine->hook(c);
}
}
void match_man_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void match_man_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
}
