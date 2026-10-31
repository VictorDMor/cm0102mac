// Original-versus-C++ checks of MATCH_ENG.CPP from real match states. See
// match_eng_checks.hpp. Three kinds of evidence, reported separately:
//   live     every listed routine's real calls during a bounded match interval
//            (the first --all calls of each and then every --every-th);
//   cases    the listed calls from the blocks' case lists, at several real states;
//   mutated  a call repeated with changed input values the original really read,
//            so branches the match did not take run on both sides.
// A mutated call in which both sides fault alike is counted apart, never as a pass.
// --corrupt is the negative control: a check must then fail.
#include "match_eng_checks.hpp"
#include "code_fragment_adapter.hpp"
#include "guest_abi.hpp"
#include "match_eng_adapter.hpp"
#include "match_replay.hpp"
#include "match_turbo.hpp"
#include "routine_check.hpp"
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

#if defined(CM_MATCH_ENG_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif

using namespace cm::aot;
using namespace cm::aot::match_eng_checks;

namespace cm::aot::match_eng_checks {
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
    std::filesystem::path input;
    unsigned advances = 600, all = 40, every = 400, mutate = 0, mutated_calls = 4, states = 3;
    std::uint32_t seed = 1;
    std::set<std::uint32_t> only;
    bool corrupt = false, live = true, cases = true, strict = false, require_all = false, show_faults = false;
} options;

struct Tally { std::uint64_t returned = 0, faulted = 0, timed_out = 0, calls = 0; };
std::map<std::uint32_t, Tally> case_tally, mutated_tally;
std::map<std::uint32_t, std::uint64_t> observed_calls;
std::uint32_t match_address = 0;
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
#if defined(CM_MATCH_ENG_COVERAGE)
    __llvm_profile_write_file();
#endif
}
// Mutated inputs can loop for a very long time on both sides.
void bounded_original(State& s, std::uint32_t entry) { alarm(30); s.budget = 4'000'000'000ULL; match_eng_run_original(s, entry); }
void bounded_rewrite(State& s, std::uint32_t entry) { alarm(30); s.budget = 4'000'000'000ULL; match_eng_run_rewrite(s, entry); }

struct Read { std::uint32_t address; unsigned size; };
struct ReadTrace final : MemoryObserver {
    std::set<std::pair<std::uint32_t, unsigned>> seen;
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void read(std::uint32_t address, const std::uint8_t*, unsigned size) override {
        if (seen.size() < 200000) seen.emplace(address, size);
    }
};

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
        const auto previous = s.memory.date_observer;
        s.memory.date_observer = &trace;
        alarm(30);
        try { s.budget = 4'000'000'000ULL; match_eng_run_original(s, entry); } catch (...) {}
        s.memory.date_observer = previous;
        for (const auto& [address, size] : trace.seen) {
            const Read read{address, size};
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

struct Comparison { bool returned = false, faulted = false, timed_out = false; std::vector<std::string> differences; };
Comparison compare_call(State& s, std::uint32_t entry, unsigned results, bool strict = false) {
    Comparison outcome;
    const auto stop_rewrite = "the rewrite stopped: ", stop_original = "the original stopped: ";
    std::vector<std::string> found;
    try {
        found = compare_routine_call(s, entry, bounded_original, bounded_rewrite, results, false, strict);
    } catch (const std::runtime_error& error) {
        if (std::string_view(error.what()).starts_with("Routine check child ended")) { outcome.timed_out = true; return outcome; }
        throw;
    }
    std::string rewrite_stop, original_stop;
    for (const auto& line : found) {
        if (line.starts_with(stop_rewrite)) rewrite_stop = line.substr(std::string_view(stop_rewrite).size());
        if (line.starts_with(stop_original)) original_stop = line.substr(std::string_view(stop_original).size());
    }
    if (!rewrite_stop.empty() && rewrite_stop == original_stop) {
        outcome.faulted = true;
        if (options.show_faults) std::cerr << "both faulted at " << hex_address(entry) << ": " << rewrite_stop << '\n';
    }
    else if (found.empty()) outcome.returned = true;
    else outcome.differences = std::move(found);
    return outcome;
}

void report_difference(std::uint32_t entry, std::string_view kind, const std::vector<std::string>& lines, std::string_view detail = {}) {
    std::cerr << kind << " difference at " << hex_address(entry);
    if (!detail.empty()) std::cerr << " (" << detail << ")";
    std::cerr << ":\n";
    for (const auto& line : lines) std::cerr << "  " << line << '\n';
    throw std::runtime_error("MATCH_ENG.CPP routine differs from the original");
}

// Repeats the call now due with changed inputs; undoes them afterwards.
void mutate_call(State& s, std::uint32_t entry, unsigned results, std::mt19937& random) {
    if (!options.mutate) return;
    auto reads = reads_of(s, entry);
    std::vector<Read> candidates;
    for (const auto& read : reads) {
        if (read.address < 0x108b3a90 || !mapped(s, read.address)) continue;  // immutable code-segment constants
        if (read.size != 1 && read.size != 2 && read.size != 4 && read.size != 8) continue;
        if (read.size == 4) {
            const auto value = std::uint32_t(s.memory.read<4>(read.address));
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
            const auto before = load();
            undo.emplace_back(read, before);
            const auto value = replacement(read, before, random);
            store(value);
            detail << hex_address(read.address) << ":" << read.size << "=" << std::hex << value << std::dec << " ";
        }
        const auto outcome = compare_call(s, entry, results, options.strict);
        for (auto item = undo.rbegin(); item != undo.rend(); ++item) {
            const auto& [read, value] = *item;
            switch (read.size) { case 1: s.memory.write<1>(read.address, value & 0xff); break; case 2: s.memory.write<2>(read.address, value & 0xffff); break;
                                 case 4: s.memory.write<4>(read.address, std::uint32_t(value)); break; default: s.memory.write<8>(read.address, value); }
        }
        ++tally.calls;
        tally.returned += outcome.returned; tally.faulted += outcome.faulted; tally.timed_out += outcome.timed_out;
        if (!outcome.differences.empty()) {
            s.memory.observer = saved_observer;
            report_difference(entry, "Mutated", outcome.differences, detail.str());
        }
    }
    s.memory.observer = saved_observer;
}

std::mt19937 live_random;
void observe_call(State& s, std::uint32_t entry, unsigned results) {
    const auto call = ++observed_calls[entry];
    if (call > options.mutated_calls) return;
    const auto pc = s.pc;
    s.pc = entry;
    mutate_call(s, entry, results, live_random);
    s.pc = pc;
}

void advance(State& s, unsigned count) {
    guest::Cpu c(s);
    const auto stack = c.sp();
    s.budget = UINT64_MAX;
    for (unsigned tick = 0; tick < count; ++tick) {
        c.set(3, match_address); c.set(4, 1); c.set_lr(return_sentinel);
        s.pc = 0x101d3f60;
        run_until(s, return_sentinel);
        if (s.pc != return_sentinel || c.sp() != stack) throw std::runtime_error("A match advance did not return");
        if (c.r(3) != 1 && c.r(3) != 4) { std::cout << "match ended at advance " << tick << " (r3 " << c.r(3) << ")\n"; break; }
    }
}

unsigned results_of(std::uint32_t) { return RoutineChecker::result_r3 | RoutineChecker::result_f1; }

void run_cases(State& s, std::mt19937& random) {
    Scenario scenario{s, match_address};
    std::vector<Case> cases;
    for (const auto& [name, list] : blocks()) list(scenario, cases);
    guest::Cpu c(s);
    const auto saved = s.registers;
    const auto saved_observer = s.memory.observer;
    for (const auto& item : cases) {
        if (!options.only.empty() && !options.only.count(item.entry)) continue;
        s.registers = saved;
        for (const auto& [n, value] : item.registers) c.set(n, value);
        if (item.f1) c.set_f(1, *item.f1);
        c.set_lr(return_sentinel);
        s.pc = item.entry;
        std::vector<std::pair<Write, std::uint32_t>> undo;
        s.memory.observer = nullptr;
        for (const auto& write : item.memory) {
            const auto before = write.size == 1 ? s.memory.read<1>(write.address) : write.size == 2 ? s.memory.read<2>(write.address) : s.memory.read<4>(write.address);
            undo.emplace_back(write, std::uint32_t(before));
            if (write.size == 1) s.memory.write<1>(write.address, write.value & 0xff);
            else if (write.size == 2) s.memory.write<2>(write.address, write.value & 0xffff);
            else s.memory.write<4>(write.address, write.value);
        }
        s.memory.observer = saved_observer;
        const auto outcome = compare_call(s, item.entry, results_of(item.entry), options.strict);
        auto& tally = case_tally[item.entry];
        ++tally.calls; tally.returned += outcome.returned; tally.faulted += outcome.faulted; tally.timed_out += outcome.timed_out;
        if (!outcome.differences.empty()) report_difference(item.entry, "Case", outcome.differences, item.label);
        // One routine can have thousands of cases; a few of them seed its mutations.
        if (tally.calls <= options.mutated_calls * 8 || tally.calls % 64 == 0) mutate_call(s, item.entry, results_of(item.entry), random);
        s.memory.observer = nullptr;
        for (auto w = undo.rbegin(); w != undo.rend(); ++w) {
            if (w->first.size == 1) s.memory.write<1>(w->first.address, w->second & 0xff);
            else if (w->first.size == 2) s.memory.write<2>(w->first.address, w->second & 0xffff);
            else s.memory.write<4>(w->first.address, w->second);
        }
        s.memory.observer = saved_observer;
    }
    s.registers = saved;
}

// Whether every routine in production returned from a comparison. A routine no case
// and no live call reached has no evidence, however clean the rest of the run is.
bool summary(const RoutineChecker* checker) {
    std::uint64_t live = 0, listed = 0, returned = 0, faulted = 0, timed = 0, cases = 0, case_faults = 0;
    std::set<std::uint32_t> reached;
    // The other modules' routines are checked on the way too; only this one's count.
    if (checker) for (const auto& [entry, totals] : checker->totals()) if (match_eng_lists(entry)) { live += totals.checked; if (totals.checked) reached.insert(entry); }
    for (const auto& [entry, tally] : mutated_tally) { returned += tally.returned; faulted += tally.faulted; timed += tally.timed_out; if (tally.returned) reached.insert(entry); }
    for (const auto& [entry, tally] : case_tally) { cases += tally.returned; case_faults += tally.faulted; if (tally.returned) reached.insert(entry); }
    listed = reached.size();
    std::cout << "live_comparisons=" << live << " case_returned=" << cases << " case_faults=" << case_faults
              << " mutated_returned=" << returned << " mutated_faults=" << faulted << " mutated_timeouts=" << timed
              << " entries_compared=" << listed << '\n';
    if (const char* file = std::getenv("CM_MATCH_ENG_REPORT")) {
        std::ofstream out(file);
        if (checker) for (const auto& [entry, totals] : checker->totals()) if (match_eng_lists(entry))
            out << "live " << hex_address(entry) << " calls=" << totals.calls << " checked=" << totals.checked << '\n';
        for (const auto& [entry, tally] : case_tally) out << "case " << hex_address(entry) << " returned=" << tally.returned << " faulted=" << tally.faulted << " timeout=" << tally.timed_out << '\n';
        for (const auto& [entry, tally] : mutated_tally) out << "mutated " << hex_address(entry) << " returned=" << tally.returned << " faulted=" << tally.faulted << " timeout=" << tally.timed_out << '\n';
    }
    if (!options.require_all) return true;
    bool complete = true;
    for (const auto entry : match_eng_production_entries())
        if (!reached.count(entry)) { std::cerr << "No comparison reached " << hex_address(entry) << '\n'; complete = false; }
    return complete;
}
}

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: cm-match-eng-checks MATCH_DIR [--advances N] [--all N] [--every N] [--mutate K] [--mutated-calls N] [--states N] [--seed N] [--only 0x..,0x..] [--corrupt] [--no-live] [--no-cases] [--strict] [--require-all] [--show-faults]\n"; return 2; }
    try {
        options.input = argv[1];
        for (int n = 2; n < argc; ++n) {
            const std::string_view option(argv[n]);
            const auto value = [&] { if (n + 1 >= argc) throw std::runtime_error("missing value"); return std::string(argv[++n]); };
            if (option == "--advances") options.advances = unsigned(std::stoul(value()));
            else if (option == "--all") options.all = unsigned(std::stoul(value()));
            else if (option == "--every") options.every = unsigned(std::stoul(value()));
            else if (option == "--mutate") options.mutate = unsigned(std::stoul(value()));
            else if (option == "--mutated-calls") options.mutated_calls = unsigned(std::stoul(value()));
            else if (option == "--states") options.states = unsigned(std::stoul(value()));
            else if (option == "--seed") options.seed = std::uint32_t(std::stoul(value()));
            else if (option == "--only") { std::stringstream list(value()); for (std::string item; std::getline(list, item, ',');) options.only.insert(std::uint32_t(std::stoul(item, nullptr, 16))); }
            else if (option == "--corrupt") options.corrupt = true;
            else if (option == "--no-live") options.live = false;
            else if (option == "--no-cases") options.cases = false;
            else if (option == "--strict") options.strict = true;
            else if (option == "--require-all") options.require_all = true;
            else if (option == "--show-faults") options.show_faults = true;
            else throw std::runtime_error("Unknown option " + std::string(option));
        }
        std::ifstream(options.input / "match.txt") >> match_address;
        if (!match_address) throw std::runtime_error("Missing original match identity");
        auto state = std::make_unique<State>();
        load_match_snapshot(*state, options.input / "initial.snapshot");
        if (state->memory.read<4>(match_address + 0x311c + 0x1a4) != match_address)
            throw std::runtime_error("Captured match lacks its actual player records");
        state->code_fragment_observer = &inputs;
        set_match_turbo_enabled(false);
        routine_child_exit = flush_child;
        live_random.seed(options.seed);
        std::mt19937 random(options.seed + 1);
        auto& s = *state;
        const auto wanted = [](std::uint32_t entry) { return options.only.empty() || options.only.count(entry) != 0; };
        RoutineChecker checker(wanted, options.all, options.every);
        checker.corrupt_first = options.corrupt;
        checker.compare_f1 = true;
        checker.strict_volatile = options.strict;
        routine_checker = &checker;
        if (options.cases) {
            run_cases(s, random);
        }
        if (options.live) {
            match_eng_call_observer = observe_call;
            advance(s, options.advances);
            match_eng_call_observer = nullptr;
        } else if (options.cases) {
            for (unsigned n = 1; n < options.states; ++n) { routine_checker = nullptr; advance(s, 150); routine_checker = &checker; run_cases(s, random); }
        }
        const bool complete = summary(&checker);
        if (options.corrupt) { std::cerr << "The negative control found no difference\n"; return 1; }
        return checker.differences() || !complete ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return options.corrupt && std::string_view(error.what()).find("differs") != std::string_view::npos ? 0 : 2; }
}
