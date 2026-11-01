#include "media_adapter.hpp"
#include "media.hpp"
#include "media_blocks.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
// Set while a checker child runs original bodies, and for the whole run with
// CM_MEDIA_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool media_declining = std::getenv("CM_MEDIA_ORIGINAL") != nullptr;
namespace {
using guest::Cpu;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry;
    unsigned results;
    void (*hook)(Cpu&);
    bool production;
};
#define MEDIA_ROUTINE(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__, true},
#define MEDIA_DRAFT(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__, false},
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "media_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef MEDIA_DRAFT
#undef MEDIA_ROUTINE
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
void run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, media_declining); }
bool run(State& s, const Routine& routine) {
    if (routine_checker) routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    Cpu c(s);
    routine.hook(c);
    c.finish();
    return true;
}
}

bool media_lists(std::uint32_t entry) { return listed(entry) != nullptr; }

std::vector<std::uint32_t> media_production_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) if (routine.production) entries.push_back(routine.entry);
    return entries;
}

bool media_dispatch_probe(State& s) {
    if (media_declining) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool try_modern_media(State& s, std::uint32_t entry) {
    if (media_declining) return false;
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    if (routine_checker && !routine_checker->wants(entry)) return false;
    const auto* routine = listed(entry);
    // Not written yet, or a draft outside a checker: the generated body runs.
    if (!routine || (!routine_checker && !routine->production)) return false;
    return run(s, *routine);
}

namespace media {
void call(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    // A checker's parent goes through the hook, so nested calls are checked too.
    // --only runs the routines left out as their originals here too.
    const bool through_hook = media_declining || (routine_checker && (!routine_check_child() || !routine_checker->wants(entry)));
    const auto* routine = through_hook ? nullptr : listed(entry);
    if (!routine || (!routine->production && !routine_checker)) { c.call(entry, back); return; }
    c.link(back);
    routine->hook(c);
}
}

void media_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void media_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
}
