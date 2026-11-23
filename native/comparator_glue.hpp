#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
// One sort's or search's memory of its comparator: the destination its glue
// reached last, and whether that call was left to the dispatcher (the owner
// declined, or the destination is not a call-free comparator); the later
// calls then make the guest call. Empty at first.
struct GlueTarget {
    std::uint32_t code = 0;
    bool declined = false;
};
// `bl 0x10781320` from a C++ caller: the generic CFM glue (r12 holds the
// callee's transition vector) through which qsort and bsearch call their
// comparators, about 45M times in a season day. Without the dispatcher: the
// bl's link (r2_save, LR), the glue's five instructions as
// import_descriptor::invoke runs them (r0 = code, r2 stored at sp+20,
// CTR = code, r2 = the callee's TOC) and its counters, then, for one of the
// game's call-free comparators, its owner, asked at every call as its page
// asks it (run_owner_in_place). Any other destination, or a declining owner,
// continues in the dispatcher, whose scope then opens after the glue, which is
// the same scope (comparator_glue.cpp). Every guest read and write, register,
// instruction count, transfer and import-descriptor counter is the dispatcher
// path's. False, with nothing changed, while a check tool or a diagnostic
// watches dispatches or the glue, below five instructions of budget, and
// after a declined call in the same sort or search: the caller then makes the
// guest call. `escapes`, when given, counts "Uncompiled original address"
// faults as MacAbi::call does.
bool call_glue_directly(State& s, std::uint32_t return_address, GlueTarget& target, std::uint64_t* escapes = nullptr);
// For checks (any thread): the direct path off (callers make the guest call,
// the reference), and counting the glue calls it makes (off by default).
void set_direct_glue_calls(bool enabled);
void count_direct_glue_calls(bool counting);
std::uint64_t direct_glue_calls();
}
