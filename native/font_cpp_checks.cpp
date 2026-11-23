// Original-versus-C++ checks of FONT.CPP from a real UI state (--world, the save-info snapshot). See font_cpp_checks.hpp.
// The module is screen-side (no random draw, no save): the checks compare the registers, the heap and every page a call wrote.
// Evidence, reported separately:
//   cases    the listed calls from the blocks' case lists: the snapshot's real font records, lists and windows,
//            and rows a case builds in a scratch area from real values with one field changed;
//   mutated  a call repeated with changed input values the original actually read, so branches the state did
//            not take run on both sides;
//   trace    with --call-trace, the calls each side makes through the dispatcher (callee, return address,
//            non-volatile registers and the arguments the callee reads, order and counts);
//   nested   the module's routines a case reaches, checked on the way.
// Real calls come from the capture replays too (cm-world-replay --check-routines). A mutated call in which
// both sides fault alike is counted apart, never as a pass. --corrupt and --corrupt-trace are the negative
// controls: a check must then fail.
#include "font_cpp_checks.hpp"
#include "application_exit.hpp"
#include "code_fragment_adapter.hpp"
#include "guest_abi.hpp"
#include "font_cpp_adapter.hpp"
#include "match_replay.hpp"
#include "match_turbo.hpp"
#include "routine_check.hpp"
#include "allocator_adapter.hpp"
#include "stream_io_adapter.hpp"
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string_view>
#include <csignal>
#include <tuple>

#if defined(CM_FONT_CPP_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif

using namespace cm::aot;
using namespace cm::aot::font_cpp_checks;
namespace cm::aot { extern bool routine_check_dead_stack_exact; }  // routine_check.cpp: compare the dead stack too

namespace {
// Every routine of the module is written native-first and compared under the native contract (r4-r10 are not compared).
bool font_cpp_native(std::uint32_t) { return true; }
// r4-r10 at return are compared for every entry (LEAGUE.CPP's integration found four cross-block differences of
// that kind that each block's own run had passed: a C++ callee leaving r4-r10 differently for a caller that reads
// leftovers, directly or through qsort). An entry is exempt only with the listing's proof that no caller can read
// them (font_cpp_volatile_free.inc), followed by hand past every callee's return: none is exempt now.
constexpr std::uint32_t volatile_free[] = {
#include "font_cpp_volatile_free.inc"
    0,
};
bool font_cpp_reads_volatile(std::uint32_t entry) { return std::ranges::find(volatile_free, entry) == std::end(volatile_free); }
// An entry whose callers read only some of r4-r10 after the call (hand-proved: font_cpp_volatile_read.inc) is
// compared on those registers alone: bit n of the mask is r(4+n). A routine that no listed register's writer is part of
// (font_cpp_volatile_read.inc gives the proof) leaves every other register dead for its callers.
struct VolatileRead { std::uint32_t entry; unsigned mask; };
constexpr VolatileRead volatile_read[] = {
#include "font_cpp_volatile_read.inc"
    {0, 0},
};
unsigned font_cpp_volatile_mask(std::uint32_t entry) {
    if (!font_cpp_reads_volatile(entry)) return 0;
    for (const auto& read : volatile_read) if (read.entry == entry) return read.mask;
    return 0x7f;
}
// The dead stack below an entry's stack pointer is free (the per-reader contract), except for the entries listed in
// font_cpp_stack_exact.inc: those are compared byte for byte there too, where a routine writes a frame a known reader reads
// (0x10767bb0 and the routines that call it at their own frame's sp) and its rewrite builds the original's frame.
constexpr std::uint32_t stack_exact[] = {
#include "font_cpp_stack_exact.inc"
    0,
};
bool font_cpp_dead_stack_free(std::uint32_t entry) { return std::ranges::find(stack_exact, entry) == std::end(stack_exact); }
bool font_cpp_native_code(std::uint32_t address) { return font_cpp_code(address); }
}

namespace cm::aot::font_cpp_checks {
namespace {
std::vector<std::pair<const char*, CaseList>>& blocks() {
    static std::vector<std::pair<const char*, CaseList>> list;
    return list;
}
}
CaseBlock::CaseBlock(const char* name, CaseList list) { blocks().emplace_back(name, std::move(list)); }
}

namespace {
struct Options {
    std::filesystem::path world;
    unsigned all = 40, every = 400, mutate = 0, mutated_calls = 4;
    std::uint32_t seed = 1;
    std::set<std::uint32_t> only;
    bool corrupt = false, null_page = false, cases = true, strict = false, require_all = false, show_faults = false, show_sites = false, call_trace = false, corrupt_trace = false, production = false;
} options;

struct Tally { std::uint64_t returned = 0, faulted = 0, timed_out = 0, calls = 0; };
std::map<std::uint32_t, Tally> case_tally, mutated_tally;
std::map<std::uint32_t, std::uint64_t> observed_calls;
unsigned clocks = 0;

struct Inputs final : CodeFragmentBoundaryObserver {
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        if (std::string_view(name) != "CarbonLib::Microseconds") return false;
        s.memory.write<8>(s.r<4>(12), 1000000000ULL + 1000000ULL * ++clocks);
        s.pc = s.r<4>(4128);
        return true;
    }
} inputs;

void flush_child() {
#if defined(CM_FONT_CPP_COVERAGE)
    __llvm_profile_write_file();
#endif
}
// Mutated inputs can loop for a very long time on both sides.
// For the entries above (all but the exempt), r4-r10 at return are copied into guest memory (a scratch page both sides map), where the
// comparison of memory sees them: the native contract leaves the registers themselves out.
constexpr std::uint32_t volatile_image = scratch_base + 0xff000, game_toc = 0x108bba90;
void save_volatile(State& s, std::uint32_t entry) {
    const auto mask = font_cpp_volatile_mask(entry);
    for (unsigned n = 4; n <= 10; ++n)
        if (mask >> (n - 4) & 1) s.memory.write<4>(volatile_image + n * 4, std::uint32_t(s.r<4>(guest::reg::gpr(n))));
}
void verify_run(State& s, const char* who);

// ---- The leaver check: the project's stack contract, per reader. The bytes the game's printf reads as leftovers are what the four leavers
// leave in their own frames: the saved registers their prologues store, the link register and the back chain. Every time a leaver is
// entered, in the original run and in the rewrite run, the check records what determines that frame (the entry's stack pointer relative to
// the checked entry's, the link register, the registers the leaver's prologue saves) and compares the two sequences in order. Where it
// observes: the original's entry into a hooked entry (font_cpp_adapter.cpp, at the entry instruction and at call sites from other
// pages: a repeat with no instruction between is the same entry), and the C++'s `guest::Frame` of the leaver (the native scope probe of
// guest_context_transfer.cpp: it runs when the frame opens, before its stores, whether the leaver is reached through the dispatcher or by a
// direct C++ call). The record goes to a log in the scratch area both sides map (count, an FNV hash of every event, then the first events),
// so the memory comparison reports a difference; CM_FONT_CPP_LEAVER_LOG=1 prints the events of a child.
struct Leaver { std::uint32_t entry; unsigned first_gpr; };
// The registers each leaver's prologue saves (first..r31): the string drawer 0x1074f800 (stmw r13), 0x107500f0 (stmw r26), the
// selector 0x10750320 (stmw r24) and the glyph cache 0x10750580 (stmw r23): tools/font_cpp_audit.LEAVERS.
constexpr Leaver leavers[] = {{0x1074f800, 13}, {0x107500f0, 26}, {0x10750320, 24}, {0x10750580, 23}};
constexpr std::uint32_t leaver_log = scratch_base + 0xe0000, leaver_stride = 96, leaver_capacity = 600;
std::uint32_t leaver_base = 0, last_leaver_entry = 0;
std::uint64_t last_leaver_executed = ~std::uint64_t(0);
void record_leaver(State& s, std::uint32_t entry, bool original) {
    const auto* leaver = std::ranges::find(leavers, entry, &Leaver::entry);
    if (leaver == std::end(leavers)) return;
    // The original's hook runs at a call site in another page and again at the entry with no instruction between: one entry. (Native code
    // charges no instructions, so the C++ side is never de-duplicated.)
    if (original) {
        if (entry == last_leaver_entry && s.executed == last_leaver_executed) return;
        last_leaver_entry = entry;
        last_leaver_executed = s.executed;
    }
    std::vector<std::uint32_t> words{entry, std::uint32_t(s.r<4>(4)) - leaver_base, std::uint32_t(s.r<4>(4128))};
    for (unsigned n = leaver->first_gpr; n < 32; ++n) words.push_back(std::uint32_t(s.r<4>(guest::reg::gpr(n))));
    const auto count = std::uint32_t(s.memory.read<4>(leaver_log));
    auto hash = std::uint32_t(s.memory.read<4>(leaver_log + 4));
    if (!count) hash = 2166136261u;
    for (const auto word : words) hash = (hash ^ word) * 16777619u;
    if (std::getenv("CM_FONT_CPP_LEAVER_LOG")) {
        std::fprintf(stderr, "LEAVER %u:", count);
        for (const auto word : words) std::fprintf(stderr, " %x", word);
        std::fprintf(stderr, "\n");
    }
    if (count < leaver_capacity)
        for (std::size_t n = 0; n < words.size(); ++n) s.memory.write<4>(leaver_log + 8 + count * leaver_stride + 4 * std::uint32_t(n), words[n]);
    s.memory.write<4>(leaver_log, count + 1);
    s.memory.write<4>(leaver_log + 4, hash);
}
// The C++ side: a native scope opens with the function's entry (guest::Frame); the original's frames are never native, so only the C++
// run (hooks not declining) is recorded here.
void record_scope(State& s, std::uint32_t function) { if (!font_cpp_declining) record_leaver(s, function, false); }
void record_original(State& s, std::uint32_t entry) { record_leaver(s, entry, true); }
// A case that answers a leaver: neither side runs its body, so the entry is recorded where the answer is given (the same entry state).
void record_answered(State& s) { record_leaver(s, s.pc, font_cpp_declining); }
}
namespace cm::aot { extern void (*font_cpp_leaver_probe)(State&, std::uint32_t); extern void (*native_scope_probe)(State&, std::uint32_t); }
namespace cm::aot::font_cpp_checks {
// A run's start (the checked entry's stack pointer): the check's origin. Also called by the end-to-end sweeps.
void install_leaver_check(State& s) {
    leaver_base = std::uint32_t(s.r<4>(4));
    last_leaver_entry = 0;
    last_leaver_executed = ~std::uint64_t(0);
    font_cpp_leaver_probe = record_original;
    native_scope_probe = record_scope;
}
}
namespace {
// Seconds a compared run may take (a mutated input can keep a loop going for a long time on both sides: the runs of a mutation are given less).
unsigned run_alarm = 60;
const std::function<void(State&)>* case_prepare = nullptr;
// The mutation of a prepared case (Case::prepare) changes values the checked call reads from the state the preparation builds, so it is applied in
// each run after the preparation: (address, size, value).
std::vector<std::tuple<std::uint32_t, unsigned, std::uint64_t>> pending_mutations;
void prepare_case(State& s) {
    if (!case_prepare || !*case_prepare) return;
    alarm(run_alarm);
    s.budget = s.executed + 4'000'000'000ULL;
    (*case_prepare)(s);
    for (const auto& [address, size, value] : pending_mutations) {
        if (size == 1) s.memory.write<1>(address, value & 0xff);
        else if (size == 2) s.memory.write<2>(address, value & 0xffff);
        else if (size == 4) s.memory.write<4>(address, std::uint32_t(value));
        else s.memory.write<8>(address, value);
    }
}
void bounded_original(State& s, std::uint32_t entry) {
    prepare_case(s);
    font_cpp_checks::install_leaver_check(s);
    alarm(run_alarm); s.budget = s.executed + 4'000'000'000ULL; font_cpp_run_original(s, entry);
    verify_run(s, "original");
    save_volatile(s, entry);
}
void bounded_rewrite(State& s, std::uint32_t entry) {
    prepare_case(s);
    font_cpp_checks::install_leaver_check(s);
    alarm(run_alarm);
    s.budget = s.executed + 4'000'000'000ULL;
    font_cpp_run_rewrite(s, entry);
    verify_run(s, "rewrite");
    save_volatile(s, entry);
    if (options.corrupt) {
        guest::Cpu c(s);
        c.set(3, c.r(3) ^ 1);
        // A native-first routine without a result is not compared on r3: spoil a byte of memory.
        if (font_cpp_native(entry)) s.memory.write<1>(scratch_base, std::uint8_t(s.memory.read<1>(scratch_base) ^ 1));
    }
}

// `value`: what the call first read there (big-endian bytes as the guest holds them), taken in the run that prepared the state.
struct Read { std::uint32_t address; unsigned size; std::uint64_t value; };
struct ReadTrace final : MemoryObserver {
    std::map<std::pair<std::uint32_t, unsigned>, std::uint64_t> seen;
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void read(std::uint32_t address, const std::uint8_t* bytes, unsigned size) override {
        if (seen.size() >= 200000 || size > 8) return;
        std::uint64_t value = 0;
        for (unsigned n = 0; n < size; ++n) value = value << 8 | bytes[n];
        seen.emplace(std::make_pair(address, size), value);
    }
};

// For a child that isn't comparing: the routing of the compared runs, with no nested checks.
RoutineChecker routed_like_compared_runs() {
    const auto* compared = routine_checker;
    return RoutineChecker([compared](std::uint32_t entry) { return compared && compared->wants(entry); }, 0, 0);
}

// What the original reads on this call, from a child running it.
std::vector<Read> reads_of(State& s, std::uint32_t entry) {
    int fds[2];
    if (::pipe(fds) != 0) throw std::runtime_error("pipe");
    std::cout.flush(); std::cerr.flush();
    const auto pid = ::fork();
    if (pid < 0) throw std::runtime_error("fork");
    if (pid == 0) {
        ::close(fds[0]);
        ReadTrace trace;
        auto silent = routed_like_compared_runs();
        routine_checker = &silent;
        prepare_case(s);
        const auto previous = s.memory.date_observer;
        s.memory.date_observer = &trace;
        alarm(30);
        try { s.budget = s.executed + 4'000'000'000ULL; font_cpp_run_original(s, entry); } catch (...) {}
        s.memory.date_observer = previous;
        for (const auto& [key, value] : trace.seen) {
            const Read read{key.first, key.second, value};
            if (::write(fds[1], &read, sizeof read) != ssize_t(sizeof read)) _exit(2);
        }
        ::close(fds[1]);
        flush_child();
        _exit(0);
    }
    ::close(fds[1]);
    std::vector<Read> reads;
    Read read{};
    while (::read(fds[0], &read, sizeof read) == ssize_t(sizeof read)) reads.push_back(read);
    ::close(fds[0]);
    int status = 0;
    ::waitpid(pid, &status, 0);
    return reads;
}

bool mapped(const State& s, std::uint32_t address) {
    for (const auto& [low, size] : s.memory.mapped_ranges())
        if (address >= low && std::uint64_t(address) < std::uint64_t(low) + size) return true;
    return false;
}

// Interesting replacements for a value the original read.
std::uint64_t replacement(const Read& read, std::uint64_t before, std::mt19937& random) {
    const auto pick = [&](std::initializer_list<std::int64_t> values) { return std::int64_t(*(values.begin() + random() % values.size())); };
    if (read.size == 1) {
        if (random() % 2) return std::uint64_t(pick({0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 15, 20, 50, 100, 127, -1, -2, -128}));
        return before + std::uint64_t(pick({1, -1, 2, -2, 4, -4}));
    }
    if (read.size == 2) {
        if (random() % 2) return std::uint64_t(pick({0, 1, 2, 3, 5, 10, 18, 30, 45, 54, 90, 100, 200, 255, 1000, 0x1ef, 0x3de, 5000, 10000, 32767, -1, -5, -100, -200, -32768}));
        return before + std::uint64_t(pick({1, -1, 10, -10, 100, -100}));
    }
    if (read.size == 4) {
        const float value = std::bit_cast<float>(std::uint32_t(before));
        const bool small = std::uint32_t(before) + 0x10000u < 0x20000u;
        if (small) return std::uint32_t(pick({0, 1, 2, 3, 4, 7, 10, 100, 1000, 0x10000, -1, -2, -100, INT32_MAX, INT32_MIN})) + 0ull;
        const auto bits = [](float f) { return std::uint64_t(std::bit_cast<std::uint32_t>(f)); };
        switch (random() % 9) {
            case 0: return bits(0.f);
            case 1: return bits(-value);
            case 2: return bits(value * 2);
            case 3: return bits(value / 2);
            case 4: return bits(1.f);
            case 5: return bits(-1.f);
            case 6: return bits(1e6f);
            case 7: return bits(std::numeric_limits<float>::quiet_NaN());
            default: return bits(value + 1);
        }
    }
    const double value = std::bit_cast<double>(before);
    const auto bits = [](double d) { return std::bit_cast<std::uint64_t>(d); };
    switch (random() % 8) {
        case 0: return bits(0.);
        case 1: return bits(-value);
        case 2: return bits(value * 2);
        case 3: return bits(value / 2);
        case 4: return bits(1.);
        case 5: return bits(-1.);
        case 6: return bits(1e9);
        default: return bits(std::numeric_limits<double>::quiet_NaN());
    }
}

std::string_view current_label;
const std::vector<std::pair<std::uint32_t, std::uint32_t>>* case_answers = nullptr;
const std::vector<Case::Poke>* case_pokes = nullptr;
const std::vector<Case::Capture>* case_captures = nullptr;
const std::vector<Case::NthAnswer>* case_nth_answers = nullptr;
const std::function<bool(State&)>* case_verify = nullptr;
bool mutating = false;
// Failed expectations (Case::verify), counted in memory the forked children share with the parent.
unsigned* expectation_failures = [] {
    void* shared = ::mmap(nullptr, sizeof(unsigned), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if (shared == MAP_FAILED) throw std::runtime_error("mmap");
    return static_cast<unsigned*>(shared);
}();
void verify_run(State& s, const char* who) {
    if (!case_verify || mutating || !*case_verify) return;
    if (!(*case_verify)(s)) {
        __atomic_fetch_add(expectation_failures, 1, __ATOMIC_SEQ_CST);
        std::cerr << "EXPECTATION FAILED in the " << who << " run [" << current_label << "]\n";
    }
}
// Dispatches of each routine with an nth answer, in this process: every compared run is a fresh child.
std::map<std::uint32_t, unsigned> nth_dispatches;
void store(State& s, const Write& write, std::uint32_t value) {
    if (write.size == 1) s.memory.write<1>(write.address, value & 0xff);
    else if (write.size == 2) s.memory.write<2>(write.address, value & 0xffff);
    else s.memory.write<4>(write.address, value);
}
struct Comparison { bool returned = false, faulted = false, timed_out = false; std::vector<std::string> differences; };
// Runs that ended without a report: on both sides, on one side only, and the original's alone by the alarm that compared after a longer one.
unsigned ended_alike = 0, ended_alone = 0, slow_original = 0;
Comparison compare_call(State& s, std::uint32_t entry, unsigned results, bool strict = false) {
    Comparison outcome;
    const auto stop_rewrite = "the rewrite stopped: ", stop_original = "the original stopped: ";
    std::vector<std::string> found;
    routine_check_record_ended = true;
    routine_check_native_contract = font_cpp_native(entry);
    routine_check_dead_stack_free = font_cpp_dead_stack_free(entry);
    routine_check_dead_stack_exact = !routine_check_dead_stack_free;
    try {
        found = compare_routine_call(s, entry, bounded_original, bounded_rewrite, results, false, strict);
    } catch (const std::runtime_error& error) {
        if (!std::string_view(error.what()).starts_with("Routine check child ended")) throw;
        // A run both sides could not finish (the alarm, a loop that a mutated input keeps going) is a timeout, alike. A child that ended on one side only is
        // not: a crash or hang of the rewrite alone, or of the original alone other than by the alarm, is a difference. The original ending alone by the alarm
        // is what a faster native rewrite does to a loop the original runs slowly: the call is repeated once with a longer alarm, and compared if both finish.
        const auto sides = routine_check_ended_sides;
        const auto alarmed = [](int status) { return (status & 0x7f) == SIGALRM; };
        const char* const who = sides == 1 ? "the original" : sides == 2 ? "the rewrite" : "both";
        if (sides == 3) {
            ++ended_alike;
            std::cerr << "timed out alike [" << current_label << "]\n";
            outcome.timed_out = true;
            return outcome;
        }
        // The instruction budget counts generated instructions only, so a loop a mutated input keeps going runs the original out of its budget at once while the
        // native rewrite, which is not charged, goes on until the alarm: both loop, alike.
        // Only in a mutated run (`mutating`): an unmutated case whose rewrite hangs while the original runs out of its budget is a finding (the rewrite hung alone).
        if (mutating && sides == 2 && alarmed(routine_check_ended_status[1]) && std::string_view(routine_check_stop_message[0]).starts_with("Native instruction budget exhausted")) {
            ++ended_alike;
            std::cerr << "timed out alike (mutated run: the original out of its budget, the rewrite at the alarm) [" << current_label << "]\n";
            outcome.timed_out = true;
            return outcome;
        }
        if (sides == 1 && alarmed(routine_check_ended_status[0]) && run_alarm < 150) {
            const auto before = run_alarm;
            run_alarm = 150;
            const auto again = compare_call(s, entry, results, strict);
            run_alarm = before;
            if (again.differences.empty()) { ++slow_original; std::cerr << "original slower than the rewrite [" << current_label << "]: repeated, compared\n"; }
            return again;
        }
        ++ended_alone;
        outcome.differences = {std::string("only ") + who + " ended without a report (status " + std::to_string(routine_check_ended_status[sides == 1 ? 0 : 1]) + ")"};
        return outcome;
    }
    std::string rewrite_stop, original_stop;
    for (const auto& line : found) {
        if (line.starts_with(stop_rewrite)) rewrite_stop = line.substr(std::string_view(stop_rewrite).size());
        if (line.starts_with(stop_original)) original_stop = line.substr(std::string_view(stop_original).size());
    }
    // The instruction budget counts generated instructions only: a native-first routine's own loops are not
    // charged, so a mutated input that keeps the original looping until the budget runs out may let the native
    // side finish. Neither side's result is known then: count it with the timeouts, not as a difference.
    if (font_cpp_native(entry) && original_stop.starts_with("Native instruction budget exhausted")) {
        outcome.timed_out = true;
        return outcome;
    }
    if (!rewrite_stop.empty() && rewrite_stop == original_stop) {
        outcome.faulted = true;
        if (options.show_faults) std::cerr << "both faulted at " << hex_address(entry) << " [" << current_label << "]: " << rewrite_stop << '\n';
    }
    else if (found.empty()) outcome.returned = true;
    else outcome.differences = std::move(found);
    return outcome;
}

// --call-trace: what the end state can't show. A callee saves r13-r31 and f14-f31 into the
// stack image, so they must match at every call, also where no callee on this path saved them.
// Each side runs in a child recording those registers at each `bl` the module makes through the
// dispatcher; a same-page call or an in-place owner shows on one side only and isn't paired. The
// k-th call from one site to one target on each side are compared, as are how many calls each
// site made and the order of the sites both sides show. A return address the rewrite alone uses
// that doesn't follow a `bl` to that target is wrong. Findings don't stop the run.
struct TracedCall {
    std::uint32_t back, target;
    bool after_bl;
    std::array<std::uint32_t, 19> gprs;
    std::array<std::uint64_t, 18> fprs;
    std::array<std::uint32_t, 8> arguments;  // r3-r10
};
// --argument-masks FILE (tools/call_arguments.py): which of r3-r10 each target reads. Those are
// compared at each call too: a callee a case answers shows nothing else of what it was given.
std::map<std::uint32_t, unsigned> argument_masks;
// A run records at most this many calls; past it a side's counts and order are unknown.
constexpr std::size_t trace_limit = 2'000'000;
std::vector<TracedCall>* traced_calls = nullptr;
bool (*scripted_services)(State&) = nullptr;
std::array<std::pair<std::uint32_t, std::uint32_t>, 4000> recent_dispatches;
unsigned dispatches_seen = 0;
// One call to `target` made from the module's code with the link register already set: a dispatch of the dispatcher, or a typed call the
// rewrite announces (font_cpp_direct_call_probe).
void record_call(State& s, std::uint32_t target) {
    recent_dispatches[dispatches_seen++ % recent_dispatches.size()] = {target, std::uint32_t(s.r<4>(4128))};
    const auto back = std::uint32_t(s.r<4>(4128));
    if (in_module(back - 4) && traced_calls->size() < trace_limit) {
        const auto word = std::uint32_t(s.memory.read<4>(back - 4));
        const auto offset = std::int32_t(word << 6) >> 6 & ~3;
        TracedCall call{back, target, (word & 0xfc000003) == 0x48000001 && back - 4 + std::uint32_t(offset) == target, {}, {}, {}};
        for (unsigned n = 13; n < 32; ++n) call.gprs[n - 13] = std::uint32_t(s.r<4>(guest::reg::gpr(n)));
        for (unsigned n = 14; n < 32; ++n) call.fprs[n - 14] = s.r<8>(guest::reg::fpr(n));
        for (unsigned n = 3; n <= 10; ++n) call.arguments[n - 3] = std::uint32_t(s.r<4>(guest::reg::gpr(n)));
        traced_calls->push_back(call);
    }
}
bool trace_probe(State& s) {
    record_call(s, s.pc);
    return scripted_services(s);
}
std::uint64_t truncated_traces = 0;
std::vector<TracedCall> trace_run(State& s, std::uint32_t entry, RoutineRunner runner) {
    int fds[2];
    if (::pipe(fds) != 0) throw std::runtime_error("pipe");
    std::cout.flush(); std::cerr.flush();
    const auto pid = ::fork();
    if (pid < 0) throw std::runtime_error("fork");
    if (pid == 0) {
        ::close(fds[0]);
        static std::vector<TracedCall> calls;
        traced_calls = &calls;
        auto silent = routed_like_compared_runs();
        routine_checker = &silent;
        dispatch_probe = trace_probe;
        font_cpp_direct_call_probe = record_call;
        try { runner(s, entry); } catch (const std::exception& error) {
            if (options.show_faults) {
                std::cerr << "fault path of " << hex_address(entry) << " (pc " << hex_address(s.pc) << "): " << error.what() << '\n';
                for (unsigned n = dispatches_seen > recent_dispatches.size() ? dispatches_seen - unsigned(recent_dispatches.size()) : 0; n < dispatches_seen; ++n) {
                    const auto& [target, back] = recent_dispatches[n % recent_dispatches.size()];
                    std::cerr << "  " << hex_address(target) << " lr " << hex_address(back) << '\n';
                }
            }
        } catch (...) {}
        for (const auto& call : calls) if (::write(fds[1], &call, sizeof call) != ssize_t(sizeof call)) _exit(2);
        ::close(fds[1]);
        flush_child();
        _exit(0);
    }
    ::close(fds[1]);
    std::vector<TracedCall> calls;
    TracedCall call{};
    for (;;) {
        std::size_t done = 0;
        auto* bytes = reinterpret_cast<char*>(&call);
        while (done < sizeof call) {
            const auto n = ::read(fds[0], bytes + done, sizeof call - done);
            if (n <= 0) break;
            done += std::size_t(n);
        }
        if (done < sizeof call) break;
        calls.push_back(call);
    }
    ::close(fds[0]);
    int status = 0;
    ::waitpid(pid, &status, 0);
    return calls;
}
struct TraceFinding { std::uint64_t count = 0; std::uint64_t original = 0, rewrite = 0; };
// Argument findings accepted with evidence for native-first entries (font_cpp_trace_accepted.inc).
struct AcceptedFinding { std::uint32_t back, target; unsigned reg; const char* reason; };  // back: the bl's address
#define FONT_CPP_TRACE_ACCEPTED(back, target, reg, reason) AcceptedFinding{back, target, reg, reason},
constexpr AcceptedFinding accepted_findings[] = {
#include "font_cpp_trace_accepted.inc"
    AcceptedFinding{0, 0, 0, "end of the list"},
};
#undef FONT_CPP_TRACE_ACCEPTED
bool accepted_finding(std::uint32_t back, std::uint32_t target, unsigned reg) {
    return std::ranges::any_of(accepted_findings, [&](const AcceptedFinding& a) {
        return a.back == back - 4 && a.target == target && a.reg == reg;
    });
}
std::uint64_t accepted_seen = 0;
constexpr unsigned wrong_return = 200, wrong_count = 201, wrong_order = 202, poke_missed = 203;
bool trace_corrupted = false, arguments_corrupted = false;
std::map<std::tuple<std::uint32_t, std::uint32_t, unsigned>, TraceFinding> trace_findings;
std::uint64_t pokes_fired = 0, pokes_unfired = 0;
std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint64_t> traced_sites;
std::uint64_t traced_pairs = 0;
void compare_traces(State& s, std::uint32_t entry) {
    const auto original = trace_run(s, entry, bounded_original);
    auto rewrite = trace_run(s, entry, bounded_rewrite);
    if (std::getenv("CM_FONT_CPP_TRACE_DUMP"))
        for (const std::vector<TracedCall>* side : {static_cast<const std::vector<TracedCall>*>(&original), static_cast<const std::vector<TracedCall>*>(&rewrite)}) {
            std::cerr << (side == static_cast<const std::vector<TracedCall>*>(&original) ? "TRACE original" : "TRACE rewrite") << " [" << current_label << "] " << side->size() << " calls:";
            for (const auto& call : *side) std::cerr << ' ' << hex_address(call.back) << (call.after_bl ? ">" : "!") << hex_address(call.target);
            std::cerr << '\n';
        }
    using Site = std::pair<std::uint32_t, std::uint32_t>;
    std::map<Site, std::size_t> original_calls;
    for (const auto& call : original) ++original_calls[{call.back, call.target}];
    if (options.corrupt_trace && !trace_corrupted) {
        // The control: out of order, one call too many, and one returning nowhere a bl does.
        const auto shown = [&](const TracedCall& call) { return original_calls.count({call.back, call.target}) != 0; };
        const auto first = std::ranges::find_if(rewrite, shown);
        const auto second = std::find_if(first, rewrite.end(), [&](const TracedCall& call) {
            return shown(call) && (call.back != first->back || call.target != first->target);
        });
        if (second != rewrite.end()) {
            std::iter_swap(first, second);
            auto extra = *first;
            rewrite.push_back(extra);
            extra.target += 4;
            extra.after_bl = false;
            rewrite.push_back(extra);
            trace_corrupted = true;
        }
    }
    std::map<Site, std::vector<const TracedCall*>> sites;
    for (const auto& call : rewrite) sites[{call.back, call.target}].push_back(&call);
    const auto record = [&](Site site, unsigned kind, std::uint64_t left, std::uint64_t right) {
        auto& finding = trace_findings[{site.first, site.second, kind}];
        if (!finding.count++) { finding.original = left; finding.rewrite = right; }
        if (options.show_faults) std::cerr << "trace finding in [" << current_label << "]: " << left << " against " << right << '\n';
    };
    // A truncated trace still pairs its calls (both record the same prefix), but how many
    // calls each site made and the order of the rest are unknown.
    const bool truncated = original.size() >= trace_limit || rewrite.size() >= trace_limit;
    truncated_traces += truncated;
    // A case's poke must fire as often on each side: on one side only, the two bodies ran from
    // different states. One that fires on neither (the path ended before its call) is counted and
    // printed: that case passes without testing what its label says. Only the calls the routine
    // makes itself are traced, which is where a poke belongs.
    if (case_pokes && !truncated) for (const auto& poke : *case_pokes) {
        const auto fired = [&](const std::vector<TracedCall>& calls) {
            return std::uint64_t(std::ranges::count(calls, poke.entry, &TracedCall::target));
        };
        const auto left = fired(original), right = fired(rewrite);
        if (left != right) record({0, poke.entry}, poke_missed, left, right);
        else if (left == 0) ++pokes_unfired;
        else ++pokes_fired;
    }
    for (const auto& [site, calls] : sites) {
        if (truncated) break;
        const auto seen = original_calls.find(site);
        if (seen == original_calls.end()) {
            // An accepted wrong return is a case's stand-in for an import (font_cpp_trace_accepted.inc).
            if (!calls.front()->after_bl && !accepted_finding(site.first, site.second, wrong_return))
                record(site, wrong_return, 0, calls.size());
        } else if (seen->second != calls.size()) {
            // A re-entry into the module's own generated code (a return the dispatcher resumes: no `bl` before it) happens only where generated code ran, so its count
            // differs by side wherever one side's checked call is C++ and a case's preparation ran generated code on both.
            if (!calls.front()->after_bl && site.first == site.second && in_module(site.second)) continue;
            record(site, wrong_count, seen->second, calls.size());
        }
    }
    // A call the original makes and the rewrite does not (an answered assertion or longjmp leaves nothing else to compare). Calls to the
    // module's own routines are left out: the rewrite makes those directly, so they never reach the dispatcher.
    if (!truncated) for (const auto& [site, count] : original_calls) {
        if (sites.count(site) || in_module(site.second)) continue;
        const auto made = std::ranges::count_if(original, [&](const TracedCall& call) { return call.back == site.first && call.target == site.second && call.after_bl; });
        if (made) record(site, wrong_count, std::uint64_t(made), 0);
    }
    const auto next_shown = [&](const std::vector<TracedCall>& calls, std::size_t& at, const auto& shown) {
        while (at < calls.size() && !shown(Site{calls[at].back, calls[at].target})) ++at;
        return at < calls.size();
    };
    // (The dispatcher's resumptions of the module's own generated code, where the target is the return address, are left out of the order as of the counts.)
    const auto both = [&](Site site) { return site.first != site.second && sites.count(site) != 0 && original_calls.count(site) != 0; };
    // A truncated trace's order past the shared calls is unknown too (a native routine's direct calls are not
    // traced, so the two prefixes cover different stretches of the run).
    for (std::size_t a = 0, b = 0; !truncated && next_shown(original, a, both) && next_shown(rewrite, b, both); ++a, ++b)
        if (original[a].back != rewrite[b].back || original[a].target != rewrite[b].target) {
            record({original[a].back, original[a].target}, wrong_order, rewrite[b].back, rewrite[b].target);
            break;
        }
    std::map<Site, std::size_t> next;
    for (const auto& call : original) {
        const auto key = std::make_pair(call.back, call.target);
        if (!call.after_bl) continue;
        const auto found = sites.find(key);
        if (found == sites.end()) continue;
        auto& k = next[key];
        if (k >= found->second.size()) continue;
        auto other = *found->second[k++];
        if (options.corrupt_trace && !traced_pairs) other.gprs[0] ^= 1;
        // The control for the arguments: the first call to a target known to read r3.
        const auto read_by_target = argument_masks.find(call.target);
        if (options.corrupt_trace && !arguments_corrupted && read_by_target != argument_masks.end() && (read_by_target->second & 1)) {
            other.arguments[0] ^= 1;
            arguments_corrupted = true;
        }
        ++traced_pairs; ++traced_sites[key];
        if (read_by_target != argument_masks.end()) for (unsigned n = 0; n < 8; ++n)
            if ((read_by_target->second >> n & 1) && call.arguments[n] != other.arguments[n]) {
                if ((font_cpp_native(entry) || font_cpp_native_code(call.back - 4)) &&
                    accepted_finding(call.back, call.target, 3 + n)) { ++accepted_seen; continue; }
                auto& finding = trace_findings[{call.back, call.target, 3 + n}];
                if (!finding.count++) { finding.original = call.arguments[n]; finding.rewrite = other.arguments[n]; }
            }
        // Native-first under the per-reader contract: only the arguments a callee reads are compared. The
        // stack audit's readers of what this unit leaves (tools/stack_contract.py module FONT_CPP.CPP; the brief
        // lists them) are outside the routines' own calls: this trace cannot see them, the paired replays do.
        // Calls that can reach an import hold the original's r13-r31 for the replay's import comparison.
    }
}
void print_trace_findings() {
    // --show-sites: each paired call site of the run (the address after its `bl`, the callee) and how many pairs it had.
    if (options.show_sites)
        for (const auto& [site, pairs] : traced_sites) std::cout << "site return " << hex_address(site.first) << " -> " << hex_address(site.second) << " pairs=" << pairs << '\n';
    std::cout << "call_trace_pairs=" << traced_pairs << " sites=" << traced_sites.size() << " findings=" << trace_findings.size()
              << " truncated_traces=" << truncated_traces << " accepted_argument_differences=" << accepted_seen << '\n';
    if (pokes_fired || pokes_unfired)
        std::cout << "pokes_fired=" << pokes_fired << " pokes_unfired=" << pokes_unfired << " (unfired: the case's path ended before the poked call)\n";
    for (const auto& [key, finding] : trace_findings) {
        const auto& [back, target, reg] = key;
        if (reg >= wrong_return) {
            std::cout << "call " << hex_address(back - 4) << " -> " << hex_address(target)
                      << (reg == wrong_return ? " returns where no bl of the original does" : reg == wrong_count ? " count"
                          : reg == poke_missed ? " a case's poke did not fire alike on both sides: times fired" : " out of order")
                      << " x" << finding.count << " original " << std::hex << finding.original << " rewrite " << finding.rewrite << std::dec << '\n';
            continue;
        }
        std::cout << "call " << hex_address(back - 4) << " -> " << hex_address(target) << (reg >= 100 ? " f" : " r") << (reg >= 100 ? reg - 100 : reg)
                  << " x" << finding.count << " original " << std::hex << finding.original << " rewrite " << finding.rewrite << std::dec << '\n';
    }
}

void report_difference(std::uint32_t entry, std::string_view kind, const std::vector<std::string>& lines, std::string_view detail = {}) {
    std::cerr << kind << " difference at " << hex_address(entry) << " [" << current_label << "]";
    if (!detail.empty()) std::cerr << " (" << detail << ")";
    std::cerr << ":\n";
    for (const auto& line : lines) std::cerr << "  " << line << '\n';
    throw std::runtime_error("FONT_CPP.CPP routine differs from the original");
}

// Repeats the call now due with changed inputs; undoes them afterwards.
void mutate_call(State& s, std::uint32_t entry, unsigned results, std::mt19937& random) {
    if (!options.mutate) return;
    struct Guard { Guard() { mutating = true; run_alarm = 8; } ~Guard() { mutating = false; run_alarm = 60; } } guard;
    const bool prepared = case_prepare && *case_prepare;
    auto reads = reads_of(s, entry);
    std::vector<Read> candidates;
    for (const auto& read : reads) {
        if (read.address < 0x108b3a90 || !mapped(s, read.address)) continue;  // immutable code-segment constants
        if (read.size != 1 && read.size != 2 && read.size != 4 && read.size != 8) continue;
        if (read.size == 4) {
            const auto value = prepared ? std::uint32_t(read.value) : std::uint32_t(s.memory.read<4>(read.address));
            if (value >= 0x10000000 && mapped(s, value)) continue;  // a pointer: invalid ones prove nothing
        }
        candidates.push_back(read);
    }
    if (candidates.empty()) return;
    auto& tally = mutated_tally[entry];
    const auto saved_observer = s.memory.observer;
    s.memory.observer = nullptr;
    for (unsigned variant = 0; variant < options.mutate; ++variant) {
        std::vector<std::pair<Read, std::uint64_t>> undo;
        std::ostringstream detail;
        for (unsigned n = 0, count = 1 + random() % 3; n < count; ++n) {
            const auto read = candidates[random() % candidates.size()];
            const auto load = [&]() -> std::uint64_t {
                switch (read.size) { case 1: return s.memory.read<1>(read.address); case 2: return s.memory.read<2>(read.address);
                                     case 4: return s.memory.read<4>(read.address); default: return s.memory.read<8>(read.address); }
            };
            const auto store = [&](std::uint64_t value) {
                switch (read.size) { case 1: s.memory.write<1>(read.address, value & 0xff); break; case 2: s.memory.write<2>(read.address, value & 0xffff); break;
                                     case 4: s.memory.write<4>(read.address, std::uint32_t(value)); break; default: s.memory.write<8>(read.address, value); }
            };
            const std::uint64_t before = prepared ? read.value : load();
            undo.emplace_back(read, before);
            const auto value = replacement(read, before, random);
            if (prepared) pending_mutations.emplace_back(read.address, read.size, value);
            else store(value);
            detail << hex_address(read.address) << ":" << read.size << "=" << std::hex << value << std::dec << " ";
        }
        const auto outcome = compare_call(s, entry, results, options.strict);
        pending_mutations.clear();
        for (auto item = undo.rbegin(); item != undo.rend(); ++item) {
            if (prepared) break;
            const auto& [read, value] = *item;
            switch (read.size) { case 1: s.memory.write<1>(read.address, value & 0xff); break; case 2: s.memory.write<2>(read.address, value & 0xffff); break;
                                 case 4: s.memory.write<4>(read.address, std::uint32_t(value)); break; default: s.memory.write<8>(read.address, value); }
        }
        if (outcome.faulted && options.show_faults) std::cerr << "  mutation: " << detail.str() << '\n';
        ++tally.calls;
        tally.returned += outcome.returned; tally.faulted += outcome.faulted; tally.timed_out += outcome.timed_out;
        if (!outcome.differences.empty()) {
            s.memory.observer = saved_observer;
            report_difference(entry, "Mutated", outcome.differences, detail.str());
        }
    }
    s.memory.observer = saved_observer;
}

// Whether an entry reached through its generated page runs as C++ in the normal app: with no
// instruction budget, the page's first instruction would stop. So the routine must return, or
// have written its saved LR before a dependency's generated code stops (the prologue is the
// first thing the C++ does). The control is the same call declining to the generated body,
// which must stop before writing anything.
enum class Route { returned, framed, stopped, faulted };
Route route_of(State& s, std::uint32_t entry, bool original) {
    const auto pid = ::fork();
    if (pid < 0) throw std::runtime_error("fork");
    if (pid == 0) {
        guest::Cpu c(s);
        routine_checker = nullptr;
        dispatch_probe = nullptr;
        font_cpp_declining = original;
        for (unsigned n = 3; n <= 10; ++n) c.set(n, 0);
        // A UI snapshot has no r2 (it is taken at an import boundary): the game's TOC, as at any call, or every TOC read faults at once.
        c.set(2, game_toc);
        c.set_lr(return_sentinel);
        const auto slot = c.sp() + 8;
        s.memory.observer = nullptr;
        s.memory.write<4>(slot, 0xdeadbeef);
        s.pc = entry;
        s.budget = s.executed;
        alarm(20);
        try {
            run_until(s, return_sentinel);
            _exit(int(Route::returned));
        } catch (const std::runtime_error& error) {
            // A guest fault on this artificial argument, not a stop at a generated instruction.
            if (std::string_view(error.what()) != "Native instruction budget exhausted") _exit(int(Route::faulted));
        } catch (...) {}
        _exit(int(s.memory.read<4>(slot) == return_sentinel ? Route::framed : Route::stopped));
    }
    int status = 0;
    ::waitpid(pid, &status, 0);
    if (!WIFEXITED(status)) return Route::stopped;
    return Route(WEXITSTATUS(status));
}
int production_proof(State& s) {
    unsigned returned = 0, framed = 0, controls = 0, failures = 0;
    const auto entries = font_cpp_production_entries();
    for (const auto entry : entries) {
        const auto route = route_of(s, entry, false);
        if (route == Route::returned || route == Route::faulted) ++returned;
        else if (route == Route::framed) ++framed;
        else { std::cerr << "Generated code ran at " << hex_address(entry) << '\n'; ++failures; }
        const auto control = route_of(s, entry, true);
        if (control == Route::stopped) ++controls;
        else { std::cerr << "The original control did not stop at " << hex_address(entry) << '\n'; ++failures; }
    }
    std::cout << "production entries=" << entries.size() << " returned_on_no_budget=" << returned << " framed_before_a_dependency=" << framed
              << " original_controls_stopped=" << controls << " failures=" << failures << '\n';
    // Until the module is complete, --require-all is left out (a block at a time).
    return failures || (options.require_all && entries.size() != module_routines) ? 1 : 0;
}

// A native-first routine leaves only the results its line declares; `tools/register_contract.py
// --after ENTRY` shows what its callers read. A routine written to the full image leaves f1 as the
// original does on every path, and r3 too unless its line declares no result: there r3 is what its
// last callee left, and once that callee is native-first it differs (the daily update 0x10196390
// ends with a call of 0x101c0fc0). r3 is never spilled as a leftover: every call sets it.
unsigned results_of(std::uint32_t entry) {
    const auto declared = font_cpp_results(entry);
    if (font_cpp_native(entry)) return declared;
    return (declared & RoutineChecker::result_r3) | RoutineChecker::result_f1;
}

// A case's Case::original_owners for its compared runs, mutations and traces, undone at the end of the case.
struct OriginalOwners {
    State& s;
    const decltype(State::stream_io_entry_original) streams;
    const decltype(State::allocator_entry_original) allocator;
    OriginalOwners(State& state, const std::vector<std::uint32_t>& entries)
        : s(state), streams(state.stream_io_entry_original), allocator(state.allocator_entry_original) {
        for (const auto entry : entries) {
            if (const auto at = std::ranges::find(stream_io_entries, entry); at != stream_io_entries.end())
                s.stream_io_entry_original[std::size_t(at - stream_io_entries.begin())] = true;
            else if (const auto in = std::ranges::find(allocator_entries, entry); in != allocator_entries.end())
                s.allocator_entry_original[std::size_t(in - allocator_entries.begin())] = true;
            else throw std::runtime_error("Case::original_owners: no original switch for " + hex_address(entry));
        }
    }
    ~OriginalOwners() { s.stream_io_entry_original = streams; s.allocator_entry_original = allocator; }
    OriginalOwners(const OriginalOwners&) = delete;
    OriginalOwners& operator=(const OriginalOwners&) = delete;
};

void run_cases(State& s, std::mt19937& random) {
    Scenario scenario{s};
    std::vector<Case> cases;
    // While writing cases: CM_FONT_CPP_BLOCK=name (or name,name: several blocks, in registration order, sharing the run's random draws) runs those blocks, and
    // CM_FONT_CPP_LABEL=text keeps the cases whose label contains it. A filtered run says
    // so, and one that selects nothing is an error: it would read as a pass.
    const char* only_block = std::getenv("CM_FONT_CPP_BLOCK");
    const char* only_label = std::getenv("CM_FONT_CPP_LABEL");
    for (const auto& [name, list] : blocks())
        if (!only_block || (std::string(",") + only_block + ",").find(std::string(",") + name + ",") != std::string::npos) list(scenario, cases);
    if (only_label) std::erase_if(cases, [&](const Case& item) { return item.label.find(only_label) == std::string::npos; });
    if (only_block || only_label) {
        std::cerr << "FILTERED RUN: " << cases.size() << " cases (CM_FONT_CPP_BLOCK/LABEL); not a full check\n";
        if (cases.empty()) throw std::runtime_error("The case filter selects no case");
    }
    guest::Cpu c(s);
    std::cerr << "state: sp " << hex_address(c.sp()) << " pc " << hex_address(s.pc) << " toc " << hex_address(c.r(2)) << "; mapped:";
    for (const auto& [low, size] : s.memory.mapped_ranges()) std::cerr << ' ' << hex_address(low) << '+' << std::hex << size << std::dec;
    std::cerr << '\n';
    const auto saved = s.registers;
    const auto saved_observer = s.memory.observer;
    for (const auto& item : cases) {
        if (!options.only.empty() && !options.only.count(item.entry)) continue;
        s.registers = saved;
        // A UI snapshot is taken at an import boundary, with r2 not in the register file: the game's TOC, as at any call.
        c.set(2, game_toc);
        // The callers' non-volatile registers, distinct and nonzero, as in a real call: a
        // rewrite that clobbers one before the original would shows at its first call or return.
        for (unsigned n = 13; n < 32; ++n) c.set(n, 0x5a000000u + n * 0x10101u);
        for (unsigned n = 14; n < 32; ++n) c.set_f(n, 1000.0 + n * 3.25);
        for (const auto& [n, value] : item.registers) c.set(n, value);
        if (item.f1) c.set_f(1, *item.f1);
        if (item.clear_fpscr) for (unsigned byte = 1280; byte < 1312; ++byte) s.registers[byte] = 0;
        c.set_lr(return_sentinel);
        s.pc = item.entry;
        std::vector<std::pair<Write, std::uint32_t>> undo;
        s.memory.observer = nullptr;
        for (const auto& write : item.memory) {
            const auto before = write.size == 1 ? s.memory.read<1>(write.address) : write.size == 2 ? s.memory.read<2>(write.address) : s.memory.read<4>(write.address);
            undo.emplace_back(write, std::uint32_t(before));
            store(s, write, write.value);
        }
        s.memory.observer = saved_observer;
        current_label = item.label;
        const OriginalOwners owners(s, item.original_owners);
        case_answers = &item.answers;
        case_pokes = &item.pokes;
        case_captures = &item.captures;
        case_nth_answers = &item.nth_answers;
        case_verify = &item.verify;
        case_prepare = &item.prepare;
        const auto outcome = compare_call(s, item.entry, results_of(item.entry), options.strict);
        auto& tally = case_tally[item.entry];
        ++tally.calls; tally.returned += outcome.returned; tally.faulted += outcome.faulted; tally.timed_out += outcome.timed_out;
        if (!outcome.differences.empty()) report_difference(item.entry, "Case", outcome.differences, item.label);
        // Every case with a poke is traced: the trace is what shows that the poke fired.
        if (options.call_trace && (tally.calls <= options.mutated_calls * 8 || tally.calls % 64 == 0 || !item.pokes.empty()))
            compare_traces(s, item.entry);
        // One routine can have thousands of cases; a few of them seed its mutations.
        if (tally.calls <= options.mutated_calls * 8 || tally.calls % 64 == 0) mutate_call(s, item.entry, results_of(item.entry), random);
        s.memory.observer = nullptr;
        for (auto w = undo.rbegin(); w != undo.rend(); ++w) {
            store(s, w->first, w->second);
        }
        s.memory.observer = saved_observer;
        case_answers = nullptr;
        case_pokes = nullptr;
        case_captures = nullptr;
        case_nth_answers = nullptr;
        case_verify = nullptr;
        case_prepare = nullptr;
    }
    s.registers = saved;
}

// Whether every routine in production returned from a comparison. A routine no case
// and no live call reached has no evidence, however clean the rest of the run is.
bool summary(const RoutineChecker* checker) {
    std::uint64_t live = 0, listed = 0, returned = 0, faulted = 0, timed = 0, cases = 0, case_faults = 0;
    std::set<std::uint32_t> reached;
    // The other modules' routines are checked on the way too; only this one's count.
    if (checker) for (const auto& [entry, totals] : checker->totals()) if (font_cpp_lists(entry)) { live += totals.checked; if (totals.checked) reached.insert(entry); }
    for (const auto& [entry, tally] : mutated_tally) { returned += tally.returned; faulted += tally.faulted; timed += tally.timed_out; if (tally.returned) reached.insert(entry); }
    for (const auto& [entry, tally] : case_tally) { cases += tally.returned; case_faults += tally.faulted; if (tally.returned) reached.insert(entry); }
    listed = reached.size();
    std::cout << "live_comparisons=" << live << " case_returned=" << cases << " case_faults=" << case_faults
              << " mutated_returned=" << returned << " mutated_faults=" << faulted << " mutated_timeouts=" << timed
              << " entries_compared=" << listed << " ended_alike=" << ended_alike << " ended_one_sided=" << ended_alone << " original_slower=" << slow_original << '\n';
    if (const char* file = std::getenv("CM_FONT_CPP_REPORT")) {
        std::ofstream out(file);
        if (checker) for (const auto& [entry, totals] : checker->totals()) if (font_cpp_lists(entry))
            out << "live " << hex_address(entry) << " calls=" << totals.calls << " checked=" << totals.checked << '\n';
        for (const auto& [entry, tally] : case_tally) out << "case " << hex_address(entry) << " returned=" << tally.returned << " faulted=" << tally.faulted << " timeout=" << tally.timed_out << '\n';
        for (const auto& [entry, tally] : mutated_tally) out << "mutated " << hex_address(entry) << " returned=" << tally.returned << " faulted=" << tally.faulted << " timeout=" << tally.timed_out << '\n';
    }
    if (!options.require_all) return true;
    bool complete = true;
    for (const auto entry : font_cpp_production_entries())
        if (!reached.count(entry)) { std::cerr << "No comparison reached " << hex_address(entry) << '\n'; complete = false; }
    return complete;
}
}

int main(int argc, char** argv) {
    const auto usage = "usage: cm-font-cpp-checks [--world SNAPSHOT] [--all N] [--every N] [--mutate K] [--mutated-calls N] [--seed N] [--only 0x..,0x..] [--corrupt] [--no-cases] [--strict] [--require-all] [--show-faults] [--call-trace] [--show-sites] [--corrupt-trace] [--argument-masks FILE] [--null-page] [--production]\n";
    try {
        for (int n = 1; n < argc; ++n) {
            const std::string_view option(argv[n]);
            const auto value = [&] { if (n + 1 >= argc) throw std::runtime_error("missing value"); return std::string(argv[++n]); };
            if (option == "--world") options.world = value();
            else if (option == "--all") options.all = unsigned(std::stoul(value()));
            else if (option == "--every") options.every = unsigned(std::stoul(value()));
            else if (option == "--mutate") options.mutate = unsigned(std::stoul(value()));
            else if (option == "--mutated-calls") options.mutated_calls = unsigned(std::stoul(value()));
            else if (option == "--seed") options.seed = std::uint32_t(std::stoul(value()));
            else if (option == "--only") { std::stringstream list(value()); for (std::string item; std::getline(list, item, ',');) options.only.insert(std::uint32_t(std::stoul(item, nullptr, 16))); }
            else if (option == "--corrupt") options.corrupt = true;
            else if (option == "--no-cases") options.cases = false;
            else if (option == "--strict") options.strict = true;
            else if (option == "--require-all") options.require_all = true;
            else if (option == "--show-faults") options.show_faults = true;
            else if (option == "--call-trace") options.call_trace = true;
            else if (option == "--show-sites") options.show_sites = options.call_trace = true;
            else if (option == "--corrupt-trace") options.call_trace = options.corrupt_trace = true;
            else if (option == "--production") options.production = true;
            // Maps low memory, so a path through a null pointer runs on as on the original machine
            // and returns to be compared, instead of counting as a fault on both sides.
            else if (option == "--null-page") { options.null_page = true; ::setenv("CM_FONT_CPP_NULL_PAGE", "1", 1); }
            else if (option == "--argument-masks") {
                std::ifstream masks(value());
                if (!masks) throw std::runtime_error("Missing argument masks");
                std::string target, mask;
                while (masks >> target >> mask) argument_masks[std::uint32_t(std::stoul(target, nullptr, 16))] = unsigned(std::stoul(mask, nullptr, 16));
            }
            else throw std::runtime_error("Unknown option " + std::string(option));
        }
        if (options.world.empty()) { std::cerr << usage; return 2; }
        routine_child_exit = flush_child;
        set_match_turbo_enabled(false);
        // The assertion's error dialog needs Toolbox imports a snapshot has no host for.
        // Both sides answer it here, so what follows the assert is compared instead of
        // stopping at the first dialog import.
        scripted_services = [](State& target) {
            // exit(): nothing follows it in the game, so a run ends here and what it
            // wrote up to this call is compared.
            constexpr std::uint32_t exit_entry = 0x1076f7d0, dialog_entry = 0x10769b60;
            if (target.pc == exit_entry) throw ApplicationExit{};
            // A case's answers for another module's routine, given on both sides. A routine
            // under check is never answered: its C++ doesn't come through here, so only the
            // original would get the answer.
            if (case_pokes) for (const auto& poke : *case_pokes)
                if (target.pc == poke.entry && !(routine_checker && routine_checker->wants(poke.entry)))
                    for (const auto& write : poke.writes) store(target, write, write.value);
            if (case_captures) for (const auto& capture : *case_captures)
                if (target.pc == capture.entry && !(routine_checker && routine_checker->wants(capture.entry))) {
                    const auto from = std::uint32_t(target.r<4>(guest::reg::gpr(capture.reg)));
                    for (std::uint32_t at = 0; at < capture.size; ++at) target.memory.write<1>(capture.to + at, target.memory.read<1>(from + at));
                }
            if (case_nth_answers && !(routine_checker && routine_checker->wants(target.pc))) {
                bool counted = false;
                for (const auto& item : *case_nth_answers) {
                    if (target.pc != item.entry) continue;
                    if (!counted) { ++nth_dispatches[item.entry]; counted = true; }
                    if (nth_dispatches[item.entry] != item.nth) continue;
                    record_answered(target);
                    guest::Cpu(target).set(3, item.answer);
                    target.pc = target.r<4>(4128);
                    return true;
                }
            }
            if (case_answers) for (const auto& [entry, answer] : *case_answers)
                if (target.pc == entry && !(routine_checker && routine_checker->wants(entry))) {
                    record_answered(target);
                    guest::Cpu(target).set(3, answer);
                    target.pc = target.r<4>(4128);
                    return true;
                }
            if (target.pc != dialog_entry) return false;
            target.pc = target.r<4>(4128);
            return true;
        };
        dispatch_probe = scripted_services;
        std::mt19937 random(options.seed + 1);
        // Other modules' routines run as their originals or their C++ alike on both sides:
        // left out, this module's hooks decline only inside the original runs.
        const auto wanted = [](std::uint32_t entry) {
            // A routine a case answers is not run in that case, so it is not under check there (the answer would reach one side only).
            if (case_answers) for (const auto& answer : *case_answers) if (answer.first == entry) return false;
            if (case_nth_answers) for (const auto& answer : *case_nth_answers) if (answer.entry == entry) return false;
            return font_cpp_lists(entry) && (options.only.empty() || options.only.count(entry) != 0);
        };
        RoutineChecker checker(wanted, options.all, options.every);
        checker.corrupt_first = options.corrupt;
        checker.compare_f1 = true;
        checker.strict_volatile = options.strict;
        int status = 0;
        if (!options.world.empty()) {
            auto state = std::make_unique<State>();
            load_match_snapshot(*state, options.world);
            state->memory.map(scratch_base, scratch_size);  // for a case's own rows; nothing of the game's lives here
            if (options.null_page) state->memory.map(0, 0x10000);
            state->code_fragment_observer = &inputs;
            if (options.production) status |= production_proof(*state);
            else if (options.cases) {
                routine_checker = &checker;
                run_cases(*state, random);
                routine_checker = nullptr;
            }
        }
        if (options.production) return status;
        const bool complete = summary(&checker);
        if (options.call_trace) print_trace_findings();
        if (options.corrupt_trace) {
            std::set<unsigned> kinds;
            for (const auto& [key, finding] : trace_findings) kinds.insert(std::get<2>(key) < 100 ? 0 : std::get<2>(key));
            return kinds.count(wrong_return) && kinds.count(wrong_count) && kinds.count(wrong_order) && kinds.count(0) ? 0 : 1;
        }
        if (options.corrupt) { std::cerr << "The negative control found no difference\n"; return 1; }
        if (*expectation_failures) { std::cerr << *expectation_failures << " expectation(s) failed\n"; return 1; }
        return checker.differences() || !complete || !trace_findings.empty() ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return (options.corrupt) && std::string_view(error.what()).find("differs") != std::string_view::npos ? 0 : 2; }
}
