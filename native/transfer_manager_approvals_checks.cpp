#include "transfer_manager_approval.hpp"
#include "transfer_manager_callbacks.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t append = 0x103ec940, find = 0x103ecb00, find_pair = 0x103ecb80,
    prune = 0x103ecc20, comparator = 0x10411c80, sort_rows = 0x10768300, search_rows = 0x10775600,
    next_pair = 0x10738e00;
constexpr std::uint32_t manager = fixture, people = fixture + 0x1000, status = fixture + 0x2000,
    globals = fixture + 0x3000, rows = fixture + 0x4000, expanded = fixture + 0x4400,
    vector = fixture + 0x4800, nations = fixture + 0x5000, nation = nations + 0x128,
    list = fixture + 0x6000, nodes = fixture + 0x6100, pairs = fixture + 0x6200,
    trace = fixture + 0x7000, pool = fixture + 0x8000, message = fixture + 0xa000,
    title = fixture + 0xa100, body = fixture + 0xa200;
constexpr std::uint32_t person(std::uint32_t id) { return people + id * 0x70; }
constexpr std::uint32_t current_year = 2006;

struct Case {
    std::uint32_t id = 2, destination = 3, start_year = current_year - 4, sort = 0;
    std::uint32_t count = 3, capacity = 8, paired_rows = 3, live_mask = 7, approved_mask = 7;
    bool null_person = false, no_storage = false, allocation_fails = false, no_pairs = false;
};
Case active;
bool use_original = false, production_route = false;

void original(State&, std::uint32_t);

void record(Cpu& c) {
    const auto index = c.u32(trace);
    require(index < 64, "Approval trace exhausted");
    c.put32(trace, index + 1);
    auto at = trace + 4 + index * 64;
    const auto add = [&](std::uint32_t word) { c.put32(at, word); at += 4; };
    add(c.s.pc);
    add(c.lr());
    for (unsigned n = 3; n <= 10; ++n) add(c.r(n));
    add(c.cr(0));
    add(c.s.r<1>(reg::xer_ca));
}

unsigned calls(Cpu& c, std::uint32_t target) {
    unsigned count = 0;
    for (unsigned n = 0; n < c.u32(trace); ++n)
        count += c.u32(trace + 4 + n * 64) == target;
    return count;
}

bool run_external(State& s) {
    if (!production_route || use_original) return false;
    const auto back = Cpu(s).lr();
    s.budget = s.executed + 15000;
    require(dispatch_page(s), "Missing external approval service");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    std::uint32_t result = 0;
    switch (target) {
    case comparator:
        record(c);
        if (use_original) {
            original(s, target);
            return true;
        }
        return false;
    case sort_rows:
        require((back == 0x103ecae0 || back == 0x103ecd40) && a == c.u32(manager + 0xa6c) &&
                b == c.u32(manager + 0xa70) && d == 16 && c.r(6) == vector, "Approval sort arguments");
        record(c);
        return run_external(s);
    case search_rows:
        require(back == 0x103ecb70 && a == c.sp() + 0x38 && c.u32(a) == active.id &&
                b == c.u32(manager + 0xa6c) && d == c.u32(manager + 0xa70) &&
                c.r(6) == 16 && c.r(7) == vector, "Approval search arguments");
        record(c);
        return run_external(s);
    case next_pair:
        require(a == list && back == 0x103ecbf0, "Transfer pair iterator arguments");
        record(c);
        return run_external(s);
    case 0x10775130:
        require(back == 0x103ec9b8 && a == (active.capacity + 30) * 16 && active.no_storage,
                "Approval allocation arguments");
        result = active.allocation_fails ? 0 : expanded;
        break;
    case 0x107751b0:
        require(back == 0x103ec9a4 && a == rows && b == (active.capacity + 30) * 16 && !active.no_storage,
                "Approval reallocation arguments");
        if (!active.allocation_fails) {
            for (unsigned offset = 0; offset < active.count * 16; offset += 4)
                c.put32(expanded + offset, c.u32(rows + offset));
            result = expanded;
        }
        break;
    case 0x10739890:
        require(back == 0x103ec9e0 && a == pool + 0x37 && b == 0 && d == 0 &&
                c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38, "Approval diagnostic split");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        break;
    case 0x1077b720:
        require(back == 0x103eca04 && a == message && b == pool + 0x4c && d == globals + 0x40 &&
                c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38 && c.r(8) == 0x1b1a &&
                c.r(9) == pool + 0x19e6, "Approval diagnostic format");
        c.put32(a, c.r(8));
        break;
    case 0x10727890:
        require((back == 0x103eca10 && a == title && b == pool + 0xa6) ||
                (back == 0x103eca20 && a == body && b == pool + 0xbe && d == message),
                "Approval diagnostic text");
        c.put32(a, b);
        break;
    case 0x10769b60:
        require(back == 0x103eca30 && a == title && b == body && d == 0 && c.u32(message) == 0x1b1a,
                "Approval diagnostic display");
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned n = 4; n <= 10; ++n) c.set(n, 0xa0000000 + n);
    c.set_cr(0, 4);
    c.set(3, result);
    c.finish();
    return true;
}

std::unique_ptr<State> fixture_state(std::uint32_t entry, const Case& test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0xc000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    if (test.null_person) s->memory.map(0, 4);
    s->pc = entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, test.null_person ? 0 : person(test.id)},
           {5, entry == find_pair ? person(test.destination) : nation}, {6, test.start_year}, {8, test.sort}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7be4, globals);
    c.put32(toc - 0x7be0, globals + 4);
    c.put32(toc - 0x7b98, globals + 8);
    c.put32(toc - 0x7b80, globals + 16);
    c.put32(toc - 0x5b20, globals + 0x40);
    c.put32(toc - 0x4ba8, vector);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x79dc, title);
    c.put32(toc - 0x79e0, body);
    c.put32(toc - 0x3c00, pool);
    c.put32(globals, 8);
    c.put32(globals + 4, people);
    c.put32(globals + 8, nations);
    c.put16(globals + 18, current_year);
    c.put32(nation + 0x68, 1);
    c.put32(vector, comparator);
    c.put32(vector + 4, toc);
    c.put32(manager + 0x9a0, status);
    c.put32(manager + 0xa6c, test.no_storage ? 0 : rows);
    c.put32(manager + 0xa70, test.count);
    c.put32(manager + 0xa74, test.capacity);
    c.put32(manager + 0xa78, test.no_pairs ? 0 : list);
    for (std::uint32_t id = 0; id <= 8; ++id) {
        c.put32(person(id), id);
        c.put32(person(id) + 0x14, id < 3 && (test.live_mask & (1u << id)) ? 0x12345678 : 0);
        c.put8(status + id * 10, (id < 3 && (test.approved_mask & (1u << id)) ? 0x20 : 0) | 5);
    }
    if (test.null_person) c.put32(0, test.id);
    for (unsigned i = 0; i < test.count; ++i) {
        c.put32(rows + i * 16, i);
        c.put32(rows + i * 16 + 4, 0x88889999);
        c.put32(rows + i * 16 + 8, (test.start_year << 16) | 0x7777);
        c.put32(rows + i * 16 + 12, 1);
    }
    c.put32(list + 4, test.paired_rows ? nodes : 0);
    c.put32(list + 0xc, nodes + 0x80);
    for (unsigned i = 0; i < test.paired_rows; ++i) {
        c.put32(nodes + i * 16, pairs + i * 8);
        c.put32(nodes + i * 16 + 8, i + 1 == test.paired_rows ? 0 : nodes + (i + 1) * 16);
        c.put32(pairs + i * 8, i ? 2 : 1);
        c.put32(pairs + i * 8 + 4, i + 1);
    }
    return s;
}

void original(State& s, std::uint32_t entry) {
    const bool previous = use_original;
    use_original = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    require(transfer_manager_reference(s), "Missing approval reference entry");
    while (s.pc != stop) {
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    use_original = previous;
}

void rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    const auto a = c.r(3), b = c.r(4), d = c.r(5), e = c.r(6), sort = c.r(8);
    switch (entry) {
    case append: append_regional_approval(c, a, b, d, e, sort); break;
    case find: find_regional_approval(c, a, b); break;
    case find_pair: find_regional_transfer_pair(c, a, b, d); break;
    case prune: prune_regional_approvals(c, a); break;
    default: throw std::runtime_error("Unknown approval entry");
    }
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    production_route = true;
    s.pc = entry;
    s.budget = s.executed;
    run_until(s, return_sentinel);
    require(s.budget == s.executed, "The approval route consumed generated instructions outside external services");
    production_route = false;
}

unsigned checked = 0;
void compare(State& s, std::uint32_t entry, RoutineRunner first, RoutineRunner second) {
    const auto differences = compare_routine_call(s, entry, first, second,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Regional approval case " + std::to_string(checked) + " at " + hex_address(entry));
    ++checked;
}

void check(std::uint32_t entry, const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    auto s = fixture_state(entry, test);
    auto control = clone(*s);
    original(*control, entry);
    Cpu c(*control);
    if (entry == append) {
        const bool grows = test.count == test.capacity;
        const auto storage = grows ? (test.allocation_fails ? 0 : expanded) : rows;
        require(c.u32(manager + 0xa6c) == storage &&
                c.u32(manager + 0xa74) == test.capacity + (grows ? 30 : 0), "Original approval capacity");
        if (storage) {
            require(c.u32(manager + 0xa70) == test.count + 1 && (c.u8(status + test.id * 10) & 0x20),
                    "Original approval append did not publish its status");
            bool found = false;
            for (unsigned i = 0; i <= test.count; ++i) {
                const auto row = storage + i * 16;
                found |= c.u32(row) == test.id && c.u32(row + 4) == 0 &&
                         c.u16(row + 8) == std::uint16_t(test.start_year) && c.u32(row + 12) == 1;
            }
            require(found, "Original approval row contents");
            require(calls(c, sort_rows) == (std::int8_t(test.sort) != 0), "Original approval optional sort");
        } else {
            require(c.u32(manager + 0xa70) == test.count && calls(c, 0x10769b60) == 1,
                    "Original approval allocation failure");
        }
    } else if (entry == find) {
        const bool found = test.id < 3 && (test.approved_mask & (1u << test.id)) && test.id < test.count;
        require(c.r(3) == (found ? rows + test.id * 16 : 0), "Original approval lookup");
        if (found) require(calls(c, comparator) != 0, "Original bsearch comparator was not reached");
    } else if (entry == find_pair) {
        std::uint32_t expected = 0;
        for (unsigned i = 0; i < test.paired_rows && !test.no_pairs; ++i)
            if (test.id == (i ? 2u : 1u) && test.destination == i + 1) expected = pairs + i * 8;
        require(c.r(3) == expected, "Original transfer pair lookup");
    } else {
        const bool aged = test.start_year < current_year - 4;
        unsigned count = 0;
        for (unsigned i = 0; i < test.count; ++i) {
            const bool alive = (test.live_mask & (1u << i)) != 0;
            const auto flag = test.approved_mask & (1u << i) ? 0x25u : 0u;
            if (!aged && alive) require(c.u32(rows + count++ * 16) == i, "Original retained approval order");
            require(c.u32(person(i) + 0x10) == (aged ? nation : 0), "Original approval nationality write");
            require(c.u8(status + i * 10) == (!aged && alive ? flag : flag & ~0x20u), "Original approval status");
        }
        require(c.u32(manager + 0xa70) == count, "Original approval removal count");
    }
    compare(*s, entry, original, selected);
}

void sequence(State& s, RoutineRunner run) {
    Cpu c(s);
    run(s, append);
    c.set({{3, manager}, {4, person(active.id)}});
    run(s, find);
    require(c.r(3) == rows + 16, "Combined approval search did not find the appended row");
    c.put16(globals + 18, current_year + 1);
    c.set(3, manager);
    run(s, prune);
    c.set({{3, manager}, {4, person(active.id)}});
    run(s, find);
    require(!c.r(3) && !c.u32(manager + 0xa70), "Combined approval did not expire the records");
}
void original_sequence(State& s, std::uint32_t) { sequence(s, original); }
void production_sequence(State& s, std::uint32_t) { sequence(s, production); }

void lose_approval_flag(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    const auto address = status + active.id * 10;
    c.put8(address, c.u8(address) & ~0x20u);
}

void alter_search_width_in_trace(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    require(c.u32(trace + 4) == search_rows, "Expected the search as the first approval call");
    c.put32(trace + 4 + 20, 15);
}

void check_negative_controls() {
    active = {.id = 1};
    auto s = fixture_state(append, active);
    const auto flag = compare_routine_call(*s, append, original, lose_approval_flag,
        RoutineChecker::result_r3, false, true);
    require(std::ranges::any_of(flag, [](const auto& line) { return line.contains("byte 3000200a"); }),
            "The approval comparison missed an altered status byte");
    s = fixture_state(find, active);
    const auto width = compare_routine_call(*s, find, original, alter_search_width_in_trace,
        RoutineChecker::result_r3, false, true);
    require(std::ranges::any_of(width, [](const auto& line) { return line.contains("byte 3000701b"); }),
            "The approval comparison missed an altered search width");
}
}

int cm::aot::transfer_manager::checks::approvals(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check(append, {.id = 2, .sort = 1, .count = 1}, production);
            check(find, {}, production);
            check(find_pair, {}, production);
            check(prune, {.live_mask = 2}, production);
            active = {.id = 2, .sort = 1, .count = 1};
            auto s = fixture_state(append, active);
            compare(*s, append, original_sequence, production_sequence);
            std::cout << checked << " regional approval production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown approval check mode");
        check(append, {.sort = 0x100});
        check(append, {.id = 1, .start_year = 0x123407d2, .sort = 1});
        check(append, {.sort = 0xff, .count = 3, .capacity = 3});
        check(append, {.count = 0, .capacity = 0, .no_storage = true});
        check(append, {.count = 0, .capacity = 0, .no_storage = true, .allocation_fails = true});
        check(append, {.count = 3, .capacity = 3, .allocation_fails = true});
        check(find, {});
        check(find, {.id = 8});
        check(find, {.id = 0, .null_person = true});
        check(find, {.id = 0, .approved_mask = 6});
        check(find, {.id = 2, .count = 2});
        check(find_pair, {});
        check(find_pair, {.id = 1, .destination = 1});
        check(find_pair, {.id = 2, .destination = 1});
        check(find_pair, {.paired_rows = 0});
        check(find_pair, {.no_pairs = true});
        check(prune, {.count = 0});
        check(prune, {.live_mask = 0});
        check(prune, {.live_mask = 2});
        check(prune, {.start_year = current_year - 5});
        check(prune, {.start_year = current_year + 1, .approved_mask = 0});
        constexpr auto seed = 0x54415052u;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 30; ++i) {
            const auto year = std::uint32_t(current_year - 6 + random() % 9);
            const auto live = std::uint32_t(random() % 8), flags = std::uint32_t(random() % 8);
            check(prune, {.start_year = year, .live_mask = live, .approved_mask = flags});
        }
        check_negative_controls();
        std::cout << checked << " regional approval comparisons (seed " << seed << ") and two negative controls\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
