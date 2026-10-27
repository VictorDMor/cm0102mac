#include "contract_manager_adapter.hpp"
#include "contract_manager_detail.hpp"
#include "routine_check.hpp"
#include "contract_query_adapter.hpp"
#include "contract_storage_adapter.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdexcept>

namespace cm::aot {
namespace {
using guest::Cpu;
// Set while a checker child runs original bodies, and for the whole run with
// CM_CONTRACT_MANAGER_ORIGINAL=1 (to compare a build's behaviour without the rewrites).
bool declining = std::getenv("CM_CONTRACT_MANAGER_ORIGINAL") != nullptr;
[[maybe_unused]] constexpr unsigned NONE = 0, R3 = RoutineChecker::result_r3, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry;
    bool verified;  // CONTRACT_MANAGER_ROUTINE (the game runs it), not a draft
    unsigned results;
    void (*hook)(Cpu&);
};
#define CONTRACT_MANAGER_ROUTINE(entry, end, results, digest, ...) Routine{entry, true, results, __VA_ARGS__},
#define CONTRACT_MANAGER_DRAFT(entry, end, results, digest, ...) Routine{entry, false, results, __VA_ARGS__},
constexpr auto sorted_routines() {
    auto table = std::to_array<Routine>({
#include "contract_manager_routines.inc"
    });
    std::ranges::sort(table, {}, &Routine::entry);
    return table;
}
#undef CONTRACT_MANAGER_DRAFT
#undef CONTRACT_MANAGER_ROUTINE
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
void run_original(State& s, std::uint32_t entry) {
    // Until the drafts take over their generated hooks, compare against
    // generated bodies rather than the older query/storage implementations.
    auto& original = contract_query_diagnostics(s).original;
    struct Restore { bool& flag; bool old; ~Restore() { flag = old; } } restore{original, original};
    auto& storage = contract_storage_diagnostics(s).entry_original;
    Restore storage_bind{storage[0], storage[0]}, storage_reserve{storage[1], storage[1]};
    original = true;
    storage[0] = storage[1] = true;
    run_original_body(s, entry, declining);
}
bool run(State& s, const Routine& routine) {
    if (routine_checker) routine_checker->check(s, routine.entry, run_original, run_rewrite, routine.results);
    const bool initializes_dates = routine.entry == 0x100939b0;
    if (initializes_dates) ++contract_manager_diagnostics(s).date_initialization_calls;
    Cpu c(s);
    routine.hook(c);
    c.finish();
    if (initializes_dates) ++contract_manager_diagnostics(s).date_initialization_returns;
    return true;
}
// Whether this run takes the routine's C++: verified ones always (a check
// run with --only runs the others' originals, so each routine is checked at
// every call from the original's own callers), drafts only under a checker.
bool runs(const Routine* routine) {
    if (declining || !routine) return false;
    return routine_checker ? routine_checker->wants(routine->entry) : routine->verified;
}
}

bool contract_manager_lists(std::uint32_t entry) { return listed(entry) != nullptr; }
ContractManagerCallHook* contract_manager_call_hook = nullptr;
unsigned contract_manager_results(std::uint32_t entry) {
    const auto* routine = listed(entry);
    return routine ? routine->results : 0;
}
// Calls within the rewritten module bypass the dispatcher while preserving
// the original call-site link and non-volatile state.
bool contract_manager::detail::call_rewritten(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    if (auto* hook = contract_manager_call_hook) [[unlikely]] {
        const auto stack = c.sp();
        c.link(back); c.s.pc = entry;
        if (!hook->call(c.s)) return false;
        if (c.s.pc != back || c.sp() != stack)
            throw std::logic_error("Hooked contract-manager call returned elsewhere: " + hex_address(entry));
        return true;
    }
    const auto* routine = listed(entry);
    if (declining || !routine || !(routine->verified || routine_checker)) return false;
    c.link(back);
    routine->hook(c);
    c.s.pc = back;
    return true;
}

bool contract_manager_dispatch_probe(State& s) {
    const auto* routine = listed(s.pc);
    return runs(routine) && run(s, *routine);
}

bool try_modern_contract_manager(State& s, std::uint32_t entry) {
    const auto* routine = listed(entry);
    return runs(routine) && run(s, *routine);
}
void contract_manager_run_original(State& s, std::uint32_t entry) { run_original(s, entry); }
void contract_manager_run_rewrite(State& s, std::uint32_t entry) { run_rewrite(s, entry); }
}
