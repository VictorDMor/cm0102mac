#include "transfer_manager_future.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t enqueue = 0x103f2ec0, remove_entry = 0x103f3540, comparator = 0x10411b70,
    sort = 0x10768300, next_member = 0x10738e00, forget_staff = 0x10363b80, primary = 0x10082f20;
constexpr std::uint32_t manager = fixture, offer = fixture + 0xb00, people = fixture + 0x1000,
    flags = fixture + 0x2000, rows = fixture + 0x3000, expanded = fixture + 0x3400,
    globals = fixture + 0x4000, date = fixture + 0x4100, shortlist = fixture + 0x4200,
    contract = fixture + 0x4300, contracts = fixture + 0x4400, vector = fixture + 0x4500,
    list = fixture + 0x5000, nodes = fixture + 0x5100, members = fixture + 0x5200,
    trace = fixture + 0x6000, pool = fixture + 0x8000, message = fixture + 0xa000,
    title = fixture + 0xa100, body = fixture + 0xa200;
constexpr auto person(unsigned id) { return people + id * 0x70; }

struct Case {
    unsigned count = 3, capacity = 8, reference = 0x41, extras = 2;
    bool no_list = false, has_array = true, allocation_fails = false, has_contract = true,
         linked_contract = true;
};
Case active;
bool original_route = false, production_route = false;

void original(State&, std::uint32_t);
void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 64, "Future transfer trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 64;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 9; ++r) add(c.r(r));
    add(c.cr(0));
}

unsigned calls(Cpu& c, std::uint32_t target) {
    unsigned count = 0;
    for (unsigned i = 0; i < c.u32(trace); ++i) count += c.u32(trace + 4 + i * 64) == target;
    return count;
}

bool run_service(State& s, std::uint32_t back, unsigned allowance) {
    if (!production_route || original_route) return false;
    s.budget = s.executed + allowance;
    require(dispatch_page(s), "Missing future transfer dependency");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    std::uint32_t answer = 0;
    switch (target) {
    case comparator:
        record(c);
        if (!original_route) return false;
        original(s, target);
        return true;
    case sort:
        require(back == 0x103f300c && a == c.u32(manager + 0xa50) && b == c.u32(manager + 0xa58) &&
                d == 12 && c.r(6) == vector && c.r(7) == active.reference, "Future transfer sort");
        record(c);
        return run_service(s, back, 15000);
    case next_member:
        require(a == list && (back == 0x103f30dc || back == 0x103f35b0), "Future transfer member iterator");
        record(c);
        return run_service(s, back, 8);
    case forget_staff:
        require(a == shortlist && d == 0 &&
                (back == 0x103f3038 ? b == person(1) : back == 0x103f30bc && (b == person(2) || b == person(3))),
                "Future transfer shortlist update");
        c.put8(shortlist + (b - people) / 0x70, 100);
        break;
    case primary:
        require(back == 0x103f306c && a == contracts && b == person(1), "Future transfer contract lookup");
        answer = active.has_contract ? contract : 0;
        break;
    case 0x10775130:
        require(back == 0x103f2f34 && a == (active.capacity + 10) * 12 && !active.has_array,
                "Future transfer allocation");
        answer = active.allocation_fails ? 0 : expanded;
        break;
    case 0x107751b0:
        require(back == 0x103f2f20 && a == rows && b == (active.capacity + 10) * 12 && active.has_array,
                "Future transfer reallocation");
        if (!active.allocation_fails) {
            for (unsigned i = 0; i < active.count * 12; i += 4) c.put32(expanded + i, c.u32(rows + i));
            answer = expanded;
        }
        break;
    case 0x10739890:
        require(back == 0x103f2f5c && a == pool + 0x37 && b == 0 && d == 0 &&
                c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38, "Future transfer diagnostic split");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        break;
    case 0x1077b720:
        require(back == 0x103f2f80 && a == message && b == pool + 0x4c && d == globals + 0x40 &&
                c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38 &&
                c.r(8) == 0x27c2 && c.r(9) == pool + 0x1b5a, "Future transfer diagnostic format");
        c.put32(a, c.r(8));
        break;
    case 0x10727890:
        require((back == 0x103f2f8c && a == title && b == pool + 0xa6) ||
                (back == 0x103f2f9c && a == body && b == pool + 0xbe && d == message),
                "Future transfer diagnostic text");
        c.put32(a, b);
        break;
    case 0x10769b60:
        require(back == 0x103f2fac && a == title && b == body && d == 0 &&
                c.u32(message) == 0x27c2, "Future transfer diagnostic display");
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xa6000000 + r);
    c.set_cr(0, 4);
    c.set(3, answer);
    c.finish();
    return true;
}

std::unique_ptr<State> fixture_state(std::uint32_t entry, const Case& test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0xc000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, offer}, {5, date}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7be0, globals);
    c.put32(toc - 0x7b7c, globals + 4);
    c.put32(toc - 0x5b20, globals + 0x40);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x79dc, title);
    c.put32(toc - 0x79e0, body);
    c.put32(toc - 0x3c00, pool);
    c.put32(toc - 0x4bbc, vector);
    c.put32(globals, people);
    c.put32(globals + 4, contracts);
    c.put32(vector, comparator);
    c.put32(vector + 4, toc);
    c.put32(manager + 0x988, shortlist);
    c.put32(manager + 0x9a0, flags);
    c.put32(manager + 0xa50, test.has_array ? rows : 0);
    c.put32(manager + 0xa54, test.capacity);
    c.put32(manager + 0xa58, test.count);
    c.put32(offer + 4, test.no_list ? 0 : list);
    c.put32(offer + 0xc, 1);
    c.put32(offer + 0x28, test.reference);
    c.put8(contract + 0x54, test.linked_contract ? 0x80 : 0);
    c.put32(contract + 0x58, 0x999);
    c.put16(date, 200);
    c.put16(date + 2, 2001);
    c.put8(date + 4, 1);
    c.put32(list + 4, test.extras ? nodes : 0);
    c.put32(list + 0xc, nodes + 0x80);
    for (unsigned i = 0; i <= test.count; ++i) {
        c.put16(rows + i * 12, 150 + i);
        c.put16(rows + i * 12 + 2, 2002 + i);
        c.put32(rows + i * 12 + 4, 0x00765400 + i);
        c.put32(rows + i * 12 + 8, 0x40 + i);
    }
    for (unsigned id = 1; id <= 3; ++id) {
        c.put32(person(id), id);
        c.put8(flags + id * 10, entry == remove_entry ? 0xf3 : 0xe3);
        if (id == 1) continue;
        const auto i = id - 2;
        c.put32(members + i * 8, id);
        c.put32(nodes + i * 16, members + i * 8);
        c.put32(nodes + i * 16 + 8, i + 1 < test.extras ? nodes + (i + 1) * 16 : 0);
    }
    return s;
}

void original(State& s, std::uint32_t entry) {
    const bool previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    require(transfer_manager_reference(s), "Missing future transfer reference");
    while (s.pc != stop) {
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    if (entry == enqueue) enqueue_future_transfer(c, c.r(3), c.r(4), c.r(5));
    else remove_future_transfer(c, c.r(3), c.r(4));
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
void compare(State& s, std::uint32_t entry, RoutineRunner first, RoutineRunner second) {
    const auto differences = compare_routine_call(s, entry, first, second,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Future transfer case " + std::to_string(checked) + " at " + hex_address(entry));
    ++checked;
}

void check(std::uint32_t entry, const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    auto s = fixture_state(entry, test);
    auto control = clone(*s);
    original(*control, entry);
    Cpu c(*control);
    const auto extras = test.no_list ? 0 : test.extras;
    if (entry == enqueue) {
        const bool grows = test.count == test.capacity;
        const auto storage = grows ? (test.allocation_fails ? 0 : expanded) : rows;
        require(c.u32(manager + 0xa50) == storage &&
                c.u32(manager + 0xa54) == test.capacity + (grows ? 10 : 0), "Original future queue capacity");
        if (storage) {
            require(c.u32(manager + 0xa58) == test.count + 1 && c.u16(storage) == 200 &&
                    c.u16(storage + 2) == 2001 && c.u32(storage + 8) == test.reference,
                    "Original future transfer date order");
            require(c.u8(offer + 0x34) == 0x13 && c.u8(offer + 0x35) == 1 &&
                    calls(c, forget_staff) == 1 + extras, "Original future transfer offer state");
            if (test.count) require(calls(c, comparator) > 0, "Future transfer sort skipped date comparator");
            for (unsigned id = 1; id <= 3; ++id)
                require(c.u8(flags + id * 10) == (id <= 1 + extras ? 0xf3 : 0xe3), "Original future transfer flags");
            require(c.u32(contract + 0x58) == (test.has_contract && test.linked_contract ? test.reference : 0x999),
                    "Original future transfer contract linkage");
        } else {
            require(c.u32(manager + 0xa58) == test.count && calls(c, 0x10769b60) == 1 &&
                    !calls(c, forget_staff), "Original future transfer allocation failure");
        }
    } else {
        const auto index = test.reference - 0x40;
        const bool found = index < test.count;
        require(c.u32(manager + 0xa58) == test.count - found, "Original future transfer removal count");
        if (found && index + 1 < test.count)
            require(c.u32(rows + index * 12 + 8) == test.reference + 1 &&
                    (c.u32(rows + index * 12 + 4) & 0xffffff) == 0x765400 + index,
                    "Original future transfer compaction");
        for (unsigned id = 1; id <= 3; ++id)
            require(c.u8(flags + id * 10) == (id <= 1 + extras ? 0xe3 : 0xf3), "Original removed transfer flags");
    }
    compare(*s, entry, original, selected);
}

void sequence(State& s, RoutineRunner run) {
    Cpu c(s);
    run(s, enqueue);
    c.set({{3, manager}, {4, offer}});
    run(s, remove_entry);
    require(c.u32(manager + 0xa58) == active.count && c.u8(flags + 10) == 0xe3 &&
            c.u8(flags + 30) == 0xe3, "Combined future transfer enqueue and removal");
}
void original_sequence(State& s, std::uint32_t) { sequence(s, original); }
void production_sequence(State& s, std::uint32_t) { sequence(s, production); }

void lose_reference(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put32(rows + 8, 0);
}

void check_negative_control() {
    active = {.reference = 0x50};
    auto s = fixture_state(enqueue, active);
    const auto differences = compare_routine_call(*s, enqueue, original, lose_reference,
        RoutineChecker::result_r3, false, true);
    require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 3000300b"); }),
            "The future transfer comparison missed an altered offer reference");
}
}

int cm::aot::transfer_manager::checks::future(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check(enqueue, {.reference = 0x50}, production);
            check(remove_entry, {}, production);
            active = {.reference = 0x50};
            auto s = fixture_state(enqueue, active);
            compare(*s, enqueue, original_sequence, production_sequence);
            std::cout << checked << " future transfer production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown future transfer check mode");
        check(enqueue, {});
        check(enqueue, {.count = 3, .capacity = 3, .no_list = true});
        check(enqueue, {.count = 0, .capacity = 0, .extras = 0, .has_array = false});
        check(enqueue, {.count = 3, .capacity = 3, .allocation_fails = true});
        check(enqueue, {.count = 0, .capacity = 0, .has_array = false, .allocation_fails = true});
        check(enqueue, {.has_contract = false});
        check(enqueue, {.linked_contract = false});
        check(remove_entry, {});
        check(remove_entry, {.reference = 0x42, .extras = 0});
        check(remove_entry, {.reference = 0x45, .no_list = true});
        check(remove_entry, {.count = 0, .has_array = false});
        constexpr auto seed = 0x54465554u;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 24; ++i) {
            const auto count = unsigned(random() % 6);
            const auto key = unsigned(0x40 + random() % 7), extras = unsigned(random() % 3);
            check(remove_entry, {.count = count, .reference = key, .extras = extras});
        }
        check_negative_control();
        std::cout << checked << " future transfer comparisons (seed " << seed << ") and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
