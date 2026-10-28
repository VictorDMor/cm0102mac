// Bounded returned comparisons against private hook-free original pages.
// Out-of-module services are scripted boundary fixtures; their internals are
// outside this module's scope. Calls, arguments, preserved registers, CR and
// stack arguments are recorded in guest memory and compared by the oracle.
#include "oceania_nations_adapter.hpp"
#include "guest_abi.hpp"
#include "semantic_io_checks.hpp"
#include "routine_check.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#if defined(CM_OCEANIA_NATIONS_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using guest::Cpu;
namespace cm::aot { bool oceania_nations_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, object = data + 0x1000,
    definitions = data + 0x2000, stages = data + 0x5000,
    entrants = data + 0x6000, clubs = data + 0x20000,
    nations = data + 0x40000, trace_at = data + 0x180000,
    stack = 0x71010000, sentinel = 0xfffffff0;
constexpr std::array entries{0x106e9940u,0x106e9ba0u,0x106e9cb0u,0x106ea780u,
    0x106eaaf0u,0x106eabc0u,0x106eac00u,0x106eada0u,0x106eaee0u,
    0x106eafa0u,0x106eb1d0u,0x106eb400u,0x106eb7a0u,0x106ebb10u,0x106ebcd0u};
struct Case {
    std::uint32_t entry;
    std::string name;
    int stage = -1, year = 2002, event = 0, phase = 0, mode = 1,
        a = 1, b = 2, d = 3, fail_allocation = 0, fail_method = -1,
        fail_service = 0, count = 24, current_stage = 0, deleting = 0,
        storage = 7, day_delta = 0, year_delta = 0, already_started = 0,
        rng_variant = 0, nation_variant = 0, start_day = 200, group = 0;
    bool null_object = false, output_flag = true, delegate = false, combined = false;
};
Case active{entries[0], "default"};
unsigned allocations = 0, calls = 0;
std::uint32_t next_allocation = 0;
bool original_mode = false;
const std::set<std::uint32_t> dependencies{
    0x100c76c0,0x100caa10,0x100caa90,0x10383410,0x10383b70,0x10384020,
    0x10384500,0x103846e0,0x10384820,0x104533a0,0x104536c0,0x104564a0,
    0x10457490,0x10459b50,0x104635b0,0x10464d00,0x104698c0,0x10476780,
    0x1047ef50,0x104854f0,0x10485570,0x10485600,0x1049e4f0,0x1049e600,
    0x1049ea00,0x1049eb30,0x104a00a0,0x104a0e40,0x104a1540,0x104a60e0,
    0x104a6660,0x104a9810,0x104a9890,0x104bc3c0,0x1056dc60,0x107156b0,
    0x107156f0,0x10727890,0x10739890,0x10739ef0,0x1074b830,0x1074bab0,
    0x10768300,0x10768810,0x10769b60,0x10773520,0x107735a0,0x10775130,
    0x10775170,0x1077b720,0x1077b770};
void trace(Cpu& c, std::uint32_t target) {
    if (++calls > 4000) throw std::runtime_error("fixture call limit");
    auto index = c.u32(trace_at);
    const auto add = [&](std::uint32_t v) { c.put32(trace_at + 4 + index++ * 4, v); };
    add(target); add(c.lr());
    for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned field = 0; field < 8; ++field) add(c.cr(field));
    // Only ABI arguments, not unused stack scratch, are call inputs.
    if (target == 0x104a9810 || target == 0x10485570) add(c.u32(c.sp() + 0x38));
    if (target == 0x1056dc60) for (unsigned off = 0x38; off <= 0x70; off += 4) add(c.u32(c.sp() + off));
    if (target == 0x104bc3c0) for (unsigned off = 0x38; off <= 0x54; off += 4) add(c.u32(c.sp() + off));
    c.put32(trace_at, index);
}
void fixture_return(Cpu& c) { c.s.pc = c.lr(); }
bool probe(State& s) {
    Cpu c(s); const auto target = s.pc;
    // Cpu::call dispatches lowered leaves first; the original must do likewise.
    if (run_leaf(s)) return true;
    if (target == 0x10781320) {
        const auto descriptor = c.r(12), entry = c.u32(descriptor);
        const auto method = c.u32(descriptor + 8);
        c.put32(c.sp() + 0x14, c.r(2));
        if (active.combined && active.fail_method != int(method) && oceania_nations_lists(entry)) {
            c.set(2, c.u32(descriptor + 4)); s.pc = entry; return true;
        }
        trace(c, entry);
        if (active.fail_method == int(method)) c.set(3, method == 0x20 ? 1 : 0);
        else if (method == 0x44) {
            c.put16(c.r(5), 3); c.put16(c.r(6), 1001);
            c.set(3, data + 0xc000);
        } else c.set(3, method == 0x20 ? 0 : 1);
        fixture_return(c); return true;
    }
    if (oceania_nations_lists(target)) {
        if (original_mode) return oceania_nations_reference(s);
        return oceania_nations_dispatch_probe(s);
    }
    if (original_mode && target >= entries[0] && target < 0x106ebdd0) return oceania_nations_reference(s);
    if (!dependencies.contains(target)) return false;
    trace(c, target);
    if (target == 0x10775130 || target == 0x10773520) {
        ++allocations;
        if (active.fail_allocation == int(allocations)) c.set(3, 0);
        else { const auto size = c.r(3), at = next_allocation; next_allocation += (size + 31) & ~15u; c.set(3, at); }
    } else if (target == 0x10739890) {
        c.put32(c.r(6), 0x4f434500); c.put32(c.r(7), 0x43505000);
    } else if (target == 0x1077b720) c.put32(c.r(3), c.r(8));
    else if (target == 0x10459b50) c.set(3, data + 0xf000);
    else if (target == 0x10476780) c.set(3, 37);
    else if (target == 0x1049ea00) {
        const auto index = (c.r(3) == object ? 0u : 2u) + (c.r(4) & 1);
        c.set(3, data + 0xd000 + index * 4);
    } else if (target == 0x100c76c0) {
        const auto id = (c.r(3) - nations) / 0x128;
        c.set(3, clubs + id * 0x248);
    } else if (target == 0x100caa10 || target == 0x100caa90) {
        const auto id = (c.r(3) - nations) / 0x128;
        c.set(3, target == 0x100caa10 ? (id % 2 == 0) : (active.nation_variant != 2 || id % 3 != 0));
    } else if (target == 0x107156f0) c.put16(c.r(3), active.start_day);
    else if (target == 0x107156b0) {
        // The date conversion's own behavior is outside this module. These
        // valid day/year pairs exercise the caller's before/equal/after logic.
        c.put16(c.r(3), active.start_day); c.put16(c.r(3) + 2, active.year);
        c.put32(c.r(3) + 4, 0x12345678);
    } else if (target == 0x104a00a0 || target == 0x1049eb30) c.set(3, active.fail_service == int(target) ? 0 : 1);
    else if (target == 0x1077b770) c.set(3, 0x1234 + active.rng_variant);
    else if (target == 0x10739ef0) {
        const auto bytes = c.s.memory.copy_out(c.r(4), c.r(5)); c.s.memory.copy_in(c.r(3), bytes);
    } else if (target == 0x10768300) {
        // A deterministic comparator service; input order and descriptor are
        // compared. Sorting internals are not owned by this module.
        std::array<std::uint32_t, 6> sorted{};
        for (unsigned i = 0; i < sorted.size(); ++i) sorted[i] = c.u32(c.r(3) + i * 4);
        std::sort(sorted.begin(), sorted.end());
        for (unsigned i = 0; i < sorted.size(); ++i) c.put32(c.r(3) + i * 4, sorted[i]);
    }
    fixture_return(c); return true;
}
void initialize(Cpu& c, const Case& test) {
    c.s.memory.map(data, 0x400000); c.s.memory.map(stack - 0x10000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x400000, 0xa5));
    c.s.memory.copy_in(stack - 0x10000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    c.set(1, stack); c.set(2, 0x108bba90); c.set_lr(sentinel);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i + 1);
    c.put32(trace_at, 0);
    const auto variable = [&](int slot, unsigned offset, std::uint32_t value) {
        c.put32(c.r(2) + std::uint32_t(slot), data + offset); c.put32(data + offset, value);
    };
    variable(-0x7b04, 0x100, test.count); variable(-0x7b98, 0x104, nations);
    variable(-0x7970, 0x108, 7); variable(-0x7650, 0x10c, 0); variable(-0x7654, 0x110, 1);
    variable(-0x7b84, 0x114, data + 0xe000 + (test.rng_variant & 4 ? 0x10 : 0));
    variable(-0x7b88, 0x118, data + 0xe010); variable(-0x7b90, 0x11c, test.rng_variant & 1);
    c.put32(c.r(2) - 0x7b8c, data + 0xe000);
    variable(-0x7be8, 0x120, clubs);
    c.put32(c.r(2) - 0x7bb8, data + 0x10000); variable(-0x7b48, 0x124, data + 0x11000);
    c.put32(c.r(2) - 0x7ba8, data + 0x12000); c.put32(c.r(2) - 0x7bac, data + 0x13000);
    for (unsigned i = 0; i < 16; ++i) c.put16(data + 0xe000 + i * 2, (test.rng_variant >> 1) & 1);
    const auto vtable = data + 0x3000, descriptors = data + 0x4000;
    c.put32(c.r(2) - 0x5cc4, vtable); c.put32(object, vtable);
    unsigned index = 0;
    for (const auto [method, entry] : std::array<std::pair<unsigned, unsigned>, 7>{{
        {0x8,0x10479000},{0x20,0x10479010},{0x38,0x10479020},{0x44,0x106e9cb0},
        {0x64,0x106ebb10},{0x94,0x106eaaf0},{0x9c,0x10479030}}}) {
        const auto descriptor = descriptors + index++ * 16;
        c.put32(vtable + method, descriptor); c.put32(descriptor, entry);
        c.put32(descriptor + 4, c.r(2)); c.put32(descriptor + 8, method);
    }
    c.put32(object + 4, definitions); c.put32(object + 8, test.storage & 1 ? data + 0xa000 : 0);
    c.put32(object + 0xc, test.storage & 2 ? stages : 0); c.put32(object + 0xbc, test.storage & 4 ? data + 0xb000 : 0);
    c.put32(object + 0x2c, 2); c.put32(object + 0x30, test.current_stage);
    c.put16(object + 0x40, test.year); c.put8(object + 0x4b, 15);
    c.put32(object + 0xb0, data + 0x15000);
    c.put16(object + 0xdc, 40); c.put16(object + 0xf0, 6); c.put32(object + 0xec, entrants);
    c.put16(object + 0xb6, test.start_day); c.put16(object + 0xb8, 0);
    c.put8(object + 0x45, test.already_started);
    c.put8(object + 0xc4, 3); c.put8(object + 0xc5, 1); c.put8(object + 0xc6, 10);
    c.put8(definitions + 0x67, 1);
    c.put16(data + 0x14000, test.start_day + test.day_delta);
    c.put16(data + 0x14002, test.year + test.year_delta); c.put32(data + 0x14004, 0x12345678);
    for (unsigned i = 0; i < 8; ++i) {
        const auto stage = data + 0x8000 + i * 0x100;
        c.put32(stages + i * 4, i % 3 == 2 ? 0 : stage); c.put32(stage, vtable);
        c.put32(stage + 0xa0, data + 0x15000); c.put32(stage + 0xb0, data + 0x15000);
        c.put32(data + 0x15000 + i * 0x44, clubs + i * 0x248);
        c.put32(data + 0xd000 + i * 4, clubs + i * 0x248);
        c.put32(entrants + i * 4, clubs + (7 - i) * 0x248);
    }
    for (int i = 0; i < std::max(test.count, 30); ++i) {
        const auto nation = nations + std::uint32_t(i) * 0x128;
        c.put32(nation + 0x74, definitions + 0x100 + std::uint32_t(i) * 0x10);
        c.put32(definitions + 0x108 + std::uint32_t(i) * 0x10, 7);
        if (test.nation_variant && i % 7 == 3) c.put32(nation + 0x74, 0);
        if (test.nation_variant && i % 7 == 4) c.put32(definitions + 0x108 + std::uint32_t(i) * 0x10, 8);
        const auto club = clubs + std::uint32_t(i) * 0x248;
        c.put32(club, i); c.put8(club + 0x23b, 1);
    }
    c.set({{3,test.null_object ? 0 : object},{4,std::uint32_t(test.stage)},
        {5,data + 0x16000},{6,data + 0x16004},{7,test.output_flag ? data + 0x16008 : 0},
        {8,data + 0x17000},{9,9},{10,10}});
    c.put16(data + 0x17030, 7); c.put16(data + 0x17032, 9); c.put16(data + 0x17034, 1);
    switch (test.entry) {
    case 0x106e9940: c.set({{4,std::uint32_t(test.year)},{5,definitions}}); break;
    case 0x106e9ba0: c.set(4, test.deleting); break;
    case 0x106ea780:
        c.set({{4,2},{5,std::uint32_t(test.event)},{6,std::uint32_t(test.phase)},
            {7,7},{8,8},{9,9}});
        c.put8(stack + 0x3b, 11); c.put8(stack + 0x3f, test.delegate); c.put32(stack + 0x40, data + 0x18000); break;
    case 0x106eac00:
        c.set({{4,2},{5,std::uint32_t(test.event)},{6,std::uint32_t(test.phase)},
            {7,7},{8,data + 0x17000},{9,9}}); break;
    case 0x106eafa0: c.set(4, test.group); break;
    case 0x106eb7a0: c.set({{4,data + 0x14000},{5,std::uint32_t(test.mode)}}); break;
    case 0x106ebcd0:
        c.set({{4,0},{5,std::uint32_t(test.phase)},{6,std::uint32_t(test.a)},
            {7,std::uint32_t(test.b)},{8,std::uint32_t(test.d)}}); break;
    default: break;
    }
    c.s.budget = 40000000;
}
void postconditions(Cpu& c) {
    const auto require = [](bool yes, const char* why) { if (!yes) throw std::runtime_error(why); };
    if (active.entry == 0x106e9cb0 && std::int8_t(active.stage) <= 1) {
        const auto stage = std::int8_t(active.stage);
        require(c.u16(data + 0x16000) == 3, "calendar round count");
        require(c.u16(data + 0x16004) == (stage < 1 ? std::uint16_t(1001 + stage) : 0), "calendar kind");
        if (active.output_flag) require(c.u8(data + 0x16008) == (stage < 1), "calendar optional flag");
        if (active.fail_allocation == 1) require(c.r(3) == 0, "calendar allocation failure result");
    }
    if (active.entry == 0x106eaaf0) {
        require(c.u16(object + 0x3c) == 1 && c.u8(object + 0xc4) == 3 &&
            c.u8(object + 0xc5) == 1 && c.u8(object + 0xc6) == 10 &&
            c.u8(object + 0x43) == 2 && c.u8(object + 0xc0) == 2 &&
            c.u8(object + 0x46) == 7 && c.u8(object + 0x47) == 3, "configuration constants");
        require(c.u16(object + 0xb6) == active.start_day && c.u16(object + 0xb8) == 0 && c.r(3) == 1,
            "configuration date/result");
    }
    if (active.entry == 0x106e9ba0 && !active.null_object) {
        require(c.u32(object + 0xbc) == 0 && c.u32(object + 0xc) == 0, "destructor releases owned storage");
    }
    if (active.entry == 0x106eada0) {
        require(c.i16(object + 0x40) == std::int16_t(active.year + 2) &&
            c.u8(object + 0x45) == 0 && c.i32(object + 0x30) == -1, "next season year/stage");
    }
    if (active.entry == 0x106eb400 && active.fail_allocation != 1) {
        require(c.u16(object + 0xf0) == 6 && c.u32(object + 0xec) != 0, "selected entrants storage");
        const auto cursor = c.u32(c.toc(-0x7b84));
        require(cursor >= data + 0xe000 && cursor <= data + 0xe010 && !(cursor & 1), "RNG cursor stays in its word table");
    }
    if (active.entry == 0x106eb7a0 && std::int8_t(active.mode) != 0 && active.day_delta == 0 &&
        active.year_delta == 0 && active.already_started == 0 && active.fail_allocation != 2 && !active.fail_service) {
        require(c.u32(object + 0xec) == 0 && c.u16(object + 0xf0) == 0 && c.u8(object + 0x45) == 1,
            "daily start releases selection and sets active flag");
    }
    if (active.entry == 0x106ebcd0) {
        auto first = std::uint32_t(active.a), second = std::uint32_t(active.b), third = std::uint32_t(active.d);
        if (std::int8_t(active.phase) < 1) {
            if (std::uint8_t(first) == 1) first = 1;
            else if (std::uint8_t(first) == 2) first = 3;
            else if (std::uint8_t(first) == 3) first = 5;
            if (std::uint8_t(second) == 1 || std::uint8_t(second) == 2) second = 1;
            else if (std::uint8_t(second) == 3) second = 5;
            if (std::uint8_t(third) == 1 || std::uint8_t(third) == 2) third = 3;
            else if (std::uint8_t(third) == 3) third = 5;
        }
        require(c.u8(data + 0xf076) == std::uint8_t(first) && c.u8(data + 0xf077) == std::uint8_t(second) &&
            c.u8(data + 0xf078) == std::uint8_t(third), "finishing-position mapping");
    }
}
void returned(State& s) {
    Cpu c(s);
    if (c.sp() != stack || c.lr() != sentinel || s.pc != sentinel)
        throw std::runtime_error("fixture did not return to caller");
    postconditions(c);
}
void original(State& s, std::uint32_t entry) {
    original_mode = true; s.pc = entry; run_until(s, sentinel); returned(s);
}
void rewrite(State& s, std::uint32_t entry) {
    original_mode = false;
    if (!try_modern_oceania_nations(s, entry)) throw std::runtime_error("production adapter declined owned entry");
    returned(s);
#if defined(CM_OCEANIA_NATIONS_COVERAGE)
    __llvm_profile_write_file();
#endif
}
enum class Corruption { rng_cursor, rng_seed, group_payload };
Corruption corruption = Corruption::rng_cursor;
void corrupt_memory(State& s, std::uint32_t entry) {
    // The ordinary runner verifies a completed return and postconditions before
    // any deliberate mutation. A matching fault cannot satisfy these controls.
    rewrite(s, entry); Cpu c(s);
    if (corruption == Corruption::rng_cursor) {
        const auto cursor = c.toc(-0x7b84); c.put32(cursor, c.u32(cursor) ^ 2);
    } else if (corruption == Corruption::rng_seed) {
        const auto seed = c.toc(-0x7b90); c.put32(seed, c.u32(seed) ^ 1);
    } else {
        const auto table = c.u32(object + 0xc), slot = table + std::uint32_t(active.group) * 4;
        c.put32(slot, c.u32(slot) ^ 4);
    }
}
std::vector<Case> cases() {
    std::vector<Case> result;
    for (auto entry : entries) result.push_back({entry,"default"});
    const auto add = [&](Case c) { result.push_back(std::move(c)); };
    for (int year : {-32768,-2,-1,0,1,2,1999,2000,2001,2002,2003,2004,32766,32767})
        for (int stage : {-128,-2,-1,0,1,2,127,255}) {
            Case c{0x106e9cb0,"calendar year/stage " + std::to_string(year) + "/" + std::to_string(stage)};
            c.year = year; c.stage = stage; add(c); c.output_flag = false; c.name += " no flag"; add(c);
            if (stage <= 1) { c.fail_allocation = 1; c.name += " OOM"; add(c); }
        }
    for (int year : {-32768,1999,2001,2002,2003,32767}) {
        Case c{0x106e9940,"construct year " + std::to_string(year)}; c.year = year; add(c);
        c.entry = 0x106eada0; c.name = "next season year " + std::to_string(year); add(c);
    }
    for (auto entry : {0x106e9940u,0x106eafa0u,0x106eb1d0u,0x106eb400u,0x106ebb10u})
        for (int fail : {1,2,3}) { Case c{entry,"allocation failure " + std::to_string(fail)}; c.fail_allocation = fail; add(c); }
    for (auto entry : {0x106e9940u,0x106eada0u}) for (int method : {0x20,0x94}) {
        Case c{entry,"virtual failure " + std::to_string(method)}; c.fail_method = method; add(c);
    }
    for (auto entry : {0x106e9ba0u,0x106eada0u}) for (int storage = 0; storage < 8; ++storage)
        for (int last : {-1,0,1,4}) {
            // A nonempty stage count requires a valid pointer table.
            if (last >= 0 && !(storage & 2)) continue;
            Case c{entry,"storage/current " + std::to_string(storage) + "/" + std::to_string(last)};
            c.storage = storage; c.current_stage = last; add(c);
        }
    for (int deleting : {-32768,-1,0,1,32767,65535,65537}) {
        Case c{0x106e9ba0,"deleting " + std::to_string(deleting)}; c.deleting = deleting; add(c);
    }
    { Case c{0x106e9ba0,"null destructor"}; c.null_object = true; add(c); }
    for (int current : {-2,-1,0,1,2,3}) { Case c{0x106eaee0,"advance stage " + std::to_string(current)}; c.current_stage = current; add(c); }
    for (auto entry : {0x106ea780u,0x106eac00u}) for (int phase : {-128,-1,0,1,2,127})
        for (int event : {-128,-2,-1,0,1,2,3,4,5,6,127}) {
            Case c{entry,"event phase/type " + std::to_string(phase) + "/" + std::to_string(event)};
            c.phase = phase; c.event = event; add(c);
            if (entry == 0x106ea780) { c.delegate = true; c.name += " delegate"; add(c); }
        }
    for (int count : {0,1,2,3,6,19,20,21,24,25,40}) for (int variant : {0,1,2})
        for (int rng : {0,1,2,3,4,5,6,7}) {
            Case c{0x106eb400,"selection count/filter/rng " + std::to_string(count) + "/" + std::to_string(variant) + "/" + std::to_string(rng)};
            c.count = count; c.nation_variant = variant; c.rng_variant = rng; add(c);
        }
    for (int mode : {0,1,-1,256}) for (int dy : {-1,0,1}) for (int dd : {-1,0,1}) {
        Case c{0x106eb7a0,"daily mode/year/day " + std::to_string(mode) + "/" + std::to_string(dy) + "/" + std::to_string(dd)};
        c.mode = mode; c.year_delta = dy; c.day_delta = dd; add(c);
        c.already_started = 1; c.name += " started"; add(c);
    }
    for (int year : {-32768,-1,0,32767}) for (int day : {-32768,-1,0,32767})
        for (int delta : {-1,0,1}) {
            Case c{0x106eb7a0,"daily signed date " + std::to_string(year) + "/" + std::to_string(day) + "/" + std::to_string(delta)};
            c.year = year; c.start_day = day; c.day_delta = delta; add(c);
        }
    for (int failure : {2,3,4}) { Case c{0x106eb7a0,"daily allocation failure " + std::to_string(failure)}; c.fail_allocation = failure; add(c); }
    for (auto service : {0x104a00a0,0x1049eb30}) { Case c{0x106eb7a0,"daily dependency failure " + std::to_string(service)}; c.fail_service = service; add(c); }
    { Case c{0x106ebb10,"ranking missing fixtures"}; c.storage = 6; add(c); }
    constexpr std::array rank_values{0,1,2,3,4,255,257};
    for (int phase : {-128,-1,0,1,2,127}) for (unsigned i = 0; i < rank_values.size(); ++i) {
        // The three ranking mappings are independent; rotated values cover
        // every outcome of each without a redundant Cartesian product.
        Case c{0x106ebcd0,"rank phase/values " + std::to_string(phase) + "/" + std::to_string(i)};
        c.phase = phase; c.a = rank_values[i]; c.b = rank_values[(i + 2) % rank_values.size()];
        c.d = rank_values[(i + 4) % rank_values.size()]; add(c);
    }
    for (int failure : {0,1,2,3}) {
        Case c{0x106eafa0,"second group allocation " + std::to_string(failure)};
        c.group = 1; c.fail_allocation = failure; add(c);
    }
    { Case c{0x106eafa0,"second group combined calendar"}; c.group = 1; c.combined = true; add(c); }
    for (auto entry : {0x106e9940u,0x106eaaf0u,0x106eada0u,0x106eafa0u,0x106eb1d0u,0x106eb7a0u}) {
        Case c{entry,"combined owned virtual callees"}; c.combined = true; add(c);
    }
    return result;
}
}
int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) { std::cerr << "Usage: cm-oceania-nations-checks <private-export> [entry]\n"; return 2; }
    try {
        auto base = std::make_unique<State>(); load_program(*base, argv[1]); dispatch_probe = probe;
        const auto selected = argc == 3 ? std::uint32_t(std::stoul(argv[2], nullptr, 0)) : 0u;
        unsigned failures = 0, compared = 0;
        for (const auto& test : cases()) {
            if (selected && selected != test.entry) continue;
            auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state);
            active = test; allocations = calls = 0; next_allocation = data + 0x80000;
            initialize(c, test); state->pc = test.entry;
            const auto differences = compare_routine_call(*state, test.entry, original, rewrite, RoutineChecker::result_r3);
            ++compared;
            std::cout << (differences.empty() ? "ok " : "DIFFERS ") << hex_address(test.entry) << " " << test.name << '\n';
            for (const auto& difference : differences) std::cout << "  " << difference << '\n';
            failures += !differences.empty();
            if (failures >= 10) break;
        }
        active = {selected ? selected : entries[0], "corruption control"}; allocations = calls = 0; next_allocation = data + 0x80000;
        auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize(c, active);
        const auto negative = compare_routine_call(*state, active.entry, original, rewrite, RoutineChecker::result_r3, true);
        if (negative.empty()) { ++failures; std::cout << "FAIL corrupted result not detected\n"; }
        else std::cout << "ok corrupted result detected\n";
        unsigned negative_controls = 1;
        for (const auto kind : {Corruption::rng_cursor,Corruption::rng_seed,Corruption::group_payload}) {
            const auto entry = kind == Corruption::group_payload ? 0x106eafa0u : 0x106eb400u;
            if (selected && selected != entry) continue;
            active = {entry,"memory corruption prerequisite"}; active.group = 1;
            allocations = calls = 0; next_allocation = data + 0x80000;
            auto checked = cm::semantic_io_checks::clone(*base); Cpu cc(*checked); initialize(cc, active);
            const auto prerequisite = compare_routine_call(*checked, entry, original, rewrite, RoutineChecker::result_r3);
            ++compared;
            if (!prerequisite.empty()) {
                ++failures; std::cout << "FAIL memory corruption prerequisite did not match\n";
                for (const auto& difference : prerequisite) std::cout << "  " << difference << '\n';
                continue;
            }
            corruption = kind;
            const auto wrong = compare_routine_call(*checked, entry, original, corrupt_memory, RoutineChecker::result_r3);
            const bool memory_detected = std::ranges::any_of(wrong, [](const auto& d) { return d.starts_with("byte "); });
            const bool stopped = std::ranges::any_of(wrong, [](const auto& d) { return d.find("stopped") != std::string::npos || d.find("fault") != std::string::npos; });
            if (!memory_detected || stopped) { ++failures; std::cout << "FAIL memory corruption control\n"; }
            else {
                ++negative_controls;
                std::cout << "ok corrupted " << (kind == Corruption::rng_cursor ? "RNG cursor" :
                    kind == Corruption::rng_seed ? "RNG seed" : "group entry payload") << " detected after returned comparison\n";
            }
        }
        std::cout << compared << " returned fixture comparisons requested, " << negative_controls << " negative controls, " << failures << " failed\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
