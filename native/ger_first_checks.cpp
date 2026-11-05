#include "ger_first_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

#include <array>
#include <random>
#include <string_view>

#ifdef CM_GER_FIRST_PROFILE_DIR
#include <unistd.h>
extern "C" int __llvm_profile_write_file();
extern "C" void __llvm_profile_set_filename(const char*);
#endif

namespace {
using namespace cm::aot;
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto collect = 0x10539590u, allocate = 0x10775130u, insert = 0x104a60e0u;
constexpr auto prize = 0x1049f540u, payment = 0x1049f6d0u;
constexpr auto calendar = 0x10537120u, configure = 0x105397b0u;
constexpr auto create = 0x10536d50u, destroy = 0x10537080u;
constexpr auto link_divisions = 0x10539bb0u, promote = 0x10539d20u;
constexpr auto object = fixture + 0x100, standings = fixture + 0x400;
constexpr auto strings = fixture + 0xc00, buffer = fixture + 0xd00;
constexpr auto save_ok = fixture + 0xe00, mode = save_ok + 4;
constexpr auto division = fixture + 0x20, total = division + 4, clubs_slot = division + 8;
constexpr auto youth_blocks = division + 12, target_id = division + 16, definitions_slot = division + 20;
constexpr auto foreign_ids = division + 24;
constexpr auto clubs = fixture + 0x10000, definitions = fixture + 0x20000;
constexpr auto schedule = fixture + 0x40000, headers = fixture + 0x1300, optional_flag = headers + 4;
constexpr auto vtable = fixture + 0x1000, descriptor = fixture + 0x1200;
constexpr auto other_division = fixture + 0x1400, external_calendar = 0x104aa0c0u, alternate_toc = toc + 0x20000;
constexpr auto later_calendar = 0x1053af10u;
constexpr auto initial_check = 0x104a6850u, external_configure = fixture + 0x1820;
constexpr auto stage_object = fixture + 0x50000, common_definition = fixture + 0x1480;
constexpr auto child_array = fixture + 0x1500;
constexpr auto division_table_slot = fixture + 0x1680, rng_state = fixture + 0x1690;
constexpr auto second_object = fixture + 0x60000, regional_object = fixture + 0x61000;
constexpr auto regional_child = fixture + 0x62000, division_table = fixture + 0x63000;
constexpr auto second_rows = fixture + 0x64000, temporary = fixture + 0x69000;
constexpr auto external_sort = fixture + 0x1830, partner = fixture + 0x1840;
constexpr auto size = 0x44u, club_stride = 0x248u, definition_size = 0x6cu;

struct Call {
    std::uint32_t target, continuation;
    std::array<std::uint32_t, 8> arguments;
    // External callees can spill preserved registers and FP status to guest
    // stack memory. Compare their complete incoming architectural state too.
    decltype(State::registers) machine;
    bool operator==(const Call&) const = default;
};
struct Trace { std::vector<Call> calls; std::uint32_t corrupted_target = 0; unsigned corrupt_argument = 2; };
Trace* active_trace = nullptr;

struct Diagnostic { std::uint32_t start; unsigned message, detail, line; };
constexpr auto diagnostics = std::to_array<Diagnostic>({
    {0x10539688, 0x40, 0x38, 0x26e}, {0x1053717c, 0x70, 0x48, 0xa1},
    {0x10536e34, 0xe0, 0x58, 0x47}, {0x10536e9c, 0xc0, 0x50, 0x4e},
    {0x10536f04, 0xa0, 0x48, 0x55}, {0x10536f70, 0x80, 0x40, 0x5e},
    {0x10536ff8, 0x60, 0x38, 0x65},
    {0x10539d78, 0x44, 0x3c, 0x354},
});

bool dependency(State& s) {
    Cpu c(s);
    const auto target = s.pc, arg = c.r(3);
    switch (target) {
        case external_sort:
            require((arg == object || arg == second_object || arg == regional_object || arg == regional_child)
                        && c.r(2) == alternate_toc, "German foreign division virtual preparation");
            break;
        case 0x104a9210:
            require(arg == object && c.r(6) == 1 && c.r(7) == 1 && c.r(8) == UINT32_MAX && c.r(9) == UINT32_MAX
                        && ((c.lr() == 0x10539c48 && c.r(4) == object && c.r(5) == second_object)
                            || (c.lr() == 0x10539ccc && c.r(4) == second_object && c.r(5) == regional_object)
                            || (c.lr() == 0x10539d08 && c.r(4) == second_object && c.r(5) == regional_child)),
                    "German division relationship arguments");
            break;
        case 0x100ce320:
            require(arg >= clubs && arg < clubs + 32 * club_stride && (arg - clubs) % club_stride == 0
                        && c.r(4) == c.sp() + 0x38 && c.r(5) == 1, "German reserve-team lookup arguments");
            break;
        case 0x104635b0:
            require(arg == temporary && c.r(4) > 0 && c.r(4) <= 32 && c.r(5) == 4 && c.lr() == 0x10539e78,
                    "German ordered selection service arguments");
            break;
        case 0x1049f780:
        case 0x1049f8e0:
            require(arg == object && c.r(4) >= clubs && c.r(4) < clubs + 32 * club_stride
                        && (c.r(4) - clubs) % club_stride == 0 && c.r(6) == 1
                        && c.r(5) == definitions + c.u32(target == 0x1049f780 ? other_division : target_id) * definition_size,
                    "German promotion/relegation arguments");
            break;
        case initial_check:
            require(arg == object && c.r(4) == 1 && c.r(2) == toc, "German initial virtual check");
            break;
        case external_configure:
        case 0x1049e4f0:
        case 0x104a00a0:
            require(arg == object, "German lifecycle object argument");
            break;
        case 0x1049eb30:
        case 0x104a6660:
        case 0x1049e600:
            require(arg == object && c.r(4) == 0, "German lifecycle setup/cleanup arguments");
            break;
        case 0x10773520:
            require(arg == 0x634, "German stage allocation size");
            break;
        case 0x104533a0:
            require(arg == stage_object && c.r(4) == object, "German stage construction arguments");
            break;
        case 0x104536c0:
            require(arg == stage_object && c.r(4) == 1, "German stage destruction arguments");
            break;
        case 0x10775170:
            require(arg == schedule || arg == temporary, "German owned buffer release");
            break;
        case 0x107735a0:
            require(arg == object, "German object deletion");
            break;
        case allocate:
            require((arg == 18 * size && c.lr() == 0x1053967c)
                        || (arg == 0xa18 && c.lr() == 0x10537174)
                        || (arg == 8 && c.lr() == 0x10536e08)
                        || (arg == c.u32(total) * 4 && c.lr() == 0x10539d70), "German allocation arguments");
            break;
        case 0x104a9810:
            require(arg == schedule && c.r(4) < 34 && c.r(5) >= 1 && c.r(5) <= 31 && c.r(6) <= 11
                        && c.r(7) <= 1 && (c.r(8) == 2 || c.r(8) == 5) && (c.r(9) == 1 || c.r(9) == 2)
                        && c.r(10) == std::uint32_t(c.i16(object + 0x40)) && c.u32(c.sp() + 0x38) == 0,
                    "German calendar date arguments");
            break;
        case 0x104a9890:
            require(arg == schedule && c.r(4) < 34 && c.r(5) < 3 && c.r(9) == 0
                        && ((c.r(6) == 6 && c.r(7) == 1 && c.r(8) == 1)
                            || (c.r(6) == UINT32_MAX && c.r(7) == UINT32_MAX && c.r(8) == UINT32_MAX)),
                    "German calendar part arguments");
            break;
        case external_calendar:
        case later_calendar:
            require((target == external_calendar && c.lr() == 0x1053705c && arg == object && c.r(2) == toc)
                        || (arg == object && c.r(4) == UINT32_MAX && c.r(5) == object + 0xa6
                        && c.r(6) == object + 0x3a && c.r(7) == 0 && c.r(2) == alternate_toc),
                    "German external virtual calendar override");
            break;
        case insert:
            require(arg == object && c.r(4) < 32 && c.r(5) >= clubs
                        && c.r(5) < clubs + 32 * club_stride && c.r(6) == 0 && c.r(7) == 0
                        && c.lr() == 0x1053972c && c.r(26) == c.r(4) + 1,
                    "German insertion arguments and pre-call increment");
            break;
        case prize:
            require(arg == object && c.r(4) == 200000 && c.lr() == 0x10539778,
                    "German competition value arguments");
            break;
        case payment:
            require(arg == object && c.r(4) == 5000000 && c.r(5) == 0 && c.lr() == 0x10539798,
                    "German initial payment arguments");
            break;
        case 0x10739890:
            require(arg == strings && c.r(4) == 0 && c.r(5) == 0
                        && std::ranges::any_of(diagnostics, [&](auto d) {
                            return c.lr() == d.start + 24 && c.r(6) == c.sp() + d.message && c.r(7) == c.sp() + d.detail; }),
                    "German diagnostic arguments");
            break;
        case 0x1077b720:
            require(arg == buffer && c.r(4) == strings + 0xe && c.r(5) == 0x10203040
                        && std::ranges::any_of(diagnostics, [&](auto d) {
                            return c.lr() == d.start + 56 && c.r(6) == c.sp() + d.message
                                && c.r(7) == c.sp() + d.detail && c.r(8) == d.line; }),
                    "German diagnostic formatting arguments");
            break;
        case 0x10769b60:
            require(arg == strings + 0x1b && c.r(4) == buffer && c.r(5) == 0, "German allocation alert arguments");
            break;
        default:
            return false;
    }
    if (active_trace) {
        Call call{target, c.lr(), {c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)}, s.registers};
        if (active_trace->corrupted_target == target) call.arguments[active_trace->corrupt_argument] ^= 4;
        active_trace->calls.push_back(call);
    }
    if (target == allocate) {
        if (c.u8(mode) == 6) c.put16(object + 0x40, c.i16(object + 0x40) + 1);
        const auto failed = c.u8(mode) == 1 || (c.u8(mode) == 17 && arg == 0xa18)
                            || (c.u8(mode) == 18 && arg == 18 * size);
        c.set(3, failed ? 0 : c.lr() == 0x10537174 ? schedule : c.lr() == 0x10536e08 ? child_array : standings);
        if (!failed && c.lr() == 0x10539d70) c.set(3, temporary);
    }
    if (target == external_sort) { c.put8(arg + 0xe2, 1); c.set(3, 0x90); }
    if (target == 0x104a9210) {
        c.put32(c.r(4) + 0x24, c.r(5));
        if (c.u8(mode) == 23 && c.lr() == 0x10539c48)
            c.put32(division_table + c.u32(target_id) * 4, 0);
        c.set(3, 0x91);
    }
    if (target == 0x100ce320) {
        c.put8(c.r(4), c.u32(arg + 0x10));
        c.set(3, c.u32(arg + 0x14));
    }
    if (target == 0x104635b0) {
        // A deterministic external-service fixture consumes every candidate
        // and models an ordered random selection; it is not a game RNG claim.
        const auto count = c.r(4);
        for (unsigned i = 0; i < count; ++i)
            require(c.u32(temporary + i * 4) >= clubs && c.u32(temporary + i * 4) < clubs + 32 * club_stride,
                    "German selected candidate pointer");
        if (count < c.r(5)) {
            // The external original reports this condition and returns with
            // its input list intact. Its caller still consumes four slots.
            c.put8(save_ok, 0);
            c.set(3, save_ok);
        } else {
            auto seed = c.u32(rng_state);
            for (unsigned i = 0; i < c.r(5); ++i) {
                seed = seed * 1664525u + 1013904223u;
                const auto j = i + seed % (count - i);
                const auto left = c.u32(temporary + i * 4), right = c.u32(temporary + j * 4);
                c.put32(temporary + i * 4, right);
                c.put32(temporary + j * 4, left);
            }
            c.put32(rng_state, seed);
            c.set(3, temporary);
        }
    }
    if (target == 0x1049f780 || target == 0x1049f8e0) {
        const auto club = c.r(4), destination = c.r(5);
        c.put32(club + 0x38, c.u32(club + 0x30));
        c.put32(club + 0x30, destination);
        if (c.u8(mode) == 21 && target == 0x1049f780) c.put32(other_division, 19);
        if (c.u8(mode) == 22 && target == 0x1049f8e0) c.put16(second_object + 0x3e, 0);
        c.set(3, 0x92);
    }
    if (target == initial_check) c.set(3, c.u8(mode) == 10 ? 1 : 0);
    if (target == external_configure) c.set(3, 0);
    if (target == 0x104a00a0) c.set(3, c.u8(mode) == 12 ? 0 : 1);
    if (target == 0x1049eb30) c.set(3, c.u8(mode) == 13 ? 0 : 1);
    if (target == 0x10773520) c.set(3, c.u8(mode) == 14 ? 0 : stage_object);
    if (target == 0x104533a0) {
        c.put32(stage_object, object);
        c.set(3, c.u8(mode) == 15 ? stage_object + 4 : stage_object);
    }
    if (target == 0x10775170) { c.put32(arg, 0xfefefefe); c.set(3, 0x98); }
    if (target == 0x104536c0 || target == 0x1049e600 || target == 0x107735a0) c.set(3, 0x96);
    if (target == 0x104a9810) {
        const auto row = schedule + c.r(4) * 76;
        for (unsigned arg = 5; arg <= 10; ++arg) c.put32(row + (arg - 5) * 4, c.r(arg));
        if (c.u8(mode) == 5 && c.r(4) == 0) c.put16(object + 0x40, c.i16(object + 0x40) + 1);
        c.set(3, 0x81);
    }
    if (target == 0x104a9890) {
        const auto row = schedule + c.r(4) * 76 + 24 + c.r(5) * 16;
        for (unsigned arg = 6; arg <= 9; ++arg) c.put32(row + (arg - 6) * 4, c.r(arg));
        c.set(3, 0x82);
    }
    if (target == external_calendar || target == later_calendar) c.set(3, c.lr() == 0x1053705c ? 0x96 : schedule);
    if (target == insert) {
        const auto row = standings + c.r(4) * size;
        c.put32(row, c.u32(c.r(5)));
        c.put8(row + 0x40, 0);
        if (c.u8(mode) == 2 && c.r(4) == 8) c.put16(object + 0x3e, 9);
        if (c.u8(mode) == 3) c.put32(division, c.u32(division) + 1);
        if (c.u8(mode) == 4 && c.r(4) == 4) c.put32(total, 5);
        c.set(3, 0x77);
    }
    if (target == prize) c.set(3, 0x55667788);
    if (target == payment) { c.put32(object + 0x24, c.r(4)); c.set(3, 0x88); }
    if (target == 0x10739890) { c.put32(c.r(6), strings); c.put32(c.r(7), strings + 4); }
    if (target == 0x1077b720) c.put32(buffer, c.r(8));
    if (target == 0x10769b60) c.set(3, 0xa4);
    c.finish();
    return true;
}

struct Services {
    decltype(dispatch_probe) previous_probe = dispatch_probe;
    RoutineChecker* previous_checker = routine_checker;
    Trace* previous_trace = active_trace;
    Services(Trace* trace, RoutineChecker* checker) {
        dispatch_probe = dependency;
        routine_checker = checker;
        active_trace = trace;
    }
    ~Services() { dispatch_probe = previous_probe; routine_checker = previous_checker; active_trace = previous_trace; }
};

std::unique_ptr<State> league_fixture(unsigned count = 18, unsigned layout = 0, int year = 2000,
                                    unsigned youth = 0, bool target_is_current = false) {
    auto s = std::make_unique<State>();
    s->registers.fill(0xa5);
    s->unique.fill(0x37);
    s->memory.map(fixture, 0x2000);
    s->memory.map(clubs, 0x6000);
    s->memory.map(definitions, 0x10000);
    s->memory.map(stack - 0x500, 0x600);
    s->memory.map(toc - 0x8000, 0x2000);
    s->memory.map(toc - 0x5000, 0x1000);
    s->memory.map(toc - 0x5b20, 4);
    Cpu c(*s);
    c.set({{1, stack}, {2, toc}, {3, object}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7b50, total);
    c.put32(toc - 0x7be8, clubs_slot);
    c.put32(toc - 0x7b04, youth_blocks);
    c.put32(toc - 0x6738, division);
    c.put32(toc - 0x6750, target_id);
    c.put32(toc - 0x7bbc, definitions_slot);
    c.put32(toc - 0x7ba8, buffer);
    c.put32(toc - 0x7bac, save_ok);
    c.put32(toc - 0x44f8, strings);
    c.put32(toc - 0x5b20, 0x10203040);
    c.put32(total, count);
    c.put32(clubs_slot, clubs);
    c.put32(division, target_is_current ? 5 : 77);
    c.put32(target_id, 5);
    c.put32(definitions_slot, definitions);
    c.put32(youth_blocks, youth);
    c.put16(object + 0x40, year);
    c.put8(save_ok, 1);
    for (unsigned i = 0; i < 100; ++i) c.put32(definitions + i * definition_size, i);
    for (unsigned i = 0; i < 4; ++i) {
        c.put32(toc - 0x6740 - i * 4, foreign_ids + i * 4);
        c.put32(foreign_ids + i * 4, 10 + i);
    }
    for (unsigned i = 0; i < 32; ++i) {
        const auto club = clubs + i * club_stride;
        const auto group = layout == 1 ? i % 7 : layout == 2 && i >= 18 ? 1 + (i - 18) % 4 : 0;
        c.put32(club, i);
        c.put32(club + 0x30, group == 6 ? 0 : definitions + (group == 0 ? c.u32(division)
                                                                             : group == 5 ? 99 : 9 + group) * definition_size);
        c.put32(club + 0x38, 0x33445566);
    }
    for (unsigned i = 0; i < 18 * size; ++i) c.put8(standings + i, 0xa5);
    s->allocator_original = true;
    s->pc = collect;
    s->budget = 20000;
    return s;
}

std::unique_ptr<State> calendar_fixture(std::uint32_t entry, int year = 2001, std::uint32_t format = UINT32_MAX) {
    auto s = league_fixture(18, 0, year);
    s->memory.map(schedule, 0x2000);
    Cpu c(*s);
    c.set({{4, format}, {5, headers}, {6, headers + 2}, {7, optional_flag}});
    c.put32(object, vtable);
    c.put32(vtable + 0x44, descriptor);
    c.put32(descriptor, calendar);
    c.put32(descriptor + 4, toc);
    c.put32(toc - 0x673c, other_division);
    c.put32(other_division, 18);
    c.put16(headers, 0x6a6a);
    c.put16(headers + 2, 0x7b7b);
    c.put8(optional_flag, 0xa5);
    s->pc = entry;
    return s;
}

std::unique_ptr<State> lifecycle_fixture(std::uint32_t entry, unsigned mode_value = 0,
                                       unsigned flags = 3, int year = 2001) {
    auto s = calendar_fixture(entry, year);
    s->memory.map(stage_object, 0x1000);
    s->memory.map(toc - 0x6000, 4);
    Cpu c(*s);
    c.put32(toc - 0x6000, vtable);
    for (const auto& [slot, offset, target] : std::array<std::array<std::uint32_t, 3>, 4>{{
            {0x94, 8, mode_value == 11 ? external_configure : configure},
            {0x20, 16, initial_check}, {0x64, 24, external_calendar}, {8, 32, destroy}}}) {
        c.put32(vtable + slot, descriptor + offset);
        c.put32(descriptor + offset, target);
        c.put32(descriptor + offset + 4, toc);
    }
    c.put32(object + 4, common_definition);
    c.put32(object + 8, flags & 1 ? stage_object : 0);
    c.put32(object + 0xbc, flags & 2 ? schedule : 0);
    c.put32(object + 0xc, 0);
    c.put8(mode, mode_value);
    c.set(4, entry == create ? std::uint32_t(year) : 1);
    c.set(5, common_definition);
    return s;
}

std::unique_ptr<State> relationships_fixture(std::uint32_t entry, bool regional = true, unsigned mode_value = 0) {
    auto s = lifecycle_fixture(entry, mode_value, 0);
    s->memory.map(second_object, 0x10000);
    Cpu c(*s);
    c.put32(toc - 0x7b5c, division_table_slot);
    c.put32(division_table_slot, division_table);
    c.put32(division_table + c.u32(division) * 4, object);
    c.put32(division_table + c.u32(other_division) * 4, second_object);
    c.put32(division_table + c.u32(target_id) * 4, regional ? regional_object : 0);
    c.put32(vtable + 0xac, descriptor + 0x40);
    c.put32(descriptor + 0x40, external_sort);
    c.put32(descriptor + 0x44, alternate_toc);
    for (auto competition : {second_object, regional_object, regional_child}) c.put32(competition, vtable);
    c.put32(regional_object + 0xc, child_array);
    c.put32(child_array, regional_child);
    c.put32(second_object + 0xb0, second_rows);
    c.put16(second_object + 0x3e, 4);
    for (unsigned i = 0; i < 4; ++i) {
        c.put32(second_rows + i * size, clubs + (28 + i) * club_stride);
        c.put8(second_rows + i * size + 0x40, i < 2 ? 3 : 0);
    }
    c.put32(rng_state, 0x13579bdf);
    return s;
}

std::unique_ptr<State> promotion_fixture(unsigned count = 18, unsigned candidates = 8, bool reserve_cases = false,
                                       unsigned mode_value = 0) {
    auto s = relationships_fixture(promote, true, mode_value);
    Cpu c(*s);
    c.put32(total, count);
    for (unsigned i = 0; i < 32; ++i)
        c.put32(temporary + i * 4, clubs + (count ? (i + 8) % count : 0) * club_stride);
    for (unsigned i = 0; i < 32; ++i) {
        const auto club = clubs + i * club_stride;
        c.put32(club + 0x2c, common_definition);
        c.put32(club + 0x30, definitions + (i < candidates ? c.u32(target_id) : c.u32(division)) * definition_size);
        c.put32(club + 0x10, 0);
        c.put32(club + 0x14, 0);
    }
    if (reserve_cases) {
        c.put32(clubs + 8 * club_stride + 0x30, 0);
        c.put32(clubs + 9 * club_stride + 0x30, definitions + c.u32(target_id) * definition_size);
        c.put32(clubs + 9 * club_stride + 0x2c, 0);
        for (unsigned i = 11; i < 15; ++i) {
            c.put32(clubs + i * club_stride + 0x30, definitions + c.u32(target_id) * definition_size);
            c.put32(clubs + i * club_stride + 0x14, partner);
            c.put32(clubs + i * club_stride + 0x10, i == 11 ? 0 : i == 12 ? 0x180 : i == 13 ? 0x100 : 0x101);
        }
    }
    c.put16(second_object + 0x3e, count >= 4 ? 4 : 0);
    for (unsigned i = 0; i < 4 && count >= 4; ++i) {
        const auto club = clubs + (count - 4 + i) * club_stride;
        c.put32(second_rows + i * size, club);
        c.put32(club + 0x30, definitions + c.u32(other_division) * definition_size);
    }
    if (mode_value == 21) c.put32(division_table + 19 * 4, second_object);
    return s;
}

struct Outcome {
    std::uint32_t result;
    unsigned instructions, inserted;
    std::string stop;
    std::array<std::uint32_t, 32> current, previous;
    unsigned save_success;
    unsigned promoted = 0, relegated = 0, relationships = 0;
};
unsigned rejected_results = 0, rejected_traces = 0;

Outcome compare_routes(const State& seed, bool negative_result = false, bool negative_trace = false,
                       bool lifecycle = false, bool observer_capacity = false) {
    auto original = clone(seed), modern = clone(seed);
    Effects expected, actual;
    expected.state = original.get(); actual.state = modern.get();
    if (observer_capacity) expected.on_read = actual.on_read = [](State& s, Effects& effects, std::uint32_t address, unsigned) {
        if (effects.mutated || address != stack + 0x18 || effects.log.size() < 2) return;
        const auto previous = effects.log[effects.log.size() - 2];
        if (previous.kind == 2 && previous.address == object + 0x2c) {
            effects.mutated = true;
            Cpu(s).put32(object + 0x2c, 2);
        }
    };
    original->memory.date_observer = &expected; modern->memory.date_observer = &actual;
    Trace old_calls, new_calls;
    const auto negative_target = seed.pc == collect || seed.pc == create ? insert
                                : seed.pc == destroy ? 0x104a6660u
                                : seed.pc == promote ? 0x1049f780u
                                : seed.pc == link_divisions ? 0x104a9210u : 0x104a9810u;
    new_calls.corrupted_target = negative_trace ? negative_target : 0;
    new_calls.corrupt_argument = seed.pc == destroy ? 0 : 2;
    if (seed.pc == promote) new_calls.corrupt_argument = 1;
    const auto execute = [lifecycle](State& s, Trace& trace, bool reference) {
        RoutineChecker no_hooks([](std::uint32_t) { return false; }, 0, 0);
        Services services(&trace, reference ? &no_hooks : nullptr);
        s.allocator_original = true;
        try {
            if (reference) ger_first_run_original(s, s.pc);
            else run_until(s, return_sentinel);
            if (lifecycle) {
                Cpu c(s);
                c.set({{3, object}, {4, 1}});
                c.set_lr(return_sentinel);
                s.pc = destroy;
                if (reference) ger_first_run_original(s, destroy);
                else run_until(s, return_sentinel);
            }
            return std::string{};
        } catch (const std::runtime_error& error) { return std::string(error.what()); }
    };
    const auto old_stop = execute(*original, old_calls, true), new_stop = execute(*modern, new_calls, false);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto label = "GER_FIRST " + hex_address(seed.pc) + " limit=" + std::to_string(seed.budget);
    require(old_stop == new_stop, label + " original=" + old_stop + " C++=" + new_stop);
    require(old_stop.empty() || old_stop == "Native instruction budget exhausted"
                || old_stop.starts_with("Unmapped data access at "), label + " unexpected stop " + old_stop);
    if (expected.bytes != actual.bytes) {
        for (unsigned i = 0; i < std::min(expected.log.size(), actual.log.size()); ++i) {
            const auto& a = expected.log[i]; const auto& b = actual.log[i];
            if (a.kind != b.kind || a.address != b.address || a.size != b.size || a.value != b.value)
                throw std::runtime_error(label + " effect=" + std::to_string(i) + " expected=" + std::to_string(a.kind)
                    + ":" + hex_address(a.address) + ":" + hex_address(a.value) + " actual=" + std::to_string(b.kind)
                    + ":" + hex_address(b.address) + ":" + hex_address(b.value));
        }
        const auto last = [](const Effects& effects) {
            std::string result;
            for (auto i = effects.log.size() > 4 ? effects.log.size() - 4 : 0; i < effects.log.size(); ++i) {
                const auto& e = effects.log[i];
                result += " " + std::to_string(e.kind) + ":" + hex_address(e.address) + ":" + hex_address(e.value);
            }
            return result;
        };
        throw std::runtime_error(label + " effect counts " + std::to_string(expected.log.size()) + "/"
            + std::to_string(actual.log.size()) + " PCs " + hex_address(original->pc) + "/" + hex_address(modern->pc)
            + " expected tail" + last(expected) + " actual tail" + last(actual));
    }
    require(original->executed == modern->executed, label + " changed budget accounting");
    if (observer_capacity) require(expected.mutated && actual.mutated, "German capacity observer not reached");
    if (negative_trace) {
        require(old_calls.calls != new_calls.calls && std::ranges::any_of(old_calls.calls,
                    [negative_target](const Call& call) { return call.target == negative_target; }), "Missed a changed German dependency argument");
        for (auto& call : new_calls.calls) if (call.target == negative_target) call.arguments[new_calls.corrupt_argument] ^= 4;
        ++rejected_traces;
    }
    if (old_calls.calls != new_calls.calls) {
        for (unsigned i = 0; i < std::min(old_calls.calls.size(), new_calls.calls.size()); ++i) {
            const auto& a = old_calls.calls[i]; const auto& b = new_calls.calls[i];
            if (a.target != b.target || a.continuation != b.continuation || a.arguments != b.arguments)
                throw std::runtime_error(label + " changed dependency arguments/order at call " + std::to_string(i));
            if (a.machine != b.machine) {
                const auto mismatch = std::mismatch(a.machine.begin(), a.machine.end(), b.machine.begin());
                throw std::runtime_error(label + " call " + std::to_string(i) + " target " + hex_address(a.target)
                    + " register byte " + std::to_string(mismatch.first - a.machine.begin()) + " expected="
                    + std::to_string(*mismatch.first) + " actual=" + std::to_string(*mismatch.second));
            }
        }
        throw std::runtime_error(label + " changed dependency count");
    }
    const Image image(*original);
    image.compare(*modern, label);
    Cpu c(*modern);
    if (negative_result) {
        c.set(3, c.r(3) ^ 1);
        bool caught = false;
        try { image.compare(*modern, label); }
        catch (const std::runtime_error& error) { caught = std::string_view(error.what()).starts_with(label + " register byte="); }
        require(caught, "Missed a wrong German result");
        c.set(3, c.r(3) ^ 1);
        ++rejected_results;
    }
    Outcome out{c.r(3), unsigned(modern->executed), unsigned(std::ranges::count_if(new_calls.calls,
        [](const Call& call) { return call.target == insert; })), new_stop, {}, {}, c.u8(save_ok)};
    const auto count_calls = [&](std::uint32_t target) {
        return unsigned(std::ranges::count_if(new_calls.calls, [target](const Call& call) { return call.target == target; }));
    };
    out.promoted = count_calls(0x1049f780);
    out.relegated = count_calls(0x1049f8e0);
    out.relationships = count_calls(0x104a9210);
    for (unsigned i = 0; i < 32; ++i) {
        out.current[i] = c.u32(clubs + i * club_stride + 0x30);
        out.previous[i] = c.u32(clubs + i * club_stride + 0x38);
    }
    return out;
}

void verify() {
    unsigned returned = 0, budgets = 0;
    for (unsigned count : {0u, 1u, 17u, 18u, 20u, 32u}) {
        auto s = league_fixture(count);
        const auto out = compare_routes(*s);
        require(out.stop.empty() && out.result == unsigned(count >= 18) && out.inserted == std::min(count, 18u),
                "German entrant count");
        ++returned;
    }
    for (unsigned layout : {0u, 1u, 2u}) for (unsigned youth : {0u, 2u, 20u}) {
        auto s = league_fixture(32, layout, 2001, youth, true);
        const auto out = compare_routes(*s);
        for (unsigned i = 0; i < 32; ++i) {
            Cpu c(*s);
            const auto original = c.u32(clubs + i * club_stride + 0x30);
            const auto id = original ? c.u32(original) : UINT32_MAX;
            const auto selected = i < (youth > 16 ? 0 : 32 - 2 * youth) && id >= 10 && id <= 13;
            require(out.current[i] == (selected ? definitions + 5 * definition_size : original)
                        && out.previous[i] == (selected ? original : 0x33445566), "German ordered 2001 reassignment");
        }
        ++returned;
    }
    for (unsigned mode_value : {1u, 2u, 3u, 4u}) {
        auto s = league_fixture(32, mode_value == 1 ? 1 : 0, 2001);
        Cpu(*s).put8(mode, mode_value);
        const auto out = compare_routes(*s);
        require(out.stop.empty() && out.result == unsigned(mode_value == 2)
                    && out.save_success == unsigned(mode_value != 1), "German allocation and callback outcome");
        ++returned;
    }
    std::minstd_rand mutations(0x474552);
    for (unsigned case_id = 0; case_id < 8; ++case_id) {
        const auto count = mutations() % 33;
        const auto layout = mutations() % 3;
        const auto year = std::int16_t(mutations());
        const auto youth = mutations() % 4;
        auto s = league_fixture(count, layout, year, youth);
        require(compare_routes(*s).stop.empty(), "German scalar mutation " + std::to_string(case_id));
        ++returned;
    }
    auto s = league_fixture(32, 1, 2001, 2, true);
    const auto out = compare_routes(*s, true, true);
    require(out.stop.empty() && out.result == 1, "German reassigned entrant success");
    ++returned;
    for (unsigned limit = 0; limit < out.instructions; limit += 11) {
        s->budget = limit;
        if (compare_routes(*s).stop == "Native instruction budget exhausted") ++budgets;
    }
    s = league_fixture();
    Cpu(*s).put8(mode, 1);
    const auto failed = compare_routes(*s);
    for (unsigned limit = 0; limit < failed.instructions; limit += 5) {
        s->budget = limit;
        if (compare_routes(*s).stop == "Native instruction budget exhausted") ++budgets;
    }
    s = league_fixture();
    Cpu(*s).set(3, fixture + 0x3000);
    require(compare_routes(*s).stop.starts_with("Unmapped data access at "), "German partial object fault");
    for (const auto format : {UINT32_MAX, 0xffu, 0x1234ffu, 0u, 1u, 0xffffff80u}) {
        auto s = calendar_fixture(calendar, 2001, format);
        const auto out = compare_routes(*s);
        require(out.stop.empty() && out.result == ((format & 0xff) == 0xff ? schedule : 0),
                "German signed-byte calendar format");
        ++returned;
    }
    for (const auto year : {-32768, -1, 0, 2000, 2001, 2002, 32767}) {
        auto s = calendar_fixture(calendar, year);
        Cpu c(*s);
        c.put8(mode, 5);
        c.set(7, 0);
        require(compare_routes(*s).result == schedule, "German live calendar year and fixed table selection");
        ++returned;
    }
    for (const auto mode_value : {0u, 1u, 6u}) {
        auto s = calendar_fixture(calendar, 2000);
        Cpu c(*s);
        c.put8(mode, mode_value);
        c.set(6, headers);
        const auto out = compare_routes(*s, true, mode_value == 0);
        require(out.stop.empty() && out.result == (mode_value == 1 ? 0 : schedule)
                    && out.save_success == unsigned(mode_value != 1), "German calendar allocation and header alias");
        ++returned;
    }
    for (const auto year : {2001, 2002}) {
        auto s = calendar_fixture(configure, year);
        const auto out = compare_routes(*s, true, year == 2001);
        require(out.stop.empty() && out.result == 1, "German configured division and real owned virtual calendar");
        ++returned;
        for (unsigned limit = 0; limit < out.instructions; limit += 31) {
            s->budget = limit;
            if (compare_routes(*s).stop == "Native instruction budget exhausted") ++budgets;
        }
        for (unsigned limit = out.instructions - 18; limit < out.instructions; ++limit) {
            s->budget = limit;
            if (compare_routes(*s).stop == "Native instruction budget exhausted") ++budgets;
        }
    }
    for (const auto target : {external_calendar, later_calendar}) {
        auto s = calendar_fixture(configure);
        Cpu c(*s);
        c.put32(descriptor, target);
        c.put32(descriptor + 4, alternate_toc);
        require(compare_routes(*s).result == 1, "German external virtual dispatch");
        ++returned;
    }
    for (const auto mode_value : {0u, 1u, 10u, 11u, 12u, 13u, 14u, 15u, 17u, 18u}) {
        auto s = lifecycle_fixture(create, mode_value, 0);
        const auto out = compare_routes(*s, mode_value == 0, mode_value == 0);
        require(out.stop.empty() && out.result == object, "German creation and failure return");
        ++returned;
    }
    for (const auto year : {-32768, -1, 0, 2000, 2002, 32767}) {
        auto s = lifecycle_fixture(create, 0, 0, year);
        require(compare_routes(*s).result == object, "German initial signed-short year");
        ++returned;
    }
    {
        auto s = lifecycle_fixture(create, 0, 0);
        Cpu(*s).put32(total, 17);
        require(compare_routes(*s).result == object, "German incomplete entrants parent diagnostic");
        ++returned;
    }
    {
        auto s = lifecycle_fixture(create, 0, 0);
        require(compare_routes(*s, false, false, false, true).result == object,
                "German observer changes fresh child capacity");
        ++returned;
    }
    for (const auto flags : {0u, 1u, 2u, 3u}) {
        auto s = lifecycle_fixture(destroy, 0, flags);
        require(compare_routes(*s, flags == 3, flags == 3).result == object, "German owned-resource destruction");
        ++returned;
    }
    for (const auto deletion : {0u, 0xffffu, 0x8000u, 0x10000u, 0x10001u}) {
        auto s = lifecycle_fixture(destroy, 0, 0);
        Cpu(*s).set(4, deletion);
        require(compare_routes(*s).result == object, "German signed-short deleting destructor");
        ++returned;
    }
    {
        auto s = lifecycle_fixture(destroy);
        Cpu(*s).set(3, 0);
        require(compare_routes(*s).result == 0, "German null destruction");
        ++returned;
    }
    for (const auto entry : {create, destroy}) {
        auto s = lifecycle_fixture(entry, 0, entry == create ? 0 : 3);
        const auto out = compare_routes(*s);
        ++returned;
        for (unsigned limit = 0; limit < out.instructions; limit += 29) {
            s->budget = limit;
            if (compare_routes(*s).stop == "Native instruction budget exhausted") ++budgets;
        }
    }
    for (const auto regional : {false, true}) {
        auto s = relationships_fixture(link_divisions, regional);
        const auto out = compare_routes(*s, true, true);
        require(out.stop.empty() && out.relationships == (regional ? 3u : 1u), "German division relationship count");
        ++returned;
    }
    {
        auto s = relationships_fixture(link_divisions, true, 23);
        require(compare_routes(*s).relationships == 1, "German relationship callback clears live regional division");
        ++returned;
    }
    for (const auto& [count, candidates] : std::array<std::pair<unsigned, unsigned>, 5>{{{0, 0}, {18, 0}, {18, 3}, {18, 4}, {18, 8}}}) {
        auto s = promotion_fixture(count, candidates);
        const auto out = compare_routes(*s, candidates == 4, candidates == 4);
        require(out.stop.empty() && out.promoted == (candidates ? 4u : 0u)
                    && out.relegated == (candidates ? 2u : 0u) && out.save_success == unsigned(candidates != 3),
                "German four-slot promotion and relegation, including retained allocator residue");
        ++returned;
    }
    for (const auto mode_value : {0u, 1u, 21u, 22u}) {
        auto s = promotion_fixture(32, 8, true, mode_value);
        const auto out = compare_routes(*s);
        require(out.stop.empty() && out.promoted == (mode_value == 1 ? 0u : 4u)
                    && out.relegated == (mode_value == 1 ? 0u : mode_value == 22 ? 1u : 2u),
                "German reserve filters, allocation and live callback mutations");
        ++returned;
    }
    for (const auto entry : {link_divisions, promote}) {
        auto s = entry == link_divisions ? relationships_fixture(entry) : promotion_fixture(32, 8, true);
        const auto out = compare_routes(*s);
        ++returned;
        for (unsigned limit = 0; limit < out.instructions; limit += 13) {
            s->budget = limit;
            if (compare_routes(*s).stop == "Native instruction budget exhausted") ++budgets;
        }
    }
    for (const auto entry : {create, destroy, collect, calendar, configure, link_divisions, promote}) {
        RoutineChecker checker([entry](std::uint32_t actual) { return actual == entry; }, 1, 1);
        checker.strict_volatile = true;
        Services services(nullptr, &checker);
        auto s = entry == collect ? league_fixture(32, 1, 2001, 2, true)
                 : entry == calendar || entry == configure ? calendar_fixture(entry)
                 : entry == link_divisions ? relationships_fixture(entry)
                 : entry == promote ? promotion_fixture() : lifecycle_fixture(entry, 0, 0);
        run_until(*s, return_sentinel);
        const auto found = checker.totals().find(entry);
        require(found != checker.totals().end() && found->second.checked == 1 && !found->second.differences,
                "German generated owner selection");
    }
    {
        auto s = lifecycle_fixture(create, 0, 0);
        require(compare_routes(*s, false, false, true).stop.empty(),
                "German original/C++ controlled creation/destruction replay");
    }
    {
        RoutineChecker excluded([](std::uint32_t) { return false; }, 0, 0);
        Services services(nullptr, &excluded);
        auto s = league_fixture();
        require(!try_modern_ger_first(*s, collect), "German excluded hook must decline");
    }
    for (const auto entry : {0u, collect + 4, UINT32_MAX}) {
        auto s = league_fixture();
        bool rejected = false;
        try { ger_first_run_rewrite(*s, entry); }
        catch (const std::logic_error& error) { rejected = std::string_view(error.what()).starts_with("Unknown GER_FIRST.CPP entry "); }
        require(rejected, "German unknown owner entry");
    }
    std::cout << "GER_FIRST " << returned << " returned comparisons (8 scalar mutations; seed 0x474552), "
              << budgets << " budget boundaries, one partial fault, " << rejected_results << " wrong result and "
              << rejected_traces << " wrong trace detected; seven checked production hooks\n";
    std::cout << "Controlled create->destroy replay matched with all seven owned routines enabled\n";
}
}

int main() {
    try {
#ifdef CM_GER_FIRST_PROFILE_DIR
        routine_child_exit = [] {
            const auto path = std::string(CM_GER_FIRST_PROFILE_DIR) + "/check-" + std::to_string(getpid()) + ".profraw";
            __llvm_profile_set_filename(path.c_str());
            if (__llvm_profile_write_file()) throw std::runtime_error("Cannot flush German First Division coverage");
        };
#endif
        verify();
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
