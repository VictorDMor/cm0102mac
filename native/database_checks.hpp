#pragma once
// cm-database-checks: DATABASE.CPP's C++ routines against an independent
// generated reference of their original bodies (make_database_reference.py).
// Each block of routines provides its fixtures in database_checks_<block>.cpp;
// database_checks.cpp runs them all (or those of --only) the same way:
// - the original and the C++ from the same state, each in a forked child,
//   comparing every written page, the heap, r1-r10, r13-r31, cr0-cr7, the
//   FPSCR and f1 (with strict_volatile: also r0, r11, r12, XER, CTR);
// - then both again in process, comparing the whole register file;
// - each on the C++ route and on the production route (the generated page
//   and its hook);
// - then the production route with no instruction budget whenever control is
//   in DATABASE.CPP's code (other modules' code may run), so no generated
//   instruction of the module runs.
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <functional>
#include <string>
#include <vector>

namespace cm::aot::database_checks {
using guest::Cpu;
// Fixture memory: `data` and `high` (above 0x7fffffff, for signed compares)
// filled with 0xa5, the stack region likewise; the trace of stubbed calls.
constexpr std::uint32_t data = 0x60000000, data_size = 0x20000, high = 0x90000000, high_size = 0x2000,
                        stack_base = 0x71000000, stack_size = 0x20000, stack = 0x71010000, sentinel = 0xfffffff0,
                        trace_at = data + 0x18000, toc = 0x108bba90;
constexpr std::uint32_t module_start = 0x1009efc0, module_end = 0x100de410;

// One call of `entry`. The base state: memory as above; r0-r31, f0-f31 and
// CR patterned; r1 the stack, r2 the TOC, LR the sentinel; XER[SO] from
// `summary_overflow`; then `setup` sets records, TOC words and arguments.
struct Fixture {
    std::uint32_t entry = 0;
    std::string description;
    std::function<void(Cpu&)> setup;
    // Answers a call instead of running the callee, in every run alike: it
    // sees s.pc at the callee, sets the results and s.pc = LR, and returns
    // true; false lets the callee run. record_call() keeps its arguments.
    std::function<bool(State&)> stub;
    bool summary_overflow = false;
    // Results the callers read (RoutineChecker::result_r3 and so on).
    unsigned results = 1 | 4;
};

// Appends the call's target, LR, r0-r10, r13-r31 and CR to the trace in
// guest memory, so the compared memory holds every stubbed call's
// arguments and preserved registers.
void record_call(Cpu& c);

// The blocks' fixtures.
std::vector<Fixture> first_fixtures();      // blocks 1-2 (database_checks_first.cpp)
std::vector<Fixture> lookup_fixtures();     // database_checks_lookups.cpp
std::vector<Fixture> record_fixtures();     // database_checks_records.cpp
std::vector<Fixture> text_fixtures();       // database_checks_text.cpp
std::vector<Fixture> generation_fixtures(); // database_checks_generation.cpp
std::vector<Fixture> search_fixtures();     // database_checks_search.cpp
std::vector<Fixture> loading_fixtures();    // database_checks_loading.cpp
}
