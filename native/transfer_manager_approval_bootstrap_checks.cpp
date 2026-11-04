#include "transfer_manager_approval_bootstrap.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t initialize = 0x103ec6d0, add_approval = 0x103ec940,
    regional = 0x103ecef0, compare_id = 0x10411c80, initialize_date = 0x10715880,
    history_table = 0x10305c60, free_history = 0x10775170, shift_date = 0x10717a10, sort = 0x10768300,
    primary_lookup = 0x10082f20;
constexpr std::uint32_t manager = fixture, people = fixture + 0x1000,
    clubs = fixture + 0x2000, nation = fixture + 0x3000, other_nation = fixture + 0x3200,
    globals = fixture + 0x4000, approvals = fixture + 0x5000, flags = fixture + 0x5100,
    vector = fixture + 0x5400, history = fixture + 0x6000, trace = fixture + 0x7100;
constexpr auto staff(std::uint32_t id) { return people + id * 0x70; }
constexpr auto club(std::uint32_t id) { return clubs + id * 0x248; }
constexpr auto frame = stack - 0x90;

struct Case {
    unsigned people_count = 4, history_count = 0, joined_year = 2004, start_year = 2004;
    int joined_day = 120, start_day = 120;
    std::array<unsigned, 4> roles{5, 6, 13, 14};
    std::array<int, 4> history_clubs{-1, 9, 2, 3};
    std::array<unsigned, 4> history_years{2002, 2002, 2003, 1999};
    bool allocated_history = false, other_restricted = false, no_restriction = false, exempt = false,
         direct_append = false;
};
Case active;
bool original_route = false, production_route = false;

void original(State&, std::uint32_t);
bool owned(std::uint32_t entry) { return entry == regional || entry == add_approval || entry == compare_id; }

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 48, "Staff approval trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 64;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 8; ++r) add(c.r(r));
    for (unsigned r = 21; r <= 25; ++r) add(c.r(r));
    add(c.cr(0));
    add(c.s.r<1>(reg::xer_ca));
}

unsigned calls(Cpu& c, std::uint32_t target) {
    unsigned count = 0;
    for (unsigned i = 0; i < c.u32(trace); ++i) count += c.u32(trace + 4 + i * 64) == target;
    return count;
}

bool external(State& s, std::uint32_t back, unsigned allowed) {
    if (!production_route || original_route) return false;
    s.budget = s.executed + allowed;
    require(dispatch_page(s), "Missing real staff approval service");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    if (owned(target)) {
        if (target == regional)
            require(back == 0x103ec778 && a == manager && (b == staff(1) || b == staff(3)) &&
                    d == club(1) && c.r(6) == 0, "Staff regional check");
        else if (target == add_approval)
            require(back == 0x103ec8fc && a == manager && (b == staff(1) || b == staff(3)) &&
                    d == nation && c.r(7) == 1 && c.r(8) == 0, "Staff approval append");
        record(c);
        if (!original_route) return false;
        original(s, target);
        return true;
    }
    std::uint32_t answer = 0;
    switch (target) {
    case initialize_date:
        require(back == 0x103ec704 && a == frame + 0x4c, "Staff default date");
        record(c);
        return external(s, back, 6);
    case shift_date:
        require(back == 0x103ec868 && a == frame + 0x44 && b == frame + 0x4c && d == 52, "Staff shifted date");
        record(c);
        return external(s, back, 5000);
    case sort:
        require(back == 0x103ec928 && a == approvals && b == c.u32(manager + 0xa70) && d == 16 && c.r(6) == vector,
                "Staff approval sort");
        record(c);
        return external(s, back, 15000);
    case 0x100ca6a0:
        require(back == 0x103ecfac && (a == staff(1) || a == staff(3)) && b == 0, "Staff national exemption");
        answer = active.exempt;
        break;
    case primary_lookup:
        require(back == 0x103ecfd4 && a == globals + 0x180 && (b == staff(1) || b == staff(3)),
                "Staff primary contract");
        break;
    case history_table:
        require(back == 0x103ec794 && a == globals + 0x80 && (b == staff(1) || b == staff(3)) &&
                d == frame + 0x38, "Staff career query");
        c.put32(d, active.history_count);
        answer = active.allocated_history ? history : 0;
        break;
    case free_history:
        require(back == 0x103ec818 && a == history, "Staff career release");
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xa5000000 + r);
    c.set_cr(0, 4);
    c.set(3, answer);
    c.finish();
    if (target == history_table && active.direct_append) {
        // The external career query provides the controlled input. From
        // this return onward both paths run without a dispatcher probe.
        dispatch_probe = nullptr;
        s.budget = 100000;
    }
    return true;
}

std::unique_ptr<State> fixture_state(const Case& test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0xa000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = initialize;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7a9c, globals);
    c.put32(toc - 0x7b80, globals + 8);
    c.put32(toc - 0x7b3c, globals + 16);
    c.put32(toc - 0x7be8, globals + 20);
    c.put32(toc - 0x7b50, globals + 24);
    c.put32(toc - 0x7be4, globals + 28);
    c.put32(toc - 0x7be0, globals + 32);
    c.put32(toc - 0x7b7c, globals + 36);
    c.put32(toc - 0x4ba8, vector);
    c.put32(toc - 0x3c00, globals + 0x200);
    c.put32(toc - 0x3fb0, globals + 0x300);
    c.put16(globals, test.start_day);
    c.put16(globals + 2, test.start_year);
    c.put16(globals + 10, 2006);
    c.put32(globals + 16, globals + 0x80);
    c.put32(globals + 20, clubs);
    c.put32(globals + 24, 4);
    c.put32(globals + 28, test.people_count);
    c.put32(globals + 32, people);
    c.put32(globals + 36, globals + 0x180);
    c.put32(vector, compare_id);
    c.put32(vector + 4, toc);
    c.put32(manager + 0x9a0, flags);
    c.put32(manager + 0xa6c, approvals);
    c.put32(manager + 0xa74, 8);
    c.put8(nation + 0x118, test.no_restriction ? 13 : 14);
    c.put32(nation + 0x68, 1);
    c.put32(club(1) + 0x2c, nation);
    c.put32(club(2) + 0x2c, nation);
    c.put32(club(3) + 0x2c, other_nation);
    for (unsigned id = 0; id < 4; ++id) {
        c.put32(staff(id), id);
        c.put8(staff(id) + 0x5e, test.roles[id]);
        c.put8(staff(id) + 0x62, id == 1 || (id == 3 && test.other_restricted) ? 11 : 2);
        c.put32(staff(id) + 0x24, id ? club(1) : 0);
        c.put16(staff(id) + 0x4c, test.joined_day);
        c.put16(staff(id) + 0x4e, test.joined_year);
        c.put8(staff(id) + 0x50, 1);
        c.put16(flags + id * 10 + 4, 0x3030);
        const auto group = history + id * 0x120;
        c.put16(group, test.history_years[id]);
        c.put32(group + 4, test.history_clubs[id]);
    }
    return s;
}

void original(State& s, std::uint32_t entry) {
    const bool previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    require(transfer_manager_reference(s), "Missing staff approval reference entry");
    while (s.pc != stop) {
        if (owned(s.pc) && !dispatch_probe) original(s, s.pc);
        else if (owned(s.pc) || !transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void rewrite(State& s, std::uint32_t) {
    Cpu c(s);
    initialize_staff_approvals(c, c.r(3));
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    production_route = true;
    s.pc = entry;
    if (dispatch_probe) s.budget = s.executed + 6; // The default date can enter the leaf table.
    run_until(s, return_sentinel);
    production_route = false;
}

unsigned checked = 0;
void check(const Case& test, unsigned count, unsigned first_year, unsigned shifts, RoutineRunner selected = rewrite) {
    active = test;
    auto s = fixture_state(test);
    auto control = clone(*s);
    const auto probe = dispatch_probe;
    original(*control, initialize);
    dispatch_probe = probe;
    Cpu c(*control);
    require(c.u32(manager + 0xa70) == count, "Original staff approval count");
    for (unsigned i = 0; i < count; ++i) {
        const auto row = approvals + i * 16;
        require(c.u32(row) == 1 + i * 2 && c.u16(row + 8) == first_year && c.u32(row + 0xc) == 1,
                "Original staff approval row");
    }
    for (unsigned i = 0; i < 4; ++i) {
        const auto roles = test.roles[i];
        const auto mask = i >= test.people_count ? 0x3030 : roles > 13 ? 0x3fff : roles > 5 ? 0x1fff : 0x3030;
        require(c.u16(flags + i * 10 + 4) == mask, "Original staff role mask");
    }
    const auto expected_nation = test.people_count > 1 && !test.no_restriction && !test.exempt && count == 0
        ? nation : 0;
    require(c.u32(staff(1) + 0x10) == expected_nation, "Original long-residency nationality");
    if (dispatch_probe && !test.direct_append) {
        require(calls(c, shift_date) == shifts, "Original staff date shift count");
        require(calls(c, sort) == 1, "Original staff approval sort count");
        if (count > 1) require(calls(c, compare_id) > 0, "Original staff approval sort did not compare rows");
    }
    const auto differences = compare_routine_call(*s, initialize, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Staff approval case " + std::to_string(checked));
    ++checked;
}
}

int cm::aot::transfer_manager::checks::approval_bootstrap(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check({.history_count = 24, .allocated_history = true, .other_restricted = true}, 2, 2003, 0, production);
            check({.joined_year = 2002}, 1, 2002, 3, production);
            check({.direct_append = true}, 1, 2004, 0, production);
            dispatch_probe = nullptr;
            check({.no_restriction = true}, 0, 0, 0, production);
            std::cout << checked << " staff approval bootstrap production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown staff approval bootstrap check mode");
        check({.people_count = 0}, 0, 0, 0);
        check({.people_count = 1}, 0, 0, 0);
        check({.no_restriction = true}, 0, 0, 0);
        check({.exempt = true}, 0, 0, 0);
        check({}, 1, 2004, 0);
        check({.joined_day = 119}, 1, 2004, 1);
        check({.joined_day = 121}, 1, 2004, 0);
        check({.joined_year = 2002}, 1, 2002, 3);
        check({.joined_year = 2001}, 0, 0, 0);
        check({.joined_year = 2005}, 1, 2005, 0);
        check({.history_count = 24, .allocated_history = true, .other_restricted = true}, 2, 2003, 0);
        check({.history_count = 13, .history_years = {2002, 2002, 2005, 1999}, .allocated_history = true}, 1, 2004, 0);
        check({.history_count = 13, .history_years = {2002, 2002, 2000, 1999}, .allocated_history = true}, 0, 0, 0);
        constexpr auto seed = 0x54424f4fu;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 24; ++i) {
            Case test{.no_restriction = true};
            for (auto& band : test.roles) band = random() % 21;
            test.people_count = random() % 5;
            check(test, 0, 0, 0);
        }
        std::cout << checked << " staff approval bootstrap comparisons (seed " << seed << ")\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
