#include "contract_screens_adapter.hpp"
#include "contract_screens.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
bool declining = std::getenv("CM_CONTRACT_SCREENS_ORIGINAL") != nullptr;

struct Routine {
    std::uint32_t entry;
    void (*body)(guest::Cpu&);
};
#define CONTRACT_SCREENS_ROUTINE(entry, ...) Routine{entry, __VA_ARGS__},
constexpr auto routines = std::to_array<Routine>({
#include "contract_screens_routines.inc"
});
#undef CONTRACT_SCREENS_ROUTINE
static_assert(std::ranges::is_sorted(routines, {}, &Routine::entry));
static_assert(std::ranges::adjacent_find(routines, {}, &Routine::entry) == routines.end());

const Routine* find(std::uint32_t entry) {
    const auto it = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    return it != routines.end() && it->entry == entry ? &*it : nullptr;
}

void run(State& s, const Routine& routine) {
    guest::Cpu cpu(s);
    routine.body(cpu);
    cpu.finish();
}
}

// Let a checker parent see nested entries through the dispatcher, including
// entries excluded from its C++ selection. The normal app calls them directly.
bool contract_screens_can_call_directly(std::uint32_t entry) {
    return !declining && (!routine_checker || (routine_check_child() && routine_checker->wants(entry)));
}

bool contract_screens_lists(std::uint32_t entry) { return find(entry) != nullptr; }

void contract_screens_run_original(State& s, std::uint32_t entry) {
    run_original_body(s, entry, declining);
}

void contract_screens_run_rewrite(State& s, std::uint32_t entry) {
    const auto* routine = find(entry);
    if (!routine) throw std::logic_error("Unknown CONTRACT_SCREENS.CPP entry " + hex_address(entry));
    run(s, *routine);
}

bool try_modern_contract_screens(State& s, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto* routine = find(entry);
    if (!routine) return false;
    if (routine_checker)
        routine_checker->check(s, entry, contract_screens_run_original, contract_screens_run_rewrite,
                               RoutineChecker::result_r3 | RoutineChecker::result_f1);
    run(s, *routine);
    return true;
}
}
