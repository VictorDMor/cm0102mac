// The generic CFM glue called from C++ without the dispatcher (comparator_glue.hpp).
//
// The dispatcher path it replaces: Cpu::call/MacAbi::call set r2_save and LR
// and run_until from 0x10781320; the glue is not a leaf, so its page asks
// try_modern_import_descriptor, which (no observer, not original, no PC
// profile, five instructions of budget left) counts the call and runs
// import_descriptor::invoke: call_descriptor's steps at 0x10781320..30, then
// the tail count. The dispatcher then dispatches the destination's page,
// whose first instruction asks its owner; run_owner_in_place is the
// generator's table of exactly those first instructions, so the owner decides
// at every call (declining, drafts, a checker's choice) as on the page. With a
// dispatch probe or a routine checker the check tools observe each dispatch,
// so the caller keeps the guest call.
//
// Scopes: the reference runs the glue and the destination inside one
// dispatcher scope opened at the call. A scope holds the stack pointer, the
// return address, the caller's native scope and the contexts recorded inside
// it (guest_context_transfer.cpp); the glue records none and keeps r1, so the
// scope run_until opens after the glue resolves and resumes every context
// transfer (setjmp/longjmp, the exception runtime) and budget stop as the
// reference's does. Only an owner run here, with no scope, could differ, so
// that is limited to the comparators below, which make no call.
#include "guest_scratch.hpp"
#include "comparator_glue.hpp"
#include "guest_context_transfer.hpp"
#include "import_descriptor.hpp"
#include "match_turbo.hpp"
#include "owner_in_place.hpp"
#include "routine_check.hpp"
#include <atomic>
#include <string_view>

namespace cm::aot {
namespace {
// Bit 0: off (the checks' reference). Bit 1: counting. 0 in the game.
constexpr int off = 1, counting = 2;
std::atomic<int> mode{0};
std::atomic<std::uint64_t> calls{0};
constexpr unsigned generic = import_descriptor::import_count;
constexpr std::uint32_t glue = import_descriptor::generic_entry;
// Register-file offsets (guest_abi.hpp reg::).
constexpr std::size_t r0 = 0, r1 = 4, r2 = 8, r12 = 48, r2_save = 2440, lr = 4128, ctr = 4132;

// The comparators most glue calls reach (season-day census, calls a day):
// PLAYER_STATS.CPP's relation order 0x10309a00 (25.3M) and SHORTLIST_MANAGER.CPP's
// pair and value orders 0x10366410 (6.7M), 0x10358f20 (3.9M), 0x10358f50
// (3.5M), and the module's three other qsort/bsearch comparators. Each makes
// no call: no branch-and-link in its original body (cm-comparator-glue-checks
// scans them) nor in its C++.
bool call_free_comparator(std::uint32_t code) {
    switch (code) {
    case 0x10309a00: case 0x10358f20: case 0x10358f50: case 0x10358f80: case 0x10358ff0: case 0x103663b0:
    case 0x10366410:
        return true;
    default:
        return false;
    }
}
// As import_descriptor_adapter.cpp's leave() classifies an exception.
void count_exception(State& s) {
    try {
        throw;
    } catch (const GuestContextTransfer&) {
        ++s.import_descriptor_transfers[generic];
    } catch (const std::runtime_error& error) {
        if (std::string_view(error.what()) == "Native instruction budget exhausted") ++s.import_descriptor_budget_stops[generic];
        else ++s.import_descriptor_faults[generic];
    } catch (...) {
        ++s.import_descriptor_faults[generic];
    }
}
}

bool call_glue_directly(State& s, std::uint32_t back, GlueTarget& target, std::uint64_t* escapes) {
    const int now = mode.load(std::memory_order_relaxed);
    if ((now & off) || target.declined || dispatch_probe || routine_checker || s.import_descriptor_observer ||
        s.import_descriptor_original || s.import_descriptor_entry_original[generic] ||
        s.import_descriptor_original_body == glue || match_pc_profile_enabled() || s.executed > s.budget ||
        5 > s.budget - s.executed) [[unlikely]]
        return false;
    if (now & counting) [[unlikely]] calls.fetch_add(1, std::memory_order_relaxed);
    // The bl.
    s.wr<4>(r2_save, s.r<4>(r2));
    s.wr<4>(lr, back);
    // The glue (import_descriptor.cpp call_descriptor), one step per instruction.
    ++s.import_descriptor_calls[generic];
    try {
        s.step(glue);
        s.wr<4>(r0, s.memory.read<4>(std::uint32_t(s.r<4>(r12))));
        s.step(glue + 4);
        s.memory.write<4>(std::uint32_t(s.r<4>(r1) + 20), s.r<4>(r2));
        s.step(glue + 8);
        s.wr<4>(ctr, s.r<4>(r0));
        s.step(glue + 12);
        s.wr<4>(r2, private_toc(s, std::uint32_t(s.memory.read<4>(std::uint32_t(s.r<4>(r12) + 4)))));
        s.step(glue + 16);
        s.pc = std::uint32_t(s.r<4>(ctr));
    } catch (...) {
        count_exception(s);
        throw;
    }
    ++s.import_descriptor_tails[generic];
    // The destination as its page starts: its owner first, which returns to LR.
    // A declining owner (or any other destination) is left to the dispatcher,
    // and the rest of this sort or search makes the guest call.
    target.code = s.pc;
    if (call_free_comparator(s.pc) && run_owner_in_place(s, s.pc)) {
        if (s.pc == back) return true;
    } else {
        target.declined = true;
    }
    try {
        run_until(s, back);
    } catch (const std::runtime_error& error) {
        if (escapes && std::string_view(error.what()).starts_with("Uncompiled original address:")) ++*escapes;
        throw;
    }
    return true;
}
void set_direct_glue_calls(bool enabled) {
    if (enabled) mode.fetch_and(~off);
    else mode.fetch_or(off);
}
void count_direct_glue_calls(bool value) {
    if (value) mode.fetch_or(counting);
    else mode.fetch_and(~counting);
}
std::uint64_t direct_glue_calls() { return calls.load(); }
}
