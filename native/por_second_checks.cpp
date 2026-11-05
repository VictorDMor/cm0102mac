#include "por_second_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

#include <array>
#include <random>
#include <string_view>

#ifdef CM_POR_SECOND_PROFILE_DIR
#include <unistd.h>
extern "C" int __llvm_profile_write_file();
extern "C" void __llvm_profile_set_filename(const char*);
#endif

namespace {
using namespace cm::aot;
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto collect = 0x10578680u, allocate = 0x10775130u;
constexpr auto calendar = 0x10577800u, configure = 0x10578840u;
constexpr auto create = 0x105773c0u, destroy = 0x105776f0u, season = 0x10578940u;
constexpr auto insert = 0x104a60e0u, reserve = 0x100ce320u, prize = 0x1049f540u;
constexpr auto object = fixture + 0x100, standings = fixture + 0x300;
constexpr auto strings = fixture + 0x800, buffer = fixture + 0xc00;
constexpr auto save_ok = fixture + 0xd00, mode = save_ok + 4;
constexpr auto division = fixture + 0x20, first_division = division + 4, other_division = division + 8;
constexpr auto club_count = division + 12, clubs_slot = division + 16;
constexpr auto second_definition = fixture + 0x60, first_definition = second_definition + 4;
constexpr auto other_definition = second_definition + 8;
constexpr auto clubs = fixture + 0x10000, partner = fixture + 0x20000;
constexpr auto schedule = fixture + 0x30000, round_count = fixture + 0xe00;
constexpr auto start_round = round_count + 2, optional_flag = round_count + 4;
constexpr auto definition = fixture + 0xe20, country = fixture + 0xf00;
constexpr auto vtable = fixture + 0x1200, descriptor = fixture + 0x1400;
constexpr auto external_calendar = fixture + 0x1800, alternate_toc = toc + 0x20000;
constexpr auto initial_check = 0x104a6850u, external_configure = external_calendar + 8;
constexpr auto finalize = 0x104aa0c0u, child_destroy = external_calendar + 16;
constexpr auto child_array = fixture + 0x1500, child = fixture + 0x1600;
constexpr auto child_table = fixture + 0x1700, child_descriptor = descriptor + 0x40;
constexpr auto stage_object = fixture + 0x40000, global_slot = fixture + 0x1840;
constexpr auto size = 0x44u, club_stride = 0x248u;

struct Call {
    std::uint32_t target, continuation;
    std::array<std::uint32_t, 8> arguments;
    bool operator==(const Call&) const = default;
};
struct Trace {
    std::vector<Call> calls;
    std::uint32_t corrupted_target = 0;
};
Trace* active_trace = nullptr;

struct Diagnostic { std::uint32_t start; unsigned message, detail, line; };
constexpr auto diagnostics = std::to_array<Diagnostic>({
    {0x105786cc, 0x44, 0x3c, 0x16c}, {0x10577864, 0x48, 0x40, 0xa8},
    {0x105774a4, 0xe0, 0x58, 0x43}, {0x1057750c, 0xc0, 0x50, 0x4a},
    {0x10577574, 0xa0, 0x48, 0x51}, {0x105775e0, 0x80, 0x40, 0x5a},
    {0x10577668, 0x60, 0x38, 0x61}, {0x10578a14, 0xb8, 0x50, 0x1e4},
    {0x10578a78, 0x98, 0x48, 0x1eb}, {0x10578adc, 0x78, 0x40, 0x1f1},
    {0x10578b44, 0x58, 0x38, 0x1f8},
});

bool dependency(State& s) {
    Cpu c(s);
    const auto target = s.pc, arg = c.r(3);
    switch (target) {
        case initial_check:
            require(arg == object && c.r(4) == 1, "Portuguese initial virtual check");
            break;
        case external_configure:
        case finalize:
        case 0x1049e4f0:
        case 0x104a00a0:
        case 0x104aa3a0:
            require(arg == object, "Portuguese lifecycle object argument");
            break;
        case 0x1049eb30:
        case 0x104a6660:
        case 0x1049e600:
            require(arg == object && c.r(4) == 0, "Portuguese lifecycle setup/cleanup argument");
            break;
        case 0x10773520:
            require(arg == 0x634, "Portuguese stage allocation size");
            break;
        case 0x104533a0:
            require(arg == stage_object && c.r(4) == object, "Portuguese stage construction arguments");
            break;
        case 0x104536c0:
        case 0x104564a0:
            require(arg == stage_object && c.r(4) == 1, "Portuguese stage lifecycle arguments");
            break;
        case child_destroy:
            require((arg == child || arg == child + 0x40) && c.r(4) == 1 && c.r(2) == alternate_toc,
                    "Portuguese child virtual destruction arguments");
            break;
        case 0x10775170:
            require(arg == schedule || arg == child_array, "Portuguese buffer release");
            break;
        case 0x107735a0:
            require(arg == object, "Portuguese object deletion");
            break;
        case 0x102d46c0:
            require(arg == 0x12345678 && c.r(4) == definition, "Portuguese country season notification");
            break;
        case allocate:
            require((arg == 18 * size && c.lr() == 0x105786c0)
                        || (arg == 0xa18 && c.lr() == 0x1057785c)
                        || (arg == 8 && c.lr() == 0x10577478), "Portuguese allocation arguments");
            break;
        case 0x104a9810:
            require(arg == schedule && c.r(4) < 34 && c.r(5) >= 1 && c.r(5) <= 31 && c.r(6) <= 11
                        && c.r(7) <= 1 && (c.r(8) == 5 || c.r(8) == 6) && c.r(9) == 1
                        && c.r(10) == std::uint32_t(c.i16(object + 0x40)) && c.u32(c.sp() + 0x38) == 0,
                    "Portuguese calendar date arguments");
            break;
        case 0x104a9890:
            require(arg == schedule && c.r(4) < 34 && c.r(5) < 2 && c.r(9) == 0
                        && ((c.r(6) == 6 && c.r(7) == 2 && c.r(8) == 1)
                            || (c.r(6) == UINT32_MAX && c.r(7) == UINT32_MAX && c.r(8) == UINT32_MAX)),
                    "Portuguese calendar part arguments");
            break;
        case external_calendar:
            require(arg == object && c.r(4) == UINT32_MAX && c.r(5) == object + 0xa6
                        && c.r(6) == object + 0x3a && c.r(7) == 0 && c.r(2) == alternate_toc,
                    "Portuguese external virtual calendar override");
            break;
        case insert:
            require(arg == object && c.r(4) < 32 && c.r(5) >= clubs
                        && c.r(5) < clubs + 32 * club_stride && c.r(6) == 0 && c.r(7) == 0,
                    "Portuguese club insertion arguments");
            break;
        case reserve:
            require(arg >= clubs && arg < clubs + 32 * club_stride && c.r(4) == c.sp() + 0x38
                        && c.r(5) == 1 && c.lr() == 0x10578780, "Portuguese reserve lookup arguments");
            break;
        case prize:
            require(arg == object && c.r(4) == 50000 && c.lr() == 0x10578818,
                    "Portuguese competition value arguments");
            break;
        case 0x10739890:
            require(arg == strings && c.r(4) == 0 && c.r(5) == 0
                        && std::ranges::any_of(diagnostics, [&](auto d) {
                            return c.lr() == d.start + 24 && c.r(6) == c.sp() + d.message && c.r(7) == c.sp() + d.detail; }),
                    "Portuguese diagnostic arguments");
            break;
        case 0x1077b720:
            require(arg == buffer && c.r(4) == strings + 0xf && c.r(5) == 0x10203040
                        && std::ranges::any_of(diagnostics, [&](auto d) {
                            return c.lr() == d.start + 56 && c.r(6) == c.sp() + d.message
                                && c.r(7) == c.sp() + d.detail && c.r(8) == d.line; }),
                    "Portuguese diagnostic formatting arguments");
            break;
        case 0x10769b60:
            require(arg == strings + 0x1c && c.r(4) == buffer && c.r(5) == 0, "Portuguese alert arguments");
            break;
        default:
            return false;
    }
    if (active_trace) {
        Call call{target, c.lr(), {c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)}};
        if (active_trace->corrupted_target == target) call.arguments[2] ^= 4;
        active_trace->calls.push_back(call);
    }
    if (target == allocate) c.set(3, c.u8(mode) == 1 ? 0 : c.lr() == 0x1057785c ? schedule
                                  : c.lr() == 0x10577478 ? child_array : standings);
    if (target == initial_check) c.set(3, c.u8(mode) == 10 ? 1 : 0);
    if (target == external_configure) c.set(3, 0);
    if (target == finalize || target == 0x104aa3a0 || target == 0x102d46c0 || target == 0x1049e600
            || target == 0x107735a0 || target == 0x104536c0 || target == 0x104564a0) c.set(3, 0x96);
    if (target == 0x104a00a0) c.set(3, c.u8(mode) == 12 ? 0 : 1);
    if (target == 0x1049eb30) c.set(3, c.u8(mode) == 13 ? 0 : 1);
    if (target == 0x10773520) c.set(3, c.u8(mode) == 14 ? 0 : stage_object);
    if (target == 0x104533a0) {
        c.put32(stage_object, object);
        c.set(3, c.u8(mode) == 15 ? stage_object + 4 : stage_object);
    }
    if (target == child_destroy) {
        c.put32(arg + 4, 0xdeadbeef);
        if (c.u8(mode) == 16) c.put32(object + 0x30, 0);
        c.set(3, 0x97);
    }
    if (target == 0x10775170) { c.put32(arg, 0xfefefefe); c.set(3, 0x98); }
    if (target == 0x104a9810) {
        const auto row = schedule + c.r(4) * 76;
        for (unsigned arg = 5; arg <= 10; ++arg) c.put32(row + (arg - 5) * 4, c.r(arg));
        if (c.u8(mode) == 4 && c.r(4) == 0) c.put16(object + 0x40, std::uint16_t(c.i16(object + 0x40) + 1));
        c.set(3, 0x81);
    }
    if (target == 0x104a9890) {
        const auto row = schedule + c.r(4) * 76 + 24 + c.r(5) * 16;
        for (unsigned arg = 6; arg <= 9; ++arg) c.put32(row + (arg - 6) * 4, c.r(arg));
        c.set(3, 0x82);
    }
    if (target == external_calendar) c.set(3, schedule);
    if (target == insert) {
        const auto row = standings + c.r(4) * size;
        c.put32(row, c.u32(c.r(5)));
        c.put8(row + 0x40, 0);
        if (c.u8(mode) == 2 && c.r(4) == 8) c.put16(object + 0x3e, 9);
        if (c.u8(mode) == 3) c.put32(division, c.u32(division) + 1);
        c.set(3, 0x77);
    }
    if (target == reserve) {
        c.put8(c.r(4), c.u32(arg + 0x10));
        c.set(3, c.u32(arg + 0x14));
    }
    if (target == prize) c.set(3, 0x55667788);
    if (target == 0x10739890) {
        c.put32(c.r(6), strings);
        c.put32(c.r(7), strings + 4);
    }
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
    ~Services() {
        dispatch_probe = previous_probe;
        routine_checker = previous_checker;
        active_trace = previous_trace;
    }
};

std::unique_ptr<State> league_fixture(unsigned count = 18, unsigned reserve_case = 0) {
    auto s = std::make_unique<State>();
    s->registers.fill(0xa5);
    s->unique.fill(0x37);
    s->memory.map(fixture, 0x2000);
    s->memory.map(clubs, 0x6000);
    s->memory.map(partner, 0x1000);
    s->memory.map(stack - 0x500, 0x600);
    s->memory.map(toc - 0x8000, 0x2000);
    s->memory.map(toc - 0x5000, 0x1000);
    s->memory.map(toc - 0x5b20, 4);
    Cpu c(*s);
    c.set({{1, stack}, {2, toc}, {3, object}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7b50, club_count);
    c.put32(toc - 0x7be8, clubs_slot);
    c.put32(toc - 0x67f0, first_division);
    c.put32(toc - 0x67f4, division);
    c.put32(toc - 0x7ba8, buffer);
    c.put32(toc - 0x7bac, save_ok);
    c.put32(toc - 0x44a0, strings);
    c.put32(toc - 0x5b20, 0x10203040);
    c.put32(club_count, count);
    c.put32(clubs_slot, clubs);
    c.put32(division, 10);
    c.put32(first_division, 9);
    c.put32(other_division, 8);
    c.put32(second_definition, 10);
    c.put32(first_definition, 9);
    c.put32(other_definition, 8);
    c.put8(save_ok, 1);
    c.put32(partner + 0x30, reserve_case == 2 ? 0 : reserve_case == 4 ? other_definition : first_definition);
    for (unsigned i = 0; i < 32; ++i) {
        const auto club = clubs + i * club_stride;
        c.put32(club, i);
        c.put32(club + 0x30, second_definition);
        c.put32(club + 0x10, reserve_case == 1 ? 0x180 : reserve_case == 5 ? 0x100 : 0);
        c.put32(club + 0x14, reserve_case == 0 ? 0 : partner);
    }
    for (unsigned i = 0; i < 18 * size; ++i) c.put8(standings + i, 0xa5);
    s->allocator_original = true;
    s->pc = collect;
    s->budget = 20000;
    return s;
}

std::unique_ptr<State> calendar_fixture(std::uint32_t entry, std::uint32_t format = UINT32_MAX) {
    auto s = league_fixture();
    s->memory.map(schedule, 0x2000);
    Cpu c(*s);
    c.set({{4, format}, {5, round_count}, {6, start_round}, {7, optional_flag}});
    c.put32(object, vtable);
    c.put32(vtable + 0x44, descriptor);
    c.put32(descriptor, calendar);
    c.put32(descriptor + 4, toc);
    c.put32(object + 4, definition);
    c.put32(definition + 8, country);
    c.put8(country + 0x120, 4);
    c.put32(toc - 0x6964, other_division);
    c.put16(object + 0x40, 2000);
    c.put16(round_count, 0x6a6a);
    c.put16(start_round, 0x7b7b);
    c.put8(optional_flag, 0xa5);
    s->pc = entry;
    return s;
}

std::unique_ptr<State> lifecycle_fixture(std::uint32_t entry, unsigned mode_value = 0,
                                       unsigned children = 3, unsigned flags = 1) {
    auto s = calendar_fixture(entry);
    s->memory.map(stage_object, 0x1000);
    s->memory.map(toc - 0x5fb8, 4);
    Cpu c(*s);
    c.put32(toc - 0x5fb8, vtable);
    c.put32(toc - 0x7428, global_slot);
    c.put32(global_slot, 0x12345678);
    for (const auto& [slot, offset, target] : std::array<std::array<std::uint32_t, 3>, 4>{{
            {0x94, 8, mode_value == 11 ? external_configure : configure},
            {0x20, 16, initial_check}, {0x64, 24, finalize}, {8, 32, destroy}}}) {
        c.put32(vtable + slot, descriptor + offset);
        c.put32(descriptor + offset, target);
        c.put32(descriptor + offset + 4, toc);
    }
    c.put32(child_table + 8, child_descriptor);
    c.put32(child_descriptor, child_destroy);
    c.put32(child_descriptor + 4, alternate_toc);
    c.put32(child, child_table);
    c.put32(child + 0x40, child_table);
    c.put32(child_array, child);
    c.put32(child_array + 4, 0);
    c.put32(child_array + 8, child + 0x40);
    c.put32(object + 0xc, children ? child_array : 0);
    c.put32(object + 0x30, children ? children - 1 : UINT32_MAX);
    c.put32(object + 8, flags & 1 ? stage_object : 0);
    c.put32(object + 0xbc, flags & 2 ? schedule : 0);
    c.put8(mode, mode_value);
    c.set(4, entry == create ? 2000 : 1);
    c.set(5, definition);
    return s;
}

struct Outcome {
    std::uint32_t result;
    unsigned instructions, inserted;
    std::string stop;
    std::vector<std::uint8_t> flags;
    unsigned save_success;
};
unsigned rejected_results = 0, rejected_traces = 0;

Outcome compare_routes(const State& seed, bool negative_result = false, bool negative_trace = false,
                       bool lifecycle = false, bool observer_capacity = false) {
    auto original = clone(seed), modern = clone(seed);
    Effects expected, actual;
    expected.state = original.get();
    actual.state = modern.get();
    if (observer_capacity) expected.on_read = actual.on_read = [](State& s, Effects& effects, std::uint32_t address, unsigned) {
        if (effects.mutated || address != stack + 0x18 || effects.log.size() < 2) return;
        const auto previous = effects.log[effects.log.size() - 2];
        if (previous.kind == 2 && previous.address == object + 0x2c) {
            effects.mutated = true;
            Cpu(s).put32(object + 0x2c, 2);
        }
    };
    original->memory.date_observer = &expected;
    modern->memory.date_observer = &actual;
    Trace old_calls, new_calls;
    const auto negative_target = seed.pc == collect || seed.pc == create || seed.pc == season ? insert
                                : seed.pc == destroy ? child_destroy : 0x104a9810u;
    new_calls.corrupted_target = negative_trace ? negative_target : 0;
    const auto execute = [lifecycle](State& s, Trace& trace, bool reference) {
        RoutineChecker no_hooks([](std::uint32_t) { return false; }, 0, 0);
        Services services(&trace, reference ? &no_hooks : nullptr);
        s.allocator_original = true;
        try {
            if (reference) por_second_run_original(s, s.pc);
            else run_until(s, return_sentinel);
            if (lifecycle) for (const auto entry : {season, destroy}) {
                Cpu c(s);
                c.set({{3, object}, {4, 1}});
                c.set_lr(return_sentinel);
                s.pc = entry;
                if (reference) por_second_run_original(s, entry);
                else run_until(s, return_sentinel);
            }
            return std::string{};
        } catch (const std::runtime_error& error) { return std::string(error.what()); }
    };
    const auto old_stop = execute(*original, old_calls, true), new_stop = execute(*modern, new_calls, false);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto label = "POR_SECOND " + hex_address(seed.pc) + " limit=" + std::to_string(seed.budget);
    require(old_stop == new_stop, label + " original=" + old_stop + " C++=" + new_stop);
    require(old_stop.empty() || old_stop == "Native instruction budget exhausted"
                || old_stop.starts_with("Unmapped data access at "), label + " unexpected stop " + old_stop);
    if (expected.bytes != actual.bytes) {
        for (unsigned i = 0; i < std::min(expected.log.size(), actual.log.size()); ++i) {
            const auto& a = expected.log[i];
            const auto& b = actual.log[i];
            if (a.kind != b.kind || a.address != b.address || a.size != b.size || a.value != b.value)
                throw std::runtime_error(label + " effect=" + std::to_string(i) + " expected=" + std::to_string(a.kind)
                    + ":" + hex_address(a.address) + ":" + hex_address(a.value) + " actual=" + std::to_string(b.kind)
                    + ":" + hex_address(b.address) + ":" + hex_address(b.value));
        }
        throw std::runtime_error(label + " changed ordered memory effect count");
    }
    require(original->executed == modern->executed, label + " changed budget accounting");
    if (negative_trace) {
        const auto target = negative_target;
        require(old_calls.calls != new_calls.calls && std::ranges::any_of(old_calls.calls,
                    [target](const Call& call) { return call.target == target; }), "Missed a changed dependency argument");
        for (auto& call : new_calls.calls) if (call.target == target) call.arguments[2] ^= 4;
        ++rejected_traces;
    }
    require(old_calls.calls == new_calls.calls, label + " changed dependency arguments or call order");
    const Image image(*original);
    image.compare(*modern, label);
    Cpu c(*modern);
    if (negative_result) {
        c.set(3, c.r(3) ^ 1);
        ++rejected_results;
        bool caught = false;
        try { image.compare(*modern, label); }
        catch (const std::runtime_error& error) { caught = std::string_view(error.what()).starts_with(label + " register byte="); }
        require(caught, "Missed a wrong collection result");
        c.set(3, c.r(3) ^ 1);
    }
    const auto inserted = std::ranges::count_if(new_calls.calls, [](const Call& call) { return call.target == insert; });
    std::vector<std::uint8_t> flags;
    for (unsigned i = 0; i < unsigned(inserted); ++i) flags.push_back(c.u8(standings + i * size + 0x40));
    return {c.r(3), unsigned(modern->executed), unsigned(inserted), new_stop, flags, c.u8(save_ok)};
}

void verify() {
    unsigned returned = 0, budgets = 0;
    for (unsigned clubs : {0u, 1u, 17u, 18u, 20u, 32u}) {
        for (unsigned reserve_case = 0; reserve_case < 6; ++reserve_case) {
            auto s = league_fixture(clubs, reserve_case);
            const auto out = compare_routes(*s);
            require(out.stop.empty() && out.result == unsigned(clubs >= 18)
                        && out.inserted == std::min(clubs, 18u), "Portuguese entrant count");
            const auto flag = reserve_case == 3 || reserve_case == 5 ? 0xfcu : 0u;
            for (auto actual : out.flags) require(actual == flag, "Portuguese first-division reserve flag");
            ++returned;
        }
    }
    for (unsigned mode_value : {1u, 2u, 3u}) {
        auto s = league_fixture(20, 3);
        Cpu(*s).put8(mode, mode_value);
        const auto out = compare_routes(*s);
        require(out.stop.empty() && out.result == unsigned(mode_value == 2)
                    && out.save_success == unsigned(mode_value != 1), "Portuguese allocation/callback outcome");
        ++returned;
    }
    auto s = league_fixture(20, 3);
    Cpu c(*s);
    c.put32(clubs + 0x30, 0);
    c.put32(clubs + club_stride + 0x30, other_definition);
    const auto out = compare_routes(*s, true, true);
    require(out.stop.empty() && out.result == 1 && out.inserted == 18, "Portuguese live membership filters");
    ++returned;
    for (unsigned limit = 0; limit < out.instructions; limit += 7) {
        s->budget = limit;
        const auto stop = compare_routes(*s).stop;
        if (stop == "Native instruction budget exhausted") ++budgets;
        else require(stop.empty(), "Unexpected Portuguese finite-budget failure");
    }
    s = league_fixture(1);
    Cpu(*s).set(3, fixture + 0x3000);
    require(compare_routes(*s).stop.starts_with("Unmapped data access at "), "Portuguese partial object fault");
    for (const auto format : {UINT32_MAX, 0xffu, 0x1234ffu, 0u, 1u, 0xffffff80u}) {
        auto s = calendar_fixture(calendar, format);
        const auto out = compare_routes(*s);
        require(out.stop.empty() && out.result == ((format & 0xff) == 0xff ? schedule : 0),
                "Portuguese signed-byte calendar format");
        ++returned;
    }
    for (const auto year : {-32768, -1, 0, 1999, 32767}) {
        auto s = calendar_fixture(calendar);
        Cpu c(*s);
        c.put16(object + 0x40, year);
        c.put8(mode, 4);
        c.set(7, 0);
        const auto out = compare_routes(*s);
        require(out.stop.empty() && out.result == schedule, "Portuguese live calendar year");
        ++returned;
    }
    for (const auto mode_value : {0u, 1u}) {
        auto s = calendar_fixture(calendar);
        Cpu c(*s);
        c.put8(mode, mode_value);
        c.set(6, round_count); // Original writes the count before the aliased starting round.
        const auto out = compare_routes(*s, true, mode_value == 0);
        require(out.stop.empty() && out.result == (mode_value ? 0 : schedule)
                    && out.save_success == unsigned(!mode_value), "Portuguese calendar allocation and alias");
        ++returned;
        for (unsigned limit = 0; limit < out.instructions; limit += 13) {
            s->budget = limit;
            if (compare_routes(*s).stop == "Native instruction budget exhausted") ++budgets;
        }
    }
    for (const auto country_flags : {0u, 4u, 0xffu}) {
        auto s = calendar_fixture(configure);
        Cpu(*s).put8(country + 0x120, country_flags);
        require(compare_routes(*s, true, country_flags == 4).result == 1,
                "Portuguese configured division and real owned virtual calendar");
        ++returned;
    }
    {
        auto s = calendar_fixture(configure);
        Cpu c(*s);
        c.put32(descriptor, external_calendar);
        c.put32(descriptor + 4, alternate_toc);
        require(compare_routes(*s).result == 1, "Portuguese external virtual dispatch");
        ++returned;
    }
    for (const auto entry : {create, season}) {
        for (const auto mode_value : {0u, 1u, 11u, 12u, 13u, 14u, 15u, 16u}) {
            auto s = lifecycle_fixture(entry, mode_value, 3, 3);
            const auto out = compare_routes(*s, mode_value == 0, mode_value == 0);
            require(out.stop.empty() && (entry != create || out.result == object),
                    "Portuguese lifecycle complete/error return");
            ++returned;
        }
        for (const auto year : {-32768, -1, 0, 32767}) {
            auto s = lifecycle_fixture(entry, 0, 0, 0);
            Cpu c(*s);
            if (entry == create) c.set(4, std::uint32_t(year));
            else c.put16(object + 0x40, year);
            const auto out = compare_routes(*s);
            require(out.stop.empty(), "Portuguese lifecycle year wrap");
            ++returned;
        }
        auto s = lifecycle_fixture(entry, 0, 3, 3);
        const auto out = compare_routes(*s);
        ++returned;
        for (unsigned limit = 0; limit < out.instructions; limit += 37) {
            s->budget = limit;
            if (compare_routes(*s).stop == "Native instruction budget exhausted") ++budgets;
        }
    }
    {
        auto s = lifecycle_fixture(create, 10);
        require(compare_routes(*s).result == object, "Portuguese initial callback short circuit");
        ++returned;
    }
    {
        auto s = lifecycle_fixture(create);
        require(compare_routes(*s, false, false, false, true).result == object,
                "Portuguese observer changes fresh child capacity");
        ++returned;
    }
    for (const auto count : {0u, 1u, 3u}) {
        auto s = lifecycle_fixture(destroy, count == 3 ? 16 : 0, count, count == 1 ? 0 : 3);
        require(compare_routes(*s, true, count == 3).result == object, "Portuguese inclusive child cleanup");
        ++returned;
    }
    for (const auto deletion : {0u, 1u, 0xffffu, 0x8000u, 0x10000u, 0x10001u}) {
        auto s = lifecycle_fixture(destroy, 0, 0, 0);
        Cpu(*s).set(4, deletion);
        require(compare_routes(*s).result == object, "Portuguese signed-short deleting destructor");
        ++returned;
    }
    {
        auto s = lifecycle_fixture(destroy);
        Cpu(*s).set(3, 0);
        require(compare_routes(*s).result == 0, "Portuguese null destruction");
        ++returned;
    }
    for (const auto entry : {create, destroy, collect, calendar, configure, season}) {
        RoutineChecker checker([entry](std::uint32_t actual) { return actual == entry; }, 1, 1);
        checker.strict_volatile = true;
        Services services(nullptr, &checker);
        auto s = entry == collect ? league_fixture(18, 3)
                 : entry == calendar || entry == configure ? calendar_fixture(entry) : lifecycle_fixture(entry);
        run_until(*s, return_sentinel);
        const auto found = checker.totals().find(entry);
        require(found != checker.totals().end() && found->second.checked == 1 && !found->second.differences,
                "Portuguese normal generated owner selection");
    }
    std::minstd_rand mutations(0x504f52);
    for (const auto entry : {create, season, destroy, calendar}) {
        for (unsigned case_id = 0; case_id < 3; ++case_id) {
            auto s = entry == calendar ? calendar_fixture(entry) : lifecycle_fixture(entry, 0, case_id, mutations() & 3);
            Cpu c(*s);
            const auto year = mutations();
            if (entry == create) c.set(4, year);
            else c.put16(object + 0x40, year);
            c.put8(country + 0x120, mutations());
            c.put32(first_division, mutations());
            c.put32(first_definition, c.u32(first_division));
            c.put32(division, mutations());
            c.put32(second_definition, c.u32(division));
            require(compare_routes(*s).stop.empty(), "Portuguese seeded scalar mutation " + std::to_string(case_id));
            ++returned;
        }
    }
    {
        auto s = lifecycle_fixture(create);
        require(compare_routes(*s, false, false, true).stop.empty(),
                "Portuguese original/C++ full lifecycle controlled replay");
    }
    {
        RoutineChecker excluded([](std::uint32_t) { return false; }, 0, 0);
        Services services(nullptr, &excluded);
        auto s = league_fixture();
        require(!try_modern_por_second(*s, collect), "Portuguese excluded hook must decline");
    }
    for (const auto entry : {0u, 0x10578844u, UINT32_MAX}) {
        auto s = league_fixture();
        bool rejected = false;
        try { por_second_run_rewrite(*s, entry); }
        catch (const std::logic_error& error) { rejected = std::string_view(error.what()).starts_with("Unknown POR_SECOND.CPP entry "); }
        require(rejected, "Portuguese unknown owner entry");
    }
    std::cout << "POR_SECOND " << returned << " returned comparisons, " << budgets
              << " budget boundaries, one partial fault, " << rejected_results << " wrong results and "
              << rejected_traces << " wrong traces detected; six checked hooks\n";
    std::cout << "Seed 0x504f52: 12 returned scalar mutations included; full lifecycle replay matched\n";
}
}

int main() {
    try {
#ifdef CM_POR_SECOND_PROFILE_DIR
        routine_child_exit = [] {
            const auto path = std::string(CM_POR_SECOND_PROFILE_DIR) + "/check-" + std::to_string(getpid()) + ".profraw";
            __llvm_profile_set_filename(path.c_str());
            if (__llvm_profile_write_file()) throw std::runtime_error("Cannot flush Portuguese Second Division coverage");
        };
#endif
        verify();
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
