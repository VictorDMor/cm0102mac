#pragma once
// The C++ SHORTLIST_MANAGER.CPP (shortlist_manager_routines.inc) as a third
// side of the shortlist families' fixture checks (club_shortlist_storage_checks.cpp,
// shortlist_links_checks.cpp, ...). Those programs step the original
// instructions (the reference) against an older builder owner, with every
// call into other code answered by a scripted dependency model. Here the
// same cases also run the new C++ (shortlist_manager_run_rewrite) against a
// second reference run:
// - The C++ reaches other code through Program (Program::call, and
//   Program::inlined for the callees it reproduces in place) or Cpu::call;
//   the adapter's call hook and dispatch_probe hand every such call to the
//   case's model (rewrite_side runs the C++ under Scope).
// - A group's C++ calls the routines of its own group directly
//   (detail::enter_direct), so the model never sees those calls; the hook
//   records their return addresses. At those, the reference runs the callee
//   too instead of a stand-in (the family's original when the family has
//   it, else the callee's C++ under the same model), and those calls are
//   not compared.
// - Compared, as docs/cpp-rewrites.md defines equivalence: the outcome (a
//   return, or the same stop with the same message); every mapped byte
//   (stack image included) and the allocator tables; at a return r1, r2,
//   r4-r10, r13-r31, r3 for routines returning it, f1, f14-f31, FPSCR,
//   every CR field and the pc; at a model's stop inside the routine (an
//   interruption, an exit) the same without r4-r10 and the CR; the calls
//   into the rest of the program in order, at the same return addresses,
//   with the preserved registers and the callee's arguments before (r3-r10
//   and f1 unless the callee is known not to read them, non_arguments; f2-f13
//   where it takes them) and r3, f1 and the preserved registers after.
// - Not compared: r0, r11, r12, CTR, XER, LR, f0, f2-f13 and Ghidra's
//   temporaries (the C++ does not reproduce them), instruction counts, the
//   builder diagnostics (step sites, access order), the state at a fault
//   (the C++ holds the original's state at calls and returns only, so
//   after an unmapped access only the message is compared), calls to the
//   compiler's helpers that the C++ inlines (transparent), and the
//   reference's calls at direct sites.
// - Not run: cases that stop the reference by instruction budget, that
//   inject registers at instruction addresses or change the original's
//   jump tables or constants (the C++ has none of these), negative controls
//   that write a byte and restore it (only an ordered-effect comparison sees
//   them), roots outside the module; and cases where only the original
//   faults on an unmapped TOC word (it loads them in its prologues) or
//   constant (the C++ has the value).
// - The models' stand-ins overwrite every volatile register; for these runs
//   they leave alone what the real callee keeps where the C++ relies on it
//   (Side::clobbers).
#include "semantic_io_checks.hpp"
#include "routine_check.hpp"
#include "shortlist_manager_adapter.hpp"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace cm::shortlist_rewrite_fixture {
using namespace cm::semantic_io_checks;
using Registers = decltype(State::registers);
using Answer = std::function<void(State&, std::uint32_t target, std::uint32_t stop)>;

inline bool module_routine(std::uint32_t entry) { return shortlist_manager_lists(entry); }
// A stop the code itself makes (an unmapped access, an enabled floating-point
// exception), as opposed to a model's (an interruption, an exit).
inline bool fault(const std::string& outcome) {
    return outcome.starts_with("Unmapped data access at ") || outcome.starts_with("Original enabled floating");
}

// The return addresses of the direct C++ calls (detail::enter_direct) the
// case's C++ run made: the reference runs those callees there too instead
// of a stand-in. The run records them itself, so a call the C++ skipped is
// a call only in the reference. The seen_ sets keep every case's.
inline std::set<std::uint32_t> direct_backs, seen_direct, seen_dispatched;

// While a rewrite runs: Program's calls (the adapter's hook) and Cpu::call's
// (dispatch_probe) all reach the model.
inline Answer answer;
inline bool probe(State& s) {
    if (!answer) return false;
    answer(s, s.pc, std::uint32_t(s.r<4>(4128)));
    return true;
}
struct Hook final : ShortlistManagerCallHook {
    bool call(State& s) override {
        seen_dispatched.insert(std::uint32_t(s.r<4>(4128)));
        return probe(s);
    }
    void direct(std::uint32_t back) override {
        direct_backs.insert(back);
        seen_direct.insert(back);
    }
};
inline Hook hook;
class Scope {
public:
    explicit Scope(Answer a)
        : previous_answer(std::move(answer)), previous_probe(dispatch_probe), previous_hook(shortlist_manager_call_hook) {
        answer = std::move(a);
        dispatch_probe = probe;
        shortlist_manager_call_hook = &hook;
    }
    ~Scope() {
        answer = std::move(previous_answer);
        dispatch_probe = previous_probe;
        shortlist_manager_call_hook = previous_hook;
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    Answer previous_answer;
    bool (*previous_probe)(State&);
    ShortlistManagerCallHook* previous_hook;
};
// The model's call(), as the answer to the C++'s calls.
template <class Model> Answer answer_of(Model& model) {
    return [&model](State& s, std::uint32_t target, std::uint32_t stop) { model.call(s, target, stop); };
}
// A model's answer to a call of a module routine it composes: the routine's
// C++ from the call (LR holds the return address) to its return.
inline void compose(State& s, std::uint32_t target, Answer a) {
    Scope scope(std::move(a));
    s.pc = target;
    shortlist_manager_run_rewrite(s, target);
}

// How a fixture model serves the two runs compared here. `reference`: the
// original's run compared with the C++ (composes the direct sites);
// `rewrite`: the C++'s run. Only the calls that line up count, also for a
// case's interruption after its n-th call.
struct Side {
    bool reference = false, rewrite = false;
    const std::set<std::uint32_t>* transparent = nullptr;
    unsigned counted = 0;
    bool active() const { return reference || rewrite; }
    bool composes(std::uint32_t, std::uint32_t stop) const { return reference && direct_backs.contains(stop); }
    bool counts(std::uint32_t target, std::uint32_t stop) const {
        return !(transparent && transparent->contains(target)) && !composes(target, stop);
    }
    // At a call: whether it counts (and count it).
    bool enter(std::uint32_t target, std::uint32_t stop) {
        const bool now = counts(target, stop);
        counted += now;
        return now;
    }
    // Which volatile registers a stand-in overwrites. The models overwrite
    // r0 and r3-r12 (and some f0-f13) at every call; for the runs compared
    // with the C++ they leave alone what the real callee keeps, where the C++
    // relies on it: rand (0x1077b770) is a leaf that writes only r0, r3, r4;
    // pow (0x10780fc0) and sqrt (0x10780fd8), CarbonLib through their glues,
    // only f1 (carbon_bridge.cpp).
    static constexpr std::uint32_t rand = 0x1077b770, pow = 0x10780fc0, sqrt = 0x10780fd8;
    bool clobbers(std::uint32_t target, unsigned gpr) const {
        if (!active()) return true;
        if (target == rand) return gpr == 0 || gpr == 3 || gpr == 4;
        return target != pow && target != sqrt;
    }
    bool clobbers_fprs(std::uint32_t target) const {  // f0 and f2-f13
        return !(active() && (target == rand || target == pow || target == sqrt));
    }
    bool writes_f1(std::uint32_t target) const { return !(active() && target == rand); }
    // Whether the model keeps its own record of the call in guest memory (a
    // call counter, the last target): not for calls the C++ does not make.
    bool records(std::uint32_t target, std::uint32_t stop) const {
        return !composes(target, stop) && !(active() && transparent && transparent->contains(target));
    }
    // After a call: whether a case interrupted after `interrupt` calls stops here.
    bool interrupts(bool now, unsigned interrupt, std::size_t calls) const {
        if (!interrupt) return false;
        return active() ? now && counted >= interrupt : calls >= interrupt;
    }
};

struct Call {
    std::uint32_t target, stop;
    Registers before, after;
};
template <class Event> std::vector<Call> calls_of(const std::vector<Event>& events) {
    std::vector<Call> calls;
    for (const auto& e : events) calls.push_back({e.target, e.stop, e.before, e.after});
    return calls;
}
struct Run {
    std::string outcome;
    std::unique_ptr<State> state;
    std::vector<Call> calls;
};

inline std::string hex(std::uint64_t value) {
    char text[24];
    std::snprintf(text, sizeof text, "%llx", static_cast<unsigned long long>(value));
    return text;
}
// Registers compared: `gprs` and `fprs` are bit sets (bit n: rN / fN).
constexpr std::uint32_t preserved_gprs = 0xffffe006u;  // r1, r2, r13-r31
constexpr std::uint32_t preserved_fprs = 0xffffc000u;  // f14-f31
constexpr std::uint32_t argument_gprs = 0x7f8u;        // r3-r10
constexpr std::uint32_t argument_fprs = 0x3ffeu;       // f1-f13
inline void registers_difference(std::vector<std::string>& found, const std::string& where, const Registers& e,
                                 const Registers& a, std::uint32_t gprs, std::uint32_t fprs, bool cr, bool fpscr) {
    const auto word = [](const Registers& r, unsigned n) { return read_be<4>(r.data() + n * 4); };
    for (unsigned n = 0; n < 32; ++n)
        if ((gprs >> n & 1) && word(e, n) != word(a, n))
            found.push_back(where + " r" + std::to_string(n) + " " + hex(word(e, n)) + " vs " + hex(word(a, n)));
    for (unsigned n = 0; n < 32; ++n) {
        if (!(fprs >> n & 1)) continue;
        const auto x = read_be<8>(e.data() + 0x4000 + n * 16), y = read_be<8>(a.data() + 0x4000 + n * 16);
        if (x != y) found.push_back(where + " f" + std::to_string(n) + " " + hex(x) + " vs " + hex(y));
    }
    for (unsigned field = 0; cr && field < 8; ++field)
        if ((e[2304 + field] ^ a[2304 + field]) & 0xf)  // a field's four bits (the fixtures fill the bytes with 0xa5)
            found.push_back(where + " cr" + std::to_string(field) + " " + std::to_string(e[2304 + field]) + " vs " +
                            std::to_string(a[2304 + field]));
    for (unsigned byte = 1280; fpscr && byte < 1312; ++byte)
        if ((e[byte] != 0) != (a[byte] != 0))  // one bit each (a fixture may fill them with 0xa5)
            found.push_back(where + " FPSCR byte " + std::to_string(byte - 1280) + " " + hex(e[byte]) + " vs " + hex(a[byte]));
}
// Volatile registers that hold no argument of a callee: at a call the C++
// need not leave there what the original leaves (docs/cpp-rewrites.md: the
// arguments; leftovers count where they are read, and at returns). Each pair
// is a callee and a register no path through its code (and its callees')
// reads before writing it: either every path writes it, or some return
// with it untouched. By the calling convention a caller does not read a
// volatile register after a call before writing it, so a leftover passed
// back reaches only the routine's return, which is compared. r3-r10 and f1
// of every other call are compared, and f2-f13 for the callees that take
// them (fp_arguments; no other routine the module calls reads them:
// shortlist_manager_volatile_audit).
inline const std::set<std::pair<std::uint32_t, std::string>> non_arguments = [] {
    // pow and sqrt (CarbonLib, carbon_bridge.cpp) read f1 (and f2) only.
    // The others were checked in their code (every path, callees included).
    const std::pair<std::uint32_t, const char*> table[] = {
        {0x10780fc0, "r3 r4 r5 r6 r7 r8 r9 r10"},  // pow
        {0x10780fd8, "r3 r4 r5 r6 r7 r8 r9 r10"},  // sqrt
        {0x1007e820, "f1 r6"}, {0x10082f20, "f1"}, {0x10082f90, "f1"}, {0x100c7d00, "f1"}, {0x100cab10, "f1"},
        {0x100cdef0, "f1 r6"}, {0x100d3040, "f1 r5 r6"}, {0x100d3150, "f1"}, {0x100d3230, "f1 r5"},
        {0x100d9520, "f1 r5"}, {0x100d9580, "f1"}, {0x100da050, "f1 r5"}, {0x10103510, "f1 r5 r6"},
        {0x1014c2e0, "f1"}, {0x1018c340, "f1"}, {0x101929b0, "f1"}, {0x10219e40, "f1"}, {0x103092f0, "f1"},
        {0x1036fbf0, "f1"}, {0x103721f0, "f1"}, {0x103ecef0, "f1"}, {0x10459b50, "f1 r5 r6"}, {0x10464160, "f1 r5"},
        {0x10717b20, "f1"}, {0x10775130, "r4"}, {0x10775550, "r4"},
    };
    std::set<std::pair<std::uint32_t, std::string>> pairs;
    for (const auto& [target, names] : table) {
        const std::string text = names;
        for (std::size_t at = 0; at < text.size();) {
            const auto end = std::min(text.find(' ', at), text.size());
            pairs.insert({target, text.substr(at, end - at)});
            at = end + 1;
        }
    }
    return pairs;
}();
inline const std::map<std::uint32_t, std::uint32_t> fp_arguments{{0x10780fc0, 0x6u}};  // pow: f1, f2
// What the rewrite's run leaves differently from the reference's.
inline std::vector<std::string> differences(const Run& e, const Run& a, unsigned results,
                                            const std::set<std::uint32_t>& transparent) {
    std::vector<std::string> found;
    if (e.outcome != a.outcome) found.push_back("outcome reference=\"" + e.outcome + "\" rewrite=\"" + a.outcome + "\"");
    // A fault (an unmapped access, an enabled floating-point exception) stops
    // inside the routine, where the C++ need not hold the original's state
    // (it reproduces it at returns and at calls): only the outcome counts.
    if (fault(e.outcome)) return found;
    const bool returned = e.outcome.empty();
    if (e.outcome == a.outcome) {
        // At a return: r4-r10, r3 for a routine returning it, f1 and the CR
        // too. At a model's stop (an interruption, an exit) inside the
        // routine, right after a call: what the call's caller may read then,
        // the preserved registers and the call's result r3, f1 (the
        // leftovers of the C++ count at calls and returns only).
        const auto gprs = preserved_gprs | (returned ? 0x7f0u | (results & RoutineChecker::result_r3 ? 8u : 0u) : 8u);
        registers_difference(found, "at the end", e.state->registers, a.state->registers, gprs, preserved_fprs | 2u,
                             returned, true);
        if (e.state->pc != a.state->pc) found.push_back("pc " + hex(e.state->pc) + " vs " + hex(a.state->pc));
    }
    const auto& x = *e.state;
    const auto& y = *a.state;
    if (x.heap_end != y.heap_end || x.native_calls != y.native_calls || x.memory_error != y.memory_error ||
        x.allocated != y.allocated || x.available != y.available)
        found.push_back("allocator tables");
    const auto ranges = x.memory.mapped_ranges();
    if (ranges != y.memory.mapped_ranges()) found.push_back("mappings");
    else
        for (const auto& [address, size] : ranges) {
            const auto p = x.memory.copy_out(address, size), q = y.memory.copy_out(address, size);
            if (p == q) continue;
            unsigned shown = 0;
            for (std::size_t i = 0; i < p.size() && shown < 8; ++i)
                if (p[i] != q[i]) {
                    found.push_back("memory " + hex(address + i) + " " + hex(p[i]) + " vs " + hex(q[i]));
                    ++shown;
                }
        }
    // The calls that line up: not the transparent helpers, and in the
    // reference not the direct sites.
    std::vector<const Call*> p, q;
    for (const auto& c : e.calls)
        if (!transparent.contains(c.target) && !direct_backs.contains(c.stop)) p.push_back(&c);
    for (const auto& c : a.calls)
        if (!transparent.contains(c.target)) q.push_back(&c);
    for (std::size_t n = 0; n < std::max(p.size(), q.size()); ++n) {
        const auto where = "call " + std::to_string(n);
        if (n >= p.size() || n >= q.size()) {
            const auto* c = n < p.size() ? p[n] : q[n];
            found.push_back(where + (n < p.size() ? " only in the reference: " : " only in the rewrite: ") +
                            hex(c->target) + " back " + hex(c->stop));
            break;
        }
        if (p[n]->target != q[n]->target || p[n]->stop != q[n]->stop) {
            found.push_back(where + " target " + hex(p[n]->target) + "/" + hex(q[n]->target) + " back " + hex(p[n]->stop) +
                            "/" + hex(q[n]->stop));
            break;
        }
        const auto target = p[n]->target;
        // Before: the preserved registers (the callee's prologue saves them)
        // and the argument registers; after: the results and the preserved ones.
        auto gprs = preserved_gprs | argument_gprs, fprs = preserved_fprs | argument_fprs;
        for (unsigned r = 3; r <= 10; ++r)
            if (non_arguments.contains({target, "r" + std::to_string(r)})) gprs &= ~(1u << r);
        for (unsigned f = 1; f <= 13; ++f)
            if (non_arguments.contains({target, "f" + std::to_string(f)})) fprs &= ~(1u << f);
        const auto before = found.size();
        const auto to = "call to " + hex(target);
        // f2-f13 only where the callee takes them; the C++ leaves them as guest::Fpu left them.
        fprs &= ~0x3ffcu;
        if (const auto f = fp_arguments.find(target); f != fp_arguments.end()) fprs |= f->second;
        registers_difference(found, to + " before", p[n]->before, q[n]->before, gprs, fprs, false, true);
        registers_difference(found, to + " after", p[n]->after, q[n]->after, preserved_gprs | 8u, preserved_fprs | 2u,
                             false, true);
        if (found.size() != before) {
            found.push_back("(at " + where + ")");
            break;
        }
    }
    return found;
}

// The tally of a fixture program's new side.
struct Tally {
    std::string name;
    unsigned compared = 0, returns = 0, stops = 0, faults = 0, negatives = 0, failures = 0;
    std::map<std::uint32_t, unsigned> roots;
    std::map<std::string, unsigned> skipped;
    std::map<std::string, unsigned> failing;  // first difference -> cases
    std::set<std::pair<std::uint32_t, std::string>> volatile_at_calls;  // argument registers that differed at calls
};
inline Tally tally;
inline void skip(const std::string& reason) { ++tally.skipped[reason]; }

enum class Negative { none, spoil, child };
struct Case {
    std::string label;
    const std::set<std::uint32_t>* transparent = nullptr;
    Negative negative = Negative::none;
    // The fixture's copies of the original's constant pools (the tables a
    // routine reads through the TOC): the C++ has their values.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> constants;
    std::function<void(State&)> spoil;  // own negative: spoils the rewrite's result (default: r3, or r31 for a routine without one)
};
// Runs one case's new side: `reference` and `rewrite` each run the case from
// a clone of `input` with a model serving that side.
inline void check(const State& input, const Case& c, const std::function<Run(const State&)>& reference,
                  const std::function<Run(const State&)>& rewrite) {
    static const bool off = std::getenv("CM_SHORTLIST_REWRITE_SIDE_OFF") != nullptr;  // the builder comparisons alone
    if (off) return;
    static const std::set<std::uint32_t> none;
    const auto& transparent = c.transparent ? *c.transparent : none;
    direct_backs.clear();
    auto actual = rewrite(input);
    const auto rewrite_direct = direct_backs;
    const auto expected = reference(input);
    direct_backs = rewrite_direct;  // not those of a callee the reference ran as C++
    for (const auto back : seen_direct)
        if (seen_dispatched.contains(back))
            throw std::runtime_error(c.label + " a call both direct and through Program, back " + hex(back));
    // An unmapped TOC word: the original loads the TOC words a routine uses
    // in its prologue, the C++ where it uses them (the game's TOC is always
    // mapped), so only the original may stop there.
    if (expected.outcome != actual.outcome && expected.outcome.starts_with("Unmapped data access at ")) {
        const auto at = std::uint32_t(std::stoul(expected.outcome.substr(24), nullptr, 16));
        const auto toc = std::uint32_t(input.r<4>(8));
        if (at - (toc - 0x8000) < 0x10000) return skip("a TOC word unmapped: the original reads it in its prologue");
        for (const auto& [begin, end] : c.constants)
            if (at >= begin && at < end) return skip("a constant of the original's pool unmapped: the C++ has its value");
    }
    if (c.negative == Negative::spoil) {
        if (c.spoil) c.spoil(*actual.state);
        else {
            const unsigned n = shortlist_manager_results(input.pc) & RoutineChecker::result_r3 ? 3 : 31;
            actual.state->wr<4>(n * 4, actual.state->r<4>(n * 4) ^ 1);
        }
    }
    const auto found = differences(expected, actual, shortlist_manager_results(input.pc), transparent);
    ++tally.compared;
    ++tally.roots[input.pc];
    if (c.negative != Negative::none) {
        if (found.empty()) {
            ++tally.failures;
            ++tally.failing["a negative control was not rejected"];
            std::cerr << tally.name << " rewrite: " << c.label << " negative control not rejected" << std::endl;
        } else {
            ++tally.negatives;
        }
        return;
    }
    if (found.empty()) {
        if (expected.outcome.empty()) ++tally.returns;
        else if (fault(expected.outcome)) ++tally.faults;
        else ++tally.stops;
        return;
    }
    ++tally.failures;
    const auto key = [](const std::string& text) {
        const auto vs = text.find(" vs ");
        if (vs == std::string::npos) return text;
        return text.substr(0, text.rfind(' ', vs - 1));
    };
    for (const auto& line : found)
        if (line.starts_with("call to ") && line.find(" before ") != std::string::npos) {
            const auto register_name = key(line).substr(key(line).rfind(' ') + 1);
            const auto target = std::uint32_t(std::stoul(line.substr(8, line.find(' ', 8) - 8), nullptr, 16));
            if (register_name.size() > 1 && (register_name[0] == 'r' || register_name[0] == 'f')) {
                const auto n = std::stoul(register_name.substr(1));
                if ((register_name[0] == 'r' && n >= 3 && n <= 10) || (register_name[0] == 'f' && n >= 1 && n <= 13))
                    tally.volatile_at_calls.insert({target, register_name});
            }
        }
    const auto first = key(found.front());
    if (++tally.failing[first] <= 3) {
        std::cerr << tally.name << " rewrite: " << c.label << " root " << hex(input.pc) << " outcome \"" << expected.outcome
                  << "\":";
        for (std::size_t n = 0; n < found.size() && n < 12; ++n) std::cerr << "\n  " << found[n];
        static const bool verbose = std::getenv("CM_SHORTLIST_REWRITE_VERBOSE") != nullptr;
        for (const Run* run : {&expected, static_cast<const Run*>(&actual)})
            for (std::size_t n = 0; verbose && n < run->calls.size(); ++n)
                std::cerr << "\n    " << (run == &expected ? "reference" : "rewrite") << " call " << n << " "
                          << hex(run->calls[n].target) << " back " << hex(run->calls[n].stop) << " r4 after "
                          << hex(read_be<4>(run->calls[n].after.data() + 16));
        std::cerr << std::endl;
    }
}
inline Negative negative(bool own, bool child) {
    return own ? Negative::spoil : child ? Negative::child : Negative::none;
}
// One case's new side for a fixture: `make(reference)` builds the case's
// model for that run (a std::unique_ptr; its `side` is set here) and
// `step(model, state)` runs the original from the state (the family's
// reference). Cases the C++ cannot run are counted as not run: roots outside
// the module, budget stops, and the fixture's own `not_run` reason.
template <class Make, class Step>
void rewrite_side(const State& input, const std::string& expected, const char* not_run, const Case& c, Make&& make,
                  Step&& step) {
    if (!module_routine(input.pc)) return skip("root outside the module");
    if (expected == "Native instruction budget exhausted") return skip("instruction budget stop");
    if (not_run) return skip(not_run);
    const auto run = [&](const State& in, bool reference) {
        Run r;
        r.state = clone(in);
        auto model = make(reference);
        model->side.reference = reference;
        model->side.rewrite = !reference;
        model->side.transparent = c.transparent;
        try {
            if (reference) step(*model, *r.state);
            else {
                Scope scope(answer_of(*model));
                shortlist_manager_run_rewrite(*r.state, r.state->pc);
            }
        } catch (const std::exception& e) {
            r.outcome = e.what();
        }
        r.calls = calls_of(model->calls);
        return r;
    };
    check(input, c, [&](const State& in) { return run(in, true); }, [&](const State& in) { return run(in, false); });
}
// Prints the tally; throws when a case differed.
inline void finish() {
    std::cout << tally.name << " rewrite: compared=" << tally.compared << " returns=" << tally.returns
              << " model_stops=" << tally.stops << " same_faults=" << tally.faults << " negatives=" << tally.negatives << " differing=" << tally.failures
              << std::endl;
    for (const auto back : seen_direct) std::cout << "  direct C++ call returning to " << hex(back) << std::endl;
    for (const auto& [root, count] : tally.roots) std::cout << "  root " << hex(root) << " cases=" << count << std::endl;
    for (const auto& [reason, count] : tally.skipped) std::cout << "  not run (" << reason << "): " << count << std::endl;
    for (const auto& [first, count] : tally.failing) std::cout << "  differing " << count << ": " << first << std::endl;
    for (const auto& [target, name] : tally.volatile_at_calls)
        std::cout << "  differing at a call: " << hex(target) << " " << name << std::endl;
    if (tally.failures) throw std::runtime_error(tally.name + ": the C++ differs from the original in " +
                                                 std::to_string(tally.failures) + " cases");
}
}
