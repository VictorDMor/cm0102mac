// Original-versus-C++ checks of media.cpp from a real career state. See
// media_checks.hpp. Two kinds of evidence, reported separately:
//   cases    the listed calls from the blocks' case lists, on the real world;
//   mutated  a call repeated with changed input values the original really read,
//            so branches the state did not take run on both sides.
// Real calls come from the capture replays (cm-world-replay --check-routines).
// A mutated call in which both sides fault alike is counted apart, never as a pass.
// --corrupt is the negative control: a check must then fail.
#include "media_checks.hpp"
#include "application_exit.hpp"
#include "code_fragment_adapter.hpp"
#include "guest_abi.hpp"
#include "media_adapter.hpp"
#include "match_replay.hpp"
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
#include <tuple>

#if defined(CM_MEDIA_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif

using namespace cm::aot;
using namespace cm::aot::media_checks;

namespace cm::aot::media_checks {
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
    unsigned all = 40, every = 400, mutate = 0, mutated_calls = 4;
    std::uint32_t seed = 1;
    std::set<std::uint32_t> only;
    bool corrupt = false, strict = false, require_all = false, show_faults = false, call_trace = false, corrupt_trace = false, production = false, null_page = false;
} options;

struct Tally { std::uint64_t returned = 0, faulted = 0, timed_out = 0, calls = 0; };
std::map<std::uint32_t, Tally> case_tally, mutated_tally;
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
#if defined(CM_MEDIA_COVERAGE)
    __llvm_profile_write_file();
#endif
}
// Mutated inputs can loop for a very long time on both sides.
void bounded_original(State& s, std::uint32_t entry) { alarm(60); s.budget = s.executed + 4'000'000'000ULL; media_run_original(s, entry); }
void bounded_rewrite(State& s, std::uint32_t entry) {
    alarm(60);
    s.budget = s.executed + 4'000'000'000ULL;
    media_run_rewrite(s, entry);
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
        try { s.budget = s.executed + 4'000'000'000ULL; media_run_original(s, entry); } catch (...) {}
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
const std::vector<std::pair<std::uint32_t, std::uint32_t>>* case_answers = nullptr;
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
        // Memory isn't compared at a fault (writes needn't be in the same order), but what the
        // imports saw up to it is, and a difference there still counts.
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
    if (back >= 0x10646d94 && back <= 0x106a8a50 && traced_calls->size() < 2'000'000) {
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
    throw std::runtime_error("media.cpp routine differs from the original");
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
        media_declining = original;
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
    const auto entries = media_production_entries();
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
    return failures || entries.size() != 75 ? 1 : 0;
}

unsigned results_of(std::uint32_t) { return RoutineChecker::result_r3 | RoutineChecker::result_f1; }

void run_cases(State& s, std::mt19937& random) {
    Scenario scenario{s};
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
        current_label = item.label;
        case_answers = &item.answers;
        const auto outcome = compare_call(s, item.entry, results_of(item.entry), options.strict);
        auto& tally = case_tally[item.entry];
        ++tally.calls; tally.returned += outcome.returned; tally.faulted += outcome.faulted; tally.timed_out += outcome.timed_out;
        if (!outcome.differences.empty()) report_difference(item.entry, "Case", outcome.differences, item.label);
        if (options.call_trace && (tally.calls <= options.mutated_calls * 8 || tally.calls % 64 == 0)) compare_traces(s, item.entry);
        // One routine can have thousands of cases; a few of them seed its mutations.
        if (tally.calls <= options.mutated_calls * 8 || tally.calls % 64 == 0) mutate_call(s, item.entry, results_of(item.entry), random);
        s.memory.observer = nullptr;
        for (auto w = undo.rbegin(); w != undo.rend(); ++w) {
            if (w->first.size == 1) s.memory.write<1>(w->first.address, w->second & 0xff);
            else if (w->first.size == 2) s.memory.write<2>(w->first.address, w->second & 0xffff);
            else s.memory.write<4>(w->first.address, w->second);
        }
        s.memory.observer = saved_observer;
        case_answers = nullptr;
    }
    s.registers = saved;
}

// Whether every routine in production returned from a comparison. A routine no case
// reached has no evidence, however clean the rest of the run is.
bool summary(const RoutineChecker* checker) {
    std::uint64_t nested = 0, returned = 0, faulted = 0, timed = 0, cases = 0, case_faults = 0;
    std::set<std::uint32_t> reached;
    // The other modules' routines are checked on the way too; only this one's count.
    if (checker) for (const auto& [entry, totals] : checker->totals()) if (media_lists(entry)) { nested += totals.checked; if (totals.checked) reached.insert(entry); }
    for (const auto& [entry, tally] : mutated_tally) { returned += tally.returned; faulted += tally.faulted; timed += tally.timed_out; if (tally.returned) reached.insert(entry); }
    for (const auto& [entry, tally] : case_tally) { cases += tally.returned; case_faults += tally.faulted; if (tally.returned) reached.insert(entry); }
    std::cout << "nested_comparisons=" << nested << " case_returned=" << cases << " case_faults=" << case_faults
              << " mutated_returned=" << returned << " mutated_faults=" << faulted << " mutated_timeouts=" << timed
              << " entries_compared=" << reached.size() << '\n';
    if (const char* file = std::getenv("CM_MEDIA_REPORT")) {
        std::ofstream out(file);
        if (checker) for (const auto& [entry, totals] : checker->totals()) if (media_lists(entry))
            out << "nested " << hex_address(entry) << " calls=" << totals.calls << " checked=" << totals.checked << '\n';
        for (const auto& [entry, tally] : case_tally) out << "case " << hex_address(entry) << " returned=" << tally.returned << " faulted=" << tally.faulted << " timeout=" << tally.timed_out << '\n';
        for (const auto& [entry, tally] : mutated_tally) out << "mutated " << hex_address(entry) << " returned=" << tally.returned << " faulted=" << tally.faulted << " timeout=" << tally.timed_out << '\n';
    }
    if (!options.require_all) return true;
    bool complete = true;
    for (const auto entry : media_production_entries())
        if (!reached.count(entry)) { std::cerr << "No comparison reached " << hex_address(entry) << '\n'; complete = false; }
    return complete;
}
}

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: cm-media-checks WORLD_SNAPSHOT [--all N] [--every N] [--mutate K] [--mutated-calls N] [--seed N] [--only 0x..,0x..] [--corrupt] [--strict] [--require-all] [--show-faults] [--call-trace] [--corrupt-trace] [--production] [--null-page] [--argument-masks FILE]\n"; return 2; }
    try {
        options.input = argv[1];
        for (int n = 2; n < argc; ++n) {
            const std::string_view option(argv[n]);
            const auto value = [&] { if (n + 1 >= argc) throw std::runtime_error("missing value"); return std::string(argv[++n]); };
            if (option == "--all") options.all = unsigned(std::stoul(value()));
            else if (option == "--every") options.every = unsigned(std::stoul(value()));
            else if (option == "--mutate") options.mutate = unsigned(std::stoul(value()));
            else if (option == "--mutated-calls") options.mutated_calls = unsigned(std::stoul(value()));
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
        state->memory.map(scratch_base, scratch_size);  // for a case's own rows; nothing of the game's lives here
        if (options.null_page) state->memory.map(0, 0x10000);
        state->code_fragment_observer = &inputs;
        routine_child_exit = flush_child;
        // The assertion's error dialog needs Toolbox imports a world snapshot has no
        // host for. Both sides answer it here, so what follows the assert is compared
        // instead of stopping at the first dialog import.
        scripted_services = [](State& target) {
            // exit(): nothing follows it in the game, so a run ends here and what it
            // wrote up to this call is compared.
            constexpr std::uint32_t exit_entry = 0x1076f7d0, dialog_entry = 0x10769b60;
            if (target.pc == exit_entry) throw ApplicationExit{};
            // A routine under check is never answered: its C++ doesn't come through here, so only the
            // original would get the answer. It runs for real on both sides.
            if (case_answers) for (const auto& [entry, answer] : *case_answers) if (target.pc == entry && !(routine_checker && routine_checker->wants(entry))) {
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
        auto& s = *state;
        // Other modules' routines run as their originals on both sides.
        if (options.production) return production_proof(s);
        const auto wanted = [](std::uint32_t entry) {
            return media_lists(entry) && (options.only.empty() || options.only.count(entry) != 0);
        };
        RoutineChecker checker(wanted, options.all, options.every);
        checker.corrupt_first = options.corrupt;
        checker.compare_f1 = true;
        checker.strict_volatile = options.strict;
        routine_checker = &checker;
        run_cases(s, random);
        const bool complete = summary(&checker);
        if (options.call_trace) print_trace_findings();
        if (options.corrupt_trace) {
            std::set<unsigned> kinds;
            for (const auto& [key, finding] : trace_findings) kinds.insert(std::get<2>(key) < 100 ? 0 : std::get<2>(key));
            return kinds.count(wrong_return) && kinds.count(wrong_count) && kinds.count(wrong_order) && kinds.count(0) ? 0 : 1;
        }
        if (options.corrupt) { std::cerr << "The negative control found no difference\n"; return 1; }
        return checker.differences() || !complete || !trace_findings.empty() ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return options.corrupt && std::string_view(error.what()).find("differs") != std::string_view::npos ? 0 : 2; }
}
