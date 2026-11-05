#include "staff_contracts_adapter.hpp"
#include "staff_contracts.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
using namespace staff_contracts;
constexpr unsigned R3 = RoutineChecker::result_r3, F1 = RoutineChecker::result_f1;
struct Routine {
    std::uint32_t entry;
    unsigned results;
    void (*run)(Cpu&);
};
template<auto Run>
void return_through_lr(Cpu& c) {
    Run(c);
    c.finish();
}

#define STAFF_CONTRACTS_ROUTINE(entry, results, ...) Routine{entry, results, return_through_lr<__VA_ARGS__>},
#define STAFF_CONTRACTS_CONTINUATION(entry, results, ...) Routine{entry, results, __VA_ARGS__},
constexpr auto routines = std::to_array<Routine>({
#include "staff_contracts_routines.inc"
});
#undef STAFF_CONTRACTS_CONTINUATION
#undef STAFF_CONTRACTS_ROUTINE

bool declining = std::getenv("CM_STAFF_CONTRACTS_ORIGINAL") != nullptr;

const Routine& find_routine(std::uint32_t entry) {
    const auto routine = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    if (routine == routines.end() || routine->entry != entry)
        throw std::logic_error("Unknown STAFF_CONTRACTS.CPP entry " + hex_address(entry));
    return *routine;
}

void run_routine(State& s, const Routine& routine) {
    Cpu cpu(s);
    routine.run(cpu);
}
}

bool staff_contracts_calls_directly() {
    // An explicit checker must see the nested entries so that it can select
    // their independent references and record them individually.
    return !declining && !routine_checker;
}

void staff_contracts_run_original(State& s, std::uint32_t entry) {
    run_original_body(s, entry, declining);
}

void staff_contracts_run_rewrite(State& s, std::uint32_t entry) {
    run_routine(s, find_routine(entry));
}

bool try_modern_staff_contracts(State& s, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto& routine = find_routine(entry);
    if (routine_checker)
        routine_checker->check(s, entry, staff_contracts_run_original, staff_contracts_run_rewrite,
                               routine.results);
    run_routine(s, routine);
    return true;
}
}
