// Returned comparisons against private hook-free originals. Scripted external
// boundaries expose call order, arguments, CR, preserved registers and return
// sites in compared memory; further cases run the real date services, and an
// optional career snapshot runs real guest records: the database, the shared
// RNG, the knockout record and the calendars. Its scripted boundaries are
// listed at real_boundaries.
#include "bra_reg_central_adapter.hpp"
#include "guest_values.hpp"
#include "match_replay.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#if defined(CM_BRA_REG_CENTRAL_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using guest::Cpu;
namespace cm::aot { bool bra_reg_central_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, object = data + 0x1000, definition = data + 0x3000, division = data + 0x3800,
    history = data + 0x7000, text_buffer = data + 0x7400, news_list = data + 0x7800, fixture_record = data + 0x7c00,
    clubs = data + 0x10000, standings = data + 0x20000, heap = data + 0x80000, log_at = data + 0x100000,
    stack = 0x71010000, sentinel = 0xfffffff0;
constexpr std::uint32_t child_destructor = 0xf0000010, child_champion = 0xf0000020,
    child_table = data + 0x1100, child_destructor_tv = data + 0x1200, child_champion_tv = data + 0x1210;
constexpr std::uint32_t construct = 0x106d73f0, destroy = 0x106d7730, calendar = 0x106d7830, collect = 0x106d8360,
    configure = 0x106d84b0, next_season = 0x106d8570, advance = 0x106d8800, knockout = 0x106d88c0, result = 0x106d8af0,
    announce = 0x106d8c50, champion = 0x106d8e30, seeds = 0x106d8e70, times = 0x106d90c0, forward = 0x106d9150,
    base_register = 0x104a6850, fixture_builder = 0x104a00a0;
constexpr std::array entries{construct, destroy, calendar, collect, configure, next_season, advance,
                             knockout, result, announce, champion, seeds, times, forward};
struct Scenario {
    std::uint32_t entry;
    std::string name;
    std::uint32_t club_total = 26, members = 15, storage = 7, fail_call = 0, club = 103;
    int year = 2001, stage = -1, event = 6, fail_allocation = 0, fail_virtual = -1, deleting = 1, last_stage = 0,
        stage_count = 1, club_count = 15, mutate_stages = 0, output_alias = 0, duplicate_row = -1;
    std::uint32_t weekday = 3, first_time = 0, second_time = 0, importance = 2, personal = 1;
    bool null_object = false, output_flag = true, null_division = false, live_calendar = false, clobber = false;
};
Scenario active{entries[0], "none"};
unsigned allocations = 0, calls = 0;
std::uint32_t next_allocation = heap;
bool reference_mode = false, production_mode = false, real_state_mode = false, corrupt_argument = false;
std::uint32_t real_stack = 0;
const std::set<std::uint32_t> real_boundaries{
    base_register, fixture_builder, 0x10769b60, 0x1074b830, 0x1074bab0,
    0x10384500, 0x10382e50, 0x10381b70, 0x103846e0, 0x102d46c0, 0x104564a0};
const std::set<std::uint32_t> dependencies{
    0x1049e4f0, 0x104a00a0, 0x1049eb30, 0x10773520, 0x104533a0, 0x104a6660, 0x10775170, 0x104536c0,
    0x1049e600, 0x107735a0, 0x10775130, 0x104a60e0, 0x104a9810, 0x104a9890, 0x10739890,
    0x1077b720, 0x10769b60, 0x104564a0, 0x102d46c0, 0x104854f0, 0x10485570, 0x10384500, 0x10382e50,
    0x10381b70, 0x103846e0, 0x10476780, 0x10450030, 0x10457490, 0x10477b40, 0x10459b50, 0x104aa430,
    0x10517310, 0x104bc3c0, 0x1047ef50, 0x10727890, 0x1074b830, 0x1074bab0};

void trace(Cpu& c, std::uint32_t target) {
    if (++calls > 4000) throw std::runtime_error("fixture call limit");
    auto index = c.u32(log_at);
    const auto add = [&](std::uint32_t word) { c.put32(log_at + 4 + index++ * 4, word); };
    add(target); add(c.lr());
    for (unsigned r = 3; r < 11; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned f = 0; f < 8; ++f) add(c.cr(f));
    for (unsigned offset = 0x38; offset <= 0x54; offset += 4) add(c.u32(c.sp() + offset));
    c.put32(log_at, index);
}
void clobber(Cpu& c, std::uint32_t target) {
    const auto mark = 0xc0000000u ^ (target << 4) ^ calls;
    for (unsigned r : {0u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u}) c.set(r, mark + r);
    c.set_cr(0, std::uint8_t(calls & 0xe)); c.set_cr(1, std::uint8_t((calls >> 1) & 0xe));
}
bool owned_body(State& s, std::uint32_t target) {
    if (reference_mode) return bra_reg_central_reference(s);
    if (production_mode) return false;
    bra_reg_central_run_rewrite(s, target);
    return true;
}
bool probe(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    if (run_leaf(s)) return true;
    if (bra_reg_central_lists(target)) return owned_body(s, target);
    if (reference_mode && target >= entries.front() && target < 0x106d9170) return bra_reg_central_reference(s);
    if (real_state_mode) {
        // These stop at file or dialog imports (no native bridge here), or need
        // a text object the fixture lacks.
        if (!real_boundaries.contains(target)) return false;
        trace(c, target);
        if (target == base_register) c.set(3, 0);       // nothing saved
        else if (target == fixture_builder) c.set(3, 1);
        s.pc = c.lr();
        return true;
    }
    if (target == 0x10781320) {
        const auto descriptor = c.r(12), entry = c.u32(descriptor);
        c.put32(c.sp() + 0x14, c.r(2));
        const unsigned method = entry == configure ? 0x94 : entry == calendar ? 0x44 : entry == seeds ? 0x64
            : entry == base_register ? 0x20 : entry == child_destructor ? 8 : entry == child_champion ? 0x38 : 0;
        if (!method) throw std::runtime_error("unexpected virtual target " + hex_address(entry));
        if (bra_reg_central_lists(entry) && active.fail_virtual != int(method)) {
            c.set(2, c.u32(descriptor + 4));
            s.pc = entry;
            return true;
        }
        trace(c, entry);
        if (entry == child_destructor && active.mutate_stages == 1) c.put32(object + 0x30, std::uint32_t(-1));
        if (entry == child_destructor && active.mutate_stages == 2) c.put32(object + 0x30, 2);
        if (entry == child_destructor && active.mutate_stages == 3) c.put32(object + 0xc, data + 0xe100);
        c.set(3, method == 0x20 ? (active.fail_virtual == 0x20 ? 1 : 0) : active.fail_virtual == int(method) ? 0 : 0x1234);
        if (active.clobber) clobber(c, entry);
        s.pc = c.lr();
        return true;
    }
    if (active.live_calendar && (target == 0x104a9810 || target == 0x104a9890 || target == 0x104854f0 || target == 0x10485570))
        return false;
    if (!dependencies.contains(target)) return false;
    if (corrupt_argument && !reference_mode) c.set(5, c.r(5) ^ 1);
    trace(c, target);
    if (target == 0x10775130 || target == 0x10773520) {
        ++allocations;
        if (active.fail_allocation == int(allocations)) c.set(3, 0);
        else {
            const auto size = c.r(3), allocated = next_allocation;
            next_allocation += (size + 31) & ~15u;
            c.set(3, allocated);
        }
    } else if (target == 0x104a60e0) c.put32(c.u32(c.r(3) + 0xb0) + c.r(4) * 0x44, c.r(5));
    else if (target == 0x10459b50) c.set(3, fixture_record);
    else if (target == 0x10476780) c.set(3, 37);
    else if (target == 0x10517310) c.set(3, 0x5151);
    else if (target == 0x10739890) { c.put32(c.r(6), 0x42524100); c.put32(c.r(7), 0x43505000); }
    else if (target == 0x1077b720) c.put32(c.r(3), c.r(8));
    else if (target != 0x10775170 && target != 0x104a9890 && target != 0x104854f0 && target != 0x10485570)
        c.set(3, active.fail_call == target ? 0 : 1);
    if (active.clobber) {
        clobber(c, target);
        if (target == 0x10775170 || target == 0x104a9890 || target == 0x104854f0 || target == 0x10485570) c.set(3, 0xc3c3c3c3);
    }
    s.pc = c.lr();
    return true;
}
void initialize(Cpu& c, const Scenario& test) {
    c.s.memory.map(data, 0x200000); c.s.memory.map(0x71000000, 0x20000);
    c.s.memory.copy_in(0x71000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    c.s.memory.copy_in(object, std::vector<std::uint8_t>(0xec, 0xa6));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    c.set(1, stack); c.set(2, 0x108bba90); c.set_lr(sentinel);
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, std::uint8_t(f + 1));
    const auto global = [&](int slot, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(slot), value); };
    const auto variable = [&](int slot, unsigned offset, std::uint32_t value) {
        global(slot, data + offset); c.put32(data + offset, value);
    };
    variable(-0x7b50, 0x100, test.club_total); variable(-0x7be8, 0x104, clubs);
    variable(-0x6b34, 0x108, 42); variable(-0x7b48, 0x10c, history);
    variable(-0x7428, 0x110, data + 0x7100);
    global(-0x7bb8, text_buffer); global(-0x7ba8, data + 0x8000); global(-0x7bac, data + 0x9000);
    c.put8(data + 0x9000, 0xa5);
    c.put32(division, 42); c.put32(division + 0x10, 41);
    c.put8(definition + 0x67, 0x85);
    c.put32(object, c.toc(-0x5ce8)); c.put32(object + 4, definition);
    c.put32(object + 8, test.storage & 2 ? data + 0xd000 : 0);
    c.put32(object + 0xc, test.storage & 4 ? data + 0xe000 : 0);
    c.put32(object + 0x2c, std::uint32_t(test.stage_count));
    c.put32(object + 0x30, std::uint32_t(test.last_stage));
    c.put16(object + 0x3e, std::uint32_t(test.club_count)); c.put16(object + 0x40, std::uint32_t(test.year));
    c.put32(object + 0xb0, standings);
    c.put32(object + 0xbc, test.storage & 1 ? data + 0xc000 : 0);
    c.put32(child_destructor_tv, child_destructor); c.put32(child_destructor_tv + 4, c.r(2));
    c.put32(child_champion_tv, child_champion); c.put32(child_champion_tv + 4, c.r(2));
    c.put32(child_table + 8, child_destructor_tv); c.put32(child_table + 0x38, child_champion_tv);
    for (unsigned i = 0; i < 8; ++i) {
        const auto child = data + 0xf000 + i * 0x100;
        c.put32(child, child_table);
        c.put32(data + 0xe000 + i * 4, i == 1 ? 0 : child);
        c.put32(data + 0xe100 + i * 4, i == 2 ? child : 0);
    }
    for (unsigned i = 0; i < 26; ++i) {
        const auto club = clubs + i * 0x248;
        c.put32(club, i);
        c.put32(club + 0x38, test.null_division && i == 0 ? 0 : i < test.members ? division : division + 0x10);
        c.put8(club + 0x23b, 0x70 + i);
        c.put32(standings + i * 0x44, 100 + i);
        c.put8(standings + i * 0x44 + 0x40, 0x11);
    }
    if (test.duplicate_row >= 0) c.put32(standings + std::uint32_t(test.duplicate_row) * 0x44, test.club);
    c.put16(news_list + 0x30, 7); c.put16(news_list + 0x32, 0xfffe); c.put16(news_list + 0x34, 0x8001);
    c.set({{3, test.null_object ? 0 : object}, {4, std::uint32_t(test.stage)}, {5, data + 0x2000}, {6, data + 0x2004},
           {7, test.output_flag ? data + 0x2008 : 0}});
    c.put16(data + 0x2000, 0xabcd); c.put16(data + 0x2004, 0xabcd); c.put8(data + 0x2008, 0xab);
    if (test.entry == construct) c.set({{4, std::uint32_t(test.year)}, {5, definition}});
    if (test.entry == destroy) c.set(4, std::uint32_t(test.deleting));
    if (test.entry == result) c.set({{4, test.club}, {5, std::uint32_t(test.event)}, {6, std::uint32_t(test.stage)}, {8, news_list}});
    if (test.entry == announce) {
        c.set({{4, test.club}, {5, std::uint32_t(test.event)}, {6, std::uint32_t(test.stage)}});
        c.put8(stack + 0x3b, test.importance); c.put8(stack + 0x3f, test.personal); c.put32(stack + 0x40, news_list);
    }
    if (test.entry == times) {
        c.set({{4, 3}, {5, std::uint32_t(test.stage)}, {6, test.weekday}, {7, test.first_time}, {8, test.second_time}});
    }
    if (test.output_alias == 1) c.set(7, c.r(5));
    if (test.output_alias == 2) c.set(6, c.r(5));
    if (test.output_alias == 3) c.set(5, object + 0x40);
    if (test.output_alias == 4) c.set(6, object + 0x40);
    c.s.budget = 5000000;
}
void postconditions(State& s) {
    Cpu c(s);
    if (c.sp() != (real_state_mode ? real_stack : stack) || c.lr() != sentinel || s.pc != sentinel) throw std::runtime_error("fixture did not return");
}
void original(State& s, std::uint32_t entry) { reference_mode = true; s.pc = entry; run_until(s, sentinel); postconditions(s); }
void rewrite(State& s, std::uint32_t entry) {
    reference_mode = false;
    if (production_mode) { s.pc = entry; run_until(s, sentinel); } else bra_reg_central_run_rewrite(s, entry);
    postconditions(s);
#if defined(CM_BRA_REG_CENTRAL_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void wrong_memory(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.put8(object + 0x40, c.u8(object + 0x40) ^ 1); }
void wrong_register(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.set(30, c.r(30) ^ 1); }
void wrong_argument(State& s, std::uint32_t entry) { corrupt_argument = true; rewrite(s, entry); }
struct ObservedWrites final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
};

std::vector<Scenario> all_cases() {
    std::vector<Scenario> cases;
    const auto add = [&](const Scenario& s) { cases.push_back(s); };
    for (auto entry : entries) add({entry, "default"});
    for (int allocation = 1; allocation <= 5; ++allocation) { Scenario t{construct, "allocation"}; t.fail_allocation = allocation; add(t); }
    { Scenario t{construct, "registered already"}; t.fail_virtual = 0x20; add(t); }
    for (int method : {0x94, 0x64, 0x44}) { Scenario t{construct, "virtual failure"}; t.fail_virtual = method; add(t); }
    for (std::uint32_t target : {0x104a00a0u, 0x1049eb30u}) { Scenario t{construct, "dependency failure"}; t.fail_call = target; add(t); }
    for (std::uint32_t members : {0u, 14u, 15u, 16u}) { Scenario t{construct, "members"}; t.members = members; add(t); }
    for (int year : {-32768, -1, 0, 32767, 65535}) for (auto entry : {construct, calendar, next_season, knockout}) {
        Scenario t{entry, "signed year"}; t.year = year; add(t);
        if (entry == calendar) { t.stage = 0; add(t); }
    }

    for (auto entry : {destroy, next_season}) for (unsigned storage = 0; storage < 8; ++storage) for (int last : {-2, -1, 0, 2, 4}) {
        Scenario t{entry, "storage"}; t.storage = storage; t.last_stage = last;
        if (!(storage & 4)) t.last_stage = -1;
        add(t);
    }
    for (int deleting : {-32768, -1, 0, 1, 32767, 65536, 65535}) { Scenario t{destroy, "deleting"}; t.deleting = deleting; add(t); }
    { Scenario t{destroy, "null object"}; t.null_object = true; add(t); }
    for (auto entry : {destroy, next_season}) for (int mutation = 1; mutation <= 3; ++mutation) {
        Scenario t{entry, "callback stage mutation"}; t.mutate_stages = mutation; add(t);
    }

    for (int stage : {-129, -128, -2, -1, 0, 1, 127, 255, 256, 511}) for (bool output : {false, true}) {
        Scenario t{calendar, "stage"}; t.stage = stage; t.output_flag = output; add(t);
        if (std::int8_t(stage) == 0 || std::int8_t(stage) == -1) { t.fail_allocation = 1; add(t); }
    }
    for (int alias = 1; alias <= 4; ++alias) for (int stage : {-1, 0}) { Scenario t{calendar, "output alias"}; t.output_alias = alias; t.stage = stage; add(t); }
    for (int year = 1998; year <= 2012; ++year) for (int stage : {-1, 0}) { Scenario t{calendar, "real calendar"}; t.year = year; t.stage = stage; t.live_calendar = true; add(t); }

    for (std::uint32_t members : {0u, 1u, 14u, 15u, 16u, 26u}) for (bool mixed : {false, true}) {
        Scenario t{collect, "club scan"}; t.members = members; t.null_division = mixed; add(t);
    }
    { Scenario t{collect, "no clubs"}; t.club_total = 0; add(t); }
    { Scenario t{collect, "allocation"}; t.fail_allocation = 1; add(t); }
    { Scenario t{configure, "calendar failure"}; t.fail_virtual = 0x44; add(t); }
    { Scenario t{configure, "calendar allocation"}; t.fail_allocation = 1; add(t); }

    for (int allocation = 1; allocation <= 3; ++allocation) { Scenario t{next_season, "allocation"}; t.fail_allocation = allocation; add(t); }
    for (int method : {0x94, 0x64}) { Scenario t{next_season, "virtual failure"}; t.fail_virtual = method; add(t); }
    for (std::uint32_t target : {0x104a00a0u, 0x1049eb30u}) { Scenario t{next_season, "dependency failure"}; t.fail_call = target; add(t); }
    for (std::uint32_t members : {14u, 16u}) { Scenario t{next_season, "members"}; t.members = members; add(t); }

    for (const auto [count, last] : {std::pair{1, -1}, {1, 0}, {1, 1}, {3, 0}, {3, -1}, {0, -1}, {0, -2},
             {int(0x80000000), 0x7fffffff}, {-1, 0}, {5, -3}}) {
        Scenario t{advance, "stage limits"}; t.stage_count = count; t.last_stage = last; add(t);
    }
    for (int allocation = 1; allocation <= 3; ++allocation) { Scenario t{knockout, "allocation"}; t.fail_allocation = allocation; add(t); }
    { Scenario t{knockout, "calendar failure"}; t.fail_virtual = 0x44; add(t); }
    { Scenario t{advance, "knockout allocation"}; t.last_stage = -1; t.fail_allocation = 1; add(t); }

    for (int stage : {-129, -1, 0, 1, 255, 256}) for (int event : {-1, 0, 1, 2, 3, 6, 7, 255, 257, 258, 262}) for (std::uint32_t club : {100u, 103u, 114u, 115u, 999u}) {
        Scenario t{result, "outcome"}; t.stage = stage; t.event = event; t.club = club; add(t);
    }
    for (int rows : {0, 1, -1}) for (int event : {1, 2, 7}) { Scenario t{result, "row count"}; t.stage = 0; t.event = event; t.club_count = rows; add(t); }
    for (int event : {1, 2, 7}) { Scenario t{result, "duplicate row"}; t.stage = 0; t.event = event; t.club = 103; t.duplicate_row = 9; add(t); }

    for (int stage : {-129, -1, 0, 1, 255, 256}) for (int event : {-1, 0, 1, 2, 3, 6, 7, 257, 262})
        for (std::uint32_t personal : {0u, 1u, 0x80u, 0xffu}) for (std::uint32_t club : {3u, 20u, 0x80000003u}) {
            Scenario t{announce, "news"}; t.stage = stage; t.event = event; t.personal = personal; t.importance = personal ^ 7; t.club = club; add(t);
        }

    { Scenario t{seeds, "no fixtures"}; t.storage = 0; add(t); }
    { Scenario t{seeds, "allocation"}; t.fail_allocation = 1; add(t); }
    for (int rows : {-1, 0, 1, 7, 8, 9, 10, 15, 16, 26}) { Scenario t{seeds, "rows"}; t.club_count = rows; add(t); }

    for (int stage : {-129, -1, 0, 1, 255, 256}) for (std::uint32_t first : {0u, 8u, 9u, 255u, 0x108u, 0x109u}) for (std::uint32_t second : {0u, 8u, 9u, 255u, 0x108u}) {
        Scenario t{times, "slots"}; t.stage = stage; t.first_time = first; t.second_time = second; t.weekday = first ^ 0x1f3; add(t);
    }
    return cases;
}

// The competition built on a real career's database: every dependency runs,
// including the shared RNG, the club history and the knockout record.
int real_career(const char* snapshot) {
    auto career = std::make_unique<State>();
    load_match_snapshot(*career, snapshot);
    Cpu saved(*career);
    // Session snapshots may begin outside a guest call: supply the original
    // TOC and a private call stack for the isolated invocations.
    saved.set(2, 0x108bba90);
    if (!saved.sp()) { career->memory.map(0x71000000, 0x20000); saved.set(1, 0x71010000); }
    real_stack = saved.sp();
    career->budget = career->executed + 200000000;
    saved.set_lr(sentinel);
    real_state_mode = true; production_mode = true;
    career->memory.map(data, 0x200000);
    saved.set(3, 0xec);
    saved.call(0x10773520, sentinel);
    const auto league = saved.r(3);
    if (!league) throw std::runtime_error("cannot allocate the real-career competition");
    const auto id = saved.u32(saved.toc(-0x6b34));
    const auto definition_address = saved.u32(saved.toc(-0x7bbc)) + id * 0x6c;
    const auto year = std::uint32_t(saved.u16(saved.toc(-0x7b80) + 2));
    unsigned members = 0;
    const auto total = saved.u32(saved.toc(-0x7b50));
    for (unsigned i = 0, club = saved.u32(saved.toc(-0x7be8)); i < total; ++i, club += 0x248) {
        const auto division_record = saved.u32(club + 0x38);
        if (division_record && saved.u32(division_record) == id) ++members;
    }
    std::cout << "real career: division " << id << ", year " << year << ", " << members << " of " << total << " clubs in the division\n";
    // Fewer clubs would stop construction at its assertion and exercise little.
    if (members != 15) throw std::runtime_error("the snapshot's division does not have 15 clubs");
    unsigned failures = 0, completed = 0;
    const auto compare_real = [&](const std::string& name, State& state, std::uint32_t entry,
                                  std::initializer_list<std::pair<unsigned, std::uint32_t>> registers) {
        Cpu c(state);
        c.set_lr(sentinel); c.set(registers);
        state.budget = state.executed + 200000000;
        active = Scenario{entry, name};
        const auto differences = compare_routine_call(state, entry, original, rewrite, RoutineChecker::result_r3 | RoutineChecker::result_f1);
        std::cout << (differences.empty() ? "ok " : "DIFFERS ") << hex_address(entry) << " real " << name << '\n';
        for (const auto& difference : differences) std::cout << "  " << difference << '\n';
        failures += !differences.empty(); completed += differences.empty();
    };
    // Original-path state for the later entries (the check's own children discard theirs).
    const auto advance_original = [&](State& state, std::uint32_t entry, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers) {
        Cpu c(state);
        c.set_lr(sentinel); c.set(registers);
        state.budget = state.executed + 200000000;
        active = Scenario{entry, "prepare"};
        original(state, entry);
    };
    { auto state = cm::semantic_io_checks::clone(*career); compare_real("construction", *state, construct, {{3, league}, {4, year}, {5, definition_address}}); }
    auto built = cm::semantic_io_checks::clone(*career);
    advance_original(*built, construct, {{3, league}, {4, year}, {5, definition_address}});
    Cpu c(*built);
    const auto low = real_stack - 0x2000;
    for (int stage : {-1, 0, 5}) {
        auto state = cm::semantic_io_checks::clone(*built);
        compare_real("calendar stage " + std::to_string(stage), *state, calendar,
                     {{3, league}, {4, std::uint32_t(stage)}, {5, low}, {6, low + 4}, {7, low + 8}});
    }
    { auto state = cm::semantic_io_checks::clone(*built); compare_real("configuration", *state, configure, {{3, league}}); }
    { auto state = cm::semantic_io_checks::clone(*built); compare_real("club collection", *state, collect, {{3, league}}); }
    { auto state = cm::semantic_io_checks::clone(*built); compare_real("next season", *state, next_season, {{3, league}}); }
    { auto state = cm::semantic_io_checks::clone(*built); compare_real("first advance", *state, advance, {{3, league}}); }
    { auto state = cm::semantic_io_checks::clone(*built); compare_real("knockout", *state, knockout, {{3, league}}); }
    { auto state = cm::semantic_io_checks::clone(*built); compare_real("seeding", *state, seeds, {{3, league}}); }
    { auto state = cm::semantic_io_checks::clone(*built); compare_real("base forward", *state, forward, {{3, league}, {4, 0}}); }
    const auto row_club = c.u32(c.u32(league + 0xb0));
    const auto club_index = c.u32(row_club);
    const auto match = low + 0x100;
    c.put16(match + 0x30, 1); c.put16(match + 0x32, 2); c.put16(match + 0x34, 3);
    for (int stage : {-1, 0}) for (int event : {-1, 1, 2, 3, 6, 7}) {
        auto state = cm::semantic_io_checks::clone(*built);
        compare_real("result stage " + std::to_string(stage) + " event " + std::to_string(event), *state, result,
                     {{3, league}, {4, row_club}, {5, std::uint32_t(event)}, {6, std::uint32_t(stage)}, {8, match}});
    }
    std::cout << "first row club index " << club_index << '\n';
    for (int stage : {-1, 0}) for (std::uint32_t personal : {0u, 1u}) for (int event : {1, 6}) {
        // The remaining combinations reach base-class news (fixtures only).
        if ((stage == -1 && event != 6) || (stage == 0 && (personal || event != 1))) continue;
        auto state = cm::semantic_io_checks::clone(*built);
        Cpu n(*state);
        n.put8(real_stack + 0x3b, 2); n.put8(real_stack + 0x3f, personal); n.put32(real_stack + 0x40, data + 0x7800);
        compare_real("news stage " + std::to_string(stage) + " event " + std::to_string(event) + " personal " + std::to_string(personal),
                     *state, announce, {{3, league}, {4, club_index}, {5, std::uint32_t(event)}, {6, std::uint32_t(stage)}});
    }
    auto staged = cm::semantic_io_checks::clone(*built);
    advance_original(*staged, advance, {{3, league}});
    { auto state = cm::semantic_io_checks::clone(*staged); compare_real("second advance", *state, advance, {{3, league}}); }
    { auto state = cm::semantic_io_checks::clone(*staged); compare_real("champion", *state, champion, {{3, league}}); }
    { auto state = cm::semantic_io_checks::clone(*staged); compare_real("season after knockout", *state, next_season, {{3, league}}); }
    { auto state = cm::semantic_io_checks::clone(*staged); compare_real("destruction with knockout", *state, destroy, {{3, league}, {4, 1}}); }
    real_state_mode = production_mode = false;
    std::cout << completed << " real-career comparisons, " << failures << " failed\n";
    return failures || completed != 30 ? 1 : 0;
}
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) { std::cerr << "Usage: cm-bra-reg-central-checks <original-segments> [career.snapshot]\n"; return 2; }
    try {
        auto base = std::make_unique<State>();
        load_program(*base, argv[1]);
        dispatch_probe = probe;
        if (argc == 3) return real_career(argv[2]);
        const auto cases = all_cases();
        unsigned failures = 0, completed = 0;
        std::map<std::uint32_t, std::array<unsigned, 2>> counts;
        const auto prepare = [&](const Scenario& test) {
            auto state = cm::semantic_io_checks::clone(*base);
            Cpu c(*state);
            active = test; allocations = calls = 0; next_allocation = heap;
            initialize(c, test);
            state->pc = test.entry;
            return state;
        };
        for (bool production : {false, true}) {
            production_mode = production;
            for (const auto& test : cases) for (bool clobbered : {false, true}) {
                auto variant = test; variant.clobber = clobbered;
                auto state = prepare(variant);
                ObservedWrites observer;
                if (production) state->memory.observer = &observer;
                const auto differences = compare_routine_call(*state, test.entry, original, rewrite, RoutineChecker::result_r3 | RoutineChecker::result_f1);
                if (differences.empty()) { ++completed; ++counts[test.entry][production]; continue; }
                std::cout << "DIFFERS " << (production ? "production " : "controlled ") << hex_address(test.entry) << ' ' << test.name
                          << " stage=" << test.stage << " event=" << test.event << " club=" << test.club
                          << " alloc=" << test.fail_allocation << " virtual=" << test.fail_virtual << " members=" << test.members
                          << " rows=" << test.club_count << " count=" << test.stage_count << " last=" << test.last_stage
                          << " storage=" << test.storage << " clobber=" << clobbered << '\n';
                for (const auto& difference : differences) std::cout << "  " << difference << '\n';
                if (++failures >= 8) { std::cout << "stopping after 8 failures\n"; return 1; }
            }
        }
        production_mode = false;
        // Stage 0, event 1 reaches a traced history call, so a wrong argument shows in the log.
        Scenario control{result, "corruption control"};
        control.stage = 0; control.event = 1;
        for (const auto wrong : {wrong_memory, wrong_register, wrong_argument}) {
            auto state = prepare(control);
            if (compare_routine_call(*state, control.entry, original, wrong, RoutineChecker::result_r3).empty()) {
                ++failures; std::cout << "FAIL corruption control\n";
            } else std::cout << "ok corruption detected\n";
        }
        // No instruction budget: any generated execution of an owned body
        // fails, and the original itself must exhaust it (a true negative).
        unsigned owned = 0;
        for (auto entry : entries) {
            Scenario test{entry, "production ownership"};
            test.stage = entry == result || entry == announce ? 0 : -1;
            test.event = 1; test.personal = 0;
            test.last_stage = entry == advance ? -1 : 0;
            auto normal = prepare(test);
            normal->budget = normal->executed;
            ObservedWrites observer;
            normal->memory.observer = &observer;
            production_mode = true;
            try { rewrite(*normal, entry); ++owned; }
            catch (const std::exception& error) { ++failures; std::cout << "FAIL ownership " << hex_address(entry) << ' ' << error.what() << '\n'; }
            production_mode = false;
            auto reference = prepare(test);
            reference->budget = reference->executed;
            bool exhausted = false;
            try { original(*reference, entry); }
            catch (const std::exception& error) { exhausted = std::string(error.what()).find("budget") != std::string::npos; }
            if (!exhausted) { ++failures; std::cout << "FAIL original budget control " << hex_address(entry) << '\n'; }
        }
        for (const auto& [entry, n] : counts) std::cout << hex_address(entry) << " returned " << n[0] << " controlled + " << n[1] << " production\n";
        std::cout << owned << '/' << entries.size() << " production ownership controls; " << completed << " returned comparisons; " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
