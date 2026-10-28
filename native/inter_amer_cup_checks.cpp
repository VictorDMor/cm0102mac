// Returned comparisons against hook-free originals. Scripted dependencies
// expose call order, arguments, CR, preserved registers and RNG effects in
// compared guest memory; selected cases also run the real calendar/sampler.
#include "inter_amer_cup_adapter.hpp"
#include "inter_amer_cup_detail.hpp"
#include "semantic_io_checks.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#if defined(CM_INTER_AMER_CUP_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using guest::Cpu;
namespace cm::aot { bool inter_amer_cup_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, object = data + 0x1000,
    definitions = data + 0x2000, clubs = data + 0x20000,
    nations = data + 0x60000, continents = data + 0x70000,
    trace_at = data + 0x180000, stack = 0x71010000, sentinel = 0xfffffff0;
constexpr std::array entries{0x106ebdd0u,0x106ec0b0u,0x106ec1a0u,
    0x106ec330u,0x106ec7b0u,0x106eca00u};
struct Case {
    std::uint32_t entry;
    std::string name;
    int stage = -1, year = 2002, count = 16, storage = 15, rounds = 3,
        deleting = 0, fail_allocation = 0, fail_virtual = -1, virtual_result = -1,
        north_active = 1, south_active = 1, missing_champion = 0,
        filter = 0, stop_index = -1, stop_result = 1, rng = 0;
    bool null_object = false, output_flag = true, real_calendar = false, real_sampler = false;
};
Case active{entries[0], "default"};
unsigned allocations = 0, calls = 0;
std::uint32_t next_allocation = 0;
bool reference_mode = false;
const std::set<std::uint32_t> dependencies{
    0x100c7d00,0x101926b0,0x104533a0,0x104536c0,0x104564a0,0x104635b0,
    0x10463a90,0x10478f20,0x10478f70,0x104854f0,0x10485570,0x10739890,
    0x10768300,0x10768810,0x10769b60,0x10773520,0x107735a0,0x10775130,
    0x10775170,0x1077b720,0x1077b770};
void trace(Cpu& c, std::uint32_t target) {
    if (++calls > 4000) throw std::runtime_error("fixture call limit");
    auto index = c.u32(trace_at);
    const auto add = [&](std::uint32_t value) { c.put32(trace_at + 4 + index++ * 4, value); };
    add(target); add(c.lr());
    for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned field = 0; field < 8; ++field) add(c.cr(field));
    if (target == 0x10485570) add(c.u32(c.sp() + 0x38));
    c.put32(trace_at, index);
}
bool probe(State& s) {
    Cpu c(s); const auto target = s.pc;
    if (run_leaf(s)) return true;
    if (target == 0x10781320) {
        const auto descriptor = c.r(12), entry = c.u32(descriptor), method = c.u32(descriptor + 8);
        c.put32(c.sp() + 0x14, c.r(2));
        if (active.fail_virtual != int(method) && inter_amer_cup_lists(entry)) {
            c.set(2, c.u32(descriptor + 4)); s.pc = entry; return true;
        }
        trace(c, entry);
        c.set(3, active.fail_virtual == int(method) ? std::uint32_t(active.virtual_result) : method == 0x20 ? 0 : 1);
        s.pc = c.lr(); return true;
    }
    if (inter_amer_cup_lists(target)) {
        if (reference_mode) return inter_amer_cup_reference(s);
        return inter_amer_cup_dispatch_probe(s);
    }
    if (reference_mode && target >= entries[0] && target < 0x106eca50) return inter_amer_cup_reference(s);
    if (active.real_calendar && (target == 0x104854f0 || target == 0x10485570)) return false;
    if (!dependencies.contains(target)) return false;
    trace(c, target);
    if (active.real_sampler && target == 0x104635b0) return false;
    if (target == 0x10775130 || target == 0x10773520) {
        ++allocations;
        if (active.fail_allocation == int(allocations)) c.set(3, 0);
        else { const auto size = c.r(3), at = next_allocation; next_allocation += (size + 31) & ~15u; c.set(3, at); }
    } else if (target == 0x101926b0) c.set(3, c.r(3) == continents ? active.north_active : active.south_active);
    else if (target == 0x10463a90) {
        const bool north = c.r(3) == definitions;
        c.set(3, active.missing_champion & (north ? 1 : 2) ? 0 : clubs + (north ? 0 : 1) * 0x248);
    } else if (target == 0x100c7d00) {
        const auto index = (c.r(3) - clubs) / 0x248;
        c.set(3, int(index) == active.stop_index ? active.stop_result : 0);
    } else if (target == 0x104635b0) {
        // Scripted sample service mutates the list and guest RNG state, making
        // a changed number/order of calls observable in the differential check.
        const auto count = c.r(4), cursor = c.toc(-0x7b84), seed = c.toc(-0x7b90);
        const auto at = c.r(3), choice = c.u32(seed) % count;
        const auto first = c.u32(at); c.put32(at, c.u32(at + choice * 4)); c.put32(at + choice * 4, first);
        c.put32(seed, c.u32(seed) * 1664525u + 1013904223u); c.put32(cursor, c.u32(cursor) + 2);
    } else if (target == 0x10768300) {
        // Sort is an external boundary. The real sampler still runs all of
        // its own RNG, arithmetic, stack and list manipulation.
        std::vector<std::uint32_t> list;
        for (unsigned i = 0; i < c.r(4); ++i) list.push_back(c.u32(c.r(3) + i * 4));
        std::sort(list.begin(), list.end());
        for (unsigned i = 0; i < list.size(); ++i) c.put32(c.r(3) + i * 4, list[i]);
    } else if (target == 0x1077b770) c.set(3, 0x1234 + active.rng);
    else if (target == 0x10739890) { c.put32(c.r(6), 0x49414300); c.put32(c.r(7), 0x43505000); }
    else if (target == 0x1077b720) c.put32(c.r(3), c.r(8));
    else if (target == 0x10478f20) {
        for (unsigned off : {8,0xa0,0xa4,0xac}) c.put32(c.r(3) + off, 0);
    } else if (target != 0x104854f0 && target != 0x10485570) c.set(3, 1);
    s.pc = c.lr(); return true;
}
void initialize(Cpu& c) {
    c.s.memory.map(data, 0x200000); c.s.memory.map(stack - 0x10000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x200000, 0xa5));
    c.s.memory.copy_in(stack - 0x10000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    c.set(1, stack); c.set(2, 0x108bba90); c.set_lr(sentinel);
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, field + 1);
    c.put32(trace_at, 0);
    const auto variable = [&](int slot, unsigned offset, std::uint32_t value) {
        c.put32(c.r(2) + std::uint32_t(slot), data + offset); c.put32(data + offset, value);
    };
    variable(-0x7b50, 0x100, active.count); variable(-0x7be8, 0x104, clubs);
    variable(-0x7bbc, 0x108, definitions); variable(-0x7520, 0x10c, continents);
    variable(-0x6b98, 0x110, 0); variable(-0x6b9c, 0x114, 1);
    variable(-0x68f8, 0x118, 0); variable(-0x6820, 0x11c, 1);
    variable(-0x7b84, 0x120, data + 0x180 + (active.rng & 4 ? 0x10 : 0));
    variable(-0x7b88, 0x124, data + 0x190); variable(-0x7b90, 0x128, active.rng & 1);
    c.put32(c.r(2) - 0x7b8c, data + 0x180);
    for (unsigned i = 0; i < 16; ++i) c.put16(data + 0x180 + i * 2, (active.rng >> 1) & 1);
    c.put32(c.r(2) - 0x7ba8, data + 0x8000); c.put32(c.r(2) - 0x7bac, data + 0x9000);
    const auto vtable = data + 0x3000, descriptors = data + 0x4000;
    c.put32(c.r(2) - 0x5cc0, vtable); c.put32(object, vtable);
    unsigned n = 0;
    for (auto [method, entry] : std::array<std::pair<unsigned, unsigned>,5>{{
        {0x20,0x10479000},{0x44,0x106ec1a0},{0x64,0x10479020},{0x94,0x106eca00},{0x9c,0x10479010}}}) {
        const auto descriptor = descriptors + n++ * 16;
        c.put32(vtable + method, descriptor); c.put32(descriptor, entry);
        c.put32(descriptor + 4, c.r(2)); c.put32(descriptor + 8, method);
    }
    c.put32(object + 4, definitions); c.put16(object + 0x40, active.year);
    c.put32(object + 8, active.storage & 1 ? data + 0xa000 : 0);
    c.put32(object + 0xa0, active.storage & 2 ? data + 0xb000 : 0);
    c.put32(object + 0xa4, active.storage & 4 ? data + 0xc000 : 0);
    c.put32(object + 0xac, active.storage & 8 ? data + 0xd000 : 0);
    c.put16(object + 0x3c, active.rounds);
    for (int i = 0; i < active.rounds; ++i) c.put32(data + 0xd000 + i * 4, i % 2 ? 0 : data + 0xe000 + i * 32);
    for (int i = 0; i < active.count; ++i) {
        const auto club = clubs + std::uint32_t(i) * 0x248, nation = nations + std::uint32_t(i) * 0x128;
        c.put32(club, i); c.put32(club + 0x2c, nation);
        c.put32(nation + 0x74, continents + (i % 2) * 0xc8); c.put32(continents + (i % 2) * 0xc8 + 8, i % 2);
        c.put16(club + 0x1ec, 6000);
        if (active.filter == 1) {
            if (i % 7 == 0) c.put16(club + 0x1ec, 1000);
            if (i % 7 == 1) c.put16(club + 0x1ec, 5000);
            if (i % 7 == 2) c.put32(club + 0x2c, 0);
            if (i % 7 == 3) c.put32(nation + 0x74, 0);
            if (i % 7 == 4) { c.put32(nation + 0x74, continents + 0x190); c.put32(continents + 0x198, 2); }
            if (i % 7 == 5) c.put16(club + 0x1ec, 0xffff);
        } else if (active.filter == 2) c.put16(club + 0x1ec, 1000);
    }
    c.set({{3,active.null_object ? 0 : object},{4,std::uint32_t(active.stage)},
        {5,data + 0xf000},{6,data + 0xf004},{7,active.output_flag ? data + 0xf008 : 0}});
    if (active.entry == entries[0]) c.set({{4,std::uint32_t(active.year)},{5,definitions}});
    if (active.entry == entries[1]) c.set(4, active.deleting);
    c.s.budget = 40000000;
}
void returned(State& s) {
    Cpu c(s);
    if (c.sp() != stack || c.lr() != sentinel || s.pc != sentinel) throw std::runtime_error("fixture did not return");
    if (active.entry == entries[2] && std::int8_t(active.stage) == -1) {
        if (c.u16(data + 0xf000) != 1 || c.u16(data + 0xf004) != 0 ||
            (active.output_flag && c.u8(data + 0xf008) != 0)) throw std::runtime_error("calendar output fields");
        if (active.fail_allocation != 1) {
            const auto round = c.r(3);
            if (c.u16(round + 0x1a) != 2 || c.u8(round + 0x23) != 2 ||
                c.u8(round + 0x24) != 7 || c.u8(round + 0x25) != 0xa5 || c.u32(round + 0x70) != 0)
                throw std::runtime_error("calendar constants/padding");
        }
    }
    if (active.entry == entries[5] && c.r(3) != 1) throw std::runtime_error("configuration result");
    if (active.entry == entries[3] && active.fail_allocation != 1) {
        const auto entrants = c.u32(object + 0xa4);
        if (c.i16(object + 0x3e) != 2 || c.u8(entrants + 6) != 0xa5) throw std::runtime_error("entrant count/padding");
        if (!active.missing_champion) {
            if (c.r(3) != 1 || c.u8(entrants + 4) || c.u8(entrants + 5) || c.u8(entrants + 12) || c.u8(entrants + 13))
                throw std::runtime_error("entrant flags/result");
            if (active.count == 0 && !active.north_active && active.year != 2001 && c.u32(entrants) != 0xa5a5a5a5)
                throw std::runtime_error("first fallback must leave first entrant untouched");
        }
    }
}
void original(State& s, std::uint32_t entry) {
    reference_mode = true; s.pc = entry; run_until(s, sentinel); returned(s);
}
void rewrite(State& s, std::uint32_t entry) {
    reference_mode = false;
    if (!try_modern_inter_amer_cup(s, entry)) throw std::runtime_error("production hook declined");
    returned(s);
#if defined(CM_INTER_AMER_CUP_COVERAGE)
    __llvm_profile_write_file();
#endif
}
unsigned corruption = 0;
void corrupt_result(State& s, std::uint32_t entry) {
    rewrite(s, entry); Cpu c(s); c.set(3, c.r(3) ^ 1);
}
void corrupt_memory(State& s, std::uint32_t entry) {
    rewrite(s, entry); Cpu c(s);
    const auto at = corruption == 0 ? c.toc(-0x7b84) : corruption == 1 ? c.toc(-0x7b90) : c.u32(object + 0xa4);
    c.put32(at, c.u32(at) ^ 1);
}
std::vector<Case> cases() {
    std::vector<Case> result;
    for (auto entry : entries) result.push_back({entry,"default"});
    for (auto entry : {entries[0],entries[2],entries[3],entries[4],entries[5]}) for (int failure : {1,2,3}) {
        Case c{entry,"allocation " + std::to_string(failure)}; c.fail_allocation = failure; result.push_back(c);
    }
    for (auto entry : {entries[0],entries[4],entries[5]}) for (int method : {0x20,0x44,0x94,0x9c})
        for (int value : {-1,0,1}) {
            Case c{entry,"virtual " + std::to_string(method) + "/" + std::to_string(value)};
            c.fail_virtual = method; c.virtual_result = value; result.push_back(c);
        }
    for (int stage : {-128,-2,-1,0,1,127,255,511}) {
        Case c{entries[2],"stage " + std::to_string(stage)}; c.stage = stage; result.push_back(c);
        c.output_flag = false; c.name += " no flag"; result.push_back(c);
    }
    for (int year : {1998,1999,2000,2001,2004,2099,2100,32767,-32768}) {
        Case c{entries[2],"real calendar " + std::to_string(year)}; c.year = year; c.real_calendar = true; result.push_back(c);
        c.entry = entries[4]; c.name = "year advance " + std::to_string(year); c.real_calendar = false; result.push_back(c);
    }
    for (auto entry : {entries[1],entries[4]}) for (int storage = 0; storage < 16; ++storage)
        for (int rounds : {-1,0,1,7}) {
            Case c{entry,"storage/rounds " + std::to_string(storage) + "/" + std::to_string(rounds)};
            c.storage = storage; c.rounds = rounds; result.push_back(c);
        }
    for (int deleting : {-32768,-1,0,1,32767,65535,65537}) {
        Case c{entries[1],"deleting " + std::to_string(deleting)}; c.deleting = deleting; result.push_back(c);
    }
    { Case c{entries[1],"null destructor"}; c.null_object = true; result.push_back(c); }
    for (int year : {2001,2002}) for (int north : {0,1,256}) for (int south : {0,1,256})
        for (int missing : {0,1,2,3}) {
            Case c{entries[3],"selection year/active/missing " + std::to_string(year) + "/" +
                std::to_string(north) + "/" + std::to_string(south) + "/" + std::to_string(missing)};
            c.year = year; c.north_active = north; c.south_active = south; c.missing_champion = missing; result.push_back(c);
        }
    for (auto entry : {entries[0],entries[4]}) for (int missing : {1,2}) {
        Case c{entry,"zero selector result continues " + std::to_string(missing)};
        c.missing_champion = missing; result.push_back(c);
    }
    for (int count : {0,1,2,3,16,30}) for (int filter : {0,1,2}) for (int stop : {-1,0,1,3}) {
        Case c{entries[3],"scan count/filter/stop " + std::to_string(count) + "/" + std::to_string(filter) + "/" + std::to_string(stop)};
        c.count = count; c.filter = filter; c.stop_index = stop; c.north_active = c.south_active = 0; result.push_back(c);
        if (stop == 1) { c.stop_result = 256; c.name += " truncated stop"; result.push_back(c); }
    }
    for (int rng = 0; rng < 8; ++rng) for (int count : {2,3,16}) {
        Case c{entries[3],"real sampler " + std::to_string(rng) + "/" + std::to_string(count)};
        c.rng = rng; c.count = count; c.north_active = c.south_active = 0; c.real_sampler = true; result.push_back(c);
    }
    return result;
}
}
int inter_amer_cup_native_checks(const char* snapshot, const char* game_directory, const char* backing);
int inter_amer_cup_retirement_checks(const char*, const char*, const char*);
int main(int argc, char** argv) {
    if (argc == 5) return inter_amer_cup_native_checks(argv[2], argv[3], argv[4]);
    if (argc == 6 && std::string_view(argv[2]) == "--retirement")
        return inter_amer_cup_retirement_checks(argv[3], argv[4], argv[5]);
    if (argc != 2) { std::cerr << "Usage: cm-inter-amer-cup-checks <original-segments> [career.snapshot game-directory private-backing]\n"; return 2; }
    try {
        auto base = std::make_unique<State>(); load_program(*base, argv[1]); dispatch_probe = probe;
        unsigned failures = 0; const auto tests = cases();
        for (const auto& test : tests) {
            active = test; allocations = calls = 0; next_allocation = data + 0x80000;
            auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize(c); state->pc = test.entry;
            const auto differences = compare_routine_call(*state, test.entry, original, rewrite,
                test.entry == entries[4] ? 0 : RoutineChecker::result_r3);
            std::cout << (differences.empty() ? "ok " : "DIFFERS ") << hex_address(test.entry) << ' ' << test.name << '\n';
            for (const auto& difference : differences) std::cout << "  " << difference << '\n';
            failures += !differences.empty();
        }
        active = tests.front(); allocations = calls = 0; next_allocation = data + 0x80000;
        auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize(c);
        const auto wrong_result = compare_routine_call(*state, active.entry, original, corrupt_result, RoutineChecker::result_r3);
        const bool result_detected = std::ranges::any_of(wrong_result,
            [](const std::string& line) { return line.starts_with("r3 "); });
        if (!result_detected) { ++failures; std::cout << "FAIL wrong-result control\n"; }
        else std::cout << "ok wrong-result control detected\n";
        active = {entries[3],"memory controls"}; active.north_active = active.south_active = 0;
        allocations = calls = 0; next_allocation = data + 0x80000;
        state = cm::semantic_io_checks::clone(*base); Cpu d(*state); initialize(d);
        for (corruption = 0; corruption < 3; ++corruption) {
            const auto negative = compare_routine_call(*state, active.entry, original, corrupt_memory, RoutineChecker::result_r3);
            const auto address = corruption == 0 ? data + 0x120 : corruption == 1 ? data + 0x128 : data + 0x80000;
            const auto prefix = "byte " + hex_address(address + 3).substr(2) + " ";
            const bool detected = std::ranges::any_of(negative,
                [&](const std::string& line) { return line.starts_with(prefix); });
            if (!detected) { ++failures; std::cout << "FAIL memory control " << corruption << '\n'; }
            else std::cout << "ok memory control " << corruption << " detected\n";
        }
        std::cout << tests.size() << " returned fixture comparisons, " << failures << " failed\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
