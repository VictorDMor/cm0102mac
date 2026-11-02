#include "news_screens_adapter.hpp"
#include "news_screens.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <vector>

namespace cm::aot {
// Set while a checker child runs original bodies, and for the whole run with
// CM_NEWS_SCREENS_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool news_screens_declining = std::getenv("CM_NEWS_SCREENS_ORIGINAL") != nullptr;
namespace {
using guest::Cpu;
constexpr unsigned R3 = RoutineChecker::result_r3;
struct Routine {
    std::uint32_t entry;
    unsigned results;
    void (*hook)(Cpu&);
};
#define NEWS_SCREENS_ROUTINE(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__},
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "news_screens_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef NEWS_SCREENS_ROUTINE
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
void run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, news_screens_declining); }
bool run(State& s, const Routine& routine) {
    if (routine_checker) routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    Cpu c(s);
    routine.hook(c);
    c.finish();
    return true;
}
}

bool news_screens_lists(std::uint32_t entry) { return listed(entry) != nullptr; }

std::vector<std::uint32_t> news_screens_production_entries() {
    std::vector<std::uint32_t> entries;
    for (const auto& routine : routines) entries.push_back(routine.entry);
    return entries;
}

bool news_screens_dispatch_probe(State& s) {
    if (news_screens_declining) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool try_modern_news_screens(State& s, std::uint32_t entry) {
    if (news_screens_declining) return false;
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    if (routine_checker && !routine_checker->wants(entry)) return false;
    const auto* routine = listed(entry);
    return routine && run(s, *routine);
}

void news_screens_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void news_screens_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
}
