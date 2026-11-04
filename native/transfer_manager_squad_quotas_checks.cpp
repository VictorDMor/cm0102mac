#include "transfer_manager_squad_quotas.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t classify = 0x103fdae0, status = 0x103f93e0, read_limits = 0x103f9550,
    select_limit_entry = 0x103f97b0, count_violations = 0x103f98d0, bridge = 0x10781320,
    fill = 0x10778300, status_callback = 0x31001000, limits_callback = 0x31002000;
constexpr std::uint32_t manager = fixture, club = fixture + 0x1000, other = fixture + 0x1300,
    nation = fixture + 0x1600, league = fixture + 0x1800, competition = fixture + 0x1900,
    records = fixture + 0x2000, vtable = fixture + 0x2700, vectors = fixture + 0x2800,
    limits = fixture + 0x2900, summary = fixture + 0x2a00, text = fixture + 0x2b00,
    lineup = fixture + 0x2c00, staff = fixture + 0x3000, national_map = fixture + 0x4000,
    globals = fixture + 0x5000, trace = fixture + 0x6000, pool = fixture + 0xa000;
constexpr std::uint32_t geography = 0x12345678, domestic_id = 901, callback_toc = toc + 0x4000;
constexpr auto record_at(unsigned index) { return records + index * 0x30; }
constexpr auto player(unsigned index) { return staff + index * 0x70; }

struct Case {
    unsigned rule_index = 7, group = 3, slots = 2, category = 1, amount = 1, age = 23, field = 5, other_field = 0xff;
    std::uint32_t foreigners = 0x1809, exempt = 0, available = 0xffff;
    bool domestic = true, disabled = false, no_nation = false, wrong_kind = false, unavailable_contest = false,
         early_result = false, exempt_competition = false, reduce = false, generated_lineup = false,
         managed_home = true, managed_away = false, with_text = true, with_summary = true, direct = false;
};
Case active;
bool original_route = false, production_route = false;

void original(State&, std::uint32_t);
void rewrite(State&, std::uint32_t);

std::int32_t mode() {
    if (active.domestic) return 1;
    if (active.disabled) return -1;
    if (active.group <= 1) return 4;
    return active.group >= 3 && active.group <= 7 ? 2 : 1;
}

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 120, "Squad quota trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 112;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    add(c.r(2));
    for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 21; r <= 31; ++r) add(c.r(r));
    add(c.r(12));
    add(c.s.r<4>(reg::ctr));
    add(c.cr(0));
}

bool external(State& s, unsigned allowance) {
    if (!production_route || original_route) return false;
    const auto back = Cpu(s).lr();
    s.budget = s.executed + allowance;
    require(dispatch_page(s), "Missing squad quota dependency");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

bool owned(std::uint32_t entry) { return entry == classify || entry == read_limits || entry == select_limit_entry; }

bool observe(State& s) {
    Cpu c(s);
    const auto entry = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    if (owned(entry)) {
        if (back == return_sentinel) return false;
        if (!original_route && !production_route) {
            rewrite(s, entry);
            return true;
        }
        return false;
    }
    std::uint32_t answer = 0;
    switch (entry) {
    case bridge:
        require(a == record_at(active.rule_index) && c.r(2) == toc &&
                (c.r(12) == vectors || c.r(12) == vectors + 8), "Squad quota transition vector");
        record(c);
        return external(s, 5);
    case status_callback:
        require((back == 0x103f94d0 || back == 0x103f9510) && a == record_at(active.rule_index) && b == club &&
                d == competition && c.r(2) == callback_toc && c.u32(c.sp() + 0x14) == toc, "Squad quota status callback");
        answer = 8;
        break;
    case limits_callback:
        require((back == 0x103f9614 || back == 0x103f9654) && a == record_at(active.rule_index) &&
                b == competition && c.r(2) == callback_toc && c.u32(c.sp() + 0x14) == toc, "Squad quota limits callback");
        if (active.category) {
            c.put8(d + active.field, active.category == 3 ? active.age : active.amount);
            if (active.category == 3) c.put8(d + 0x17, active.amount);
            if (active.other_field < 0x2c) c.put8(d + active.other_field, active.amount + 2);
        }
        break;
    case fill:
        require((b == 0 && d == 6) || (b == 0xffffffff && d == 0x2c), "Squad quota memory initialization");
        record(c);
        return external(s, 2000);
    case 0x10494d20:
        require(back == 0x103fdb64 && a == globals + 0x100, "Squad quota availability");
        answer = active.disabled;
        break;
    case 0x10192130:
        require(back == 0x103fdb8c && a == league, "Squad quota association index");
        answer = active.group;
        break;
    case 0x100c7d00:
        require(back == 0x103f9410 && a == club, "Squad quota domestic club gate");
        answer = active.exempt_competition;
        break;
    case 0x10003dc0:
        require(back == 0x103f942c && a == globals + 0x104 && b == club && d == 0, "Squad quota early result gate");
        answer = active.early_result;
        break;
    case 0x103af800:
        require(back == 0x103f997c && a == globals + 0x108 && b == club && d < 11 + active.slots &&
                c.r(6) == unsigned(active.managed_home || active.managed_away), "Squad quota generated lineup");
        if (active.available & (1u << d)) answer = player(d);
        break;
    case 0x100cab10:
    case 0x100ca6a0: {
        const auto slot = (a - staff) / 0x70;
        require(a == player(slot) && b == (active.no_nation ? 0 : nation), "Squad quota participant nationality");
        if (entry == 0x100cab10) {
            require(d == 1 && (back == 0x103f99b4 || back == 0x103f9a0c), "Squad quota foreign participant");
            answer = (active.foreigners >> slot) & 1;
        } else {
            require(back == 0x103f9a20, "Squad quota exempt participant");
            answer = (active.exempt >> slot) & 1;
        }
        break;
    }
    case 0x10727890:
        require((a == text || a == text + 4) && b >= pool + 0x1c2a && b <= pool + 0x212c,
                "Squad quota message");
        c.put32(a, b - pool);
        answer = a;
        break;
    case 0x1077dbd0:
        require(a == text && c.u32(text), "Squad quota message append");
        answer = 4;
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xae000000 + r);
    c.set(3, answer);
    c.set_cr(0, 4);
    c.finish();
    return true;
}

std::unique_ptr<State> make_state(std::uint32_t entry) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0xd000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, competition}});
    c.set_lr(return_sentinel);
    const auto contest = active.unavailable_contest ? 0 : competition;
    switch (entry) {
    case classify: c.set({{5, limits}, {6, limits + 4}, {7, limits + 8}}); break;
    case status: c.set({{4, club}, {5, competition}}); break;
    case read_limits: c.set({{4, contest}, {5, limits}, {6, active.reduce}}); break;
    case select_limit_entry: c.set({{4, contest}, {5, summary}}); break;
    case count_violations:
        c.set({{4, club}, {5, contest}, {6, active.with_text ? text : 0}, {7, active.generated_lineup ? 0 : lineup},
               {9, active.with_summary ? summary : 0}});
        break;
    }
    c.put32(toc - 0x7aa8, globals);
    c.put32(toc - 0x7b54, globals + 4);
    c.put32(toc - 0x7b24, globals + 8);
    c.put32(toc - 0x7a6c, globals + 12);
    c.put32(toc - 0x7b44, national_map);
    c.put32(toc - 0x4b9c, pool);
    c.put32(globals, domestic_id);
    c.put32(globals + 4, globals + 0x100);
    c.put32(globals + 8, globals + 0x104);
    c.put32(globals + 12, globals + 0x108);
    c.put32(national_map + active.group * 0x4c, active.no_nation ? 0 : nation);
    c.put32(league, active.domestic ? domestic_id : 902);
    c.put32(league + 4, geography);
    c.put32(competition + 0x14, league);
    c.put32(competition + 0x1c, club);
    c.put32(competition + 0x20, active.managed_away ? club : other);
    c.put8(competition + 0x40, !active.managed_home);
    c.put8(competition + 0x41, !active.managed_away);
    c.put8(competition + 0x4e, active.slots << 4);
    c.put32(club + 0x2c, active.no_nation ? 0 : nation);
    c.put32(nation + 0x74, geography);
    c.put32(vtable + 0x20, vectors);
    c.put32(vtable + 0x24, vectors + 8);
    c.put32(vectors, status_callback);
    c.put32(vectors + 4, callback_toc);
    c.put32(vectors + 8, limits_callback);
    c.put32(vectors + 12, callback_toc);
    for (unsigned i = 0; i < 27; ++i) {
        c.put32(manager + 0x9e0 + i * 4, record_at(i));
        c.put32(record_at(i), vtable);
        c.put32(record_at(i) + 0xc, i == active.rule_index ? active.no_nation ? 0 : nation : 1);
        c.put32(record_at(i) + 0x10, i == active.rule_index ? geography : 1);
        c.put8(record_at(i) + 0x14, i == active.rule_index && !active.wrong_kind ? mode() : 3);
        c.put32(lineup + i * 4, active.available & (1u << i) ? player(i) : 0);
        c.put8(player(i) + 0x5d, active.foreigners & (1u << i) ? active.age + 1 : active.age);
    }
    return s;
}

void original(State& s, std::uint32_t entry) {
    const bool previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    while (s.pc != stop) {
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    const auto a = c.r(3), b = c.r(4), d = c.r(5);
    switch (entry) {
    case classify: resolve_squad_quota_category(c, b, d, c.r(6), c.r(7)); break;
    case status: team_squad_quota_status(c, a, b, d); break;
    case read_limits: read_squad_quota_limits(c, a, b, d, c.r(6)); break;
    case select_limit_entry: select_squad_quota_limit(c, a, b, d); break;
    case count_violations: squad_violates_quota(c, a, b, d, c.r(6), c.r(7), c.r(9)); break;
    default: throw std::runtime_error("Unknown squad quota entry");
    }
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    production_route = true;
    s.pc = entry;
    s.budget = s.executed + (active.direct ? 280 : 0);
    run_until(s, return_sentinel);
    production_route = false;
}

unsigned checked = 0;
void check(std::uint32_t entry, const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    dispatch_probe = active.direct ? nullptr : observe;
    auto s = make_state(entry);
    auto reference = clone(*s);
    original(*reference, entry);
    Cpu c(*reference);
    const bool available = mode() > 0 && mode() != 4 && test.rule_index < 27 && !test.wrong_kind && !test.unavailable_contest;
    if (entry == classify) {
        require(c.i8(limits) == mode() &&
                c.u32(limits + 4) == (mode() == -1 || test.no_nation ? 0 : nation) &&
                c.u32(limits + 8) == (mode() == -1 || (test.domestic && test.no_nation) ? 0 : geography),
                "Original squad quota classification");
    } else if (entry == status) {
        require(c.r(3) == (!test.exempt_competition && test.early_result ? 9 : available ? 8 : 0),
                "Original squad quota status");
    } else if (entry == select_limit_entry) {
        require(c.u8(summary) == (available ? test.category : 0), "Original selected squad quota");
    } else if (entry == count_violations) {
        const auto slots = (1u << (test.slots + 11)) - 1;
        const auto players = test.foreigners & test.available & slots & (test.category == 1 ? ~test.exempt : 0xffffffffu);
        const auto total = std::popcount(players);
        require(c.r(3) == (available && test.category && total > std::int8_t(test.amount)), "Original squad quota violations");
        if (available && test.category && test.with_summary)
            require(c.u8(summary) == test.category && c.u8(summary + 2) == std::popcount(players & 0x7ffu) &&
                    c.u8(summary + 3) == std::popcount(players & ~0x7ffu) && c.u8(summary + 4) == total &&
                    c.u8(summary + 5) == test.amount, "Original squad quota counters");
    }
    const auto differences = compare_routine_call(*s, entry, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Squad quota case " + std::to_string(checked) + " at " + hex_address(entry));
    ++checked;
}

void remove_violations(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put8(summary + 4, 0);
}
}

int cm::aot::transfer_manager::checks::squad_quotas(int argc, char** argv) {
    try {
        install_check_coverage();
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check(status, {}, production);
            check(status, {.group = 7, .domestic = false}, production);
            check(read_limits, {.category = 2, .field = 0, .reduce = true}, production);
            check(select_limit_entry, {.category = 3, .field = 0x16}, production);
            check(count_violations, {}, production);
            check(count_violations, {.rule_index = 27, .direct = true}, production);
            std::cout << checked << " squad quota production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown squad quota check mode");
        check(classify, {});
        check(classify, {.no_nation = true});
        check(classify, {.domestic = false, .disabled = true});
        for (unsigned group = 0; group <= 9; ++group) check(classify, {.group = group, .domestic = false});
        check(status, {});
        check(status, {.rule_index = 0, .group = 3, .domestic = false});
        check(status, {.group = 0, .domestic = false});
        check(status, {.domestic = false, .disabled = true});
        check(status, {.wrong_kind = true});
        check(status, {.early_result = true});
        check(status, {.early_result = true, .exempt_competition = true});
        check(read_limits, {.unavailable_contest = true});
        check(read_limits, {.domestic = false, .disabled = true});
        check(read_limits, {.group = 0, .domestic = false});
        check(read_limits, {.group = 3, .domestic = false});
        check(read_limits, {});
        check(read_limits, {.other_field = 3, .reduce = true});
        for (unsigned field : {3u, 0u, 2u, 0x16u}) {
            check(read_limits, {.field = field, .reduce = true});
            check(select_limit_entry, {.category = field == 3 ? 1u : field == 0x16 ? 3u : 2u, .field = field});
        }
        check(read_limits, {.category = 0, .reduce = true});
        check(select_limit_entry, {.category = 0});
        check(select_limit_entry, {.unavailable_contest = true});
        check(count_violations, {.category = 0});
        check(count_violations, {.with_text = false, .with_summary = false});
        check(count_violations, {.amount = 0x80, .foreigners = 0});
        check(count_violations, {.category = 3, .amount = 5, .field = 0x16, .available = 0x3});
        check(count_violations, {.category = 3, .field = 0x16, .with_text = false});
        check(count_violations, {.amount = 9, .generated_lineup = true, .managed_home = false});
        check(count_violations, {.amount = 1, .exempt = 0x8, .available = 0x180d, .generated_lineup = true,
                                 .managed_home = false, .managed_away = true});
        for (unsigned category : {1u, 2u, 3u}) {
            const auto field = category == 1 ? 5u : category == 2 ? 2u : 0x16u;
            check(count_violations, {.category = category, .amount = 0, .field = field, .foreigners = 1});
            check(count_violations, {.category = category, .amount = 0, .field = field});
            check(count_violations, {.category = category, .amount = 1, .field = field});
            check(count_violations, {.category = category, .amount = 2, .field = field});
        }
        active = {};
        dispatch_probe = observe;
        auto s = make_state(count_violations);
        const auto differences = compare_routine_call(*s, count_violations, original, remove_violations,
            RoutineChecker::result_r3, false, true);
        require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 30002a04"); }),
                "The squad quota comparison missed the altered foreign-player count");
        std::cout << checked << " squad quota comparisons and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
