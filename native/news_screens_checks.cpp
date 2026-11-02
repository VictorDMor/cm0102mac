// Original-versus-C++ checks of NEWS_SCREENS.CPP from a real career state, the checker of
// media.cpp with one change: each case runs in a forked child of its own, because a screen's
// values live in the screen manager and a case sets them through the game's own routines, on
// the original's routes. Two kinds of evidence, reported separately:
//   cases    the listed calls on the real world, after their setup;
//   mutated  a call repeated with changed input values the original really read,
//            so branches the state did not take run on both sides.
// Real calls come from the capture replays (cm-world-replay --check-routines).
// A mutated call in which both sides fault alike is counted apart, never as a pass.
// --corrupt is the negative control: a check must then fail.
#include "aot_runtime.hpp"
#include "application_exit.hpp"
#include "code_fragment_adapter.hpp"
#include "guest_abi.hpp"
#include "news_screens_adapter.hpp"
#include "news_screens_detail.hpp"
#include "match_replay.hpp"
#include "routine_check.hpp"
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>

#if defined(CM_NEWS_SCREENS_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif

using namespace cm::aot;

namespace {
struct Options {
    std::filesystem::path input;
    unsigned jobs = 8, mutate = 0;
    std::uint32_t seed = 1;
    std::set<std::uint32_t> only;
    bool corrupt = false, strict = false, require_all = false, show_faults = false, call_trace = false, corrupt_trace = false, production = false, null_page = false;
} options;

struct Tally { std::uint64_t returned = 0, faulted = 0, timed_out = 0, calls = 0; };
std::map<std::uint32_t, Tally> mutated_tally;
unsigned clocks = 0;

struct Inputs final : CodeFragmentBoundaryObserver {
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        const std::string_view import(name);
        if (import == "CarbonLib::Microseconds") s.memory.write<8>(s.r<4>(12), 1000000000ULL + 1000000ULL * ++clocks);
        else if (import == "CarbonLib::GetDateTime") s.memory.write<4>(s.r<4>(12), 3000000000u + ++clocks);
        else return false;
        s.pc = s.r<4>(4128);
        return true;
    }
} inputs;

void flush_child() {
#if defined(CM_NEWS_SCREENS_COVERAGE)
    __llvm_profile_write_file();
#endif
}
// Mutated inputs can loop for a very long time on both sides.
void bounded_original(State& s, std::uint32_t entry) { alarm(60); s.budget = s.executed + 4'000'000'000ULL; news_screens_run_original(s, entry); }
void bounded_rewrite(State& s, std::uint32_t entry) {
    alarm(60);
    s.budget = s.executed + 4'000'000'000ULL;
    news_screens_run_rewrite(s, entry);
    if (options.corrupt) { guest::Cpu c(s); c.set(3, c.r(3) ^ 1); }
}

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
        const auto previous = s.memory.date_observer;
        s.memory.date_observer = &trace;
        alarm(30);
        try { s.budget = s.executed + 4'000'000'000ULL; news_screens_run_original(s, entry); } catch (...) {}
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

std::string_view current_label;
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
        if (options.show_faults) std::cerr << "both faulted at " << hex_address(entry) << " [" << current_label << "]: " << rewrite_stop << '\n';
        // Registers aren't compared at a fault, but memory, the heap tables and what the imports
        // saw up to it are, and a difference there still counts.
        std::erase_if(found, [&](const std::string& line) { return line.starts_with(stop_rewrite) || line.starts_with(stop_original); });
        if (!found.empty()) { outcome.faulted = false; outcome.differences = std::move(found); }
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
std::vector<TracedCall>* traced_calls = nullptr;
bool (*scripted_services)(State&) = nullptr;
std::array<std::pair<std::uint32_t, std::uint32_t>, 24> recent_dispatches;
unsigned dispatches_seen = 0;
bool trace_probe(State& s) {
    recent_dispatches[dispatches_seen++ % recent_dispatches.size()] = {s.pc, std::uint32_t(s.r<4>(4128))};
    const auto back = std::uint32_t(s.r<4>(4128));
    if (back >= 0x102ce714 && back <= 0x102d0a50 && traced_calls->size() < 2'000'000) {
        const auto word = std::uint32_t(s.memory.read<4>(back - 4));
        const auto offset = std::int32_t(word << 6) >> 6 & ~3;
        TracedCall call{back, s.pc, (word & 0xfc000003) == 0x48000001 && back - 4 + std::uint32_t(offset) == s.pc, {}, {}, {}};
        for (unsigned n = 13; n < 32; ++n) call.gprs[n - 13] = std::uint32_t(s.r<4>(guest::reg::gpr(n)));
        for (unsigned n = 14; n < 32; ++n) call.fprs[n - 14] = s.r<8>(guest::reg::fpr(n));
        for (unsigned n = 3; n <= 10; ++n) call.arguments[n - 3] = std::uint32_t(s.r<4>(guest::reg::gpr(n)));
        traced_calls->push_back(call);
    }
    return scripted_services(s);
}
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
constexpr unsigned wrong_return = 200, wrong_count = 201, wrong_order = 202;
bool trace_corrupted = false, arguments_corrupted = false;
std::map<std::tuple<std::uint32_t, std::uint32_t, unsigned>, TraceFinding> trace_findings;
std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint64_t> traced_sites;
std::uint64_t traced_pairs = 0;
void compare_traces(State& s, std::uint32_t entry) {
    const auto original = trace_run(s, entry, bounded_original);
    auto rewrite = trace_run(s, entry, bounded_rewrite);
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
    };
    for (const auto& [site, calls] : sites) {
        const auto seen = original_calls.find(site);
        if (seen == original_calls.end()) {
            if (!calls.front()->after_bl) record(site, wrong_return, 0, calls.size());
        } else if (seen->second != calls.size()) {
            record(site, wrong_count, seen->second, calls.size());
        }
    }
    const auto next_shown = [&](const std::vector<TracedCall>& calls, std::size_t& at, const auto& shown) {
        while (at < calls.size() && !shown(Site{calls[at].back, calls[at].target})) ++at;
        return at < calls.size();
    };
    const auto both = [&](Site site) { return sites.count(site) != 0 && original_calls.count(site) != 0; };
    for (std::size_t a = 0, b = 0; next_shown(original, a, both) && next_shown(rewrite, b, both); ++a, ++b)
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
        for (unsigned n = 0; n < 19; ++n) if (call.gprs[n] != other.gprs[n]) {
            auto& finding = trace_findings[{call.back, call.target, 13 + n}];
            if (!finding.count++) { finding.original = call.gprs[n]; finding.rewrite = other.gprs[n]; }
        }
        const auto reads = argument_masks.find(call.target);
        if (reads != argument_masks.end()) for (unsigned n = 0; n < 8; ++n)
            if ((reads->second >> n & 1) && call.arguments[n] != other.arguments[n]) {
                auto& finding = trace_findings[{call.back, call.target, 3 + n}];
                if (!finding.count++) { finding.original = call.arguments[n]; finding.rewrite = other.arguments[n]; }
            }
        for (unsigned n = 0; n < 18; ++n) if (call.fprs[n] != other.fprs[n]) {
            auto& finding = trace_findings[{call.back, call.target, 100 + 14 + n}];
            if (!finding.count++) { finding.original = call.fprs[n]; finding.rewrite = other.fprs[n]; }
        }
    }
}
void print_trace_findings() {
    std::cout << "call_trace_pairs=" << traced_pairs << " sites=" << traced_sites.size() << " findings=" << trace_findings.size() << '\n';
    for (const auto& [key, finding] : trace_findings) {
        const auto& [back, target, reg] = key;
        if (reg >= wrong_return) {
            std::cout << "call " << hex_address(back - 4) << " -> " << hex_address(target)
                      << (reg == wrong_return ? " returns where no bl of the original does" : reg == wrong_count ? " count" : " out of order")
                      << " x" << finding.count << " original " << std::hex << finding.original << " rewrite " << finding.rewrite << std::dec << '\n';
            continue;
        }
        std::cout << "call " << hex_address(back - 4) << " -> " << hex_address(target) << (reg >= 100 ? " f" : " r") << (reg >= 100 ? reg - 100 : reg)
                  << " x" << finding.count << " original " << std::hex << finding.original << " rewrite " << finding.rewrite << std::dec << '\n';
    }
}

void report_difference(std::uint32_t entry, std::string_view kind, const std::vector<std::string>& lines, std::string_view detail = {}) {
    std::cerr << kind << " difference at " << hex_address(entry);
    if (!detail.empty()) std::cerr << " (" << detail << ")";
    std::cerr << ":\n";
    for (const auto& line : lines) std::cerr << "  " << line << '\n';
    throw std::runtime_error("NEWS_SCREENS.CPP routine differs from the original");
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
        news_screens_declining = original;
        for (unsigned n = 3; n <= 10; ++n) c.set(n, 0);
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
    const auto entries = news_screens_production_entries();
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
    return failures || entries.size() != 8 ? 1 : 0;
}

constexpr std::uint32_t scratch_base = 0x60000000, scratch_size = 0x100000;
// Set by a case's setup, for the comparison that follows.
bool allocation_fails = false;
int forced_letter = -1;
const std::vector<std::pair<std::uint32_t, std::uint32_t>>* case_answers = nullptr;
std::uint32_t reply_item = 0;

// A case's preparation and inputs: the game's own routines on their original routes.
struct Fixture {
    State& s;
    std::uint32_t r32(std::uint32_t at) const { return std::uint32_t(s.memory.read<4>(at)); }
    std::uint32_t toc(int slot) const { return r32(0x108bba90 + std::uint32_t(slot)); }
    std::uint32_t screens() const { return toc(-0x7bb0); }
    std::uint32_t row() const { return std::uint32_t(std::int32_t(std::int16_t(s.memory.read<2>(screens() + 0x2c58)))) * 0x2c0; }
    std::uint32_t manager() const { return r32(screens() + row() + 4); }
    std::uint32_t table() const { return r32(screens() + row() + 0x24); }
    void write8(std::uint32_t at, std::uint32_t value) const { s.memory.write<1>(at, value & 0xff); }
    void write16(std::uint32_t at, std::uint32_t value) const { s.memory.write<2>(at, value & 0xffff); }
    void write32(std::uint32_t at, std::uint32_t value) const { s.memory.write<4>(at, value); }
    std::uint32_t call(std::uint32_t entry, std::initializer_list<std::uint32_t> arguments) const {
        guest::Cpu c(s);
        const auto saved = s.registers;
        const auto declining = news_screens_declining;
        news_screens_declining = true;
        unsigned n = 3;
        for (const auto value : arguments) c.set(n++, value);
        c.set_lr(return_sentinel);
        s.pc = entry;
        s.budget = s.executed + 4'000'000'000ULL;
        run_until(s, return_sentinel);
        const auto result = c.r(3);
        s.registers = saved;
        news_screens_declining = declining;
        return result;
    }
    void select() const { write16(screens() + 0x2c58, 0); }
    std::uint32_t news() const { return r32(toc(-0x7bf0)); }
    std::uint32_t value(std::uint32_t index) const { return call(0x1073ef70, {screens(), index}); }
    void set(std::uint32_t index, std::uint32_t value) const { call(0x1073f1e0, {screens(), index, value, 0}); }
    // The news screen for the manager, as the game opens it, its first drawing done.
    void open(std::uint32_t flag = 0) const { call(0x102ce710, {manager(), flag}); }
    void draw() const { call(0x102cea30, {}); }
    // A control's words in the screen's table, and the action a dialog leaves behind.
    std::uint32_t control(std::int16_t id) const { return table() + 0xc0000 + std::uint32_t(std::int32_t(id)) * 0x17c; }
    void set_control(std::int16_t id, std::uint32_t action, std::uint32_t value) const {
        write32(control(id) - 0x5794, action);
        write32(control(id) - 0x5754, value);
    }
    void set_pending(std::uint32_t action) const { write16(screens() + 0x260000 - 0x7e62, action); }
    std::uint32_t unread(std::uint32_t tab) const { return call(0x102cd440, {news(), manager(), tab}); }
    // A message for the manager from the game's own routines, as the send button posts one.
    void publish(std::uint32_t text_seed) const {
        constexpr std::uint32_t item = scratch_base + 0x8000, text = scratch_base + 0x8800;
        call(0x102bdf90, {item, 0x1388, 0});
        call(0x102cb4b0, {item, 0, r32(manager())});
        call(0x102cb4b0, {item, 1, 1});
        call(0x102cb4b0, {item, 2, text_seed});
        write_text(text, "Zebra " + std::to_string(text_seed));
        call(0x102cb630, {item, 3, text, 0});
        call(0x102cbc10, {news(), item, manager(), 0});
    }
    void write_text(std::uint32_t at, const std::string& text) const {
        for (std::size_t n = 0; n <= text.size(); ++n) write8(at + std::uint32_t(n), n < text.size() ? std::uint8_t(text[n]) : 0);
    }
    void set_filter(const std::string& text) const { write_text(value(8), text); }
    // The list the screen drew, row by row.
    std::uint32_t item(std::uint32_t n) const { return value(2) + n * 0xe4; }
    std::uint32_t items() const { return value(3); }
    void mark_items(bool read) const { for (std::uint32_t n = 0; n < items(); ++n) { write8(item(n) + 0xe0, read); write8(item(n) + 4, 0); } }
    void set_priority(std::uint32_t priority) const { for (std::uint32_t n = 0; n < items(); ++n) write8(item(n) + 4, priority); }
    void select_item(std::uint32_t n) const { if (n < items()) set(9, r32(item(n) + 0xdc)); }
    void shift_today(int years) const { const auto today = toc(-0x7b80); write16(today + 2, std::uint32_t(s.memory.read<2>(today + 2)) + std::uint32_t(years)); }
    void set_language(std::uint32_t language) const { write8(0x108bba90 - 0x35ac, language); }
    // The manager's inbox as empty, so nothing needs attention.
    void empty_inbox() const { write32(r32(news() + 8) + (r32(manager()) - r32(toc(-0x7be4)) + 16) * 12, 0xffffffff); }
    void fail_allocation() const { allocation_fails = true; }
    // Someone else as the screen's person, whose name starts with a vowel or not.
    void use_manager_named(bool vowel) const {
        const auto people = r32(toc(-0x7be0));
        constexpr std::uint32_t name = scratch_base + 0x9000;
        for (std::uint32_t n = 0; n < 400; ++n) {
            call(0x100caeb0, {people + n * 0x70, name, 0x190, 1});
            const auto letter = char(s.memory.read<1>(name));
            if (letter && std::string_view("AEIOUYaeiouy").contains(letter) == vowel) { write32(screens() + row() + 4, people + n * 0x70); return; }
        }
    }
    void set_highlight(bool on) const {
        const auto word = r32(r32(toc(-0x7ac8)) + 4);
        write32(r32(toc(-0x7ac8)) + 4, on ? word | 0x20000 : word & ~0x20000u);
    }
    void set_screen_flag(bool on) const { call(0x1073fe80, {screens(), manager(), on}); }
    void choose_reply(std::uint32_t from_person) const {
        call(0x102cb4b0, {item(0), 1, from_person});
        reply_item = r32(item(0) + 0xdc);
    }
    void flag_people(std::uint32_t count) const {
        const auto people = r32(toc(-0x7be0));
        const auto total = r32(toc(-0x7be4));
        for (std::uint32_t n = 0; n < 16; ++n) write8(people + (total - n - 1) * 0x70 + 0x6b, n < count);
    }
};
constexpr std::uint32_t the_manager = 0xfeedfeed, the_item = 0xfeedfeee;
struct Case {
    std::string label;
    std::uint32_t entry;
    std::vector<std::uint32_t> arguments = {};
    std::function<void(const Fixture&)> setup = {};
    // Another module's routines answered on both sides (entry, r3) when dispatched, with no
    // body run: for paths that depend on what the snapshot can't produce.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {};
};

std::vector<Case> all_cases() {
    std::vector<Case> cases;
    constexpr std::uint32_t open_news = 0x102ce710, draw_news = 0x102cea30, news_command = 0x102cfcd0, close_news = 0x102d0230,
        open_message = 0x102d02b0, open_reply = 0x102d0370, draw_message = 0x102d0500, message_command = 0x102d07b0;
    using Setup = std::function<void(const Fixture&)>;
    // The first group is the human manager's; the news screen open on it, its first drawing done.
    const Setup selected = [](const Fixture& f) { f.select(); };
    const Setup opened = [](const Fixture& f) { f.select(); f.open(); };
    const Setup drawn = [](const Fixture& f) { f.select(); f.open(); f.draw(); };
    const auto then = [](const Setup& first, Setup next) { return Setup([first, next](const Fixture& f) { first(f); next(f); }); };
    const auto messages = [](unsigned count) {
        return Setup([count](const Fixture& f) { f.select(); for (unsigned n = 0; n < count; ++n) f.publish(n); });
    };

    // Opening.
    for (const auto flag : {0u, 1u, 0xffu, 0x80u})
        cases.push_back({"open flag " + std::to_string(flag), open_news, {the_manager, flag}, selected});
    cases.push_back({"open for nobody", open_news, {0, 0}, selected});
    cases.push_back({"open with the default screen", open_news, {the_manager, 0}, opened});
    cases.push_back({"open, the manager refuses", open_news, {the_manager, 0}, selected, {{0x1073e350, 0}}});
    cases.push_back({"open, no memory", open_news, {the_manager, 0}, then(selected, [](const Fixture& f) { f.fail_allocation(); })});

    // Drawing: each tab, first and later drawings, each way into the list.
    for (std::uint32_t tab = 0; tab < 8; ++tab) {
        const auto on_tab = [tab](const Fixture& f) { f.set(1, tab); };
        cases.push_back({"draw tab " + std::to_string(tab), draw_news, {}, then(opened, on_tab)});
        cases.push_back({"redraw tab " + std::to_string(tab), draw_news, {}, then(drawn, on_tab)});
        cases.push_back({"messages, tab " + std::to_string(tab), draw_news, {}, then(then(messages(12), [](const Fixture& f) { f.open(); }), on_tab)});
    }
    cases.push_back({"redraw", draw_news, {}, drawn});
    cases.push_back({"reload", draw_news, {}, then(drawn, [](const Fixture& f) { f.set(4, 1); })});
    cases.push_back({"reload, no list", draw_news, {}, then(opened, [](const Fixture& f) { f.set(4, 1); })});
    cases.push_back({"check flag on", draw_news, {}, then(opened, [](const Fixture& f) { f.set(0, 1); })});
    cases.push_back({"check flag, nothing pending, refused", draw_news, {}, then(opened, [](const Fixture& f) { f.set(0, 1); f.empty_inbox(); f.set_screen_flag(false); })});
    cases.push_back({"check flag, nothing pending, allowed", draw_news, {}, then(opened, [](const Fixture& f) { f.set(0, 1); f.empty_inbox(); f.set_screen_flag(true); })});
    cases.push_back({"redraw with the list kept", draw_news, {}, then(drawn, [](const Fixture& f) { f.set(4, 0); })});
    // Value 8 is never null in play (open_news returns before registering when the allocation fails).
    cases.push_back({"no filter", draw_news, {}, then(then(messages(4), [](const Fixture& f) { f.open(); f.draw(); }), [](const Fixture& f) { f.mark_items(false); f.set(4, 0); f.set(9, 0); f.set(8, 0); })});
    cases.push_back({"no filter, priority", draw_news, {}, then(then(messages(4), [](const Fixture& f) { f.open(); f.draw(); }), [](const Fixture& f) { f.mark_items(false); f.set_priority(1); f.set(4, 0); f.set(9, 0); f.set(8, 0); })});
    for (const auto word : {"the", "a", "zzzzqqq", "", "Zebra"}) {
        const std::string text(word);
        cases.push_back({"filter '" + text + "'", draw_news, {}, then(then(messages(4), [](const Fixture& f) { f.open(); f.draw(); }), [text](const Fixture& f) { f.set_filter(text); })});
        cases.push_back({"filter, no selection '" + text + "'", draw_news, {}, then(then(messages(4), [](const Fixture& f) { f.open(); }), [text](const Fixture& f) { f.set_filter(text); })});
    }
    cases.push_back({"selected id gone", draw_news, {}, then(drawn, [](const Fixture& f) { f.set(9, 0x7fffffff); })});
    cases.push_back({"nothing selected, list drawn", draw_news, {}, then(drawn, [](const Fixture& f) { f.set(9, 0); })});
    for (const auto scroll : {0u, 1u, 3u, 50u, 0xffffffffu}) {
        const auto scrolled = [scroll](const Fixture& f) { f.set(10, scroll); };
        cases.push_back({"scroll " + std::to_string(scroll), draw_news, {}, then(then(messages(30), [](const Fixture& f) { f.open(); f.draw(); }), scrolled)});
        cases.push_back({"scroll, last selected " + std::to_string(scroll), draw_news, {}, then(then(then(messages(30), [](const Fixture& f) { f.open(); f.draw(); }), scrolled), [](const Fixture& f) { f.select_item(29); })});
    }
    cases.push_back({"many messages", draw_news, {}, then(messages(40), [](const Fixture& f) { f.open(); })});
    for (const auto scroll : {0u, 20u, 25u, 28u, 29u, 31u})
        cases.push_back({"choosing, scroll " + std::to_string(scroll), draw_news, {}, then(then(messages(30), [](const Fixture& f) { f.open(); f.draw(); }), [scroll](const Fixture& f) { f.set(4, 0); f.set(9, 0); f.set(10, scroll); })});
    cases.push_back({"other year", draw_news, {}, then(drawn, [](const Fixture& f) { f.shift_today(1); })});
    cases.push_back({"not read", draw_news, {}, then(then(messages(6), [](const Fixture& f) { f.open(); f.draw(); }), [](const Fixture& f) { f.mark_items(false); f.set(9, 0); })});
    cases.push_back({"read", draw_news, {}, then(then(messages(6), [](const Fixture& f) { f.open(); f.draw(); }), [](const Fixture& f) { f.mark_items(true); })});
    cases.push_back({"priority", draw_news, {}, then(then(messages(6), [](const Fixture& f) { f.open(); f.draw(); }), [](const Fixture& f) { f.set_priority(1); f.mark_items(false); f.set(9, 0); })});
    for (const auto language : {0u, 1u, 2u, 3u}) {
        const auto set_language = [language](const Fixture& f) { f.set_language(language); };
        cases.push_back({"language " + std::to_string(language), draw_news, {}, then(opened, set_language)});
        cases.push_back({"language, vowel " + std::to_string(language), draw_news, {}, then(opened, [language](const Fixture& f) { f.set_language(language); f.use_manager_named(true); })});
        cases.push_back({"language, consonant " + std::to_string(language), draw_news, {}, then(opened, [language](const Fixture& f) { f.set_language(language); f.use_manager_named(false); })});
    }
    for (const int letter : std::initializer_list<int>{'A', 'E', 'I', 'O', 'U', 'Y', 'a', 'e', 'i', 'o', 'u', 'y', '@', 'B', 'z', 0x7f, 0xc9})
        cases.push_back({"title letter " + std::to_string(letter), draw_news, {}, then(opened, [letter](const Fixture& f) { f.set_language(2); forced_letter = letter; })});
    cases.push_back({"the other owner's colours", draw_news, {}, then(opened, [](const Fixture& f) { f.write32(f.manager() + 0x20, f.r32(f.manager() + 0x24)); f.write32(f.manager() + 0x24, 0); })});
    cases.push_back({"no club", draw_news, {}, then(opened, [](const Fixture& f) { f.write32(f.manager() + 0x24, 0); })});
    cases.push_back({"no club, no nation", draw_news, {}, then(opened, [](const Fixture& f) { f.write32(f.manager() + 0x24, 0); f.write32(f.manager() + 0x20, 0); })});
    cases.push_back({"filter with text", draw_news, {}, then(drawn, [](const Fixture& f) { f.set_filter("a"); })});
    for (const auto waiting : {0u, 1u}) for (const auto highlight : {false, true}) {
        const auto setup = [waiting, highlight](const Fixture& f) {
            f.write16(f.screens() + f.row() + 0x14, waiting);
            f.set_highlight(highlight);
        };
        cases.push_back({"waiting " + std::to_string(waiting) + (highlight ? " highlight" : ""), draw_news, {}, then(then(messages(3), [](const Fixture& f) { f.open(); f.draw(); }), setup)});
    }

    // Commands: what the action word says, through the control's record or the dialog's leftover.
    const std::vector<std::uint32_t> actions{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0x3e8, 0x3e9, 0x3ea, 0xffff, 0x8000};
    for (const auto action : actions) {
        const auto label = "action " + std::to_string(action);
        cases.push_back({label + " left behind", news_command, {0xffffffff}, then(drawn, [action](const Fixture& f) { f.set_pending(action); })});
        for (const auto value : {0u, 1u, 3u})
            cases.push_back({label + " control value " + std::to_string(value), news_command, {5}, then(drawn, [action, value](const Fixture& f) { f.set_pending(0); f.set_control(5, action, value); })});
        cases.push_back({label + " control zero, left behind", news_command, {5}, then(drawn, [action](const Fixture& f) { f.set_pending(action); f.set_control(5, 0, 2); })});
        cases.push_back({label + " unread", news_command, {0xffffffff}, then(then(messages(4), [](const Fixture& f) { f.open(); f.draw(); }), [action](const Fixture& f) { f.mark_items(false); f.set(6, f.unread(0)); f.set_pending(action); })});
        cases.push_back({label + " read", news_command, {0xffffffff}, then(then(messages(4), [](const Fixture& f) { f.open(); f.draw(); }), [action](const Fixture& f) { f.mark_items(true); f.set(6, f.unread(0)); f.set_pending(action); })});
        cases.push_back({label + " on tab 1", news_command, {0xffffffff}, then(then(messages(4), [](const Fixture& f) { f.open(); f.draw(); }), [action](const Fixture& f) { f.mark_items(false); f.set(1, 1); f.set(6, f.unread(0)); f.set_pending(action); })});
    }
    for (const auto [handled, common] : {std::pair{0u, 0u}, std::pair{0u, 7u}, std::pair{0u, 0xffffffffu}, std::pair{0x80000000u, 3u}, std::pair{0xfffffff7u, 0u}})
        cases.push_back({"default, answered " + std::to_string(handled) + "," + std::to_string(common), news_command, {7}, then(drawn, [](const Fixture& f) { f.set_pending(0); f.set_control(7, 0, 0); }), {{0x102a54b0, handled}, {0x102a83a0, common}}});
    cases.push_back({"tab already chosen", news_command, {5}, then(drawn, [](const Fixture& f) { f.set_pending(0); f.set_control(5, 1, 0); })});
    cases.push_back({"open a callback", news_command, {5}, then(then(messages(2), [](const Fixture& f) { f.open(); f.draw(); }), [](const Fixture& f) { f.set_pending(0); f.set(6, f.unread(0)); f.set_control(5, 3, f.toc(-0x50ec)); })});
    cases.push_back({"unread changed", news_command, {0xffffffff}, then(drawn, [](const Fixture& f) { f.set(6, f.value(6) + 1); })});
    cases.push_back({"check flag, allowed", news_command, {0xffffffff}, then(drawn, [](const Fixture& f) { f.set(0, 1); f.set_screen_flag(true); })});
    cases.push_back({"check flag, refused", news_command, {0xffffffff}, then(drawn, [](const Fixture& f) { f.set(0, 1); f.set_screen_flag(false); })});
    cases.push_back({"no control", news_command, {0xffffffff}, drawn});
    for (const std::int32_t control : {0, 1, 2, 3, 10, 200, -2, 0x7fff}) cases.push_back({"control " + std::to_string(control), news_command, {std::uint32_t(control)}, drawn});

    // Closing.
    cases.push_back({"close", close_news, {}, drawn});
    cases.push_back({"close, nothing listed", close_news, {}, opened});

    // The dialog.
    cases.push_back({"message to the manager", open_message, {the_manager, 0}, drawn});
    cases.push_back({"message to all", open_message, {0, 0}, drawn});
    cases.push_back({"message, answering", open_message, {the_manager, 7}, drawn});
    for (const auto from_person : {0u, 1u}) {
        const auto label = std::string(from_person ? "reply to a person" : "reply to nobody");
        cases.push_back({label, open_reply, {the_manager, the_item}, then(messages(3), [from_person](const Fixture& f) { f.open(); f.draw(); f.choose_reply(from_person); })});
    }
    cases.push_back({"reply, no item", open_reply, {the_manager, 0x7ffffff0}, drawn});
    cases.push_back({"reply, no item, dialog already open", open_reply, {the_manager, 0x7ffffff0}, then(drawn, [](const Fixture& f) { f.call(open_message, {0, 0}); })});
    for (const auto from_person : {0u, 1u})
        cases.push_back({"reply, dialog already open " + std::to_string(from_person), open_reply, {the_manager, the_item},
                         then(messages(3), [from_person](const Fixture& f) { f.open(); f.draw(); f.choose_reply(from_person); f.call(open_message, {0, 0}); })});
    cases.push_back({"message, dialog already open", open_message, {the_manager, 0}, then(drawn, [](const Fixture& f) { f.call(open_message, {0, 0}); })});
    const auto dialog = [&](std::uint32_t recipient_is_manager, const char* text) {
        return then(drawn, [recipient_is_manager, text = std::string(text)](const Fixture& f) {
            f.call(open_message, {recipient_is_manager ? f.manager() : 0, 0});
            f.write_text(f.value(1), text);
        });
    };
    for (const auto to_manager : {0u, 1u}) for (const auto text : {"", "Hello"}) {
        const auto label = std::string(to_manager ? " to the manager" : " to all") + (*text ? " with text" : " empty");
        cases.push_back({"draw dialog" + label, draw_message, {}, dialog(to_manager, text)});
        // The dialog's own actions end here; any other goes on to the common screens, whose bodies read the
        // disk in a way no snapshot answers, so those run answered: their arguments are still compared.
        for (const auto action : {6u, 7u, 8u})
            cases.push_back({"dialog action " + std::to_string(action) + label, message_command, {0xffffffff}, then(dialog(to_manager, text), [action](const Fixture& f) { f.set_pending(action); })});
        for (const auto action : actions) {
            for (const auto [handled, common] : {std::pair{0u, 0u}, std::pair{0u, 7u}, std::pair{0xfffffff7u, 0u}})
                cases.push_back({"dialog action " + std::to_string(action) + " answered " + std::to_string(handled) + "," + std::to_string(common) + label, message_command, {0xffffffff},
                                 then(dialog(to_manager, text), [action](const Fixture& f) { f.set_pending(action); }), {{0x102a54b0, handled}, {0x102a83a0, common}}});
        }
        cases.push_back({"dialog no buffer" + label, message_command, {0xffffffff}, then(dialog(to_manager, text), [](const Fixture& f) { f.set(1, 0); f.set_pending(7); })});
        cases.push_back({"send, no buffer" + label, message_command, {0xffffffff}, then(dialog(to_manager, text), [](const Fixture& f) { f.set(1, 0); f.set_pending(8); })});
        cases.push_back({"dialog control" + label, message_command, {5}, then(dialog(to_manager, text), [](const Fixture& f) { f.set_pending(0); f.set_control(5, 8, 0); })});
    }
    for (const auto humans : {0u, 1u, 2u, 16u}) cases.push_back({"send to " + std::to_string(humans) + " others", message_command, {0xffffffff}, then(dialog(0, "Hi"), [humans](const Fixture& f) { f.flag_people(humans); f.set_pending(8); })});
    return cases;
}

// What the forked cases add up, in memory shared with the parent.
struct Shared {
    struct Entry { std::atomic<std::uint64_t> cases, returned, faulted, timed_out, mutated, mutated_returned, mutated_faulted, mutated_timed_out; };
    std::array<Entry, 8> entries;
    std::atomic<std::uint64_t> findings, traced_pairs;
    std::atomic<unsigned> finding_kinds;
};
Shared* shared = nullptr;
unsigned slot_of(std::uint32_t entry) {
    const auto list = news_screens_production_entries();
    return unsigned(std::ranges::find(list, entry) - list.begin());
}

constexpr unsigned all_results = RoutineChecker::result_r3 | RoutineChecker::result_f1;

// One case, in its own child: its setup, its comparison, its trace and its mutations.
int run_case(State& s, const Case& item, unsigned index) {
    Fixture fixture{s};
    const auto saved = s.registers;
    if (item.setup) item.setup(fixture);
    s.registers = saved;
    guest::Cpu c(s);
    // Case lists name the manager by a sentinel: the state decides who it is.
    unsigned next_argument = 3;
    for (const auto value : item.arguments) c.set(next_argument++, value == the_manager ? fixture.manager() : value == the_item ? reply_item : value);
    c.set_lr(return_sentinel);
    s.pc = item.entry;
    s.budget = s.executed + 4'000'000'000ULL;
    current_label = item.label;
    case_answers = &item.answers;
    auto& tally = shared->entries[slot_of(item.entry)];
    const auto outcome = compare_call(s, item.entry, all_results, options.strict);
    ++tally.cases; tally.returned += outcome.returned; tally.faulted += outcome.faulted; tally.timed_out += outcome.timed_out;
    if (options.show_faults && !outcome.returned) std::cerr << "not returned: " << item.label << '\n';
    if (outcome.timed_out) std::cerr << "A comparison child hung or died: " << item.label << '\n';
    if (!outcome.differences.empty()) report_difference(item.entry, "Case", outcome.differences, item.label);
    if (options.call_trace) compare_traces(s, item.entry);
    std::mt19937 random(options.seed + index);
    mutate_call(s, item.entry, all_results, random);
    if (const auto found = mutated_tally.find(item.entry); found != mutated_tally.end()) {
        tally.mutated += found->second.calls; tally.mutated_returned += found->second.returned;
        tally.mutated_faulted += found->second.faulted; tally.mutated_timed_out += found->second.timed_out;
    }
    if (options.call_trace) {
        shared->traced_pairs += traced_pairs;
        shared->findings += trace_findings.size();
        for (const auto& [key, finding] : trace_findings) shared->finding_kinds |= std::get<2>(key) >= wrong_return ? 1u << (std::get<2>(key) - wrong_return) : 8u;
        print_trace_findings();
    }
    return 0;
}

int run_cases(State& s) {
    const auto cases = all_cases();
    std::size_t next = 0;
    unsigned running = 0;
    int failures = 0, caught = 0;
    constexpr int caught_by_the_control = 3;
    const auto reap = [&] {
        int status = 0;
        if (::wait(&status) <= 0) throw std::runtime_error("wait");
        --running;
        if (options.corrupt && WIFEXITED(status) && WEXITSTATUS(status) == caught_by_the_control) ++caught;
        else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) ++failures;
    };
    while (next < cases.size()) {
        const auto index = next++;
        const auto& item = cases[index];
        if (!options.only.empty() && !options.only.count(item.entry)) continue;
        while (running >= options.jobs) reap();
        std::cout.flush(); std::cerr.flush();
        const auto pid = ::fork();
        if (pid < 0) throw std::runtime_error("fork");
        if (pid == 0) {
            int code = 2;
            try { code = run_case(s, item, unsigned(index)); }
            catch (const std::exception& error) {
                std::cerr << "[" << item.label << "] " << error.what() << '\n';
                if (options.corrupt && std::string_view(error.what()).find("differs") != std::string_view::npos) code = caught_by_the_control;
            }
            std::cout.flush(); std::cerr.flush();
            flush_child();
            _exit(code);
        }
        ++running;
    }
    while (running) reap();
    // The control must have been caught by a comparison, with nothing else going wrong.
    if (options.corrupt && !caught) { std::cerr << "The negative control found no difference\n"; ++failures; }
    return failures;
}

// Whether every entry returned from a comparison. A routine no case reached has no evidence,
// however clean the rest of the run is.
bool summary() {
    std::uint64_t cases = 0, returned = 0, faulted = 0, timed = 0, mutated = 0, mutated_returned = 0, mutated_faulted = 0, mutated_timed = 0;
    unsigned reached = 0;
    bool complete = true;
    const auto entries = news_screens_production_entries();
    for (std::size_t n = 0; n < entries.size(); ++n) {
        const auto& tally = shared->entries[n];
        cases += tally.cases; returned += tally.returned; faulted += tally.faulted; timed += tally.timed_out;
        mutated += tally.mutated; mutated_returned += tally.mutated_returned; mutated_faulted += tally.mutated_faulted; mutated_timed += tally.mutated_timed_out;
        std::cout << hex_address(entries[n]) << " cases=" << tally.cases << " returned=" << tally.returned << " faulted=" << tally.faulted
                  << " timeout=" << tally.timed_out << " mutated=" << tally.mutated << " mutated_returned=" << tally.mutated_returned
                  << " mutated_faulted=" << tally.mutated_faulted << " mutated_timeout=" << tally.mutated_timed_out << '\n';
        if (tally.returned) ++reached;
        else if (options.require_all) { std::cerr << "No comparison returned at " << hex_address(entries[n]) << '\n'; complete = false; }
    }
    std::cout << "cases=" << cases << " case_returned=" << returned << " case_faults=" << faulted << " case_timeouts=" << timed
              << " mutated=" << mutated << " mutated_returned=" << mutated_returned << " mutated_faults=" << mutated_faulted
              << " mutated_timeouts=" << mutated_timed << " entries_compared=" << reached << " call_trace_pairs=" << shared->traced_pairs
              << " trace_findings=" << shared->findings << '\n';
    // A child that hung or died compared nothing, and one side alone may have done it.
    return complete && !timed && !mutated_timed;
}
}

namespace {
// The title's jump table in the game's data against the C++ test: all 256 first bytes.
void check_title_table(State& s) {
    constexpr std::uint32_t table = 0x108ce92c, vowel_label = 0x102ced24, other_label = 0x102ced38;
    for (int value = -128; value < 128; ++value) {
        const auto letter = std::int8_t(value);
        const auto index = std::uint32_t(std::int32_t(letter)) - 0x41;
        const auto label = index > 0x38 ? other_label : std::uint32_t(s.memory.read<4>(table + index * 4));
        if ((label == vowel_label) != news_screens::detail::starts_with_vowel(letter)) throw std::runtime_error("NEWS_SCREENS.CPP routine differs from the original: title table at " + std::to_string(value));
    }
}
}

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: cm-news-screens-checks SNAPSHOT [--jobs N] [--mutate K] [--seed N] [--only 0x..,0x..] [--corrupt] [--strict] [--require-all] [--show-faults] [--call-trace] [--corrupt-trace] [--production] [--null-page] [--argument-masks FILE]\n"; return 2; }
    try {
        options.input = argv[1];
        for (int n = 2; n < argc; ++n) {
            const std::string_view option(argv[n]);
            const auto value = [&] { if (n + 1 >= argc) throw std::runtime_error("missing value"); return std::string(argv[++n]); };
            if (option == "--jobs") options.jobs = unsigned(std::stoul(value()));
            else if (option == "--mutate") options.mutate = unsigned(std::stoul(value()));
            else if (option == "--seed") options.seed = std::uint32_t(std::stoul(value()));
            else if (option == "--only") { std::stringstream list(value()); for (std::string item; std::getline(list, item, ',');) options.only.insert(std::uint32_t(std::stoul(item, nullptr, 16))); }
            else if (option == "--corrupt") options.corrupt = true;
            else if (option == "--strict") options.strict = true;
            else if (option == "--require-all") options.require_all = true;
            else if (option == "--show-faults") options.show_faults = true;
            else if (option == "--call-trace") options.call_trace = true;
            else if (option == "--corrupt-trace") options.call_trace = options.corrupt_trace = true;
            else if (option == "--production") options.production = true;
            // Maps low memory, so a path through a null pointer runs on as on the original machine
            // and returns to be compared, instead of counting as a fault on both sides.
            else if (option == "--null-page") options.null_page = true;
            else if (option == "--argument-masks") {
                std::ifstream masks(value());
                if (!masks) throw std::runtime_error("Missing argument masks");
                std::string target, mask;
                while (masks >> target >> mask) argument_masks[std::uint32_t(std::stoul(target, nullptr, 16))] = unsigned(std::stoul(mask, nullptr, 16));
            }
            else throw std::runtime_error("Unknown option " + std::string(option));
        }
        auto state = std::make_unique<State>();
        load_match_snapshot(*state, options.input);
        state->memory.map(scratch_base, scratch_size);  // a case's own rows; nothing of the game's lives here
        if (options.null_page) state->memory.map(0, 0x10000);
        state->code_fragment_observer = &inputs;
        routine_child_exit = flush_child;
        // The assertion's error dialog needs Toolbox imports a world snapshot has no
        // host for. Both sides answer it here, so what follows the assert is compared
        // instead of stopping at the first dialog import.
        scripted_services = [](State& target) {
            constexpr std::uint32_t exit_entry = 0x1076f7d0, dialog_entry = 0x10769b60, allocate_entry = 0x107735f0;
            // The person's portrait comes from the disk: both sides answer "none", another module's
            // business (it only matters here as a call made, with its arguments).
            constexpr std::uint32_t person_screen = 0x107136d0;
            if (target.pc == person_screen) { target.pc = target.r<4>(4128); return true; }
            if (case_answers) for (const auto& [entry, answer] : *case_answers) if (target.pc == entry) {
                guest::Cpu(target).set(3, answer);
                target.pc = target.r<4>(4128);
                return true;
            }
            // A name that starts with the case's letter, so each way through the title's table is taken.
            constexpr std::uint32_t person_name = 0x100caeb0;
            if (forced_letter >= 0 && target.pc == person_name) {
                guest::Cpu c(target);
                const auto name = c.r(4);
                c.put8(name, std::uint32_t(forced_letter)); c.put8(name + 1, 'x'); c.put8(name + 2, 0);
                target.pc = target.r<4>(4128);
                return true;
            }
            if (target.pc == exit_entry) throw ApplicationExit{};
            if (allocation_fails && target.pc == allocate_entry) {
                guest::Cpu(target).set(3, 0);
                target.pc = target.r<4>(4128);
                return true;
            }
            if (target.pc != dialog_entry) return false;
            target.pc = target.r<4>(4128);
            return true;
        };
        dispatch_probe = scripted_services;
        auto& s = *state;
        // A snapshot taken outside a routine has no TOC pointer in r2.
        guest::Cpu(s).set(2, 0x108bba90);
        check_title_table(s);
        // Other modules' routines run as their originals on both sides.
        if (options.production) return production_proof(s);
        shared = static_cast<Shared*>(::mmap(nullptr, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0));
        if (shared == MAP_FAILED) throw std::runtime_error("mmap");
        new (shared) Shared{};
        const auto wanted = [](std::uint32_t entry) {
            return news_screens_lists(entry) && (options.only.empty() || options.only.count(entry) != 0);
        };
        RoutineChecker checker(wanted, 0, 0);
        checker.compare_f1 = true;
        checker.strict_volatile = options.strict;
        routine_checker = &checker;
        const auto failures = run_cases(s);
        const bool complete = summary();
        if (options.corrupt_trace) return shared->finding_kinds == 0xf ? 0 : 1;
        if (options.corrupt) return failures ? 1 : 0;
        return failures || !complete || shared->findings ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
