// cm-database-checks: runs every block's fixtures (database_checks.hpp).
// Usage: cm-database-checks ORACLE_DIR [--only 0xENTRY,0xENTRY...]
#include "database_checks.hpp"
#include "database_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool database_reference(State&); }
namespace cm::aot::database_checks {
void record_call(Cpu& c) {
    const auto size = c.u32(trace_at);
    auto at = trace_at + 4 + size * 4;
    const auto add = [&](std::uint32_t value) {
        c.put32(at, value);
        at += 4;
    };
    add(c.s.pc);
    add(c.lr());
    for (unsigned r = 0; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned f = 0; f < 8; ++f) add(c.cr(f));
    c.put32(trace_at, (at - trace_at - 4) / 4);
}
}

namespace {
using namespace cm::aot::database_checks;
bool original_mode = false, production_mode = false, ownership_mode = false;
const Fixture* active = nullptr;

bool in_module(std::uint32_t pc) { return pc >= module_start && pc < module_end; }

bool probe(State& s) {
    if (ownership_mode) {
        // No budget while DATABASE.CPP's code runs; other modules' code may.
        s.budget = in_module(s.pc) ? s.executed : std::numeric_limits<std::uint64_t>::max();
    }
    if (active && active->stub && active->stub(s)) return true;
    if (!database_lists(s.pc)) return false;
    if (original_mode) return database_reference(s);
    if (production_mode) return false;  // the generated page and its hook
    database_run_rewrite(s, s.pc);
    return true;
}

void base(Cpu& c, const Fixture& f) {
    c.s.memory.map(data, data_size);
    c.s.memory.map(high, high_size);
    c.s.memory.map(stack_base, stack_size);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(data_size, 0xa5));
    c.s.memory.copy_in(high, std::vector<std::uint8_t>(high_size, 0xa5));
    c.s.memory.copy_in(stack_base, std::vector<std::uint8_t>(stack_size, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    for (unsigned n = 0; n < 32; ++n) c.set_f(n, 1.25 + n);
    for (unsigned n = 0; n < 8; ++n) c.set_cr(n, n + 1);
    c.set(1, stack);
    c.set(2, toc);
    c.set_lr(sentinel);
    c.s.wr<1>(reg::xer_so, f.summary_overflow);
    c.put32(trace_at, 0);
    if (f.setup) f.setup(c);
    c.s.budget = c.s.executed + 50'000'000;
}

void original(State& s, std::uint32_t entry) {
    original_mode = true;
    s.pc = entry;
    run_until(s, sentinel);
}
void rewrite(State& s, std::uint32_t entry) {
    original_mode = false;
    if (production_mode) {
        s.pc = entry;
        run_until(s, sentinel);
    } else {
        database_run_rewrite(s, entry);
    }
}
void corrupt_result(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    c.set(3, c.r(3) ^ 1);
}
void corrupt_trace(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    const auto at = trace_at + 4 + 4 * (2 + 4);  // the first call's r4
    c.put8(at, c.u8(at) ^ 1);
}

// The whole register file, byte for byte: r0-r31, f0-f31, XER, the FPSCR,
// CR, LR and CTR (compare_routine_call leaves the volatile FPRs out).
std::string register_differences(State& a, State& b) {
    const std::array<std::pair<std::size_t, std::size_t>, 7> ranges{{
        {reg::gpr(0), reg::gpr(32)}, {reg::fpr(0), reg::fpr(32)}, {1024, 1028}, {1280, 1312},
        {reg::cr(0), reg::cr(8)}, {reg::lr, reg::lr + 4}, {reg::ctr, reg::ctr + 4}}};
    for (const auto& [begin, end] : ranges)
        for (auto at = begin; at < end; ++at)
            if (a.r<1>(at) != b.r<1>(at))
                return "register byte " + std::to_string(at) + " original " + std::to_string(a.r<1>(at)) +
                       " rewrite " + std::to_string(b.r<1>(at));
    return {};
}

struct Observed final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
};
}

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    std::set<std::uint32_t> only;
    for (int i = 2; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--only" && i + 1 < argc) {
            std::istringstream list(argv[++i]);
            for (std::string item; std::getline(list, item, ',');) only.insert(std::uint32_t(std::stoul(item, nullptr, 16)));
        } else {
            std::cerr << "Unknown option " << argv[i] << '\n';
            return 2;
        }
    }
    try {
        auto program = std::make_unique<State>();
        load_program(*program, argv[1]);
        dispatch_probe = probe;
        std::vector<Fixture> all;
        for (const auto provider : {first_fixtures, lookup_fixtures, text_fixtures, generation_fixtures,
                                    search_fixtures, loading_fixtures})
            for (auto& f : provider())
                if (only.empty() || only.contains(f.entry)) all.push_back(std::move(f));
        unsigned completed = 0, failures = 0;
        std::map<std::uint32_t, std::array<unsigned, 2>> counts;
        for (const bool production : {false, true}) {
            production_mode = production;
            for (const auto& f : all) {
                active = &f;
                auto s = cm::semantic_io_checks::clone(*program);
                Cpu c(*s);
                base(c, f);
                Observed observer;
                if (production) s->memory.observer = &observer;
                auto differences = compare_routine_call(*s, f.entry, original, rewrite, f.results, false, true);
                if (differences.empty()) {
                    auto a = cm::semantic_io_checks::clone(*s), b = cm::semantic_io_checks::clone(*s);
                    try {
                        original(*a, f.entry);
                        if (production) b->memory.observer = &observer;
                        rewrite(*b, f.entry);
                        if (auto found = register_differences(*a, *b); !found.empty()) differences.push_back(found);
                    } catch (const std::exception& e) {
                        differences.push_back(std::string("in-process run failed: ") + e.what());
                    }
                }
                if (!differences.empty()) {
                    std::cout << "DIFFERS " << hex_address(f.entry) << " production=" << production << ' '
                              << f.description << '\n';
                    for (const auto& d : differences) std::cout << "  " << d << '\n';
                    if (++failures >= 6) break;
                } else {
                    ++completed;
                    ++counts[f.entry][production ? 1 : 0];
                }
            }
            if (failures) break;
        }
        production_mode = false;
        // The comparison must fail when the rewrite is wrong: its result, and
        // (where it calls stubbed code) the arguments it passes.
        std::set<std::uint32_t> controlled;
        for (const auto& f : all) {
            if (!controlled.insert(f.entry).second) continue;
            active = &f;
            for (const auto spoiler : {corrupt_result, corrupt_trace}) {
                auto s = cm::semantic_io_checks::clone(*program);
                Cpu c(*s);
                base(c, f);
                if (spoiler == corrupt_trace) {
                    // Only where the rewrite records a call.
                    auto probe_run = cm::semantic_io_checks::clone(*s);
                    rewrite(*probe_run, f.entry);
                    if (Cpu(*probe_run).u32(trace_at) == 0) continue;
                }
                if (compare_routine_call(*s, f.entry, original, spoiler, f.results, false, true).empty()) {
                    ++failures;
                    std::cout << "FAIL corruption control " << hex_address(f.entry) << '\n';
                }
            }
        }
        // Production ownership: no instruction of DATABASE.CPP's generated
        // code runs on the normal route.
        std::map<std::uint32_t, bool> owned;
        unsigned owned_calls = 0;
        production_mode = ownership_mode = true;
        for (const auto& f : all) {
            active = &f;
            auto s = cm::semantic_io_checks::clone(*program);
            Cpu c(*s);
            base(c, f);
            s->budget = s->executed;
            try {
                rewrite(*s, f.entry);
                ++owned_calls;
                owned.try_emplace(f.entry, true);
            } catch (const std::exception& e) {
                owned[f.entry] = false;
                if (++failures <= 6)
                    std::cout << "FAIL production ownership " << hex_address(f.entry) << ": " << e.what() << '\n';
            }
        }
        production_mode = ownership_mode = false;
        active = nullptr;
        if (only.empty())
            for (const auto entry : database_entries())
                if (!owned.contains(entry)) {
                    ++failures;
                    std::cout << "FAIL no fixture for " << hex_address(entry) << '\n';
                }
        for (const auto& [entry, n] : counts)
            std::cout << hex_address(entry) << " returned " << n[0] << " controlled + " << n[1] << " production\n";
        const auto entries_owned = std::ranges::count_if(owned, [](const auto& item) { return item.second; });
        std::cout << entries_owned << '/' << owned.size() << " routines owned in production over " << owned_calls
                  << " calls; " << controlled.size() << " routines' corruption controls; " << completed
                  << " returned comparisons; " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
