#include "transfer_manager_nation_rules.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t staff_transfer = 0x103fa850, offer_rules = 0x103f8820, notification = 0x103faba0,
    club_rule = 0x1040c1a0, bridge = 0x10781320;
constexpr std::uint32_t manager = fixture, club_table = fixture + 0x1000, club = club_table + 0x248,
    employer = club + 0x248, person = fixture + 0x1800, offer = fixture + 0x1900,
    records = fixture + 0x2000, vtable = fixture + 0x2400, vectors = fixture + 0x2600,
    output = fixture + 0x2800, globals = fixture + 0x2900, trace = fixture + 0x3000;
constexpr std::uint32_t callback_toc = toc + 0x10, callback_base = 0x31001000,
    original_country = 0x9efedcab, context = 0x12345678;
constexpr std::array methods{0xcu, 0x10u, 0x14u, 0x2cu, 0x38u};

struct Case {
    unsigned match = 4, retry = 0, first_result = 0, second_result = 0, club_flag = 1, closed = 0;
    int first_reason = 20, second_reason = 32;
    bool out = true, employed = true, foreign_employer = false;
};
Case active;
bool production_route = false;
std::uint32_t current = 0;
constexpr auto record_at(unsigned index) { return records + index * 0x20; }

std::uint32_t method() {
    switch (current) {
    case staff_transfer: return 0x2c;
    case offer_rules: return 0x14;
    case notification: return active.closed ? 0x10 : 0xc;
    case club_rule: return 0x38;
    default: throw std::runtime_error("Unknown nation rule");
    }
}

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 12, "Nation rule trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 80;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    add(c.r(2));
    for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
    add(c.r(12));
    add(c.s.r<4>(reg::ctr));
    add(c.cr(0));
}

unsigned calls(Cpu& c, std::uint32_t entry) {
    unsigned count = 0;
    for (unsigned i = 0; i < c.u32(trace); ++i) count += c.u32(trace + 4 + i * 80) == entry;
    return count;
}

bool observe(State& s) {
    Cpu c(s);
    const auto entry = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    std::uint32_t answer = 0;
    switch (entry) {
    case bridge:
        require(a == record_at(active.match) && c.r(2) == toc && c.r(12) == vectors + method() * 2,
                "Nation rule method and descriptor");
        record(c);
        if (!production_route) return false;
        s.budget = s.executed + 5;
        require(dispatch_page(s), "Missing original transition vector bridge");
        run_until(s, back);
        require(s.executed == s.budget, "Nation rule used instructions beyond the bridge");
        return true;
    case 0x1014c2e0:
        require(back == 0x103f8904 && a == club && !b && d == 1 && !c.r(6) && !c.r(7),
                "Nation rule retry eligibility");
        answer = active.club_flag;
        break;
    case 0x102cb570:
        require(back == 0x1040c1d8 && a == club && !b, "Nation rule club index");
        answer = active.match;
        break;
    default:
        if (entry != callback_base + method() * 4) return false;
        require(c.r(2) == callback_toc && c.u32(c.sp() + 0x14) == toc &&
                c.s.r<4>(reg::ctr) == entry, "Nation rule real bridge state");
        if (current == offer_rules) {
            const bool retry = back == 0x103f8960;
            require((retry || back == 0x103f88c4) && a == record_at(active.match) && b == club &&
                    d == person && c.r(6) == offer && c.r(7) == context && c.r(8) == c.sp() + 0x38 &&
                    c.u32(person + 0xc) == (retry ? 500 + active.match : original_country), "Nation offer rule arguments");
            c.put16(c.r(8), retry ? active.second_reason : active.first_reason);
            answer = retry ? active.second_result : active.first_result;
        } else if (current == staff_transfer) {
            require(back == 0x103fa8ac && a == record_at(active.match) && b == club && d == person,
                    "Nation staff-transfer rule arguments");
            answer = active.first_result;
        } else if (current == notification) {
            require(back == (active.closed ? 0x103fac34 : 0x103fac20) && b == offer, "Nation offer notification arguments");
            c.put32(output + 4, method());
            answer = 0x1919;
        } else {
            require(back == 0x1040c1fc && b == club && d == person && c.r(6) == output,
                    "Indexed nation rule arguments");
            c.put32(output + 4, 0x1119);
            answer = 0x2119;
        }
    }
    record(c);
    for (unsigned r = 4; r <= 12; ++r) c.set(r, 0xaa000000 + r);
    c.set(3, answer);
    c.set_cr(0, 4);
    c.finish();
    return true;
}

std::unique_ptr<State> make_state() {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0x5000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = current;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, club}});
    c.set_lr(return_sentinel);
    switch (current) {
    case staff_transfer: c.set(5, person); break;
    case offer_rules: c.set({{5, context}, {6, person}, {7, offer}, {8, active.retry}, {9, active.out ? output : 0}}); break;
    case notification: c.set(4, offer); break;
    case club_rule: c.set({{5, person}, {6, output}}); break;
    }
    c.put32(toc - 0x7be8, globals);
    c.put32(globals, club_table);
    c.put32(club + 0x2c, 500 + active.match);
    c.put32(employer + 0x2c, 500 + active.match + unsigned(active.foreign_employer));
    c.put32(person + 0xc, original_country);
    c.put32(person + 0x24, active.employed ? employer : 0);
    c.put32(offer + 8, 1);
    c.put8(offer + 0x24, active.closed);
    c.put16(output, 0xbeef);
    for (unsigned i = 0; i < 27; ++i) {
        c.put32(manager + 0x9e0 + i * 4, record_at(i));
        c.put32(record_at(i), vtable);
        c.put32(record_at(i) + 0xc, 500 + i);
    }
    for (auto slot : methods) {
        const auto vector = vectors + slot * 2;
        c.put32(vtable + slot, vector);
        c.put32(vector, callback_base + slot * 4);
        c.put32(vector + 4, callback_toc);
    }
    return s;
}

void original(State& s, std::uint32_t entry) {
    s.pc = entry;
    while (s.pc != return_sentinel) {
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
}

void rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    const auto a = c.r(3), b = c.r(4), d = c.r(5);
    switch (entry) {
    case staff_transfer: nation_allows_staff_transfer(c, a, b, d); break;
    case offer_rules: check_nation_offer_rules(c, a, b, d, c.r(6), c.r(7), c.r(8), c.r(9)); break;
    case notification: dispatch_nation_offer_notification(c, a, b); break;
    case club_rule: dispatch_club_nation_rule(c, a, b, d, c.r(6)); break;
    default: throw std::runtime_error("Unknown nation rules entry");
    }
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
void check(std::uint32_t entry, const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    current = entry;
    auto s = make_state();
    auto reference = clone(*s);
    original(*reference, entry);
    Cpu c(*reference);
    const bool found = test.match < 27;
    unsigned rule_calls = found;
    require(c.r(2) == toc && c.u32(person + 0xc) == original_country, "Original nation rule restored context");
    if (entry == staff_transfer) {
        rule_calls = found && test.employed && !test.foreign_employer;
        require(c.r(3) == (rule_calls ? test.first_result : 1), "Original staff transfer rule");
    } else if (entry == offer_rules) {
        const auto first_accepted = std::int8_t(test.first_result) != 0;
        const auto retried = found && !first_accepted && std::int8_t(test.retry) && std::int8_t(test.club_flag) &&
            (test.first_reason == 17 || test.first_reason == 20 || test.first_reason == 21 || test.first_reason == 31);
        const auto second_accepted = std::int8_t(test.second_result) != 0;
        rule_calls += retried;
        const auto answer = !found ? 1 : retried ? unsigned(second_accepted) :
            first_accepted || !std::int8_t(test.retry) ? test.first_result : 0;
        const auto reason = test.out ? !found ? 0 : retried && !second_accepted ? test.second_reason : test.first_reason : 0xbeef;
        require(c.r(3) == answer && c.u16(output) == std::uint16_t(reason), "Original offer rule result and reason");
    } else {
        const auto effect = found ? entry == notification ? method() : 0x1119 : 0;
        require(c.u32(output + 4) == effect, "Original nation rule effect");
    }
    require(calls(c, bridge) == rule_calls, "Original nation rule callback count");
    const auto differences = compare_routine_call(*s, entry, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Nation rule case " + std::to_string(checked) + " at " + hex_address(entry));
    ++checked;
}

void leak_nation(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put32(person + 0xc, 500 + active.match);
}
}

int cm::aot::transfer_manager::checks::nation_rules(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check(staff_transfer, {.match = 26, .first_result = 0x80}, production);
            check(offer_rules, {.retry = 1, .second_result = 1}, production);
            check(offer_rules, {.match = 27}, production);
            check(notification, {}, production);
            check(notification, {.closed = 0x80}, production);
            check(club_rule, {.match = 26}, production);
            std::cout << checked << " nation rule production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown nation rule check mode");
        check(staff_transfer, {.match = 0, .first_result = 1});
        check(staff_transfer, {.match = 26, .first_result = 0x80});
        check(staff_transfer, {.match = 27});
        check(staff_transfer, {.employed = false});
        check(staff_transfer, {.foreign_employer = true});
        check(offer_rules, {.match = 27});
        check(offer_rules, {.match = 27, .out = false});
        check(offer_rules, {.match = 0, .first_result = 0x80});
        check(offer_rules, {.first_result = 0x100, .out = false});
        check(offer_rules, {.retry = 1, .club_flag = 0});
        check(offer_rules, {.retry = 1, .club_flag = 0, .out = false});
        for (int reason : {17, 19, 20, 21, 22, 31, 32, -1}) {
            check(offer_rules, {.retry = 0x80, .first_reason = reason});
            check(offer_rules, {.retry = 1, .second_result = 0xff, .first_reason = reason});
            check(offer_rules, {.retry = 1, .first_reason = reason, .out = false});
        }
        check(notification, {});
        check(notification, {.match = 26, .closed = 0xff});
        check(notification, {.match = 27});
        check(club_rule, {.match = 0});
        check(club_rule, {.match = 26});
        active = {.retry = 1};
        current = offer_rules;
        auto s = make_state();
        const auto differences = compare_routine_call(*s, current, original, leak_nation,
            RoutineChecker::result_r3, false, true);
        require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 3000180c"); }),
                "The nation rule comparison missed the leaked nationality");
        std::cout << checked << " nation rule comparisons and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
