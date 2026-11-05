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
struct Routine {
    std::uint32_t entry;
    void (*run)(Cpu&);
};
#define STAFF_CONTRACTS_ROUTINE(entry, ...) Routine{entry, __VA_ARGS__},
constexpr auto routines = std::to_array<Routine>({
#include "staff_contracts_routines.inc"
});
#undef STAFF_CONTRACTS_ROUTINE

bool declining = std::getenv("CM_STAFF_CONTRACTS_ORIGINAL") != nullptr;
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
    const auto routine = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    if (routine == routines.end() || routine->entry != entry)
        throw std::logic_error("Unknown STAFF_CONTRACTS.CPP entry " + hex_address(entry));
    Cpu cpu(s);
    routine->run(cpu);
    cpu.finish();
}

bool try_modern_staff_contracts(State& s, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(s, entry, staff_contracts_run_original, staff_contracts_run_rewrite,
                               RoutineChecker::result_r3);
    staff_contracts_run_rewrite(s, entry);
    return true;
}
}
