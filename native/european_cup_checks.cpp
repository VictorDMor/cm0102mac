// Returned comparisons against private hook-free originals. Scripted external
// boundaries expose call order, arguments, CR, preserved registers and return
// sites in compared memory; the shared sort, date services and calendar
// builders can run for real, and an optional career snapshot runs real guest
// records with the boundaries listed at real_boundaries scripted.
#include "european_cup_adapter.hpp"
#include "guest_values.hpp"
#include "match_replay.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#if defined(CM_EUROPEAN_CUP_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using guest::Cpu;
namespace cm::aot { bool european_cup_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, object = data + 0x1000, definition = data + 0x3000, other_cup = data + 0x3800,
    history = data + 0x7000, text_buffer = data + 0x7400, news_list = data + 0x7800, match_record = data + 0x7c00,
    fixture_record = data + 0x7d00, stage_array = data + 0xe000, stage_objects = data + 0xf000, clubs = data + 0x10000,
    nations = data + 0x50000, entrants = data + 0x58000, qualifiers = data + 0x59000, other_entrants = data + 0x5a000,
    other_qualifiers = data + 0x5b000, rounds = data + 0x5c000, group_tables = data + 0x5e000, standings = data + 0x5f000, samples = data + 0x62000,
    heap = data + 0x80000, log_at = data + 0x100000, stack = 0x71010000, sentinel = 0xfffffff0;
constexpr std::uint32_t child_table = data + 0x1100, child_destructor = 0xf0000010, child_champion = 0xf0000020,
    child_destructor_tv = data + 0x1200, child_champion_tv = data + 0x1210, other_seed = 0xf0000030,
    other_table = data + 0x1300, other_seed_tv = data + 0x1400;
constexpr std::uint32_t construct = 0x104e9fb0, destroy = 0x104ea300, award = 0x104ea450, calendar = 0x104ea620, announce = 0x104eb290,
    collect_qualifiers = 0x104eb770, champion = 0x104eb900, result = 0x104eb940, next_season = 0x104ebc00, configure = 0x104ebf30,
    collect_entrants = 0x104ebf80, advance = 0x104ec210, first_draw = 0x104ec300, random_below = 0x104ed110, second_draw = 0x104ed690,
    knockout = 0x104ee2e0, entered = 0x104ee7f0, seeds = 0x104eead0, times = 0x104eee20, compare_seeds = 0x104ef100, stage_name = 0x104ef130,
    compare_draw = 0x104ef310, zero = 0x104ef370, base_register = 0x104831a0, base_after = 0x10479100;
constexpr std::array entries{construct, destroy, award, calendar, announce, collect_qualifiers, champion, result, next_season, configure,
                             collect_entrants, advance, first_draw, random_below, second_draw, knockout, entered, seeds, times, compare_seeds,
                             stage_name, compare_draw, zero};
struct Scenario {
    std::uint32_t entry;
    std::string name;
    int stage = -1, event = 1, year = 2001, last_stage = -1, stage_count = 13, fail_allocation = 0, fail_virtual = -1, deleting = 1;
    int nations = 8, entrant_count = 0x47, promoted = 16, scan = 0x47, mutate_stages = 0, rounds = 3;
    std::uint32_t club = 3, seed = 1, personal = 0, importance = 2, storage = 63, fail_call = 0, position_a = 0, position_b = 1;
    std::uint32_t bound = 8, weekday = 1, first_time = 1, second_time = 1, group = 0x449, size = 0x40;
    int home = 2, away = 1, second_leg = -1, match_stage = 3, selector = 1, tier_shift = 0, first_id = 0, second_id = 0, first_kind = 0;
    bool null_object = false, enabled_out = true, null_reputation = false, null_other = false, near_end = false, clobber = false, live = false;
    bool same_nation_all = false;
};
Scenario active{entries[0], "none"};
unsigned allocations = 0, calls = 0;
std::uint32_t next_allocation = heap;
bool reference_mode = false, production_mode = false, real_state_mode = false, corrupt_argument = false, ownership_mode = false;
std::uint32_t real_stack = 0;
const std::set<std::uint32_t> real_boundaries{
    0x10769b60, 0x1074b830, 0x1074bab0, 0x10384500, 0x103846e0, 0x10384820, 0x10383410, 0x10383b70, 0x104564a0, 0x105bd480, 0x1056dc60,
    0x104bc3c0, 0x104a0e40, 0x1047ef50, 0x10452010, 0x104533a0, 0x104698c0, 0x10450030, 0x1047bfb0, 0x104b79a0, 0x10103510, 0x100f71f0,
    0x104b3b60, 0x102d46c0};
const std::set<std::uint32_t> dependencies{
    0x10478f20, 0x10478f70, 0x104b3b60, 0x104533a0, 0x104536c0, 0x104564a0, 0x10773520, 0x107735a0, 0x10775130, 0x10775170, 0x10775200,
    0x10739890, 0x1077b720, 0x10769b60, 0x104b79a0, 0x10103510, 0x100f71f0, 0x10452010, 0x1047bfb0, 0x104854f0, 0x10485570,
    0x10485600, 0x104a9810, 0x104a9890, 0x107156f0, 0x10717b20, 0x104648c0, 0x10450030, 0x10727890, 0x1074b830, 0x1074bab0, 0x104698c0,
    0x10476780, 0x10384500, 0x103846e0, 0x10384820, 0x10383410, 0x10383b70, 0x1076f7d0, 0x1077b770, 0x1056dc60, 0x104a0e40,
    0x105bd480, 0x104bc3c0, 0x1047ef50, 0x10451d10, 0x10477b40, 0x10457490, 0x10459b50, 0x10464010, 0x10476f40};

void trace(Cpu& c, std::uint32_t target) {
    if (++calls > 12000) throw std::runtime_error("fixture call limit");
    auto index = c.u32(log_at);
    const auto add = [&](std::uint32_t word) { c.put32(log_at + 4 + index++ * 4, word); };
    add(target); add(c.lr());
    for (unsigned r = 3; r < 11; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned f = 0; f < 8; ++f) add(c.cr(f));
    for (unsigned offset = 0x38; offset <= 0x70; offset += 4) add(c.u32(c.sp() + offset));
    c.put32(log_at, index);
}
void clobber(Cpu& c, std::uint32_t target) {
    const auto mark = 0xc0000000u ^ (target << 4) ^ calls;
    for (unsigned r : {0u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u}) c.set(r, mark + r);
    c.set_cr(0, std::uint8_t(calls & 0xe)); c.set_cr(1, std::uint8_t((calls >> 1) & 0xe));
}
bool owned_body(State& s, std::uint32_t target) {
    if (reference_mode) return european_cup_reference(s);
    if (production_mode) return false;
    european_cup_run_rewrite(s, target);
    return true;
}
bool probe(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    if (ownership_mode && target == 0x10768300) { s.pc = c.lr(); return true; }
    if (run_leaf(s)) return true;
    if (european_cup_lists(target)) return owned_body(s, target);
    if (reference_mode && target >= entries.front() && target < 0x104ef380) return european_cup_reference(s);
    if (real_state_mode) {
        if (!real_boundaries.contains(target)) return false;
        trace(c, target);
        if (target == 0x1056dc60 || target == 0x104bc3c0) { c.put32(c.r(3), 0x44444444); }
        s.pc = c.lr();
        return true;
    }
    if (target == 0x10781320) {
        const auto descriptor = c.r(12), entry = c.u32(descriptor);
        c.put32(c.sp() + 0x14, c.r(2));
        const int method = entry == configure ? 0x94 : entry == calendar ? 0x44 : entry == seeds ? 0x64 : entry == base_register ? 0x20
            : entry == base_after ? 0x9c : entry == child_destructor ? 8 : entry == child_champion ? 0x38 : entry == other_seed ? 0x165 : -1;
        if (entry == compare_seeds || entry == compare_draw) {
            c.set(2, c.u32(descriptor + 4));
            s.pc = entry;
            return true;
        }
        if (method < 0) throw std::runtime_error("unexpected virtual target " + hex_address(entry));
        if (european_cup_lists(entry) && active.fail_virtual != method && active.fail_virtual != method + 0x1000) {
            c.set(2, c.u32(descriptor + 4));
            s.pc = entry;
            return true;
        }
        trace(c, entry);
        if (entry == child_destructor && active.mutate_stages == 1) c.put32(object + 0x30, std::uint32_t(-1));
        if (entry == child_destructor && active.mutate_stages == 2) c.put32(object + 0x30, 2);
        if (entry == child_destructor && active.mutate_stages == 3) c.put32(object + 0xc, data + 0xe100);
        c.set(3, method == 0x20 ? (active.fail_virtual == 0x20 ? 1 : 0) : active.fail_virtual == method ? 0xffffffff : 0x1234);
        if (active.fail_virtual == method + 0x1000) c.set(3, 0);
        if (active.clobber) clobber(c, entry);
        s.pc = c.lr();
        return true;
    }
    if (active.live && (target == 0x104a9810 || target == 0x104a9890 || target == 0x104854f0 || target == 0x10485570 || target == 0x10485600 ||
                        target == 0x107156f0 || target == 0x10717b20))
        return false;
    if (!dependencies.contains(target)) return false;
    if (corrupt_argument && !reference_mode) c.set(5, c.r(5) ^ 1);
    trace(c, target);
    const bool keeps_r3 = target == 0x10775170 || target == 0x104a9890 || target == 0x104854f0 || target == 0x10485570 || target == 0x10485600;
    if (target == 0x10775130 || target == 0x10773520 || target == 0x10775200) {
        ++allocations;
        if (active.fail_allocation == int(allocations)) c.set(3, 0);
        else {
            const auto size = target == 0x10775200 ? c.r(3) * c.r(4) : c.r(3), allocated = next_allocation;
            next_allocation += (size + 31) & ~15u;
            c.set(3, allocated);
        }
    } else if (target == 0x10459b50) c.set(3, fixture_record);
    else if (target == 0x10476780) c.set(3, 37);
    else if (target == 0x104648c0) c.set(3, std::uint32_t(active.first_id));
    else if (target == 0x10464010) c.set(3, std::uint32_t(active.group));
    else if (target == 0x1077b770) c.set(3, 0x12340 + calls);
    else if (target == 0x10739890) { c.put32(c.r(6), 0x45555200); c.put32(c.r(7), 0x43505000); }
    else if (target == 0x1077b720) c.put32(c.r(3), c.r(8));
    else if (!keeps_r3) c.set(3, active.fail_call == target ? 0 : 1);
    if (active.clobber) {
        clobber(c, target);
        if (keeps_r3) c.set(3, 0xc3c3c3c3);
    }
    s.pc = c.lr();
    return true;
}
std::uint32_t mix(std::uint32_t x) { x ^= x << 13; x ^= x >> 17; x ^= x << 5; return x; }
void initialize(Cpu& c, const Scenario& test) {
    // Paths past a failed allocation read and write through null; the page
    // lets them continue, as on the original machine.
    c.s.memory.map(0, 0x10000);
    c.s.memory.map(data, 0x200000); c.s.memory.map(0x71000000, 0x20000);
    c.s.memory.copy_in(0x71000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    c.s.memory.copy_in(object, std::vector<std::uint8_t>(0xb4, 0xa6));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    c.set(1, stack); c.set(2, 0x108bba90); c.set_lr(sentinel);
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, std::uint8_t(f + 1));
    const auto global = [&](int slot, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(slot), value); };
    const auto variable = [&](int slot, unsigned offset, std::uint32_t value) { global(slot, data + offset); c.put32(data + offset, value); };
    const unsigned club_total = 120;
    variable(-0x7b50, 0x100, club_total); variable(-0x7be8, 0x104, clubs); variable(-0x7a30, 0x108, 42);
    variable(-0x7a4c, 0x10c, 5); variable(-0x7bbc, 0x110, data + 0x2000); variable(-0x7b48, 0x114, history);
    variable(-0x7a48, 0x118, test.null_reputation ? 0 : data + 0x6000); variable(-0x7ab8, 0x11c, data + 0x6100);
    variable(-0x7aa4, 0x120, data + 0x6200); variable(-0x7b5c, 0x124, data + 0x6300);
    global(-0x7bb8, text_buffer); global(-0x7ba8, data + 0x8000); global(-0x7bac, data + 0x9000);
    c.put8(data + 0x9000, 0xa5);
    // The random stream, walked by cursor through a table seeded per scenario.
    const std::uint32_t sample_count = 0x400;
    std::uint32_t x = test.seed * 2654435761u + 1;
    for (std::uint32_t i = 0; i < sample_count; ++i) { x = mix(x); c.put16(samples + i * 2, x); }
    variable(-0x7b90, 0x130, 0x4321); global(-0x7b8c, samples);
    variable(-0x7b88, 0x134, samples + sample_count * 2);
    variable(-0x7b84, 0x138, test.near_end ? samples + sample_count * 2 - 4 : samples + (test.seed % 64) * 2);
    c.put32(data + 0x6200, data + 0x6280);
    c.put32(data + 0x6280 + 4, data + 0x62c0);
    c.put32(data + 0x6300 + 5 * 4, test.null_other ? 0 : other_cup);
    c.put32(other_cup, other_table); c.put32(other_table + 0x64, other_seed_tv);
    c.put32(other_seed_tv, other_seed); c.put32(other_seed_tv + 4, c.r(2));
    c.put16(other_cup + 0x3e, 0x40); c.put16(other_cup + 0x36, 2);
    c.put32(other_cup + 0x14, other_entrants); c.put32(other_cup + 0xa4, other_qualifiers);
    c.put8(definition + 0x67, 0x85); c.put8(definition + 0x6a, 0x86);
    c.put32(definition, 42);
    c.put8(data + 0x2000 + 5 * 0x6c + 0x67, 0x87);
    c.put32(object, c.toc(-0x6094)); c.put32(object + 4, definition);
    c.put32(object + 8, test.storage & 2 ? data + 0xd000 : 0);
    c.put32(object + 0xc, test.storage & 4 ? stage_array : 0);
    c.put32(object + 0x14, test.storage & 1 ? entrants : 0);
    c.put32(object + 0x2c, std::uint32_t(test.stage_count)); c.put32(object + 0x30, std::uint32_t(test.last_stage));
    c.put16(object + 0x36, std::uint32_t(test.entrant_count)); c.put16(object + 0x38, std::uint32_t(test.scan));
    c.put16(object + 0x3a, 7); c.put16(object + 0x3c, std::uint32_t(test.rounds)); c.put16(object + 0x3e, 0x37);
    c.put16(object + 0x40, std::uint32_t(test.year)); c.put8(object + 0x4a, 1);
    c.put32(object + 0xa0, test.storage & 8 ? rounds : 0);
    c.put32(object + 0xa4, test.storage & 16 ? qualifiers : 0);
    c.put32(object + 0xac, test.storage & 32 ? group_tables : 0);
    for (unsigned i = 0; i < 6; ++i) { c.put32(group_tables + i * 4, i == 1 ? 0 : heap + 0x70000 + i * 16); c.put16(rounds + i * 0x78 + 0x1c, 2 + i); c.put16(rounds + i * 0x78 + 0x80, 0x100 + i); }
    c.put32(child_destructor_tv, child_destructor); c.put32(child_destructor_tv + 4, c.r(2));
    c.put32(child_champion_tv, child_champion); c.put32(child_champion_tv + 4, c.r(2));
    c.put32(child_table + 8, child_destructor_tv); c.put32(child_table + 0x38, child_champion_tv);
    for (unsigned i = 0; i < 13; ++i) {
        const auto child = stage_objects + i * 0x100;
        c.put32(child, child_table);
        c.put32(stage_array + i * 4, i == 1 && test.mutate_stages == 9 ? 0 : child);
        c.put32(data + 0xe100 + i * 4, i == 2 ? child : 0);
    }
    c.put32(child_table + 0x8, child_destructor_tv);
    // Clubs: tier bands in database order, some of another competition.
    for (unsigned i = 0; i < club_total; ++i) {
        const auto club = clubs + i * 0x248;
        const unsigned nation = test.same_nation_all ? 0 : (mix(i * 977 + test.seed) % unsigned(std::max(1, test.nations)));
        c.put32(club + 0x14, i % 11 == 10 ? 41 : 42);
        c.put32(club + 0x2c, nations + nation * 0x100);
        c.put16(club + 0x1ec, 2000 - i);
        c.put8(club + 0x23b, 0x70 + i % 8);
        c.put8(club + 0x242, 0x80 + i % 3);
        c.put8(club + 0x243, std::uint32_t((i + std::uint32_t(test.tier_shift)) % 5));
    }
    for (unsigned n = 0; n < 16; ++n) c.put16(nations + n * 0x100 + 0xac, 30 - n % 7);
    // Entrants and qualifiers as collection leaves them.
    const unsigned promoted = unsigned(std::max(0, test.promoted));
    for (unsigned i = 0; i < 0x47; ++i) {
        c.put32(entrants + i * 8, clubs + (i * 7 + test.seed) % club_total * 0x248);
        c.put8(entrants + i * 8 + 4, i < 16 ? 7 : i < 34 ? 8 : i < 53 ? 13 : 0);
        c.put8(entrants + i * 8 + 5, 0);
    }
    for (unsigned i = 0; i < 0x37; ++i) {
        const auto club = clubs + (i * 13 + 16 + test.seed) % club_total * 0x248;
        c.put32(qualifiers + i * 8, club);
        const unsigned kept = i < promoted ? 1 : i < promoted + 16 ? 2 : 0;
        c.put8(qualifiers + i * 8 + 4, 0); c.put8(qualifiers + i * 8 + 5, kept);
    }
    // Each group's four standing rows, read through the real 0x1049ea00.
    for (unsigned g = 0; g < 13; ++g) for (unsigned place = 0; place < 4; ++place) {
        const auto club = clubs + ((g * 4 + place) * 3 + test.seed * 5) % club_total * 0x248;
        c.put32(stage_objects + g * 0x100 + 0xb0, standings + g * 0x200);
        c.put32(standings + g * 0x200 + place * 0x44, club);
    }
    c.put16(news_list + 0x30, 7); c.put16(news_list + 0x32, 0xfffe); c.put16(news_list + 0x34, 3);
    c.put8(match_record + 0x42, std::uint32_t(test.match_stage));
    c.put8(match_record + 0x43, std::uint32_t(test.home)); c.put8(match_record + 0x44, std::uint32_t(test.away));
    c.put8(match_record + 0x47, std::uint32_t(test.second_leg)); c.put8(match_record + 0x48, std::uint32_t(test.away));
    c.put32(match_record + 0x1c, clubs); c.put32(match_record + 0x20, clubs + 0x248);
    c.put16(match_record + 0x30, 9); c.put16(match_record + 0x32, 8); c.put16(match_record + 0x34, 1);
    const auto club_address = clubs + test.club * 0x248;
    c.set({{3, test.null_object ? 0 : object}, {4, std::uint32_t(test.stage)}, {5, data + 0x2800}, {6, data + 0x2804}, {7, test.enabled_out ? data + 0x2808 : 0}});
    c.put16(data + 0x2800, 0xabcd); c.put16(data + 0x2804, 0xabcd); c.put8(data + 0x2808, 0xab);
    if (test.entry == construct) c.set({{4, std::uint32_t(test.year)}, {5, definition}});
    if (test.entry == destroy) c.set(4, std::uint32_t(test.deleting));
    if (test.entry == award) c.set({{4, match_record}, {5, 0x77}});
    if (test.entry == result) {
        c.set({{4, club_address}, {5, std::uint32_t(test.event)}, {6, std::uint32_t(test.stage)}, {8, news_list}, {9, 0x99}});
        c.put16(news_list + 0x34, test.position_a);
    }
    if (test.entry == announce) {
        c.set({{4, test.club}, {5, std::uint32_t(test.event)}, {6, std::uint32_t(test.stage)}, {7, 0x7070}, {8, news_list}, {9, 0x9090}});
        c.put8(stack + 0x3b, test.importance); c.put8(stack + 0x3f, test.personal); c.put32(stack + 0x40, news_list);
    }
    if (test.entry == random_below) c.set(3, test.bound);
    if (test.entry == second_draw) c.set(4, std::uint32_t(test.stage));
    if (test.entry == entered) { c.set({{4, data + 0x2810}, {5, std::uint32_t(test.selector)}}); }
    if (test.entry == times) c.set({{4, 3}, {5, std::uint32_t(test.stage)}, {6, test.weekday}, {7, test.first_time}, {8, test.second_time}});
    if (test.entry == compare_seeds) {
        c.put32(data + 0x2820, clubs + test.position_a * 0x248); c.put32(data + 0x2824, clubs + test.position_b * 0x248);
        c.set({{3, data + 0x2820}, {4, data + 0x2824}});
    }
    if (test.entry == compare_draw) {
        c.put32(data + 0x2830, clubs + test.position_a * 0x248); c.put8(data + 0x2834, test.importance); c.put8(data + 0x2836, test.seed & 1);
        c.put32(data + 0x2838, clubs + test.position_b * 0x248); c.put8(data + 0x283c, test.personal); c.put8(data + 0x283e, (test.seed >> 1) & 1);
        c.set({{3, data + 0x2830}, {4, data + 0x2838}});
    }
    if (test.entry == stage_name) {
        c.set({{3, object}, {4, text_buffer}, {5, test.size}, {6, std::uint32_t(test.first_id)}, {7, std::uint32_t(test.second_id)},
               {8, std::uint32_t(test.first_kind)}, {9, club_address}});
    }
    c.s.budget = 20000000;
}
void postconditions(State& s) {
    Cpu c(s);
    if (c.sp() != (real_state_mode ? real_stack : stack) || c.lr() != sentinel || s.pc != sentinel) throw std::runtime_error("fixture did not return");
}
void original(State& s, std::uint32_t entry) { reference_mode = true; s.pc = entry; run_until(s, sentinel); postconditions(s); }
void rewrite(State& s, std::uint32_t entry) {
    reference_mode = false;
    if (production_mode) { s.pc = entry; run_until(s, sentinel); } else european_cup_run_rewrite(s, entry);
    postconditions(s);
#if defined(CM_EUROPEAN_CUP_COVERAGE)
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

std::vector<Scenario> all_cases(bool quick) {
    std::vector<Scenario> cases;
    const auto add = [&](const Scenario& s) { cases.push_back(s); };
    for (auto entry : entries) add({entry, "default"});
    // Construction.
    for (int allocation = 1; allocation <= 7; ++allocation) { Scenario t{construct, "allocation"}; t.fail_allocation = allocation; add(t); }
    { Scenario t{construct, "registered already"}; t.fail_virtual = 0x20; add(t); }
    { Scenario t{construct, "no reputation"}; t.null_reputation = true; add(t); }
    for (int method : {0x94, 0x9c}) { Scenario t{construct, "virtual failure"}; t.fail_virtual = method + 0x1000; add(t); }
    for (int year : {-32768, -1, 0, 32767, 65535}) { Scenario t{construct, "signed year"}; t.year = year; add(t); }
    // Destruction and the next season, every storage combination.
    for (auto entry : {destroy, next_season}) for (unsigned storage = 0; storage < 64; storage += 1) for (int last : {-2, -1, 0, 2}) {
        Scenario t{entry, "storage"}; t.storage = storage; t.last_stage = (storage & 4) ? last : -1; add(t);
    }
    for (int deleting : {-32768, -1, 0, 1, 32767, 65536, 65535}) { Scenario t{destroy, "deleting"}; t.deleting = deleting; add(t); }
    { Scenario t{destroy, "null object"}; t.null_object = true; add(t); }
    for (auto entry : {destroy, next_season}) for (int mutation = 1; mutation <= 3; ++mutation) {
        Scenario t{entry, "stage mutation"}; t.mutate_stages = mutation; t.last_stage = 3; add(t);
    }
    for (int allocation = 1; allocation <= 6; ++allocation) { Scenario t{next_season, "allocation"}; t.fail_allocation = allocation; t.storage = 63; add(t); }
    for (int method : {0x94, 0x9c}) { Scenario t{next_season, "negative virtual"}; t.fail_virtual = method; add(t); }
    { Scenario t{next_season, "no reputation"}; t.null_reputation = true; add(t); }
    for (int tiers : {0, 1, 2, 3, 4}) { Scenario t{next_season, "tiers"}; t.tier_shift = tiers; add(t); }
    // Prizes and points.
    for (int stage : {-129, -2, -1, 0, 5, 11, 12, 13, 127}) for (const auto [home, away] : {std::pair{2, 1}, {1, 2}, {1, 1}, {-1, 1}, {1, -1}})
        for (int second : {-1, 0, 3}) {
            Scenario t{award, "prize"}; t.match_stage = stage; t.home = home; t.away = away; t.second_leg = second; add(t);
        }
    // Calendars.
    for (int stage : {-129, -128, -2, -1, 0, 3, 4, 7, 8, 9, 10, 11, 12, 13, 127, 255, 256, 511}) for (bool output : {false, true}) {
        Scenario t{calendar, "stage"}; t.stage = stage; t.enabled_out = output; add(t);
        t.fail_allocation = 1; add(t);
        for (int year : {1999, 2000, 2001}) { t.fail_allocation = 0; t.year = year; add(t); }
    }
    if (!quick) for (int stage : {-1, 0, 4, 8, 10, 12}) for (int year : {1999, 2000, 2001, 2004, 2020}) {
        Scenario t{calendar, "real calendar"}; t.stage = stage; t.year = year; t.live = true; add(t);
    }
    // Qualifiers and entrants.
    for (int count : {-1, 0, 1, 30, 0x37, 0x47}) { Scenario t{collect_qualifiers, "entrants"}; t.entrant_count = count; add(t); }
    { Scenario t{collect_qualifiers, "allocation"}; t.fail_allocation = 1; add(t); }
    for (unsigned storage : {1u, 0u}) for (int shift : {0, 1, 2, 3, 4}) { Scenario t{collect_entrants, "bands"}; t.storage = storage; t.tier_shift = shift; add(t); }
    { Scenario t{collect_entrants, "allocation"}; t.fail_allocation = 1; add(t); }
    { Scenario t{configure, "default"}; t.fail_virtual = 0x44; add(t); }
    // Results.
    for (int stage : {-129, -2, -1, 0, 7, 8, 11, 12, 13, 255}) for (int event : {-2, -1, 0, 1, 2, 3, 6, 7}) for (std::uint32_t at : {0u, 1u}) {
        Scenario t{result, "outcome"}; t.stage = stage; t.event = event; t.position_a = at; add(t);
    }
    for (int stage : {-129, -2, -1, 0, 7, 8, 11, 12, 13, 255}) for (int event : {-2, -1, 0, 1, 2, 6, 7}) for (std::uint32_t personal : {0u, 1u, 0x80u})
        for (int id : {0, 0xfa, 0x104}) {
            Scenario t{announce, "news"}; t.stage = stage; t.event = event; t.personal = personal; t.importance = personal ^ 7; t.first_id = id; add(t);
        }
    // Stage advance.
    for (const auto [count, last] : {std::pair{13, -1}, {13, 0}, {13, 1}, {13, 7}, {13, 8}, {13, 10}, {13, 11}, {13, 12}, {0, -1}, {0, -2}, {int(0x80000000), 0x7fffffff}, {-1, 0}}) {
        Scenario t{advance, "stage limits"}; t.stage_count = count; t.last_stage = last; t.storage = 63; add(t);
    }
    // The random helper.
    for (std::uint32_t bound : {0u, 1u, 2u, 7u, 8u, 65534u, 65535u, 65536u, 65537u, 100000u, 327670u, 2000000u, 0x7fffffffu, 0x80000000u, 0x80000001u, 0xffff0000u, 0xffff0001u, 0xffff0002u, 0xffffffffu, 0xfffffff0u})
        for (bool end : {false, true}) for (std::uint32_t seed : {1u, 2u}) {
            Scenario t{random_below, "bound"}; t.bound = bound; t.near_end = end; t.seed = seed; add(t);
        }
    { std::uint32_t x = 0x9e3779b9; for (unsigned i = 0; i < 200; ++i) { x = mix(x); Scenario t{random_below, "swept bound"}; t.bound = x >> (i % 24); t.seed = i; t.near_end = i % 5 == 0; add(t); } }
    // Draws: every allocation failure, then many seeds and nation patterns.
    for (int allocation = 1; allocation <= 2; ++allocation) { Scenario t{first_draw, "allocation"}; t.fail_allocation = allocation; add(t); }
    for (int allocation = 3; allocation <= 8; ++allocation) { Scenario t{first_draw, "group allocation"}; t.fail_allocation = allocation; add(t); }
    for (int promoted : {0, 15, 16, 17, 20}) { Scenario t{first_draw, "promoted"}; t.promoted = promoted; add(t); }
    { Scenario t{first_draw, "no other cup"}; t.null_other = true; add(t); }
    for (int allocation = 1; allocation <= 12; ++allocation) { Scenario t{second_draw, "allocation"}; t.fail_allocation = allocation; t.stage = 0x14; add(t); }
    { Scenario t{second_draw, "no other cup"}; t.null_other = true; t.stage = 0x14; add(t); }
    for (int attempt : {0, 19, 20, 21, 127}) for (int nations : {1, 2, 4, 16}) { Scenario t{second_draw, "attempt"}; t.stage = attempt; t.nations = nations; add(t); }
    for (int allocation = 1; allocation <= 3; ++allocation) { Scenario t{knockout, "allocation"}; t.fail_allocation = allocation; add(t); }
    const unsigned seeds_tried = quick ? 8 : 64;
    for (std::uint32_t seed = 1; seed <= seeds_tried; ++seed) for (int nations : {1, 2, 3, 5, 8, 16}) {
        Scenario t{first_draw, "seed"}; t.seed = seed; t.nations = nations; t.near_end = seed % 7 == 0; add(t);
        t.entry = second_draw; t.name = "seed"; t.stage = 0x14; add(t); t.stage = 0; add(t);
        t.entry = knockout; add(t);
    }
    { Scenario t{first_draw, "one nation"}; t.same_nation_all = true; add(t); t.entry = second_draw; t.stage = 0x14; add(t); }
    // Lists, seeds, times, comparators and names.
    for (int selector : {-1, 0, 1, 2, 3, 4, 256 + 1}) for (int last : {0, 10, 11, 12}) for (int scan : {0, 1, 0x47}) {
        Scenario t{entered, "selector"}; t.selector = selector; t.last_stage = last; t.scan = scan; add(t);
        if (selector >= 1 && selector <= 3) { t.fail_allocation = 1; add(t); }
    }
    { Scenario t{seeds, "no fixtures"}; t.storage = 5; add(t); }
    { Scenario t{seeds, "allocation"}; t.fail_allocation = 1; add(t); }
    for (int method : {0x44}) { Scenario t{seeds, "virtual"}; t.fail_virtual = method; add(t); }
    for (int rounds : {-1, 0, 1, 3, 4}) for (int count : {0, 1, 0x47}) { Scenario t{seeds, "shape"}; t.rounds = rounds; t.entrant_count = count; add(t); }
    for (int stage : {-129, -1, 0, 7, 8, 11, 12, 13, 255}) for (std::uint32_t value : {0u, 1u, 2u, 3u, 4u, 5u, 0x10u, 0x11u, 0x12u, 0x20u, 0x21u, 0x22u, 0x2eu, 0x2fu, 0x30u, 0xffu, 0x103u, 0x121u, 0xffffff01u}) {
        Scenario t{times, "slot"}; t.stage = stage; t.weekday = value; add(t);
        t.weekday = 1; t.first_time = value; add(t);
        t.first_time = 1; t.second_time = value; add(t);
    }
    for (std::uint32_t a : {0u, 1u, 5u, 6u, 10u}) for (std::uint32_t b : {0u, 1u, 5u, 6u, 10u}) {
        Scenario t{compare_seeds, "pair"}; t.position_a = a; t.position_b = b; add(t);
    }
    for (std::uint32_t a : {0u, 1u, 3u, 8u}) for (std::uint32_t b : {0u, 1u, 3u, 8u}) for (std::uint32_t one : {1u, 2u, 0x80u}) for (std::uint32_t two : {1u, 2u, 0x80u}) for (std::uint32_t seed = 0; seed < 4; ++seed) {
        Scenario t{compare_draw, "pair"}; t.position_a = a; t.position_b = b; t.importance = one; t.personal = two; t.seed = seed + 8; add(t);
    }
    for (std::uint32_t size : {0x1du, 0x1eu, 0x40u, 0xffffffffu}) for (std::uint32_t group : {0x449u, 0x44au, 0u}) for (int kind : {-1, 0, 1, 2, 3, 5, 6, 7, 8, 9, 0x106, 0x107}) for (const auto [first, second] : {std::pair{0, 0}, {0, 1}, {1, 0}, {0x10000, 0x10000}}) {
        Scenario t{stage_name, "name"}; t.size = size; t.group = group; t.first_kind = kind; t.first_id = first; t.second_id = second; add(t);
    }
    return cases;
}

// The live object on a real career's database: everything but the listed
// boundaries runs, including the shared RNG and the real sort.
int real_career(const char* snapshot) {
    auto career = std::make_unique<State>();
    load_match_snapshot(*career, snapshot);
    Cpu saved(*career);
    saved.set(2, 0x108bba90);
    if (!saved.sp()) { career->memory.map(0x71000000, 0x20000); saved.set(1, 0x71010000); }
    real_stack = saved.sp();
    career->budget = career->executed + 400000000;
    saved.set_lr(sentinel);
    real_state_mode = true; production_mode = true;
    career->memory.map(data, 0x200000);
    const auto id = saved.u32(saved.toc(-0x7a30));
    const auto cup = saved.u32(saved.u32(saved.toc(-0x7b5c)) + id * 4);
    const auto year = std::uint32_t(saved.u16(saved.toc(-0x7b80) + 2));
    if (!cup || saved.u32(cup) != saved.toc(-0x6094)) throw std::runtime_error("the snapshot has no live Champions League object");
    std::cout << "real career: object " << hex_address(cup) << ", year " << year << ", stage " << saved.i32(cup + 0x30)
              << ", entrants " << saved.i16(cup + 0x36) << '\n';
    unsigned failures = 0, completed = 0;
    const auto compare_real = [&](const std::string& name, State& state, std::uint32_t entry, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers) {
        Cpu c(state);
        c.set_lr(sentinel); c.set(registers);
        state.budget = state.executed + 400000000;
        active = Scenario{entry, name};
        const auto differences = compare_routine_call(state, entry, original, rewrite, RoutineChecker::result_r3 | RoutineChecker::result_f1);
        std::cout << (differences.empty() ? "ok " : "DIFFERS ") << hex_address(entry) << " real " << name << '\n';
        for (const auto& difference : differences) std::cout << "  " << difference << '\n';
        failures += !differences.empty(); completed += differences.empty();
    };
    const auto advance_original = [&](State& state, std::uint32_t entry, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers) {
        Cpu c(state);
        c.set_lr(sentinel); c.set(registers);
        state.budget = state.executed + 400000000;
        active = Scenario{entry, "prepare"};
        original(state, entry);
    };
    // The live season: the second phase and knockout draws from its real
    // standings, list and result routines on its real records.
    const auto at_stage = [&](std::int32_t stage) {
        auto state = cm::semantic_io_checks::clone(*career);
        Cpu(*state).put32(cup + 0x30, std::uint32_t(stage));
        return state;
    };
    { auto state = at_stage(7); compare_real("advance to second phase", *state, advance, {{3, cup}}); }
    { auto state = at_stage(7); compare_real("second draw", *state, second_draw, {{3, cup}, {4, 0}}); }
    // A draw that stopped at an assertion would compare equal without having
    // dealt anything, so require each to reach its last stage on the original path.
    const auto reaches = [&](std::int32_t from, std::uint32_t entry, std::int32_t last) {
        auto state = at_stage(from);
        advance_original(*state, entry, {{3, cup}, {4, 0}});
        const auto stage = Cpu(*state).i32(cup + 0x30);
        if (stage != last) { ++failures; std::cout << "FAIL " << hex_address(entry) << " ended at stage " << stage << '\n'; }
        else std::cout << "ok " << hex_address(entry) << " reaches stage " << last << '\n';
    };
    reaches(-1, first_draw, 7);
    reaches(7, second_draw, 11);
    reaches(11, knockout, 12);
    { auto state = at_stage(11); compare_real("advance to knockout", *state, advance, {{3, cup}}); }
    { auto state = at_stage(11); compare_real("knockout draw", *state, knockout, {{3, cup}}); }
    for (std::uint32_t selector : {1u, 2u, 3u, 4u}) { auto state = at_stage(11); compare_real("live list " + std::to_string(selector), *state, entered, {{3, cup}, {4, real_stack - 0x2000}, {5, selector}}); }
    { auto state = cm::semantic_io_checks::clone(*career); compare_real("destruction", *state, destroy, {{3, cup}, {4, 0}}); }
    const auto live_club = saved.u32(saved.u32(cup + 0x14));
    const auto low_match = real_stack - 0x1000;
    saved.put16(low_match + 0x30, 1); saved.put16(low_match + 0x32, 2); saved.put16(low_match + 0x34, 0);
    for (int stage : {-1, 0, 8, 12}) for (int event : {-2, -1, 0, 1, 2, 6}) {
        // The knockout's round record doesn't exist yet at this stage of the snapshot's season.
        if (stage == 12 && event == 0) continue;
        auto state = cm::semantic_io_checks::clone(*career);
        compare_real("result stage " + std::to_string(stage) + " event " + std::to_string(event), *state, result,
                     {{3, cup}, {4, live_club}, {5, std::uint32_t(event)}, {6, std::uint32_t(stage)}, {8, low_match}, {9, 0}});
    }
    // News, prizes and names on the live records.
    const auto club_index = (live_club - saved.u32(saved.toc(-0x7be8))) / 0x248;
    saved.put8(low_match + 0x42, 3); saved.put8(low_match + 0x43, 2); saved.put8(low_match + 0x44, 1); saved.put8(low_match + 0x47, 0xff);
    saved.put32(low_match + 0x1c, live_club); saved.put32(low_match + 0x20, live_club + 0x248);
    { auto state = cm::semantic_io_checks::clone(*career); compare_real("prize", *state, award, {{3, cup}, {4, low_match}, {5, 0}}); }
    for (int stage : {-1, 0, 8, 12}) for (int event : {-2, 1, 2, 6}) for (std::uint32_t personal : {0u, 1u}) {
        auto state = cm::semantic_io_checks::clone(*career);
        Cpu n(*state);
        n.put8(real_stack + 0x3b, 2); n.put8(real_stack + 0x3f, personal); n.put32(real_stack + 0x40, data + 0x7800);
        compare_real("news stage " + std::to_string(stage) + " event " + std::to_string(event) + " personal " + std::to_string(personal), *state, announce,
                     {{3, cup}, {4, club_index}, {5, std::uint32_t(event)}, {6, std::uint32_t(stage)}, {7, 0}, {8, low_match}, {9, 0}});
    }
    for (std::uint32_t first : {0u, 0x449u, 0x44au, 5u}) for (std::uint32_t kind : {0u, 1u, 2u, 3u, 6u, 9u}) {
        auto state = cm::semantic_io_checks::clone(*career);
        compare_real("name " + std::to_string(first) + " kind " + std::to_string(kind), *state, stage_name,
                     {{3, cup}, {4, data + 0x7400}, {5, 0x40}, {6, first}, {7, 0}, {8, kind}, {9, live_club}});
    }
    { auto state = cm::semantic_io_checks::clone(*career); compare_real("next season with clubs eliminated", *state, next_season, {{3, cup}}); }
    { auto state = at_stage(-1); compare_real("first draw", *state, first_draw, {{3, cup}}); }
    { auto state = at_stage(-1); compare_real("first advance", *state, advance, {{3, cup}}); }
    // The start of a season: every entrant back in this competition's zone.
    auto fresh = cm::semantic_io_checks::clone(*career);
    { Cpu start(*fresh); for (unsigned i = 0; i < 0x47; ++i) start.put32(start.u32(start.u32(cup + 0x14) + i * 8) + 0x14, id); }
    { auto state = cm::semantic_io_checks::clone(*fresh); compare_real("next season", *state, next_season, {{3, cup}}); }
    advance_original(*fresh, next_season, {{3, cup}});
    const auto low = real_stack - 0x2000;
    for (int stage : {-1, 0, 4, 8, 10, 12}) {
        auto state = cm::semantic_io_checks::clone(*fresh);
        compare_real("calendar stage " + std::to_string(stage), *state, calendar, {{3, cup}, {4, std::uint32_t(stage)}, {5, low}, {6, low + 4}, {7, low + 8}});
    }
    for (const auto [name, entry] : {std::pair{"configuration", configure}, {"entrants", collect_entrants}, {"qualifiers", collect_qualifiers}, {"seeding", seeds}}) {
        auto state = cm::semantic_io_checks::clone(*fresh);
        compare_real(name, *state, entry, {{3, cup}});
    }
    for (std::uint32_t bound : {8u, 65536u, 300000u, 2000000000u}) { auto state = cm::semantic_io_checks::clone(*fresh); compare_real("random " + std::to_string(bound), *state, random_below, {{3, bound}}); }
    real_state_mode = production_mode = false;
    std::cout << completed << " real-career comparisons, " << failures << " failed\n";
    return failures || !completed ? 1 : 0;
}
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) { std::cerr << "Usage: cm-european-cup-checks <original-segments> [career.snapshot|--quick]\n"; return 2; }
    try {
        auto base = std::make_unique<State>();
        load_program(*base, argv[1]);
        dispatch_probe = probe;
        const bool quick = argc == 3 && std::string(argv[2]) == "--quick";
        if (argc == 3 && !quick) return real_career(argv[2]);
        const auto cases = all_cases(quick);
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
        std::cout << cases.size() << " cases\n";
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
                          << " stage=" << test.stage << " event=" << test.event << " seed=" << test.seed << " nations=" << test.nations
                          << " alloc=" << test.fail_allocation << " virtual=" << test.fail_virtual << " last=" << test.last_stage
                          << " storage=" << test.storage << " bound=" << test.bound << " selector=" << test.selector << " clobber=" << clobbered << '\n';
                for (std::size_t i = 0; i < differences.size() && i < 12; ++i) std::cout << "  " << differences[i] << '\n';
                if (++failures >= 10) { std::cout << "stopping after 10 failures\n"; return 1; }
            }
        }
        production_mode = false;
        Scenario control{result, "corruption control"};
        control.stage = 12; control.event = 1;
        for (const auto wrong : {wrong_memory, wrong_register, wrong_argument}) {
            auto state = prepare(control);
            if (compare_routine_call(*state, control.entry, original, wrong, RoutineChecker::result_r3).empty()) {
                ++failures; std::cout << "FAIL corruption control\n";
            } else std::cout << "ok corruption detected\n";
        }
        corrupt_argument = false;
        // No instruction budget: any generated execution of an owned body
        // fails, and the original itself must exhaust it (a true negative).
        unsigned owned = 0;
        for (auto entry : entries) {
            Scenario test{entry, "production ownership"};
            test.stage = entry == second_draw ? 0x14 : entry == calendar ? 12 : -1;
            // The draws read standing rows through a leaf of another module,
            // which counts instructions; this is far below any owned draw body.
            const std::uint64_t allowance = entry == first_draw || entry == second_draw || entry == knockout || entry == advance ? 256 : 0;
            ownership_mode = true;
            auto normal = prepare(test);
            normal->budget = normal->executed + allowance;
            ObservedWrites observer;
            normal->memory.observer = &observer;
            production_mode = true;
            try { rewrite(*normal, entry); ++owned; }
            catch (const std::exception& error) { std::cout << "ownership " << hex_address(entry) << ' ' << error.what() << '\n'; }
            production_mode = false;
            auto reference = prepare(test);
            reference->budget = reference->executed + allowance;
            bool exhausted = false;
            try { original(*reference, entry); }
            catch (const std::exception& error) { exhausted = std::string(error.what()).find("budget") != std::string::npos; }
            ownership_mode = false;
            if (!exhausted) { ++failures; std::cout << "FAIL original budget control " << hex_address(entry) << '\n'; }
        }
        if (owned != entries.size()) ++failures;
        for (const auto& [entry, n] : counts) std::cout << hex_address(entry) << " returned " << n[0] << " controlled + " << n[1] << " production\n";
        std::cout << owned << '/' << entries.size() << " production ownership controls; " << completed << " returned comparisons; " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
