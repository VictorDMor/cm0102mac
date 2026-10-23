#pragma once
// Differential checks of routines rewritten in C++ against the generated
// original, inside a replay (cm-world-replay --check-routines). At a checked
// call two child processes start from the same live state: one runs the
// original body, the other the rewrite, each to the routine's return. Each
// reports its registers, heap tables and every page of guest memory it wrote;
// the parent compares what the game can observe and then continues the replay
// itself. Forking gives both runs the parent's exact state, imports included
// (a child reads the import transcript through its own file offset).
#include "aot_runtime.hpp"
#include <functional>
#include <map>
#include <string>

namespace cm::aot {
// Runs one call of `entry` from its entry state to its return.
using RoutineRunner = void (*)(State&, std::uint32_t entry);

class RoutineChecker {
public:
    // Every call of an entry up to `all`, then one in `every`; throws at the
    // first difference.
    RoutineChecker(std::function<bool(std::uint32_t)> wanted, std::uint64_t all, std::uint64_t every);
    // Called by a family hook at an entry. True when it checked the call; the
    // hook then runs the rewrite as usual. Never true inside a child.
    // `results`: the result registers the routine's callers read.
    static constexpr unsigned result_r3 = 1, result_r4 = 2, result_f1 = 4;
    bool check(State& s, std::uint32_t entry, RoutineRunner original, RoutineRunner rewrite, unsigned results);
    // Whether the checker looks at this entry (check tools run only these as
    // C++ when the generator has not hooked them).
    bool wants(std::uint32_t entry) const { return wanted_(entry); }
    struct Totals { std::uint64_t calls = 0, checked = 0, differences = 0; };
    const std::map<std::uint32_t, Totals>& totals() const { return totals_; }
    std::uint64_t differences() const;
    // Shows that the comparison fails when it should: flips one byte of the
    // rewrite's result before comparing, for the first checked call.
    bool corrupt_first = false;

private:
    std::function<bool(std::uint32_t)> wanted_;
    std::uint64_t all_, every_;
    std::map<std::uint32_t, Totals> totals_;
};
// Installed by the replay tool; null in the app.
extern RoutineChecker* routine_checker;
// True in a checker child; families then run nested calls of their own
// routines without checking them again.
bool routine_check_child();

// Check tools only (null in the app): asked before the dispatcher runs the
// page at s.pc. It may run the call itself (C++ routines the generator has
// not hooked yet, or a census) and return true with s.pc at the return
// address; false lets the page run.
extern bool (*dispatch_probe)(State&);

// Runs the generated body of `entry` from its entry state to its return
// (r1 back at its entry value), with the family's hook declining meanwhile.
void run_original_body(State& s, std::uint32_t entry, bool& declining);
}
