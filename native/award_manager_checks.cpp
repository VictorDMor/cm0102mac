#include "award_manager_adapter.hpp"
#include "guest_values.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

#include <array>
#include <iostream>
#include <random>
#include <unistd.h>

#if defined(CM_AWARD_MANAGER_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif

namespace cm::aot { bool award_manager_reference(State&); }
using namespace cm::aot;
using namespace cm::aot::guest;

namespace {
constexpr std::uint32_t module_start = 0x10436080, module_end = 0x10437cc0;
constexpr std::uint32_t data = 0x60000000, manager = data + 0x1000, lists = data + 0x2000,
    shortlist = data + 0x3000, arena = data + 0x4000, trace_address = data + 0x8000,
    stack = 0x71010000, returned = 0xfffffff0;
constexpr std::uint32_t groups = data + 0x19000, group = data + 0x19100,
    award = data + 0x19200, participant = data + 0x19300, assignment = data + 0x19400,
    nation = data + 0x19500, vtable = data + 0x19600, descriptor = data + 0x19700,
    history = data + 0x19800, stream = data + 0x19900, message = data + 0x19a00,
    output = data + 0x19c00, labels = data + 0x19e00, people = data + 0x1a000,
    clubs = data + 0x1b000, managers = data + 0x1c000, nations = data + 0x20000,
    virtual_target = 0x60024000;
constexpr std::uint32_t seed_base = 0x41574152;
struct Scenario {
    unsigned allocations = 0, fail_at = 0, volume_failures = 0, open = 1;
    bool legacy = false, super_cup = true, eligible = true, valid_name = true;
};
Scenario scenario;
std::uint32_t next_object;

bool reference, production, fail_allocation, external_call_active;
constexpr auto entries = std::to_array<std::uint32_t>({
#define AWARD_MANAGER_ROUTINE(entry, end, results, digest, ...) entry,
#include "award_manager_routines.inc"
#undef AWARD_MANAGER_ROUTINE
});
enum class Corruption { none, result, argument, target, order };
Corruption corruption = Corruption::none;

void trace(Cpu& c, std::uint32_t target) {
    auto count = c.u32(trace_address);
    if (count >= 0x10000 / 4 - 48) throw std::runtime_error("AWARD_MANAGER fixture exceeded trace space");
    const auto add = [&](std::uint32_t value) { c.put32(trace_address + 4 + 4 * count++, value); };
    add(target ^ (!reference && corruption == Corruption::target ? 4 : 0));
    add(c.lr());
    add(c.sp());
    add(c.r(2));
    if (target == 0x10781320) add(c.r(12));
    for (unsigned r = 3; r <= 10; ++r)
        add(c.r(r) ^ (!reference && r == 3 && corruption == Corruption::argument ? 1 : 0));
    for (unsigned r = 13; r <= 31; ++r) add(c.r(r));
    for (unsigned field = 0; field < 8; ++field) add(c.cr(field));
    c.put32(trace_address, count);
    // In the assertion sequence, exchanging the last two trace records
    // preserves their contents while deliberately violating call order.
    if (!reference && corruption == Corruption::order && target == 0x10769b60) {
        constexpr auto words = 39u;
        const auto earlier = trace_address + 4 + 4 * (count - 2 * words);
        const auto later = earlier + 4 * words;
        for (unsigned i = 0; i < words; ++i) {
            const auto offset = 4 * i;
            const auto first = c.u32(earlier + offset);
            c.put32(earlier + offset, c.u32(later + offset));
            c.put32(later + offset, first);
        }
    }
}

bool fixture(State& s) {
    const auto target = s.pc;
    if (target >= module_start && target < module_end) {
        if (reference) return award_manager_reference(s);
        if (production) {
            s.budget = s.executed;
            return false;
        }
        if (!award_manager_lists(target)) throw std::runtime_error("unimplemented owned call");
        award_manager_run_rewrite(s, target);
        return true;
    }
    Cpu c(s);
    if (external_call_active && target != virtual_target) return false;
    // The reference uses its own pages with all owner hooks and leaf inlining
    // disabled. Short leaf services shared with Cpu::call run on both routes.
    if (run_leaf(s)) return true;
    switch (target) {
    case 0x10775130:
    case 0x107751b0: {
        trace(c, target);
        const auto bytes = c.r(target == 0x10775130 ? 3 : 4);
        if (bytes == 0 || bytes > 0x100 || bytes % 4 != 0 ||
            (target == 0x107751b0 && c.r(3) != lists))
            throw std::runtime_error("invalid shortlist allocation fixture");
        if (!fail_allocation && target == 0x107751b0)
            for (unsigned offset = 0; offset < bytes - 20; offset += 4)
                c.put32(arena + offset, c.u32(lists + offset));
        c.set(3, fail_allocation ? 0 : arena);
        break;
    }
    case 0x10775200:
        trace(c, target);
        if (c.r(3) != 29 || c.r(4) != 4) throw std::runtime_error("invalid award table allocation");
        c.set(3, groups);
        for (unsigned i = 0; i < 29; ++i) c.put32(groups + 4 * i, 0);
        break;
    case 0x10773520: {
        trace(c, target);
        const auto bytes = c.r(3);
        if (bytes != 8 && bytes != 16 && bytes != 28) throw std::runtime_error("invalid award allocation size");
        const auto object = next_object;
        next_object += 32;
        if (next_object > data + 0x29000) throw std::runtime_error("award fixture allocation arena exhausted");
        c.set(3, ++scenario.allocations == scenario.fail_at ? 0 : object);
        break;
    }
    case 0x1077fee0:
        trace(c, target);
        if (c.r(3) || c.r(4) || c.u8(c.r(5)) == 0) throw std::runtime_error("invalid volume request");
        c.put32(c.sp() + 0x14, c.r(2));
        c.put16(c.r(6), 1);
        c.set(3, scenario.volume_failures ? 0xffffffdd : 0);
        if (scenario.volume_failures) --scenario.volume_failures;
        break;
    case 0x10778300: {
        trace(c, target);
        const auto address = c.r(3);
        if (c.r(4) != 0 || (c.r(5) != 0x40 && c.r(5) != 0xc0))
            throw std::runtime_error("invalid notification clear");
        c.s.memory.clear(address, c.r(5));
        break;
    }
    case 0x107475b0:
        trace(c, target);
        if (c.r(3) != stream || c.r(5) > 1 || c.r(6) != 0x16 || c.r(7) != 4)
            throw std::runtime_error("invalid award stream open");
        c.set(3, scenario.open ? stream + 0x20 : 0);
        break;
    case 0x10747900:
        trace(c, target);
        if (c.u32(c.r(3)) != stream + 0x20) throw std::runtime_error("invalid award stream close");
        c.put32(c.r(3), 0);
        break;
    case 0x10746280:
        trace(c, target);
        c.set(3, stream);
        break;
    case 0x101926b0:
        trace(c, target);
        c.set(3, scenario.super_cup);
        break;
    case 0x10781320:
    case 0x107156f0:
    case 0x107156b0:
    case 0x10715880: {
        trace(c, target);
        if (target == 0x10781320 && c.r(12) != descriptor)
            throw std::runtime_error("invalid award virtual descriptor");
        struct ExternalCall {
            ExternalCall() { external_call_active = true; }
            ~ExternalCall() { external_call_active = false; }
        } external_call;
        if (production && !reference) s.budget = s.executed + 10000;
        run_until(s, c.lr());
        if (production && !reference) s.budget = s.executed;
        return true;
    }
    case virtual_target:
        trace(c, target);
        if (c.r(2) != 0x108bba94) throw std::runtime_error("award virtual target has the wrong TOC");
        c.put32(c.r(3) + 0x38, c.r(4));
        c.set(3, 0x84);
        break;
    case 0x10439c10:
        trace(c, target);
        if (c.r(3) != shortlist || c.r(4) != participant)
            throw std::runtime_error("invalid award shortlist lookup");
        c.set(3, scenario.eligible ? 0 : 0xffffffff);
        break;
    case 0x102cb570:
        trace(c, target);
        if (c.r(3) != message || c.r(4) >= 10) throw std::runtime_error("invalid award news field");
        c.set(3, c.u32(message + 4 * c.r(4)));
        break;
    case 0x102cd0c0:
        trace(c, target);
        if (c.r(3) != message || c.r(4) != participant || c.r(6) < 4 || c.r(6) >= 10)
            throw std::runtime_error("invalid award news update");
        c.put32(message + 4 * c.r(6), c.r(7));
        break;
    case 0x1030d6e0:
        trace(c, target);
        for (unsigned i = 0; i < 4; ++i) c.put32(c.r(4) + 4 * i, i == 3 ? 0 : i + 50);
        break;
    case 0x100cb590:
        trace(c, target);
        c.put32(c.r(6), 0x54657374);
        c.put8(c.r(6) + 4, 0);
        break;
    case 0x100cbde0:
        trace(c, target);
        c.set(3, scenario.valid_name);
        break;
    case 0x10727890:
        trace(c, target);
        c.put32(c.r(3), c.r(4));
        c.put32(c.r(3) + 4, c.r(7));
        break;
    case 0x1074b830:
    case 0x1074b960:
    case 0x1074bab0:
        trace(c, target);
        if (c.r(3) != output) throw std::runtime_error("invalid award news output");
        c.put32(output + 4 * (target == 0x1074b830 ? 0 : target == 0x1074b960 ? 1 : 2), c.r(4));
        break;
    case 0x10446c10:
    case 0x1044e480: {
        trace(c, target);
        if (c.r(4) != stream + 0x20) throw std::runtime_error("invalid award child stream");
        const auto count = c.u32(stream + 0x20);
        c.put32(stream + 0x24 + 8 * count, c.r(3));
        c.put32(stream + 0x28 + 8 * count, c.u32(c.r(3) + 4));
        c.put32(stream + 0x20, count + 1);
        break;
    }
    case 0x10748bb0:
    case 0x1044e220:
        trace(c, target);
        c.put32(c.r(3), c.r(4));
        c.put32(c.r(3) + 4, c.r(5));
        break;
    case 0x1044e5a0:
    case 0x1044ec10:
        trace(c, target);
        c.put32(c.r(3) + 8, c.r(4));
        break;
    case 0x10748ce0:
        trace(c, target);
        if (c.r(5) > 8) throw std::runtime_error("invalid award history output capacity");
        if (c.r(5)) c.put32(c.r(4), c.r(6));
        c.set(3, c.r(5) ? 1 : 0);
        break;
    case 0x10749fd0:
    case 0x10748c80:
    case 0x1044e270:
    case 0x104443b0:
    case 0x107735a0:
    case 0x10775170:
        trace(c, target);
        c.put32(c.r(3), 0);
        break;
    case 0x100a4c70:
        trace(c, target);
        c.put32(data + 0x18000, 1);
        break;
    case 0x10739890:
        trace(c, target);
        c.put32(c.r(6), 0x41574400);
        c.put32(c.r(7), 0x43505000);
        // The next call is the one-instruction version-text leaf. Cpu::call
        // reaches it before the dispatcher probe and consumes this allowance.
        if (production && !reference) s.budget = s.executed + 1;
        break;
    case 0x1077b720:
        trace(c, target);
        c.put32(c.r(3), c.r(8));
        break;
    case 0x10769b60:
        trace(c, target);
        c.set(3, 1);
        break;
    case 0x10435880:
    case 0x10439fe0:
    case 0x1043a840:
    case 0x1043c980:
    case 0x1043d160:
    case 0x1043e5b0:
    case 0x1043f350:
    case 0x1043fe40:
    case 0x10440620:
    case 0x10440c70:
    case 0x104422a0:
    case 0x10446d90:
    case 0x10447af0:
    case 0x104483a0:
    case 0x10449710:
    case 0x10449de0:
    case 0x1044c0b0:
    case 0x1044c7d0:
    case 0x105e4dc0:
    case 0x106adfe0:
    case 0x106ae880:
    case 0x106aef60:
    case 0x106afa70:
    case 0x106b01d0:
    case 0x106b0f30:
    case 0x106b1a10:
    case 0x106b2310:
    case 0x106b2d30:
    case 0x106b32f0:
    case 0x106b3fc0:
    case 0x106b47e0:
    case 0x106b4eb0:
    case 0x106b5420:
        trace(c, target);
        c.put32(c.r(3), target);
        c.put32(c.r(3) + 4, c.r(4));
        break;
    default:
        return false;
    }
    s.pc = c.lr();
    return true;
}

void prepare(Cpu& c, std::uint32_t seed) {
    c.s.memory.map(data, 0x30000);
    c.s.memory.map(stack - 0x10000, 0x20000);
    c.s.memory.copy_in(stack - 0x10000, std::vector<std::uint8_t>(0x20000, 0xa5));
    std::mt19937 random(seed);
    for (unsigned r = 0; r < 32; ++r) c.set(r, random());
    for (unsigned r = 0; r < 32; ++r) c.set_f(r, 0.125 + r);
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, field + 1);
    c.set(1, stack);
    c.set(2, 0x108bba90);
    c.set_lr(returned);
    c.put32(c.r(2) - 0x7ba8, data + 0x400);
    c.put32(c.r(2) - 0x7bac, data + 0x500);
    c.put8(c.toc(-0x7bac), 1);
    c.s.budget = c.s.executed + 1000000;
}

void check_return(State& s) {
    const Cpu c(s);
    if (c.sp() != stack || c.lr() != returned || s.pc != returned)
        throw std::runtime_error("AWARD_MANAGER call did not return");
}

void original(State& s, std::uint32_t entry) {
    reference = true;
    next_object = data + 0x28000;
    alarm(15);
    s.pc = entry;
    run_until(s, returned);
    check_return(s);
    alarm(0);
}

void modern(State& s, std::uint32_t entry) {
    reference = false;
    next_object = data + 0x28000;
    alarm(15);
    if (production) {
        // Owned code starts with an exhausted budget. The probe grants a
        // budget only upon reaching a legitimate external service.
        s.budget = s.executed;
        s.pc = entry;
        run_until(s, returned);
    } else {
        award_manager_run_rewrite(s, entry);
    }
    check_return(s);
    alarm(0);
    Cpu c(s);
    if (corruption == Corruption::result) c.set(3, c.r(3) ^ 1);
#if defined(CM_AWARD_MANAGER_COVERAGE)
    const auto path = std::string(CM_AWARD_MANAGER_PROFILE_DIR) + "/check-" + std::to_string(getpid()) + ".profraw";
    __llvm_profile_set_filename(path.c_str());
    if (__llvm_profile_write_file() != 0) throw std::runtime_error("AWARD_MANAGER coverage flush failed");
#endif
}

struct ObservedWrites final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
};

void original_control(const State& base) {
    ObservedWrites writes;
    dispatch_probe = nullptr;
    for (auto entry : entries) {
        auto s = cm::semantic_io_checks::clone(base);
        Cpu c(*s);
        prepare(c, seed_base);
        s->memory.observer = &writes;
        s->budget = s->executed;
        s->pc = entry;
        if (award_manager_dispatch_probe(*s)) throw std::runtime_error("original override accepted the C++ probe");
        try {
            run_until(*s, returned);
        } catch (const std::exception& e) {
            if (std::string_view(e.what()) == "Native instruction budget exhausted" && s->pc == entry)
                continue;
            throw;
        }
        throw std::runtime_error("original did not exhaust its budget at " + hex_address(entry));
    }
    std::cout << entries.size() << " original instruction-budget controls passed\n";
}

void compare_case(State& s, std::uint32_t entry, std::string_view name,
                  std::string_view expected_difference = {}) {
    ObservedWrites writes;
    if (production) s.memory.observer = &writes;
    const auto differences = compare_routine_call(s, entry, original, modern,
                                                  RoutineChecker::result_r3 | RoutineChecker::result_f1);
    s.memory.observer = nullptr;
    const auto contains = [&](const std::string& difference) {
        return difference.find(expected_difference) != std::string::npos;
    };
    if (expected_difference.empty() ? differences.empty() : std::ranges::any_of(differences, contains)) return;
    for (const auto& difference : differences) std::cerr << "  " << difference << '\n';
    throw std::runtime_error(hex_address(entry) + " " + std::string(name) + ": unexpected comparison result");
}

unsigned check_shortlist_registration(const State& base) {
    unsigned returned_cases = 0;
    for (unsigned mode = 0; mode < 5; ++mode) {
        for (unsigned seed = 1; seed <= 12; ++seed) {
            auto s = cm::semantic_io_checks::clone(base);
            Cpu c(*s);
            prepare(c, seed);
            const auto size = mode <= 1 ? 0 : 3 + seed % 9;
            c.put16(manager + 0x18, size);
            c.put16(manager + 0x1a, size + (mode == 4));
            c.put32(manager + 0x14, mode < 2 ? 0 : lists);
            for (unsigned i = 0; i < size; ++i) c.put32(lists + 4 * i, shortlist + i * 0x50);
            c.set(3, manager);
            c.set(4, shortlist);
            fail_allocation = mode == 1 || mode == 3;
            compare_case(*s, 0x10437730, "shortlist registration");
            ++returned_cases;
            if (mode == 0 && seed == 1 && !production) {
                corruption = Corruption::result;
                compare_case(*s, 0x10437730, "result control", "r3");
                corruption = Corruption::argument;
                compare_case(*s, 0x10437730, "argument control", "600080");
                corruption = Corruption::target;
                compare_case(*s, 0x10437730, "call target control", "600080");
                corruption = Corruption::none;
            }
            if (mode == 1 && seed == 1 && !production) {
                corruption = Corruption::order;
                compare_case(*s, 0x10437730, "call order control", "60008");
                corruption = Corruption::none;
            }
        }
    }
    return returned_cases;
}

unsigned check_recent_history(const State& base) {
    unsigned returned_cases = 0;
    for (int year : {-32768, 0, 1999, 2000, 2001, 2026, 32767}) {
        for (unsigned mode = 0; mode < 3; ++mode) {
            auto s = cm::semantic_io_checks::clone(base);
            Cpu c(*s);
            prepare(c, std::uint32_t(year));
            c.set(3, mode == 0 ? 0 : manager);
            c.put32(manager + 0x14, mode == 2 ? lists : 0);
            c.put16(lists + 0xa, year);
            compare_case(*s, 0x10437b20, "recent history");
            ++returned_cases;
        }
    }
    return returned_cases;
}

void initialize_records(Cpu& c, unsigned variant, unsigned seed) {
    const auto toc = c.r(2);
    unsigned global_offset = 0x100;
    const auto global = [&](int slot, unsigned value) {
        c.put32(toc + unsigned(slot), data + global_offset);
        c.put32(data + global_offset, value);
        global_offset += 4;
    };
    constexpr std::array nation_ids{
        -0x7508, -0x7510, -0x7b68, -0x750c, -0x7a10, -0x7594, -0x7a00, -0x7af4, -0x7a28,
        -0x7574, -0x7a14, -0x7a24, -0x7504, -0x79fc, -0x7a1c, -0x7a18, -0x7514, -0x7a20,
        -0x75a8, -0x7598, -0x7590, -0x7650, -0x7578, -0x75b0, -0x758c, -0x757c};
    for (unsigned i = 0; i < nation_ids.size(); ++i) {
        global(nation_ids[i], i);
        c.put8(nations + i * 0x128 + 0x120, seed % 2 ? 1 : 255);
    }
    for (int slot : {-0x70f0, -0x70fc, -0x70d8}) global(slot, scenario.legacy ? 0xffffffff : 1);
    global(-0x7a70, 0);
    global(-0x7520, nations);
    global(-0x7b98, nations);
    global(-0x7be4, 64);
    global(-0x7be0, people);
    global(-0x7be8, clubs);
    global(-0x7bf0, message);
    c.put32(toc - 0x7bb8, output + 0x40);
    c.put32(toc - 0x7bb0, managers);
    c.put32(managers + 4, participant);
    c.put16(managers + 0x2c58, 0);
    c.put32(manager, variant & 1 ? 0 : history + 0x20);
    c.put32(manager + 4, variant & 2 ? 0 : history + 0x40);
    c.put32(manager + 8, groups);
    c.put32(manager + 0x10, variant & 1 ? 0 : history + 0x60);
    c.put32(manager + 0xc, variant & 2 ? 0 : history + 0x80);
    c.put32(manager + 0x14, variant & 1 ? 0 : lists);
    c.put32(manager + 0xdc, variant & 4 ? 2 : 0);
    c.put32(groups + 4 * (variant & 8 ? 28 : 0), group);
    c.put32(group, vtable);
    c.put32(group + 4, lists);
    c.put16(group + 8, variant & 4 ? 0 : 2);
    c.put32(group + 0x10, variant & 2 ? 0 : nation);
    c.put8(group + 0x14, variant & 1 ? 0 : 1);
    c.put32(lists, award);
    c.put32(lists + 4, award + 0x40);
    c.put32(award, vtable);
    c.put32(award + 8, labels);
    c.put32(award + 12, seed * 17);
    c.put32(award + 0x24, shortlist);
    c.put32(award + 0x40 + 8, labels + 4);
    c.put32(award + 0x40 + 12, seed * 19);
    c.put32(participant + 0x14, variant & 4 ? 0 : assignment);
    c.put8(participant + 0x3a, variant & 16 ? 1 : 0);
    c.put32(assignment + 8, variant & 8 ? 0 : nation);
    c.put32(vtable + 0xc, descriptor);
    c.put32(vtable + 0x10, descriptor);
    c.put32(vtable + 0x14, descriptor);
    c.put32(descriptor, virtual_target);
    c.put32(descriptor + 4, 0x108bba94);
    c.put8(labels + 0x63, 3);
    c.put8(labels + 0x64, 5);
    c.put8(clubs + 0x23b, 7);
    c.put32(shortlist, shortlist + 0x100);
    c.put16(shortlist + 0x40, 7);
    c.put16(shortlist + 0x42, 2002);
    c.put32(shortlist + 0x100, 0);
    c.put32(shortlist + 0x104, variant & 4 ? 0xffffffff : 0);
}

template <class Setup>
void check_cases(const State& base, std::uint32_t entry, unsigned count, Setup setup) {
    for (unsigned i = 0; i < count; ++i) {
        auto s = cm::semantic_io_checks::clone(base);
        Cpu c(*s);
        scenario = {};
        prepare(c, seed_base + i);
        initialize_records(c, i, seed_base + i);
        c.set(3, manager);
        setup(c, i);
        compare_case(*s, entry, "case " + std::to_string(i));
    }
}

unsigned check_lifecycle(const State& base) {
    check_cases(base, 0x10436080, 16, [](Cpu& c, unsigned i) {
        // The original interleaves Pascal volume templates until the byte
        // following the copied odd template. It reads that byte from its
        // incoming stack, so seed the terminator as in the new-game capture.
        c.put8(stack - 0x360 + 0x61, 0);
        scenario.volume_failures = i & 1;
        scenario.open = i & 2;
        if (!(i & 4))
            for (unsigned nation_index = 0; nation_index < 26; ++nation_index)
                c.put8(nations + nation_index * 0x128 + 0x120, 0);
        scenario.fail_at = i > 7 ? i - 7 : 0;
        scenario.super_cup = !(i & 8);
        for (int slot : {-0x70f0, -0x70fc, -0x70d8}) c.put32(c.toc(slot), i & 8 ? 0xffffffff : 1);
        c.set(4, i & 2 || i & 4 ? stream : 0);
    });
    check_cases(base, 0x104362d0, 24, [](Cpu& c, unsigned i) {
        if (i & 4) c.put32(manager + 8, 0);
        c.set(3, i & 8 ? 0 : manager);
        c.set(4, i & 16 ? 1 : i & 2 ? 0xffffffff : 0);
    });
    check_cases(base, 0x104365b0, 36, [](Cpu& c, unsigned i) {
        c.set(4, i & 1 ? stream : 0);
        scenario.legacy = i & 1;
        scenario.super_cup = i < 32;
        scenario.fail_at = i <= 29 ? i : 0;
        if (i >= 32)
            for (unsigned j = 0; j < 26; ++j) c.put8(nations + j * 0x128 + 0x120, i == 35 && j % 2 ? 255 : 0);
        for (int slot : {-0x70f0, -0x70fc, -0x70d8}) c.put32(c.toc(slot), scenario.legacy ? 0xffffffff : 1);
    });
    check_cases(base, 0x10436dc0, 12, [](Cpu& c, unsigned i) {
        c.set(4, i ? stream : 0);
        scenario.open = i != 1;
        if (i > 1) scenario.fail_at = i - 2 + 23;
    });
    check_cases(base, 0x10436f60, 12, [](Cpu& c, unsigned i) {
        c.set(4, i ? stream : 0);
        scenario.open = i != 1;
    });
    return 100;
}

unsigned check_updates(const State& base) {
    check_cases(base, 0x104363e0, 34, [](Cpu& c, unsigned i) {
        if (i >= 32) {
            // The original reads +0x3a before checking for a null
            // participant. The classic Mac low-memory window permits it.
            c.s.memory.map(0, 0x100);
            c.put8(0x3a, i == 33 ? 1 : 0);
        }
        c.set(4, i >= 32 ? 0 : participant);
        c.set(5, 0x1f3);
    });
    check_cases(base, 0x104364e0, 16, [](Cpu& c, unsigned i) {
        if (i & 8) c.put32(groups + 28 * 4, 0);
        c.set(4, output);
    });
    for (auto entry : {0x104370f0u, 0x10437160u}) {
        check_cases(base, entry, 8, [](Cpu& c, unsigned i) {
            c.put32(manager, history + 0x20);
            c.put32(manager + 4, history + 0x40);
            leave(c, {{4, i * 31}, {5, output}, {6, i}});
        });
    }
    for (auto entry : {0x10437130u, 0x104371a0u}) {
        check_cases(base, entry, 2, [](Cpu& c, unsigned) {
            c.put32(manager, history + 0x20);
            c.put32(manager + 4, history + 0x40);
        });
    }
    check_cases(base, 0x104371d0, 24, [](Cpu& c, unsigned i) {
        c.set(4, i & 16 ? 0 : nation);
    });
    return 94;
}

unsigned check_news(const State& base) {
    check_cases(base, 0x104372c0, 2, [](Cpu& c, unsigned i) {
        c.put32(award + 0x40, vtable);
        leave(c, {{4, 0}, {5, i}, {6, message}, {7, participant}, {8, output}});
    });
    check_cases(base, 0x10437310, 32, [](Cpu& c, unsigned i) {
        const bool published = i & 1, detail = i & 2;
        scenario.eligible = !(i & 8);
        scenario.valid_name = !(i & 16);
        c.put32(message, i & 8 ? 28 : 0);
        c.put32(message + 8, i == 4 || i == 6 ? 8 : 7);
        c.put32(message + 12, i == 16 || i == 18 ? 2003 : 2002);
        c.put32(message + 16, published ? 1 : 0);
        c.put32(message + 20, i & 4 ? 0xffffffff : 0);
        if (i == 0 || i == 2) c.put32(award + 0x24, 0);
        leave(c, {{4, message}, {5, output}, {6, detail}});
    });
    return 34;
}

unsigned check_queries(const State& base) {
    for (auto entry : {0x10437850u, 0x10437910u}) {
        check_cases(base, entry, 8, [entry](Cpu& c, unsigned i) {
            constexpr std::array ids{47u, 48u, 49u, 63u, 64u, 65u, 66u, 48u};
            c.put32(participant, ids[i]);
            c.put32(manager + 0x1c, i != 7 ? 5 : 11);
            c.put32(manager + 0x28, 5);
            c.put32(manager + 0xd0, 5);
            c.put32(manager + 0xdc, 5);
            leave(c, {{4, i == 6 && entry == 0x10437910 ? 0 : participant}, {5, 5}, {6, 31}, {7, 19}});
        });
    }
    for (auto entry : {0x10437a00u, 0x10437a90u}) {
        check_cases(base, entry, 16, [entry](Cpu& c, unsigned i) {
            const auto seed = seed_base + i;
            const auto key = entry == 0x10437a00 ? (i & 1 ? labels + 4 : labels) : seed * (i & 1 ? 19 : 17);
            c.set(4, i & 2 ? key + 1 : key);
            if (i == 6 || i == 14) c.put16(group + 8, 0xffff);
        });
    }
    check_cases(base, 0x10437b50, 51, [](Cpu& c, unsigned i) {
        if (i >= 48) {
            constexpr std::array slots{-0x7a00, -0x7a28, -0x7514};
            leave(c, {{3, 0}, {4, participant}});
            c.put32(participant + 0x14, assignment);
            c.put32(assignment + 8, nation);
            c.put32(nation + 0x68, c.u32(c.toc(slots[i - 48])));
            return;
        }
        c.set(3, i & 1 ? 0 : clubs);
        c.set(4, i & 2 ? 0 : participant);
        c.put32(clubs + 0xc, i & 4 ? 0 : nation);
        c.put32(participant + 0x14, i & 8 ? 0 : assignment);
        c.put32(assignment + 8, i & 16 ? 0 : nation + 0x80);
        constexpr std::array slots{-0x750c, -0x7510, -0x7a00, -0x7a28, -0x7514, -0x7508};
        c.put32(nation + 0x68, c.u32(c.toc(slots[(i / 2) % slots.size()])));
        c.put32(nation + 0x80 + 0x68, c.u32(c.toc(slots[(i / 3) % slots.size()])));
    });
    return 99;
}

void check_nested_routing(const State& base) {
    constexpr std::uint32_t constructor = 0x10436080, load_entry = 0x10436dc0, initialize = 0x104365b0;
    for (auto excluded : {0u, load_entry, initialize}) {
        auto s = cm::semantic_io_checks::clone(base);
        Cpu c(*s);
        scenario = {};
        prepare(c, seed_base);
        initialize_records(c, 0, seed_base);
        c.put8(stack - 0x360 + 0x61, 0);
        leave(c, {{3, manager}, {4, stream}});
        next_object = data + 0x28000;
        RoutineChecker selection([=](auto entry) { return award_manager_lists(entry) && entry != excluded; }, 0, 0);
        routine_checker = &selection;
        s->pc = constructor;
        try {
            run_until(*s, returned);
            check_return(*s);
            if (excluded) throw std::runtime_error("excluded entry took the C++ route");
        } catch (const std::exception& e) {
            if (!excluded || std::string_view(e.what()) != "Native instruction budget exhausted" || s->pc != excluded)
                throw;
        }
        routine_checker = nullptr;
        const auto& calls = selection.totals();
        if (calls.at(constructor).calls != 1 ||
            (excluded == load_entry ? calls.contains(load_entry) : calls.at(load_entry).calls != 1) ||
            (excluded ? calls.contains(initialize) : calls.at(initialize).calls != 1))
            throw std::runtime_error("nested award checker did not reach the selected entries");
    }
    auto s = cm::semantic_io_checks::clone(base);
    Cpu c(*s);
    scenario = {};
    prepare(c, seed_base);
    initialize_records(c, 0, seed_base);
    c.put8(stack - 0x360 + 0x61, 0);
    leave(c, {{3, manager}, {4, stream}});
    RoutineChecker combined([](auto entry) { return award_manager_lists(entry); }, 0, 0);
    routine_checker = &combined;
    compare_case(*s, constructor, "combined selected child");
    routine_checker = nullptr;
    std::cout << "Selected and excluded nested routes passed\n";
}

}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) return 2;
    const std::string_view option = argc == 3 ? argv[2] : "";
    production = option == "--production";
    if (!option.empty() && !production && option != "--original-control") return 2;
    try {
        auto base = std::make_unique<State>();
        load_program(*base, argv[1]);
        if (option == "--original-control") {
            original_control(*base);
            return 0;
        }
        dispatch_probe = fixture;
        const auto registrations = check_shortlist_registration(*base);
        const auto history = check_recent_history(*base);
        const auto lifecycle = check_lifecycle(*base);
        const auto updates = check_updates(*base);
        const auto news = check_news(*base);
        const auto queries = check_queries(*base);
        if (production) check_nested_routing(*base);
        std::cout << registrations + history + lifecycle + updates + news + queries
                  << " returned comparisons across all AWARD_MANAGER entries\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
