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
constexpr auto size = 0x44u, club_stride = 0x248u, definition_size = 0x6cu;

struct Call {
    std::uint32_t target, continuation;
    std::array<std::uint32_t, 8> arguments;
    bool operator==(const Call&) const = default;
};
struct Trace { std::vector<Call> calls; std::uint32_t corrupted_target = 0; };
Trace* active_trace = nullptr;

bool dependency(State& s) {
    Cpu c(s);
    const auto target = s.pc, arg = c.r(3);
    switch (target) {
        case allocate:
            require((arg == 18 * size && c.lr() == 0x1053967c)
                        || (arg == 0xa18 && c.lr() == 0x10537174), "German allocation arguments");
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
            require(arg == object && c.r(4) == UINT32_MAX && c.r(5) == object + 0xa6
                        && c.r(6) == object + 0x3a && c.r(7) == 0 && c.r(2) == alternate_toc,
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
                        && ((c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38 && c.lr() == 0x105396a0)
                            || (c.r(6) == c.sp() + 0x70 && c.r(7) == c.sp() + 0x48 && c.lr() == 0x10537194)),
                    "German diagnostic arguments");
            break;
        case 0x1077b720:
            require(arg == buffer && c.r(4) == strings + 0xe && c.r(5) == 0x10203040
                        && ((c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38 && c.r(8) == 0x26e && c.lr() == 0x105396c0)
                            || (c.r(6) == c.sp() + 0x70 && c.r(7) == c.sp() + 0x48 && c.r(8) == 0xa1 && c.lr() == 0x105371b4)),
                    "German diagnostic formatting arguments");
            break;
        case 0x10769b60:
            require(arg == strings + 0x1b && c.r(4) == buffer && c.r(5) == 0, "German allocation alert arguments");
            break;
        default:
            return false;
    }
    if (active_trace) {
        Call call{target, c.lr(), {c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)}};
        if (active_trace->corrupted_target == target) call.arguments[2] ^= 4;
        active_trace->calls.push_back(call);
    }
    if (target == allocate) {
        if (c.u8(mode) == 6) c.put16(object + 0x40, c.i16(object + 0x40) + 1);
        c.set(3, c.u8(mode) == 1 ? 0 : c.lr() == 0x10537174 ? schedule : standings);
    }
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
    if (target == external_calendar || target == later_calendar) c.set(3, schedule);
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

struct Outcome {
    std::uint32_t result;
    unsigned instructions, inserted;
    std::string stop;
    std::array<std::uint32_t, 32> current, previous;
    unsigned save_success;
};
unsigned rejected_results = 0, rejected_traces = 0;

Outcome compare_routes(const State& seed, bool negative_result = false, bool negative_trace = false) {
    auto original = clone(seed), modern = clone(seed);
    Effects expected, actual;
    expected.state = original.get(); actual.state = modern.get();
    original->memory.date_observer = &expected; modern->memory.date_observer = &actual;
    Trace old_calls, new_calls;
    const auto negative_target = seed.pc == collect ? insert : 0x104a9810u;
    new_calls.corrupted_target = negative_trace ? negative_target : 0;
    const auto execute = [](State& s, Trace& trace, bool reference) {
        RoutineChecker no_hooks([](std::uint32_t) { return false; }, 0, 0);
        Services services(&trace, reference ? &no_hooks : nullptr);
        s.allocator_original = true;
        try {
            if (reference) ger_first_run_original(s, s.pc);
            else run_until(s, return_sentinel);
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
        throw std::runtime_error(label + " changed ordered memory effect count");
    }
    require(original->executed == modern->executed, label + " changed budget accounting");
    if (negative_trace) {
        require(old_calls.calls != new_calls.calls && std::ranges::any_of(old_calls.calls,
                    [negative_target](const Call& call) { return call.target == negative_target; }), "Missed a changed German dependency argument");
        for (auto& call : new_calls.calls) if (call.target == negative_target) call.arguments[2] ^= 4;
        ++rejected_traces;
    }
    require(old_calls.calls == new_calls.calls, label + " changed dependency arguments or call order");
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
        auto s = league_fixture(mutations() % 33, mutations() % 3, std::int16_t(mutations()), mutations() % 4);
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
    for (const auto entry : {collect, calendar, configure}) {
        RoutineChecker checker([entry](std::uint32_t actual) { return actual == entry; }, 1, 1);
        checker.strict_volatile = true;
        Services services(nullptr, &checker);
        auto s = entry == collect ? league_fixture(32, 1, 2001, 2, true) : calendar_fixture(entry);
        run_until(*s, return_sentinel);
        const auto found = checker.totals().find(entry);
        require(found != checker.totals().end() && found->second.checked == 1 && !found->second.differences,
                "German generated owner selection");
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
              << rejected_traces << " wrong trace detected; three checked production hooks\n";
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
