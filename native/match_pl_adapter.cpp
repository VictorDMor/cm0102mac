#include "match_pl_adapter.hpp"
#include "match_pl.hpp"
#include "match_pl_goal_line.hpp"
#include "match_pl_influence.hpp"
#include "match_pl_obstruction.hpp"
#include "match_pl_player_response.hpp"
#include "match_pl_pressure.hpp"
#include "match_pl_shot_event.hpp"
#include "match_turbo.hpp"
#include "routine_check.hpp"
#include "ball_action.hpp"
#include "player_movement.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
// Set while a checker child runs original bodies, and for the whole run with
// CM_MATCH_PL_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
thread_local const State* original_shot_state = nullptr;
bool declining = std::getenv("CM_MATCH_PL_ORIGINAL") != nullptr;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry;
    unsigned results;
    void (*hook)(Cpu&);
    bool production;
};
#define MATCH_PL_ROUTINE(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__, true},
#define MATCH_PL_DRAFT(entry, end, results, digest, ...) Routine{entry, results, __VA_ARGS__, false},
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "match_pl_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef MATCH_PL_DRAFT
#undef MATCH_PL_ROUTINE
constexpr auto routines = sorted_routines();
static_assert(std::ranges::adjacent_find(routines, {}, &Routine::entry) == routines.end(), "a routine is listed twice");

// The aligned guest entries fit in a byte-indexed sparse table. Zero is
// a hole; the other slots are one-based indices in the verified inventory.
constexpr auto first_entry = routines.front().entry;
constexpr auto last_entry = routines.back().entry;
static_assert(routines.size() <= UINT8_MAX);
static_assert(std::ranges::all_of(routines, [](const auto& routine) { return routine.entry % 16 == 0; }));
constexpr auto slots = [] {
    std::array<std::uint8_t, (last_entry - first_entry) / 16 + 1> table{};
    for (std::size_t i = 0; i < routines.size(); ++i)
        table[(routines[i].entry - first_entry) / 16] = std::uint8_t(i + 1);
    return table;
}();

const Routine* listed(std::uint32_t entry) {
    const auto offset = entry - first_entry;
    if (offset > last_entry - first_entry || offset % 16) return nullptr;
    const auto slot = slots[offset / 16];
    return slot ? &routines[slot - 1] : nullptr;
}
void run_routine(State& s, const Routine& routine) {
    Cpu c(s);
    routine.hook(c);
    // These roots own their continuations, including live table branches.
    if (routine.entry != player_movement::entry && routine.entry != ball_action::entry &&
        routine.entry != match_pl::shot_event_entry) c.finish();
}
void run_rewrite(State& s, std::uint32_t entry) { run_routine(s, *listed(entry)); }
void run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, declining); }
bool run(State& s, const Routine& routine) {
    if (routine_checker) routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    run_routine(s, routine);
    return true;
}
}

bool match_pl_lists(std::uint32_t entry) { return listed(entry) != nullptr; }

bool match_pl_dispatch_probe(State& s) {
    if (declining || (s.pc == match_pl::shot_event_entry && !match_pl::shot_event_uses_cpp(s))) return false;
    const auto* routine = listed(s.pc);
    return routine && (!routine_checker || routine_checker->wants(s.pc)) && run(s, *routine);
}

bool match_pl_route_allowed(std::uint32_t entry) {
    // A check run with --only runs the other routines' originals, so each
    // routine is checked at every call from the original's own callers.
    return !declining && (!routine_checker || routine_checker->wants(entry));
}

bool try_modern_match_pl(State& s, std::uint32_t entry) {
    if (entry == match_pl::shot_event_entry ? !match_pl::shot_event_uses_cpp(s) : !match_pl_route_allowed(entry)) return false;
    const auto* routine = listed(entry);
    if (routine && !routine->production && !routine_checker) return false;
    if (!routine) throw std::runtime_error("Unknown MATCH_PL.CPP routine " + hex_address(entry));
    return run(s, *routine);
}
void match_pl_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void match_pl_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
}

namespace cm::aot::match_pl {
bool shot_event_uses_cpp(const State& state) {
    return original_shot_state != &state && !match_pc_profile_enabled() && match_pl_route_allowed(shot_event_entry);
}

OriginalShotEventScope::OriginalShotEventScope(const State& state) noexcept
    : previous_(original_shot_state) {
    original_shot_state = &state;
}
OriginalShotEventScope::~OriginalShotEventScope() {
    original_shot_state = previous_;
}
}
