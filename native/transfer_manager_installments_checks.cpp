#include "transfer_manager_installments.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t register_terms = 0x103f1b30, settle = 0x103f2250,
    append_sell_on = 0x103f1e60, append_first_team = 0x103f1fb0, append_international = 0x103f2100,
    compare_sell_on = 0x104119e0, compare_appearance = 0x10411a10, qsort = 0x10768300,
    get_contract = 0x10082f20, get_finance = 0x10103510, debit = 0x100f70e0, credit = 0x100f7050,
    totals = 0x100132f0, append_list = 0x10738d10, next_list = 0x10738e00, remove_list = 0x10738e60,
    calloc = 0x10775200, free = 0x10775170;
constexpr std::uint32_t manager = fixture, list = manager + 0x9a4, offer = fixture + 0x1000,
    staff = fixture + 0x1200, clubs = fixture + 0x2000, globals = fixture + 0x3000,
    contract = fixture + 0x3200, accounts = fixture + 0x3400, flags = fixture + 0x3800,
    seller_flags = fixture + 0x3900, buyer_flags = fixture + 0x3a00, records = fixture + 0x4000,
    nodes = fixture + 0x4400, fees = fixture + 0x4800, vectors = fixture + 0x4c00,
    constants = fixture + 0x5000, trace = fixture + 0x6000;
constexpr auto club(unsigned id) { return clubs + id * 0x248; }
constexpr auto account(unsigned id) { return accounts + id * 0x40; }

struct Case {
    std::int32_t amount = 315, first = 0, international = 0;
    int months = 0, percent = 0, contract_kind = 1;
    unsigned club_count = 5, old_rows = 0, missing_reserve_account = 0, allocation_mask = 3, contract_low_bits = 0;
    bool has_contract = true, inherited_seller_account = false, negative_reserve = false,
         direct_fee_calls = false;
};
Case active;
bool original_route = false, production_route = false;

void original(State&, std::uint32_t);
bool owned(std::uint32_t entry) {
    return entry == append_sell_on || entry == append_first_team || entry == append_international ||
           entry == compare_sell_on || entry == compare_appearance;
}

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 128, "Installment trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 96;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 8; ++r) add(c.r(r));
    for (unsigned r = 22; r <= 31; ++r) add(c.r(r));
    add(c.cr(0));
    add(c.s.r<1>(reg::xer_ca));
}

unsigned calls(Cpu& c, std::uint32_t target) {
    unsigned count = 0;
    for (unsigned i = 0; i < c.u32(trace); ++i) count += c.u32(trace + 4 + i * 96) == target;
    return count;
}

bool real_service(State& s, std::uint32_t back, unsigned budget) {
    if (!production_route || original_route) return false;
    s.budget = s.executed + budget;
    require(dispatch_page(s), "Missing installment service");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

void append_fixture(Cpu& c, std::uint32_t source) {
    const auto count = c.u16(list);
    const auto node = nodes + count * 16, row = records + count * 16, tail = c.u32(list + 8);
    for (unsigned offset = 0; offset < 16; offset += 4)
        c.put32(row + offset, c.u32(source + offset));
    c.put32(node, row);
    c.put32(node + 8, 0);
    if (tail) c.put32(tail + 8, node);
    else c.put32(list + 4, node);
    c.put32(list + 8, node);
    c.put16(list, count + 1);
}

void remove_fixture(Cpu& c, std::uint32_t row) {
    std::uint32_t previous = 0;
    for (auto node = c.u32(list + 4); node; node = c.u32(node + 8)) {
        if (c.u32(node) == row) {
            const auto next = c.u32(node + 8);
            c.put32(previous ? previous + 8 : list + 4, next);
            if (c.u32(list + 8) == node) c.put32(list + 8, previous);
            c.put16(list, c.u16(list) - 1);
            return;
        }
        previous = node;
    }
    throw std::runtime_error("Missing paid installment");
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    if (owned(target)) {
        if (target == append_sell_on || target == append_first_team || target == append_international) {
            const auto offset = target == append_sell_on ? 0x58 : target == append_first_team ? 0x48 : 0x38;
            require(a == manager && b == c.sp() + offset, "Installment fee record pointer");
        }
        record(c);
        if (!original_route) return false;
        original(s, target);
        return true;
    }
    std::uint32_t answer = 0;
    switch (target) {
    case get_contract:
        require(back == 0x103f1b8c && a == globals + 0x100 && b == staff, "Installment primary contract");
        answer = active.has_contract ? contract : 0;
        break;
    case get_finance:
        require(a == globals + 0x200 && b >= clubs && b < club(5) && (b - clubs) % 0x248 == 0,
                "Installment finance lookup");
        if (back == 0x103f1cac && (active.missing_reserve_account & 1)) break;
        if (back == 0x103f1cbc && (active.missing_reserve_account & 2)) break;
        if (b != club(4)) answer = account((b - clubs) / 0x248);
        break;
    case totals:
        require(back == 0x103f1c44 && a == globals + 0x300 && b == std::uint32_t(active.amount) && d == club(1) &&
                c.r(6) == (active.has_contract && active.contract_kind != 5 ? club(2) : 0), "Transfer totals");
        c.put32(a, b);
        break;
    case debit:
    case credit:
        require(a >= accounts && a < account(4) && (a - accounts) % 0x40 == 0 &&
                b == (std::int32_t(d) < 0 ? 0xffffffffu : 0), "Signed installment account adjustment");
        c.put32(a + 4, c.u32(a + 4) + (target == debit ? 0u - d : d));
        break;
    case calloc:
        require((back == 0x103f2280 || back == 0x103f2294) && a == active.club_count && b == 1,
                "Installment club flags allocation");
        if (back == 0x103f2280 && (active.allocation_mask & 1)) answer = seller_flags;
        if (back == 0x103f2294 && (active.allocation_mask & 2)) answer = buyer_flags;
        break;
    case free:
        require((back == 0x103f2538 && a == seller_flags) || (back == 0x103f2548 && a == buyer_flags),
                "Installment club flags release");
        break;
    case append_list:
        require(back == 0x103f1d60 && a == list && b == c.sp() + 0x68 && d == 16, "Installment list append");
        append_fixture(c, b);
        break;
    case remove_list:
        require(back == 0x103f242c && a == list && b >= records && b < records + 0x100,
                "Installment list removal");
        remove_fixture(c, b);
        break;
    case next_list:
        require(back == 0x103f2464 && a == list, "Installment list iteration");
        record(c);
        return real_service(s, back, 8);
    case qsort:
        require(a >= fees && a < fees + 0x300 && d == 16 &&
                (c.r(6) == vectors || c.r(6) == vectors + 8), "Installment fee sort");
        record(c);
        return real_service(s, back, 15000);
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xb2000000 + r);
    c.set_cr(0, 4);
    c.set(3, answer);
    c.finish();
    if (target == credit && active.direct_fee_calls) {
        dispatch_probe = nullptr;
        s.budget = 100000;
    }
    return true;
}

std::unique_ptr<State> fixture_state(std::uint32_t entry, const Case& test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0xa000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, offer}, {28, test.inherited_seller_account ? account(2) : 0}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7be8, globals);
    c.put32(toc - 0x7be0, globals + 4);
    c.put32(toc - 0x7b7c, globals + 8);
    c.put32(toc - 0x7b78, globals + 12);
    c.put32(toc - 0x7ab8, globals + 16);
    c.put32(toc - 0x7afc, globals + 20);
    c.put32(toc - 0x7b50, globals + 24);
    c.put32(toc - 0x4ba0, constants);
    c.put32(toc - 0x4bb0, vectors);
    c.put32(toc - 0x4bb4, vectors + 8);
    c.put32(toc - 0x3c00, globals + 0x400);
    c.put32(globals, clubs);
    c.put32(globals + 4, staff - 0x70);
    c.put32(globals + 8, globals + 0x100);
    c.put32(globals + 16, globals + 0x200);
    c.put32(globals + 20, globals + 0x300);
    c.put32(globals + 24, test.club_count);
    c.put32(staff, 1);
    c.put32(contract + 4, 2);
    c.put8(contract + 0x38, std::uint32_t(test.contract_kind * 4) | test.contract_low_bits);
    c.put32(manager + 0x9a0, flags);
    c.put32(offer + 8, 1);
    c.put32(offer + 0xc, 1);
    c.put32(offer + 0x20, std::uint32_t(test.amount));
    c.put32(offer + 0x40, std::uint32_t(test.first));
    c.put8(offer + 0x44, 5);
    c.put32(offer + 0x48, std::uint32_t(test.international));
    c.put8(offer + 0x4c, 9);
    c.put8(offer + 0x50, std::uint32_t(test.percent));
    c.put8(offer + 0x51, std::uint32_t(test.months));
    c.put_f64(constants + 8, 0x1.000008p52);
    c.put_f64(constants + 0x130, 4.2);
    c.put_f64(constants + 0x138, 0.2);
    c.put_f64(constants + 0xb0, 0.1);
    c.put32(vectors, compare_sell_on);
    c.put32(vectors + 4, toc);
    c.put32(vectors + 8, compare_appearance);
    c.put32(vectors + 12, toc);
    for (unsigned id = 0; id < 5; ++id) {
        c.put32(club(id), id);
        c.put32(account(id) + 0x1c, test.negative_reserve ? 0u - 10 - id : 10 + id);
    }
    for (unsigned table = 0; table < 3; ++table) {
        const auto registry = manager + 0x9b4 + table * 12;
        c.put32(registry, fees + table * 0x100);
        c.put32(registry + 4, 8);
    }
    for (unsigned i = 0; i < test.old_rows; ++i) {
        constexpr std::array months{1, 4, 5, 8, 9, 0xff};
        const auto scratch = records + 0x100;
        c.put32(scratch, std::uint32_t(test.amount + std::int32_t(i)));
        c.put32(scratch + 4, i == 4 ? 3 : 1);
        c.put32(scratch + 8, i == 5 ? 3 : 2);
        c.put8(scratch + 12, months[i]);
        append_fixture(c, scratch);
    }
    return s;
}

void original(State& s, std::uint32_t entry) {
    const bool previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    require(transfer_manager_reference(s), "Missing installment reference");
    while (s.pc != stop) {
        if (owned(s.pc) && !dispatch_probe) original(s, s.pc);
        else if (owned(s.pc) || !transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    if (entry == register_terms) register_transfer_consideration(c, c.r(3), c.r(4));
    else settle_transfer_installments(c, c.r(3));
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
    require(differences.empty(), "Installment case " + std::to_string(checked) + " at " + hex_address(entry));
    ++checked;
}

void check(std::uint32_t entry, const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    auto s = fixture_state(entry, test);
    auto control = clone(*s);
    const auto probe = dispatch_probe;
    original(*control, entry);
    dispatch_probe = probe;
    Cpu c(*control);
    if (entry == register_terms) {
        const auto payment = std::uint32_t(test.months > 0 ? test.amount / test.months : test.amount);
        require(c.u32(account(1) + 4) == 0u - payment && c.u32(globals + 0x300) == std::uint32_t(test.amount),
                "Original up-front payment");
        const auto flags_expected = (test.percent > 0 ? 8u : 0u) | (test.first > 0 ? 2u : 0u) |
                                    (test.international > 0 ? 4u : 0u);
        require(c.u8(flags + 10) == flags_expected, "Original transfer fee registration flags");
        require(c.u16(list) == (test.months > 0), "Original scheduled payment");
        if (test.months > 0)
            require(c.u32(records) == payment && c.u8(records + 12) == test.months - 1, "Original installment record");
    } else {
        const auto expected_count = test.old_rows ? test.old_rows - 1 : 0;
        require(c.u16(list) == expected_count && calls(c, debit) == test.old_rows && calls(c, credit) == test.old_rows,
                "Original installment settlement");
        require(c.u32(account(0) + 0x1c) == (test.club_count ? 0 : 10), "Original idle club reserve");
    }
    compare(*s, entry, original, selected);
}

void sequence(State& s, RoutineRunner run) {
    Cpu c(s);
    run(s, register_terms);
    c.set(3, manager);
    run(s, settle);
    require(!c.u16(list) && c.u32(account(1) + 4) == 0u - std::uint32_t(active.amount),
            "Combined installment payment");
}
void original_sequence(State& s, std::uint32_t) { sequence(s, original); }
void production_sequence(State& s, std::uint32_t) { sequence(s, production); }

void drop_an_installment(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    c.put8(records + 12, c.u8(records + 12) - 1);
}

void check_negative_control() {
    active = {.months = 4};
    auto s = fixture_state(register_terms, active);
    const auto differences = compare_routine_call(*s, register_terms, original, drop_an_installment,
        RoutineChecker::result_r3, false, true);
    require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 3000400c"); }),
            "The installment comparison missed an altered remaining count");
}
}

int cm::aot::transfer_manager::checks::installments(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check(register_terms, {.first = 55, .international = 100, .months = 3, .percent = 15}, production);
            check(register_terms, {.first = 55, .international = 100, .percent = 15, .direct_fee_calls = true}, production);
            check(settle, {.old_rows = 6}, production);
            active = {.amount = 320, .first = 55, .international = 100, .months = 2, .percent = 15};
            auto s = fixture_state(register_terms, active);
            compare(*s, register_terms, original_sequence, production_sequence);
            std::cout << checked << " transfer installment production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown installment check mode");
        check(register_terms, {});
        check(register_terms, {.has_contract = false});
        check(register_terms, {.contract_kind = 5, .inherited_seller_account = true});
        check(register_terms, {.contract_kind = -1});
        check(register_terms, {.contract_kind = -1, .contract_low_bits = 2});
        check(register_terms, {.amount = -315, .first = 50, .international = 90, .months = 3, .percent = 15});
        check(register_terms, {.amount = 315, .months = 2, .missing_reserve_account = 3});
        check(register_terms, {.amount = 315, .months = -1, .percent = -1});
        check(register_terms, {.amount = std::int32_t(0x80000000u)});
        check(settle, {});
        check(settle, {.club_count = 0, .allocation_mask = 0});
        check(settle, {.old_rows = 6});
        check(settle, {.amount = -315, .old_rows = 6, .negative_reserve = true});
        constexpr auto seed = 0x54494e53u;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 24; ++i) {
            const auto amount = std::int32_t(random() % 2000000) - 1000000;
            const auto months = int(random() % 16);
            const auto missing = unsigned(random() % 4);
            check(register_terms, {.amount = amount, .months = months, .missing_reserve_account = missing});
        }
        check_negative_control();
        std::cout << checked << " transfer installment comparisons (seed " << seed << ") and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
