#include "transfer_manager_offer_state.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t club_response = 0x103e6e30, staff_response = 0x103e6f70,
    clear_flag = 0x103f6190, filter = 0x103fb390, next = 0x10738e00;
constexpr std::uint32_t manager = fixture, offers = fixture + 0x1000, offer = fixture + 0x1100,
    linked_offer = fixture + 0x1200, list = fixture + 0x1300, rows = fixture + 0x1400,
    nodes = fixture + 0x1500, flags = fixture + 0x1600, pool = fixture + 0x2000,
    message = fixture + 0x3000, globals = fixture + 0x3100, trace = fixture + 0x4000;

struct Case {
    std::int32_t index = 1, count = 2, person = 8;
    unsigned kind = 3, linked_kind = 4, stage = 3, items = 2, present_response = 0, status = 0xa5;
    bool empty_slot = false, no_links = false, closed = false, direct = false;
};
Case active;
std::uint32_t current_entry = 0;
bool original_route = false, production_route = false;

void original(State&, std::uint32_t);

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 12, "Offer response trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 64;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 8; ++r) add(c.r(r));
    for (unsigned r = 27; r <= 31; ++r) add(c.r(r));
    add(c.cr(0));
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    if (target == next) {
        require(a == list && (back == 0x103e70a0 || back == 0x103f61fc), "Offer response iterator");
        if (!active.direct) record(c);
        if (!production_route || original_route) return false;
        s.budget = s.executed + 8;
        require(dispatch_page(s), "Missing offer response iterator");
        if (s.pc != back) run_until(s, back);
        s.budget = s.executed;
        return true;
    }
    if (target == filter) {
        require(a == manager && ((b == offer && back == 0x103f61b0) ||
                (b == linked_offer && back == 0x103f61ec)), "Offer status filter arguments");
        if (!active.direct) record(c);
        if (!original_route) return false;
        original(s, target);
        return true;
    }
    const bool negative = active.index < 0;
    const auto split = (current_entry == club_response ? 0x103e6e74u : 0x103e6fb4u) + (negative ? 0 : 0x60);
    const auto line = (current_entry == club_response ? 0xd91u : 0xdc3u) + (negative ? 0 : 5);
    switch (target) {
    case 0x10739890:
        require(back == split && a == pool + 0x37 && !b && !d &&
                c.r(6) == c.sp() + (negative ? 0x68 : 0x48) && c.r(7) == c.sp() + (negative ? 0x40 : 0x38),
                "Offer index assertion split");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        if (production_route && !original_route) s.budget = s.executed + 2;
        break;
    case 0x1077b720:
        require(back == split + 0x20 && a == message && b == pool + 0x6ff && d == globals + 4 && c.r(8) == line,
                "Offer index assertion format");
        c.put32(message, line);
        break;
    case 0x10769b60:
        require(back == split + 0x30 && a == pool + 0x70c && b == message && !d && c.u32(message) == line,
                "Offer index assertion dialog");
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xa4000000 + r);
    c.set(3, 0);
    c.set_cr(0, 4);
    c.finish();
    return true;
}

std::unique_ptr<State> make_state() {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0x6000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = current_entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, current_entry == clear_flag ? offer : std::uint32_t(active.index)},
           {5, std::uint32_t(active.person)}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x7bac, globals);
    c.put32(toc - 0x5b20, globals + 4);
    c.put8(globals, 1);
    c.put32(manager, offers);
    c.put32(manager + 4, active.count);
    c.put32(manager + 0x9a0, flags);
    c.put32(offers, linked_offer);
    c.put32(offers + 4, active.empty_slot ? 0 : offer);
    c.put32(offer, active.no_links ? 0 : list);
    c.put32(offer + 4, list);
    c.put32(offer + 0xc, 1);
    c.put8(offer + 0x24, active.closed);
    c.put8(offer + 0x34, active.kind);
    c.put8(offer + 0x36, active.stage);
    c.put8(linked_offer + 0x34, active.linked_kind);
    c.put8(linked_offer + 0x36, active.stage);
    c.put8(flags + 10, active.status);
    c.put32(list + 4, active.items ? nodes : 0);
    c.put32(list + 0xc, 0xdeadfeed);
    for (unsigned i = 0; i < active.items; ++i) {
        c.put32(nodes + i * 16, rows + i * 8);
        c.put32(nodes + i * 16 + 8, i + 1 < active.items ? nodes + (i + 1) * 16 : 0);
        c.put32(rows + i * 8, current_entry == clear_flag ? 0 : 7 + i);
        c.put8(rows + i * 8 + 4, i ? 0 : active.present_response);
    }
    return s;
}

void original(State& s, std::uint32_t entry) {
    const bool previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    while (s.pc != stop) {
        if (s.pc == filter && entry != filter) {
            require(observe(s), "Missing original offer filter");
            continue;
        }
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    switch (entry) {
    case club_response: mark_offer_club_response(c, c.r(3), std::int32_t(c.r(4))); break;
    case staff_response: mark_offer_staff_response(c, c.r(3), std::int32_t(c.r(4)), std::int32_t(c.r(5))); break;
    default: clear_inactive_offer_flag(c, c.r(3), c.r(4)); break;
    }
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    production_route = true;
    s.pc = entry;
    s.budget = s.executed;
    if (active.direct) {
        dispatch_probe = nullptr;
        s.budget += 8 * (active.items + 1);
    }
    run_until(s, return_sentinel);
    dispatch_probe = observe;
    production_route = false;
}

unsigned checked = 0;
bool inactive(unsigned kind) {
    return kind == 2 || kind == 3 || kind == 4 || kind == 16 || kind == 20 || (kind == 15 && active.stage >= 3);
}

void check(std::uint32_t entry, const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    current_entry = entry;
    auto s = make_state();
    auto baseline = clone(*s);
    original(*baseline, entry);
    Cpu c(*baseline);
    const bool invalid = test.index < 0 || test.index >= test.count;
    if (entry == clear_flag) {
        const bool clear = inactive(test.kind) && (test.no_links || !test.items || inactive(test.linked_kind));
        require(c.u8(flags + 10) == (clear ? test.status & 0xfe : test.status), "Original inactive offer flag");
    } else if (invalid) {
        require(c.u8(globals) == 0 && c.u8(offer + 0x34) == test.kind && c.u32(trace) == 3,
                "Original invalid offer index");
    } else if (entry == club_response) {
        const auto kind = test.empty_slot || test.closed ? test.kind : test.kind == 6 ? 8 : 7;
        require(c.u8(offer + 0x34) == kind, "Original club response status");
    } else {
        bool any_response = false;
        for (unsigned i = 0; i < test.items; ++i) {
            const auto response = !test.empty_slot && std::int32_t(7 + i) == test.person ? 1 :
                i == 0 ? test.present_response : 0;
            require(c.u8(rows + i * 8 + 4) == response, "Original staff response row");
            any_response |= response != 0;
        }
        const auto kind = test.empty_slot || any_response ? test.kind : test.kind == 7 ? 8 : 6;
        require(c.u8(offer + 0x34) == kind, "Original staff response status");
    }
    const auto differences = compare_routine_call(*s, entry, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Offer response case " + std::to_string(checked));
    ++checked;
}

void restore_flag(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put8(flags + 10, active.status);
}
}

int cm::aot::transfer_manager::checks::offer_state(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check(club_response, {.kind = 6}, production);
            check(club_response, {.index = -1}, production);
            check(staff_response, {}, production);
            check(staff_response, {.person = -1, .kind = 7}, production);
            check(clear_flag, {.linked_kind = 5}, production);
            check(clear_flag, {.direct = true}, production);
            std::cout << checked << " offer response production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown offer response check mode");
        for (auto entry : {club_response, staff_response}) {
            check(entry, {});
            check(entry, {.index = -1});
            check(entry, {.index = 2});
            check(entry, {.empty_slot = true});
        }
        check(club_response, {.kind = 6});
        check(club_response, {.closed = true});
        check(staff_response, {.person = -1, .kind = 7});
        check(staff_response, {.person = -1, .kind = 5});
        check(staff_response, {.person = -1, .kind = 7, .present_response = 0xff});
        check(staff_response, {.kind = 7, .items = 0});
        check(clear_flag, {});
        check(clear_flag, {.kind = 5});
        check(clear_flag, {.linked_kind = 5});
        check(clear_flag, {.kind = 15, .stage = 2});
        check(clear_flag, {.kind = 15, .stage = 3});
        check(clear_flag, {.status = 0xa4});
        check(clear_flag, {.no_links = true});
        check(clear_flag, {.items = 0});
        active = {};
        current_entry = clear_flag;
        auto s = make_state();
        const auto differences = compare_routine_call(*s, clear_flag, original, restore_flag,
            RoutineChecker::result_r3, false, true);
        require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 3000160a"); }),
                "The offer response comparison missed the original inactive flag");
        std::cout << checked << " offer response comparisons and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
