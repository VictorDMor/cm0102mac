#include "transfer_manager_adapter.hpp"
#include "transfer_manager_approval.hpp"
#include "transfer_manager_approval_bootstrap.hpp"
#include "transfer_manager_callbacks.hpp"
#include "transfer_manager_candidate_clubs.hpp"
#include "transfer_manager_collections.hpp"
#include "transfer_manager_construct.hpp"
#include "transfer_manager_fees.hpp"
#include "transfer_manager_future.hpp"
#include "transfer_manager_history.hpp"
#include "transfer_manager_installments.hpp"
#include "transfer_manager_lifecycle.hpp"
#include "transfer_manager_load.hpp"
#include "transfer_manager_nation_rules.hpp"
#include "transfer_manager_nations.hpp"
#include "transfer_manager_offer_contracts.hpp"
#include "transfer_manager_offer_state.hpp"
#include "transfer_manager_pending.hpp"
#include "transfer_manager_permissions.hpp"
#include "transfer_manager_rejection.hpp"
#include "transfer_manager_save.hpp"
#include "transfer_manager_schedule.hpp"
#include "transfer_manager_selection.hpp"
#include "transfer_manager_squad_quotas.hpp"
#include "transfer_manager_valuation.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;

// The original route remains available for comparisons of complete call graphs.
bool declining = std::getenv("CM_TRANSFER_MANAGER_ORIGINAL") != nullptr;

struct Routine {
    std::uint32_t entry;
    void (*body)(Cpu&);
};
constexpr auto routines = std::to_array<Routine>({
#define TRANSFER_MANAGER_ROUTINE(entry, ...) Routine{entry, __VA_ARGS__},
#include "transfer_manager_routines.inc"
#undef TRANSFER_MANAGER_ROUTINE
});
static_assert(std::ranges::is_sorted(routines, {}, &Routine::entry));
static_assert(std::ranges::adjacent_find(routines, {}, &Routine::entry) == routines.end());

const Routine* lookup(std::uint32_t entry) {
    const auto it = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    return it != routines.end() && it->entry == entry ? &*it : nullptr;
}

void run_rewrite(State& s, const Routine& routine) {
    Cpu c(s);
    routine.body(c);
    c.finish();
}
}

bool transfer_manager_lists(std::uint32_t entry) { return lookup(entry) != nullptr; }

bool transfer_manager_direct_calls_enabled() {
    // Checker and probe paths need to see every owned call. In particular
    // --only can ask the dispatcher to decline a callee's handwritten body.
    return !declining && !routine_checker && !dispatch_probe;
}

void transfer_manager_run_original(State& s, std::uint32_t entry) {
    run_original_body(s, entry, declining);
}

void transfer_manager_run_rewrite(State& s, std::uint32_t entry) {
    const auto* routine = lookup(entry);
    if (!routine) throw std::logic_error("Unknown transfer-manager entry " + hex_address(entry));
    run_rewrite(s, *routine);
}

bool try_modern_transfer_manager(State& s, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    const auto* routine = lookup(entry);
    if (!routine) throw std::logic_error("Unknown transfer-manager entry " + hex_address(entry));
    if (routine_checker)
        routine_checker->check(s, entry, transfer_manager_run_original, transfer_manager_run_rewrite,
                               RoutineChecker::result_r3);
    run_rewrite(s, *routine);
    return true;
}
}
