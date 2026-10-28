// Callable sampler paths; the signed reduced bound cannot reach a second
// wide level. Artificial stack aliases are separate from ordinary pool cases.
#include "news_adapter.hpp"
#include "news_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <iostream>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool news_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x62000000, bias_at = data, cursor_at = data + 4,
    end_at = data + 8, words = data + 0x100, constants = data + 0x200,
    fp_log = data + 0x300, stack = 0x73010000, sentinel = 0xfffffff0;
struct Scenario {
    std::uint32_t bound, bias = 0, first = 0, second = 0;
    unsigned cursor = 0, end = 6, sticky = 0, alias = 0;
};
Scenario active{};
bool reference_mode = false;
unsigned mutation = 0;
bool probe(State& s) {
    if (run_leaf(s)) return true;
    if (reference_mode && s.pc >= 0x102bdf90 && s.pc < 0x102ce710) return news_reference(s);
    if (news_lists(s.pc)) { news_run_rewrite(s, s.pc); return true; }
    return false;
}
void initialize(Cpu& c) {
    c.s.memory.map(data, 0x1000); c.s.memory.map(stack - 0x10000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x1000, 0xa5));
    c.s.memory.copy_in(stack - 0x10000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) { c.set(r, 0x55000000 + r * 0x101); c.set_f(r, r + 0.25); }
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, field + 1);
    for (unsigned n = 0; n < 32; ++n)
        c.s.wr<1>(1280 + n, active.sticky == 2 ? n == 14 : active.sticky ? (n % 3 == 0) : 0);
    c.s.wr<1>(reg::xer_so, 1); c.s.wr<1>(reg::xer_ca, 1);
    c.set(1, stack); c.set(2, 0x108bba90); c.set_lr(sentinel); c.set(3, active.bound);
    c.put32(c.r(2) - 0x7b90, active.bound ? bias_at : 0xdead0000);
    c.put32(c.r(2) - 0x7b84, active.bound ? cursor_at : 0xdead1000);
    c.put32(c.r(2) - 0x7b88, active.bound ? end_at : 0xdead2000);
    c.put32(c.r(2) - 0x7b8c, words); c.put32(c.r(2) - 0x510c, constants);
    c.put32(bias_at, active.bias); c.put32(cursor_at, words + active.cursor);
    c.put32(end_at, words + active.end);
    c.put16(words, active.first); c.put16(words + 2, active.first);
    c.put16(words + 4, active.second); c.put16(words + 6, active.second);
    c.put_f64(constants + 8, 65535.0); c.put_f64(constants + 0x18, 4503601774854144.0);
    c.put32(c.r(2) - 0x34c4, 0x12345678);
    // Deliberately supported guest aliases, not normal career-state evidence.
    if (active.alias == 1) c.put32(cursor_at, stack - 0x80 + 0x4a);
    if (active.alias == 2) {
        c.put32(c.r(2) - 0x7b90, stack - 0x80 + 0x44);
        c.put32(stack - 0x80 + 0x44, active.bias);
    }
    if (active.alias == 3) {
        c.put32(c.r(2) - 0x7b84, stack - 0x80 + 0x48);
        c.put32(stack - 0x80 + 0x48, words);
    }
    if (active.alias == 1) c.put32(end_at, 0xffffffff);
    if (active.alias == 4) {
        c.put32(c.r(2)-0x510c,stack-0x80+0x2c);
        c.put_f64(stack-0x80+0x34,65535.0);
        c.put_f64(stack-0x80+0x44,4503601774854144.0);
    }
    if (active.alias == 5) {
        c.s.memory.map(0,0x1000); c.put16(0,active.first);
        c.put32(cursor_at,0xfffffffe); c.put32(end_at,0);
    }
    c.s.budget = 1000000;
}
void returned(State& s) {
    Cpu c(s); using cm::semantic_io_checks::require;
    require(c.sp() == stack && c.lr() == sentinel && s.pc == sentinel, "sampler did not return");
    // Compare all volatile FP values, FPSCR and the final arithmetic carry.
    for (unsigned n = 0; n < 4; ++n) c.put64(fp_log + n * 8, c.s.r<8>(reg::fpr(n)));
    for (unsigned n = 0; n < 32; ++n) c.put8(fp_log + 32 + n, c.s.r<1>(1280 + n));
    c.put8(fp_log + 64, c.s.r<1>(reg::xer_ca));
    if (active.alias) return;
    const auto n = std::int32_t(active.bound);
    const auto draws = n == 0 ? 0u : (n >= -65535 && n <= 65535) || std::int32_t(active.bound * 10u) / 65535 == 0 ? 1u : 2u;
    auto position = words + active.cursor, bias = active.bias, lcg = 0x12345678u;
    for (unsigned step = 0; step < draws; ++step) {
        position += 2;
        if (position > words + active.end) { position = words; lcg = lcg * 0x41c64e6d + 0x3039; bias = (lcg >> 16) & 0x7fff; }
    }
    require(c.u32(cursor_at) == position && c.u32(bias_at) == bias && c.toc(-0x34c4) == lcg, "sampler cursor/bias/LCG postcondition");
    if (n == 0) require(c.r(3) == 0, "zero sampler result");
    else if (n >= -65535 && n <= 65535) {
        const auto sum = std::int32_t(std::uint32_t(c.u16(position)) + bias);
        const auto rem = std::int64_t(sum) % std::int64_t(n);
        require(c.r(3) == std::uint32_t(n > 0 ? rem : -rem), "small signed remainder postcondition");
    }
    if (active.bound == 131070 && active.bias == 0 && active.first == 65534 && active.second == 19)
        require(c.r(3) == 131069, "wide upper clamp postcondition");
}
void original(State& s, std::uint32_t entry) { reference_mode = true; s.pc = entry; run_until(s, sentinel); returned(s); }
void rewrite(State& s, std::uint32_t entry) {
    reference_mode = false; news_run_rewrite(s, entry); returned(s);
    Cpu c(s);
    if (mutation == 1) c.put32(cursor_at, c.u32(cursor_at) ^ 2);
    if (mutation == 2) c.put32(c.r(2) - 0x34c4, c.toc(-0x34c4) ^ 1);
}
}
namespace cm::aot {
unsigned check_news_sampling(const State& base) {
    struct Restore { bool (*previous)(State&); ~Restore() { dispatch_probe = previous; } } restore{dispatch_probe};
    dispatch_probe = probe;
    std::vector<Scenario> cases{{0}};
    for (auto bound : {1u,2u,17u,65535u,0xffffffffu,0xfffffffeu,0xffffffefu,0xffff0001u})
        for (auto bias : {0u,1u,32767u,0x80000000u,0xffffffffu})
            for (auto word : {0u,1u,16u,65534u,65535u}) cases.push_back({bound,bias,word,word});
    for (auto bound : {65536u,131070u,214748364u,214748365u,429496729u,429496730u,0x7fffffffu,0x80000000u,0xffff0000u,0xfffe0002u})
        for (auto first : {0u,1u,65534u,65535u}) for (auto second : {0u,1u,19u,65535u})
            for (unsigned sticky : {0u,1u,2u}) cases.push_back({bound,0,first,second,0,6,sticky});
    for (auto bound : {65536u,0xffff0000u,0x7fffffffu,0x80000000u})
        for (auto bias : {0x80000000u,0xffffffffu,0x7fffffffu})
            for (auto word : {0u,1u,65535u}) cases.push_back({bound,bias,word,19});
    for (auto bound : {17u,65536u,0xffff0000u,0x7fffffffu})
        for (unsigned cursor : {0u,2u,4u,6u}) for (unsigned end : {0u,2u,4u,6u})
            cases.push_back({bound,7,31,47,cursor,end});
    for (unsigned alias : {1u,2u,3u,4u,5u}) {
        Scenario test{alias == 3 ? 0x7fffffffu : 131070u,3,11,19}; test.alias = alias; cases.push_back(test);
    }
    cases.push_back({std::uint32_t(-655350),0,1,13});
    unsigned failures = 0;
    for (const auto& test : cases) {
        active = test;
        auto state = cm::semantic_io_checks::clone(base); Cpu c(*state); initialize(c);
        const auto difference = compare_routine_call(*state, 0x102c9880, original, rewrite, RoutineChecker::result_r3 | RoutineChecker::result_f1);
        if (!difference.empty()) {
            ++failures; std::cout << "FAIL sampler bound=" << std::int32_t(test.bound) << " bias=" << test.bias << " word=" << test.first << ',' << test.second << " cursor=" << test.cursor << " end=" << test.end << " alias=" << test.alias << '\n';
            for (const auto& line : difference) std::cout << line << '\n';
        }
    }
    active = Scenario{65536,7,31,47};
    for (mutation = 0; mutation <= 2; ++mutation) {
        auto state = cm::semantic_io_checks::clone(base); Cpu c(*state); initialize(c);
        if (compare_routine_call(*state, 0x102c9880, original, rewrite, RoutineChecker::result_r3, mutation == 0).empty()) {
            ++failures; std::cout << "FAIL sampler negative control " << mutation << '\n';
        } else std::cout << "ok sampler corruption detected " << mutation << '\n';
    }
    mutation = 0;
    std::cout << cases.size() << " sampler comparisons (5 artificial aliases/wrap cases), " << failures << " failed\n";
    return failures;
}
}
