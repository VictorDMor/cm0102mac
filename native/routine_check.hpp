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
#include <vector>
#include <filesystem>

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
    // Also compare r0, r3 (when not a result), r11, r12, XER and CTR at
    // return, to find a leftover that later reaches memory. r4-r10 and every
    // CR field are always compared.
    bool strict_volatile = false;
    // Also compare f1 for routines whose result is not f1: an operation
    // capture that ends at such a routine's return compares f1 (r3, r4, f1
    // and the preserved registers).
    bool compare_f1 = false;

private:
    std::function<bool(std::uint32_t)> wanted_;
    std::uint64_t all_, every_;
    std::map<std::uint32_t, Totals> totals_;
};
// Installed by the replay tool; null in the app.
extern RoutineChecker* routine_checker;

// Synthetic calls, for routines no capture reaches: when the replay
// dispatches `trigger` for the `nth` time (from 1; with from=, counting only
// dispatches whose return address is `from`), `target` is called with
// the given registers from that state in two forked children (original and
// rewrite) and compared; the replay itself goes on untouched. A value is a
// number, `@rN` (a register at the trigger) or `[EXPR]` (the word at EXPR),
// followed by any +off / -off. Runs that stop at the same point with the
// same message at an import the recording has no output for (such as an
// error alert) are compared there. Matching internal faults are reported
// as failed checks, not as equivalence: their intermediate state is not a
// comparison boundary.
struct SyntheticCall {
    std::uint32_t trigger = 0, nth = 1, target = 0;
    std::uint32_t from = 0;  // from=: count only dispatches returning here (0: all)
    bool made = false;       // set once the call has run
    unsigned results = RoutineChecker::result_r3;
    std::vector<std::pair<unsigned, std::string>> registers;
    // Memory the call sees (m8=/m16=/m32=ADDRESS:VALUE): made before both
    // runs and undone after, for states the recording never had.
    struct Write { unsigned size; std::string address, value; };
    std::vector<Write> writes;
    // expect8=/expect16=/expect32=ADDRESS:VALUE require final memory in
    // both completed children. Expressions use the trigger registers and
    // the initialized input memory, just like register assignments.
    std::vector<Write> expected_memory;
    std::string line;
};
std::vector<SyntheticCall> read_synthetic_calls(const std::filesystem::path& file);
// Runs the synthetic calls due at s.pc; never changes `s`. Returns the
// number of calls that differed (each printed).
unsigned run_synthetic_calls(State& s, std::vector<SyntheticCall>& calls, RoutineRunner original, RoutineRunner rewrite,
                             bool strict_volatile);
// How many synthetic calls have run so far (a line whose trigger never
// dispatches runs none: hooked routines do not dispatch; the replay tool
// fails a run in which a listed call was never made).
std::uint64_t synthetic_calls_made();
// Fixture checks (no capture): runs `entry` from the state as it is, with
// r1, LR and the arguments already set, once as the original body and once
// as the rewrite, each in a forked child, and returns what differs (empty
// when the runs leave the same state; r0, r11, r12, XER and CTR too with
// `strict_volatile`). `corrupt` spoils the rewrite's result, which must then
// differ.
std::vector<std::string> compare_routine_call(State& s, std::uint32_t entry, RoutineRunner original, RoutineRunner rewrite,
                                              unsigned results, bool corrupt = false, bool strict_volatile = false);
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
