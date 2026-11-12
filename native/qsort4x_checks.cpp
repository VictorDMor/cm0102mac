// The sort family against its generated reference, from fixtures: every entry,
// row widths and counts around each sort's edges, input orders, a scripted
// comparator (consistent, constant, inconsistent, register-clobbering) and one
// of the game's own. Each comparator call is traced with every register and
// the sort's frame, so the order of the calls and what a comparator could
// save are compared, not only the sorted rows.
#include "qsort4x_adapter.hpp"
#include "qsort4x.hpp"
#include "routine_check.hpp"
#include "native_sort8.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <iostream>
#include <map>
#include <string_view>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool qsort4x_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, rows_at = data + 0x1000, vector_at = data + 0x100,
    scripted_code = data + 0x200, trace_at = data + 0x40000, detail_at = trace_at + 0x10,
    stack = 0x71010000, sentinel = 0xfffffff0, game_toc = 0x108bba90;
constexpr std::uint32_t entry_sort = 0x10768300, entry_bytes = 0x10768340, entry_words = 0x107684c0,
    entry_pairs = 0x10768660, module_end = 0x10768808, glue = 0x10781320, failed_assertion = 0x107755c0;
constexpr std::array entries{entry_sort, entry_bytes, entry_words, entry_pairs};
// The pair comparator of the club shortlists (0x103663b0), through the game's own vector.
constexpr std::int32_t game_pair_comparator = -0x6298;
constexpr unsigned detailed_calls = 48, words_per_call = 64;

enum class Order { ascending, descending, equal, random, few_keys };
enum class Compare { by_key, always_equal, always_before, always_after, erratic, game_pairs };
struct Case {
    std::uint32_t entry = entry_sort, width = 8, count = 5;
    Order order = Order::random;
    Compare compare = Compare::by_key;
    bool clobber = false;
    std::uint32_t seed = 1;
};
Case active;
bool original_mode = false, production_mode = false, ownership_mode = false;
std::uint32_t comparator_calls = 0, script = 0;

std::uint32_t next(std::uint32_t& state) { return state = state * 1664525u + 1013904223u; }

// One word of the trace; the running digest is kept in guest memory, which
// the comparison covers, and the first calls are kept whole.
void note(Cpu& c, std::uint32_t value) {
    c.put32(trace_at + 4, (c.u32(trace_at + 4) ^ value) * 16777619u);
    const auto used = c.u32(trace_at + 8);
    if (used < detailed_calls * words_per_call) {
        c.put32(detail_at + used * 4, value);
        c.put32(trace_at + 8, used + 1);
    }
}
void trace_call(Cpu& c, std::uint32_t target) {
    c.put32(trace_at, c.u32(trace_at) + 1);
    note(c, target); note(c, c.lr()); note(c, std::uint32_t(c.s.r<4>(reg::ctr)));
    for (unsigned r = 0; r < 32; ++r) note(c, c.r(r));
    for (unsigned f = 0; f < 8; ++f) note(c, c.cr(f));
    note(c, std::uint32_t(c.s.r<1>(reg::xer_ca)));
    // The caller's frame: back chain, saved TOC, and the prologue image above it.
    note(c, c.u32(c.sp())); note(c, c.u32(c.sp() + 0x14));
    for (std::uint32_t at = 0x70 - 0x2c; at <= 0x70 + 8; at += 4) note(c, c.u32(c.sp() + at));
}
std::int32_t key(Cpu& c, std::uint32_t row) {
    if (active.width == 0) return 0;
    return active.width >= 4 ? c.i32(row) : c.i8(row);
}
bool probe(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    if (target >= entry_sort && target < module_end) {
        if (original_mode) return qsort4x_reference(s);
        if (production_mode) return false;
        if (qsort4x_lists(target)) { qsort4x_run_rewrite(s, target); return true; }
        return false;
    }
    if (target == scripted_code || target == failed_assertion ||
        (ownership_mode && target == 0x103663b0)) {
        if (++comparator_calls > 200000) throw std::runtime_error("fixture call bound");
        trace_call(c, target);
        std::int32_t answer = 0;
        if (target == failed_assertion) answer = 0x5a5a0000;
        else if (target == 0x103663b0) answer = c.i32(c.r(3) + 4) - c.i32(c.r(4) + 4);
        else switch (active.compare) {
            case Compare::by_key: { const auto a = key(c, c.r(3)), b = key(c, c.r(4)); answer = a < b ? -7 : a > b ? 9 : 0; break; }
            case Compare::always_equal: answer = 0; break;
            case Compare::always_before: answer = std::int32_t(0x80000000); break;
            case Compare::always_after: answer = 0x7fffffff; break;
            default: answer = std::int32_t(next(script) >> 13) % 5 - 2; break;
        }
        if (active.clobber) {
            for (unsigned r = 4; r <= 12; ++r) c.set(r, 0x41000000 + comparator_calls * 0x100 + r);
            c.set(0, 0x40404040); c.set(2, 0x7e7e0000);
            for (unsigned f : {0u, 1u, 5u, 6u, 7u}) c.set_cr(f, (comparator_calls + f) & 0xf);
            c.s.wr<1>(reg::xer_ca, 0); c.s.wr<4>(reg::ctr, 0x43434343);
        }
        // With a budget of none left, any generated instruction of the family fails.
        if (ownership_mode) s.budget = s.executed;
        c.set(3, std::uint32_t(answer));
        s.pc = c.lr();
        return true;
    }
    if (ownership_mode && target == glue) {
        // The comparator glue is a dependency: its five instructions may run.
        s.budget = s.executed + 5;
        return false;
    }
    return run_leaf(s);
}
void initialize(Cpu& c) {
    c.s.memory.map(data, 0x80000);
    c.s.memory.map(0x71000000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x80000, 0xa5));
    c.s.memory.copy_in(0x71000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, f + 1);
    c.set(1, stack); c.set(2, game_toc); c.set_lr(sentinel);
    c.put32(vector_at, scripted_code); c.put32(vector_at + 4, 0x6e6e0000);
    c.put32(trace_at, 0); c.put32(trace_at + 4, 2166136261u); c.put32(trace_at + 8, 0);
    auto state = active.seed * 2654435761u + 12345;
    const auto bytes = std::uint64_t(active.width) * active.count;
    if (bytes > 0x30000) throw std::runtime_error("fixture rows too large");
    for (std::uint32_t row = 0; row < active.count; ++row) {
        std::uint32_t value = 0;
        switch (active.order) {
            case Order::ascending: value = row * 3; break;
            case Order::descending: value = (active.count - row) * 3; break;
            case Order::equal: value = 77; break;
            case Order::random: value = next(state) >> 8; break;
            case Order::few_keys: value = (next(state) >> 20) % 3; break;
        }
        const auto at = rows_at + row * active.width;
        for (std::uint32_t i = 0; i < active.width; ++i) c.put8(at + i, next(state) >> 24);  // payload tells equal keys apart
        if (active.width >= 8) { c.put32(at, value - 40); c.put32(at + 4, (value % 5) - 2); }
        else if (active.width >= 4) c.put32(at, value - 40);
        else if (active.width) c.put8(at, value - 40);
    }
    const bool game = active.compare == Compare::game_pairs;
    c.set(3, rows_at); c.set(4, active.count); c.set(5, active.width);
    c.set(6, game ? c.toc(game_pair_comparator) : vector_at);
    c.s.budget = c.s.executed + 50000000;
    comparator_calls = 0; script = active.seed * 40503u + 7;
}
void original(State& s, std::uint32_t entry) { original_mode = true; s.pc = entry; run_until(s, sentinel); }
void rewrite(State& s, std::uint32_t entry) {
    original_mode = false;
    if (production_mode) { s.pc = entry; run_until(s, sentinel); }
    else qsort4x_run_rewrite(s, entry);
}
void spoil_row(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.put8(rows_at, c.u8(rows_at) ^ 1); }
void spoil_frame(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.put8(stack - 0x2c, c.u8(stack - 0x2c) ^ 1); }
void spoil_trace(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.put32(trace_at + 4, c.u32(trace_at + 4) ^ 1); }
void spoil_register(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.set(5, c.r(5) ^ 1); }
struct Observed final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
};
std::string describe(const Case& t) {
    return hex_address(t.entry) + " width=" + std::to_string(t.width) + " count=" + std::to_string(t.count) +
        " order=" + std::to_string(int(t.order)) + " compare=" + std::to_string(int(t.compare)) +
        " clobber=" + std::to_string(t.clobber) + " seed=" + std::to_string(t.seed);
}
std::vector<Case> all_cases() {
    std::vector<Case> cases;
    const auto widths = [](std::uint32_t entry) -> std::vector<std::uint32_t> {
        // The entry with every remainder of 8; each sort alone with widths it
        // is sent and widths only a direct call could give it (the assertion
        // of the word and pair sorts, and a remainder the swap leaves alone).
        if (entry == entry_sort) return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 16, 20, 24, 40};
        if (entry == entry_bytes) return {0, 1, 2, 3, 5, 7, 8, 13};
        if (entry == entry_words) return {0, 4, 8, 12, 2, 6, 9};
        return {0, 8, 16, 24, 4, 12, 7};
    };
    std::uint32_t seed = 0;
    for (auto entry : entries) for (auto width : widths(entry))
        for (std::uint32_t count : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 16u, 31u, 64u, 150u}) {
            if (width == 0 && count > 9) continue;
            const auto add = [&](Order order, Compare compare) {
                ++seed;  // the clobbering comparator on about half the cases, whatever their order and comparator
                Case t{entry, width, count, order, compare, ((seed * 2654435761u) >> 16 & 1) != 0, seed};
                cases.push_back(t);
            };
            for (auto order : {Order::ascending, Order::descending, Order::equal, Order::random, Order::few_keys})
                add(order, Compare::by_key);
            for (auto compare : {Compare::always_equal, Compare::always_before, Compare::always_after, Compare::erratic})
                add(Order::random, compare);
            add(Order::few_keys, Compare::erratic);
            if (width >= 8 && width % 8 == 0 && entry != entry_bytes && entry != entry_words) {
                add(Order::random, Compare::game_pairs);
                add(Order::few_keys, Compare::game_pairs);
            }
        }
    return cases;
}
}
int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) return 2;
    try {
        auto base = std::make_unique<State>();
        load_program(*base, argv[1]);
        dispatch_probe = probe;
        constexpr unsigned results = RoutineChecker::result_r3;
        if (argc == 3 && std::string_view(argv[2]) == "--original-routing") {
            // With CM_QSORT4X_ORIGINAL=1 no route may reach the C++.
            for (auto entry : entries) {
                active = Case{entry, 8, 6};
                auto state = cm::semantic_io_checks::clone(*base);
                Cpu c(*state); initialize(c);
                production_mode = true; state->pc = entry;
                if (try_modern_qsort4x(*state, entry) || qsort4x_dispatch_probe(*state))
                    throw std::runtime_error("original override accepted a replacement");
                const auto before = state->executed;
                qsort4x::call(c, entry, sentinel);
                if (state->executed == before || !comparator_calls)
                    throw std::runtime_error("original override did not execute the original body");
            }
            std::cout << "original override entry/probe/internal routes passed\n";
            return 0;
        }
        if (argc == 3) return 2;
        const auto cases = all_cases();
        unsigned completed = 0, failures = 0;
        std::map<std::uint32_t, std::array<unsigned, 2>> counts;
        for (bool production : {false, true}) {
            production_mode = production;
            for (const auto& test : cases) {
                active = test;
                auto s = cm::semantic_io_checks::clone(*base);
                Cpu c(*s); initialize(c);
                Observed observer;
                if (production) s->memory.observer = &observer;
                const auto differences = compare_routine_call(*s, test.entry, original, rewrite, results, false, true);
                if (!differences.empty()) {
                    std::cout << "DIFFERS production=" << production << ' ' << describe(test) << '\n';
                    for (const auto& d : differences) std::cout << "  " << d << '\n';
                    if (++failures >= 6) break;
                } else { ++completed; ++counts[test.entry][production ? 1 : 0]; }
            }
            if (failures) break;
        }
        // The entry handing a pair sort to the fused 8-byte sort (tools/native_sort8.py), which takes it only
        // without an instruction budget or a memory observer: the production route for the shortlists.
        unsigned fused = 0;
        production_mode = true;
        for (std::uint32_t count : {2u, 3u, 9u, 64u, 150u}) for (auto order : {Order::random, Order::few_keys}) {
            active = Case{entry_sort, 8, count, order, Compare::game_pairs, false, count};
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s); initialize(c);
            s->budget = UINT64_MAX;
            const auto differences = compare_routine_call(*s, entry_sort, original, rewrite, results, false, true);
            for (const auto& d : differences) std::cout << "  fused: " << d << '\n';
            const auto before = native_sort8_calls();
            rewrite(*s, entry_sort);
            if (!differences.empty() || native_sort8_calls() != before + 1) {
                ++failures; std::cout << "FAIL fused pair sort through the entry, count=" << count << '\n';
            } else ++fused;
        }
        production_mode = false;
        // The comparison must fail when a row, the prologue image, the call trace or a register differs.
        const std::pair<const char*, RoutineRunner> controls[]{
            {"row", spoil_row}, {"frame", spoil_frame}, {"trace", spoil_trace}, {"register", spoil_register}};
        for (const auto& [name, runner] : controls) for (auto entry : entries) {
            active = Case{entry, 8, 9};
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s); initialize(c);
            if (compare_routine_call(*s, entry, original, runner, results, false, true).empty()) {
                ++failures; std::cout << "FAIL " << name << " control at " << hex_address(entry) << '\n';
            }
        }
        if (!failures) std::cout << "ok spoiled row, frame, trace and register detected at every entry\n";
        // Production ownership: with no instruction budget, the normal route
        // must sort without running a generated instruction of the family,
        // and the reference must stop at once.
        unsigned ownership = 0;
        for (auto entry : entries) for (auto compare : {Compare::by_key, Compare::game_pairs}) {
            active = Case{entry, 8, 12, Order::random, compare};
            auto normal = cm::semantic_io_checks::clone(*base);
            Cpu nc(*normal); initialize(nc);
            normal->budget = normal->executed;
            Observed observer; normal->memory.observer = &observer;
            production_mode = true; ownership_mode = true;
            try { rewrite(*normal, entry); if (!comparator_calls) throw std::runtime_error("no comparator call"); ++ownership; }
            catch (const std::exception& e) { ++failures; std::cout << "FAIL production ownership " << hex_address(entry) << ' ' << e.what() << '\n'; }
            ownership_mode = false; production_mode = false;
            auto ref = cm::semantic_io_checks::clone(*base);
            Cpu rc(*ref); initialize(rc);
            ref->budget = ref->executed;
            bool rejected = false;
            try { original(*ref, entry); }
            catch (const std::exception& e) { rejected = std::string_view(e.what()) == "Native instruction budget exhausted"; }
            if (!rejected) { ++failures; std::cout << "FAIL original budget control\n"; }
        }
        for (const auto& [entry, n] : counts)
            std::cout << hex_address(entry) << " returned " << n[0] << " controlled + " << n[1] << " production\n";
        std::cout << ownership << "/8 production ownership controls; " << fused << "/10 fused pair sorts through the entry; "
                  << completed << " returned comparisons; " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
