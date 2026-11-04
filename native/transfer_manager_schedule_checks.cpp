#include "transfer_manager_schedule.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <chrono>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;
using namespace std::chrono;

constexpr std::uint32_t schedule_entry = 0x103eb4a0, advance = 0x10717890, distance = 0x10718130,
    append = 0x10738d10, object_allocate = 0x10773520, data_allocate = 0x107735f0;
constexpr std::uint32_t manager = fixture + 0x1000, offer = fixture + 0x2000, globals = fixture + 0x3000,
    pool = fixture + 0x4000, message = fixture + 0x5000, trace = fixture + 0x6000,
    node = fixture + 0x7000, data = fixture + 0x7100, move_vector = fixture + 0x7200;
constexpr std::uint32_t move_import = 0x10bf7bbc;

struct Case {
    int ordinal = 150, year = 2001, days = 12, offset = 0, phase = 0, cursor = 0, past = 0;
    bool initialized = false, allocation_fails = false, existing = false;
};
Case active;
bool original_route = false, production_route = false;

auto today() { return sys_days{year{active.year}/January/1} + days{active.ordinal - 1}; }
int offset_phase() { return std::int8_t(active.phase + active.offset); }
int extra_days() { return std::max(0, offset_phase() / 3); }
int final_phase() { return offset_phase() - 3 * extra_days(); }
int delay() { const auto d = std::int8_t(active.days); return d >= 50 ? 10 : d; }
sys_days scheduled_date() { return today() + days{delay() + extra_days()}; }
sys_days origin_date() { return today() - days{active.initialized ? active.past : 0}; }
int difference() {
    const auto start = origin_date(), end = scheduled_date();
    const auto start_year = year_month_day{start}.year(), end_year = year_month_day{end}.year();
    auto distance = int((end - start).count());
    if (end_year < start_year) {
        // The original backward year loop in 0x10718130 uses year - 1
        // to test for a leap year. Its offset is observable at New Year.
        distance += int((start_year - years{1}).is_leap()) - int((end_year - years{1}).is_leap());
    }
    return distance;
}
int slot_day() {
    auto index = int(std::int8_t(difference() + active.cursor));
    while (index >= 50) index -= 50;
    return index;
}
std::uint32_t destination() { return manager + 0x18 + std::uint32_t(slot_day() * 48 + final_phase() * 16); }

void set_date(Cpu& c, std::uint32_t address, sys_days value) {
    const auto actual_year = year_month_day{value}.year();
    c.put16(address, (value - sys_days{actual_year/January/1}).count() + 1);
    c.put16(address + 2, int(actual_year));
    c.put8(address + 4, actual_year.is_leap());
}

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 56, "Transfer schedule trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 64;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 8; ++r) add(c.r(r));
    for (unsigned r = 25; r <= 30; ++r) add(c.r(r));
    add(c.cr(0));
}

bool service(State& s, unsigned allowed = 5000) {
    if (!production_route || original_route) return false;
    const auto back = Cpu(s).lr();
    s.budget = s.executed + allowed;
    require(dispatch_page(s), "Missing transfer schedule service");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

bool observe(State& s) {
    Cpu c(s);
    const auto entry = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    if (entry == advance && (back == 0x103eb56c || back == 0x103eb5b0)) {
        require((back == 0x103eb56c && a == c.sp() + 0x40 && b == globals && std::int32_t(d) == delay()) ||
                (back == 0x103eb5b0 && a == c.sp() + 0x38 && b == offer + 0x14 && d == 1),
                "Transfer schedule date shift");
        record(c);
        return service(s);
    }
    if (entry == distance) {
        require(back == 0x103eb5e8 && a == offer + 0x14 && b == c.u32(manager + 0x97c) &&
                d == c.u32(manager + 0x980), "Transfer schedule date interval");
        record(c);
        return service(s);
    }
    if (entry == append) {
        require(back == 0x103eb634 && a == destination() && b == offer + 0x28 && d == 4, "Transfer schedule bucket");
        record(c);
        return service(s);
    }
    std::uint32_t answer = 0;
    switch (entry) {
    case object_allocate:
        require((back == 0x10738d4c || back == 0x10738d88) && a == 16, "Transfer schedule list node");
        answer = active.allocation_fails ? 0 : node;
        break;
    case data_allocate:
        require(a == 4 && c.r(28) == node, "Transfer schedule list data");
        answer = data;
        break;
    case 0x10739890:
        require(back == 0x103eb51c && a == pool + 0x37 && !b && !d &&
                c.r(6) == c.sp() + 0x50 && c.r(7) == c.sp() + 0x48, "Transfer schedule diagnostic split");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        if (production_route && !original_route) s.budget = s.executed + 2;
        break;
    case 0x1077b720:
        require(back == 0x103eb53c && a == message && b == pool + 0x6ff &&
                d == globals + 0x14 && c.r(8) == 0x18b8, "Transfer schedule diagnostic format");
        c.put32(message, c.r(8));
        break;
    case 0x10769b60:
        require(back == 0x103eb54c && a == pool + 0x70c && b == message && !d &&
                c.u32(message) == 0x18b8, "Transfer schedule diagnostic dialog");
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xa7000000 + r);
    c.set(3, answer);
    c.set_cr(0, 4);
    c.finish();
    return true;
}

std::unique_ptr<State> make_state() {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0x8000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = schedule_entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, offer},
           {5, std::uint32_t(active.days)}, {6, std::uint32_t(active.offset)}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7b80, globals);
    c.put32(toc - 0x79ec, globals + 0x8);
    c.put32(toc - 0x7bac, globals + 0xc);
    c.put32(toc - 0x5b20, globals + 0x14);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x3fac, globals + 0x80);
    c.put32(toc - 0x3fb4, globals + 0x90);
    c.put32(toc - 0x3fc0, globals + 0xa0);
    c.put32(toc - 0x7e64, move_vector);
    c.put32(move_vector, move_import);
    c.put32(move_vector + 4, toc + 0x4000);
    set_date(c, globals, today());
    c.put8(globals + 8, active.phase);
    c.put8(globals + 0xc, 1);
    c.put8(manager + 0x978, active.cursor);
    c.put8(manager + 0x984, 0x7e);
    c.put8(manager + 0x981, 0x77);
    c.put32(offer + 0x28, 0x12345678);
    if (active.initialized) set_date(c, manager + 0x97c, origin_date());
    if (active.existing) {
        c.put16(destination(), 1);
        c.put32(destination() + 4, node + 0x10);
        c.put32(destination() + 8, node + 0x10);
    }
    return s;
}

void original(State& s, std::uint32_t entry) {
    original_route = true;
    s.pc = entry;
    while (s.pc != return_sentinel) {
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = false;
}

void rewrite(State& s, std::uint32_t) {
    Cpu c(s);
    schedule_transfer_offer(c, c.r(3), c.r(4), c.r(5), c.r(6));
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    production_route = true;
    s.pc = entry;
    s.budget = s.executed;
    run_until(s, return_sentinel);
    production_route = false;
}

unsigned checked = 0;
void check(const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    auto s = make_state();
    auto control = clone(*s);
    original(*control, schedule_entry);
    Cpu c(*control);
    const auto expected = scheduled_date();
    const auto expected_year = year_month_day{expected}.year();
    const auto expected_day = (expected - sys_days{expected_year/January/1}).count() + 1;
    require(c.u16(offer + 0x14) == expected_day && c.u16(offer + 0x16) == int(expected_year) &&
            c.u8(offer + 0x18) == expected_year.is_leap() && c.i8(offer + 0x1c) == final_phase(),
            "Original offer schedule date and phase");
    require(c.u8(manager + 0x984) == (test.initialized ? 0x7e : test.phase) &&
            c.u8(globals + 0xc) == (std::int8_t(test.days) < 50), "Original offer schedule validity");
    const auto list = destination();
    require(c.u16(list) == unsigned(test.existing) + !test.allocation_fails, "Original offer schedule list size");
    if (!test.allocation_fails)
        require(c.u32(list + 8) == node && c.u32(node) == data && c.u32(data) == 0x12345678,
                "Original offer schedule list item");
    const auto differences = compare_routine_call(*s, schedule_entry, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Transfer offer scheduling case " + std::to_string(checked));
    ++checked;
}

void wrong_phase(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put8(offer + 0x1c, 0xff);
}
}

int cm::aot::transfer_manager::checks::schedule(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check({.ordinal = 366, .year = 2000, .days = 0, .offset = 4, .phase = 2}, production);
            check({.days = 50, .cursor = 22, .past = 90, .initialized = true, .existing = true}, production);
            check({.days = -3, .offset = 2, .phase = 2, .cursor = 6, .allocation_fails = true}, production);
            std::cout << checked << " transfer schedule production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown offer schedule check mode");
        check({});
        check({.days = 0, .offset = 1, .phase = 2});
        check({.days = 49, .offset = 10, .phase = 2, .cursor = 48});
        check({.days = 50, .cursor = 22, .past = 90, .initialized = true, .existing = true});
        check({.ordinal = 366, .year = 2000, .days = 0, .offset = 4, .phase = 2});
        check({.ordinal = 2, .year = 2001, .days = -3, .cursor = 10});
        check({.days = -3, .offset = 2, .phase = 2, .cursor = 6, .allocation_fails = true});
        check({.days = 0x100, .offset = 0xff, .cursor = 3});
        check({.days = 0, .cursor = 49});
        check({.days = 0, .cursor = 50});
        check({.days = 0, .cursor = 100, .existing = true});
        active = {};
        auto s = make_state();
        const auto differences = compare_routine_call(*s, schedule_entry, original, wrong_phase,
            RoutineChecker::result_r3, false, true);
        require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 3000201c"); }),
                "The transfer schedule comparison missed an incorrect phase");
        std::cout << checked << " transfer schedule comparisons and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
