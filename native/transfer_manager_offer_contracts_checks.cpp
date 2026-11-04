#include "transfer_manager_offer_contracts.hpp"
#include "transfer_manager_permissions.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t remove_offer = 0x103ec440, forget_offer = 0x103ec4f0,
    mark_offer = 0x103ec5d0, test_offer = 0x103ecda0, regional = 0x103ecef0, gate = 0x103ee1a0,
    primary_lookup = 0x10082f20, pending_lookup = 0x1007f720, remove_primary = 0x1007f840,
    remove_secondary = 0x1007f930, forget = 0x1036f100, iterator = 0x10738e00;
constexpr std::uint32_t manager = fixture, offer = fixture + 0xb00, people = fixture + 0x1000,
    clubs = fixture + 0x2000, seller = clubs + 0x248, buyer = clubs + 2 * 0x248,
    nationals = fixture + 0x3000, domestic = nationals + 0x128, foreign = nationals + 0x250,
    globals = fixture + 0x4000, ledger = fixture + 0x4100, contracts = fixture + 0x4200,
    index = fixture + 0x4500, shortlist = fixture + 0x5000, club_lists = fixture + 0x5200,
    seller_rows = fixture + 0x5400, buyer_rows = fixture + 0x5b00, list = fixture + 0x6300,
    nodes = fixture + 0x6400, secondary_rows = fixture + 0x6500, pending_list = fixture + 0x6700,
    pending_nodes = fixture + 0x6800, pending_rows = fixture + 0x6900, trace = fixture + 0x7100;
constexpr auto player(std::uint32_t id) { return people + id * 0x70; }
constexpr auto pending(std::uint32_t id) { return pending_rows + (id - 1) * 0x88; }

struct Case {
    unsigned extras = 2, pending_mask = 7, exempt_mask = 0;
    bool no_list = false, primary = true, buyer_restricts = false, seller_restricts = false,
         real_dependencies = false;
};
Case active;
bool original_route = false, production_route = false;

void original(State&, std::uint32_t);

void trace_call(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 48, "Offer contract trace exhausted");
    c.put32(trace, count + 1);
    auto cursor = trace + 4 + count * 64;
    const auto add = [&](std::uint32_t word) { c.put32(cursor, word); cursor += 4; };
    add(c.s.pc);
    add(c.lr());
    for (unsigned r = 3; r <= 8; ++r) add(c.r(r));
    for (unsigned r = 25; r <= 31; ++r) add(c.r(r));
    add(c.cr(0));
}

unsigned calls(Cpu& c, std::uint32_t entry) {
    unsigned count = 0;
    for (unsigned i = 0; i < c.u32(trace); ++i) count += c.u32(trace + 4 + i * 64) == entry;
    return count;
}

bool real_iterator(State& s, std::uint32_t back) {
    if (!production_route || original_route) return false;
    s.budget = s.executed + 8;
    require(dispatch_page(s), "Missing list iterator");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

bool observe(State& s) {
    Cpu c(s);
    const auto entry = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    std::uint32_t answer = 0;
    switch (entry) {
    case regional:
        require(a == manager && c.r(6) == 0 &&
                (back == 0x103ecdf0 ? b == player(1) && d == buyer :
                 back == 0x103ece90 && (b == player(2) || b == player(3)) &&
                 d == (active.primary ? seller : 0)), "Offer regional approval arguments");
        trace_call(c);
        if (original_route) {
            original(s, entry);
            return true;
        }
        return false;
    case gate:
        require(a == manager && (back == 0x103ece10 ? b == player(1) :
                back == 0x103eceb0 && (b == player(2) || b == player(3))), "Offer approval gate arguments");
        trace_call(c);
        if (original_route) {
            original(s, entry);
            return true;
        }
        return false;
    case 0x100ca6a0:
        require(back == 0x103ecfac && a >= player(1) && a <= player(3) && (a - people) % 0x70 == 0 && b == 0,
                "National exemption arguments");
        answer = (active.exempt_mask >> ((a - people) / 0x70 - 1)) & 1;
        break;
    case primary_lookup:
        require(a == ledger && ((back == 0x103ecfd4 && b >= player(1) && b <= player(3)) ||
                ((back == 0x103ec534 || back == 0x103ec614 || back == 0x103ece50) && b == player(1))),
                "Offer primary lookup arguments");
        answer = active.primary && b == player(1) ? contracts : 0;
        if (active.real_dependencies) {
            trace_call(c);
            return false;
        }
        break;
    case pending_lookup:
        require(a == ledger && (back == 0x103ec64c ? b == 1 && d == 2 :
                back == 0x103ec688 && (b == 2 || b == 3) && d == (active.primary ? 1u : 0u)),
                "Offer pending lookup arguments");
        answer = (active.pending_mask & (1u << (b - 1))) && (b == 1 || active.primary) ? pending(b) : 0;
        if (active.real_dependencies) {
            trace_call(c);
            return false;
        }
        break;
    case remove_primary:
        require(back == 0x103ec488 && a == ledger && b == player(1) && d == buyer, "Primary pending removal");
        if (active.real_dependencies) {
            trace_call(c);
            return false;
        }
        c.put8(pending(1) + 0x64, 0);
        break;
    case remove_secondary:
        require(back == 0x103ec4c4 && a == ledger && (b == player(2) || b == player(3)) &&
                d == 0x12345678, "Secondary pending removal");
        if (active.real_dependencies) {
            trace_call(c);
            return false;
        }
        c.put8(pending((b - people) / 0x70) + 0x64, 0);
        break;
    case forget:
        require(a == shortlist && (back == 0x103ec574 ? b == buyer && d == player(1) :
                back == 0x103ec5a8 && b == (active.primary ? seller : 0) && (d == player(2) || d == player(3))),
                "Offer shortlist cleanup arguments");
        if (active.real_dependencies) {
            trace_call(c);
            return false;
        }
        if (b != 0) c.put32((b == buyer ? buyer_rows : seller_rows + ((d - people) / 0x70 - 2) * 0x2c) + 0x18,
                           0xffffffff);
        break;
    case iterator:
        require((a == list && (back == 0x103ec4d0 || back == 0x103ec5b0 || back == 0x103ec69c || back == 0x103ececc))
                || (active.real_dependencies && a == pending_list &&
                    (back == 0x1007f788 || back == 0x1007f8a0 || back == 0x1007f994)), "Offer list iterator arguments");
        trace_call(c);
        return real_iterator(s, back);
    default:
        return false;
    }
    trace_call(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xc1000000 + r);
    c.set_cr(0, 4);
    c.set(3, answer);
    c.finish();
    return true;
}

std::unique_ptr<State> fixture_state(std::uint32_t entry, const Case& test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0x9000);
    s->memory.map(toc - 0x8000, 0x4000);
    s->memory.map(stack - 0x2000, 0x3000);
    if (!test.primary && test.extras && !test.no_list) s->memory.map(0, 4);
    s->pc = entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned n = 0; n < 8; ++n) c.set_cr(n, n);
    for (unsigned n = 1024; n < 1028; ++n) s->wr<1>(n, 0);
    for (unsigned n = 1280; n < 1312; ++n) s->wr<1>(n, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, offer}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7be8, globals);
    c.put32(toc - 0x7be0, globals + 4);
    c.put32(toc - 0x7b7c, globals + 8);
    c.put32(toc - 0x7b78, globals + 12);
    c.put32(toc - 0x7b9c, globals + 16);
    c.put32(toc - 0x7be4, globals + 20);
    c.put32(globals, clubs);
    c.put32(globals + 4, people);
    c.put32(globals + 8, ledger);
    c.put32(globals + 12, 0xdeadbeef);
    c.put32(globals + 16, index);
    c.put32(globals + 20, 4);
    c.put32(ledger, contracts);
    c.put32(ledger + 0x34, test.real_dependencies && test.pending_mask ? pending_list : 0);
    c.put32(contracts + 4, 1);
    c.put8(contracts + 0x38, 5 * 4);
    c.put32(seller, 1);
    c.put32(buyer, 2);
    c.put32(seller + 0x2c, test.seller_restricts ? foreign : domestic);
    c.put32(buyer + 0x2c, test.buyer_restricts ? foreign : domestic);
    c.put8(domestic + 0x118, 13);
    c.put8(foreign + 0x118, 14);
    c.put32(manager + 0x988, shortlist);
    c.put32(shortlist + 0xdc, club_lists);
    c.put32(club_lists + 12, seller_rows);
    c.put8(club_lists + 16, 1);
    c.put32(club_lists + 24, buyer_rows);
    c.put8(club_lists + 28, 1);
    c.put32(buyer_rows + 0x10, 1);
    c.put32(buyer_rows + 0x18, 44);
    c.put32(offer + 4, test.no_list ? 0 : list);
    c.put32(offer + 8, 2);
    c.put32(offer + 0xc, 1);
    c.put8(offer + 0x10, 7);
    c.put32(offer + 0x28, 0x12345678);
    c.put32(list + 4, test.extras ? nodes : 0);
    c.put32(list + 0xc, nodes + 0x80);
    for (unsigned id = 1; id <= 3; ++id) {
        c.put32(player(id), id);
        c.put32(index + id * 100, id == 1 && test.primary ? 0 : 0xffffffff);
        c.put32(pending(id), id);
        c.put32(pending(id) + 4, id == 1 ? 2 : 1);
        c.put8(pending(id) + 0x64, 1);
        if (id > 1) {
            const auto row = id - 2;
            c.put32(seller_rows + row * 0x2c + 0x10, id);
            c.put32(seller_rows + row * 0x2c + 0x18, 44);
            c.put32(secondary_rows + row * 8, id);
            c.put8(secondary_rows + row * 8 + 4, 7);
            c.put32(nodes + row * 16, secondary_rows + row * 8);
            c.put32(nodes + row * 16 + 8, row + 1 < test.extras ? nodes + (row + 1) * 16 : 0);
        }
    }
    auto next = std::uint32_t(0);
    for (unsigned id : {3u, 2u, 1u}) {
        if (!(test.pending_mask & (1u << (id - 1)))) continue;
        const auto node = pending_nodes + id * 16;
        c.put32(node, pending(id));
        c.put32(node + 8, next);
        next = node;
    }
    c.put32(pending_list + 4, next);
    return s;
}

void original(State& s, std::uint32_t entry) {
    const bool previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    require(transfer_manager_reference(s), "Missing offer contract reference");
    while (s.pc != stop) {
        if (!dispatch_probe && (s.pc == regional || s.pc == gate)) original(s, s.pc);
        else if (s.pc == regional || s.pc == gate || !transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    switch (entry) {
    case remove_offer: remove_offer_pending_contracts(c, c.r(4)); break;
    case forget_offer: forget_offer_approaches(c, c.r(3), c.r(4)); break;
    case mark_offer: mark_offer_contracts_for_withdrawal(c, c.r(4)); break;
    case test_offer: offer_requires_regional_approval(c, c.r(3), c.r(4)); break;
    default: throw std::runtime_error("Unknown offer contract entry");
    }
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    production_route = true;
    s.pc = entry;
    if (dispatch_probe) s.budget = s.executed;
    run_until(s, return_sentinel);
    production_route = false;
}

unsigned comparisons = 0;
void compare(State& s, std::uint32_t entry, RoutineRunner first, RoutineRunner second) {
    const auto differences = compare_routine_call(s, entry, first, second,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& line : differences) std::cerr << line << '\n';
    require(differences.empty(), "Offer contract case " + std::to_string(comparisons) + " at " + hex_address(entry));
    ++comparisons;
}

void check(std::uint32_t entry, const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    auto s = fixture_state(entry, test);
    auto control = clone(*s);
    original(*control, entry);
    Cpu c(*control);
    const auto extras = test.no_list ? 0 : test.extras;
    if (entry == remove_offer) {
        require(c.u8(offer + 0x10) == 0, "Original primary offer flag");
        for (unsigned i = 0; i < 2; ++i)
            require(c.u8(secondary_rows + i * 8 + 4) == (i < extras ? 0 : 7), "Original secondary offer flag");
    } else if (entry == forget_offer) {
        require(c.u32(globals + 12) == (test.primary ? contracts : 0), "Original shortlist contract cache");
        require(c.u32(buyer_rows + 0x18) == 0xffffffff, "Original buyer shortlist approach");
        for (unsigned i = 0; i < 2; ++i)
            require(c.u32(seller_rows + i * 0x2c + 0x18) == (test.primary && i < extras ? 0xffffffffu : 44u),
                    "Original seller shortlist approach");
    } else if (entry == mark_offer) {
        require(c.u32(globals + 12) == (test.primary ? contracts : 0), "Original withdrawal contract cache");
        for (unsigned id = 1; id <= 3; ++id) {
            const bool marked = (test.pending_mask & (1u << (id - 1))) &&
                                (id == 1 || (test.primary && id - 1 <= extras));
            require(c.u8(pending(id) + 0x64) == (marked ? 4 : 1), "Original withdrawal status");
        }
    } else {
        bool expected = test.buyer_restricts && !(test.exempt_mask & 1);
        if (test.seller_restricts && test.primary)
            for (unsigned i = 0; i < extras; ++i) expected |= !(test.exempt_mask & (2u << i));
        require(c.r(3) == expected, "Original offer regional decision");
        if (dispatch_probe) require(calls(c, gate) == expected, "Original approval gate call count");
    }
    compare(*s, entry, original, selected);
}

void sequence(State& s, RoutineRunner run) {
    Cpu c(s);
    for (const auto entry : {test_offer, mark_offer, forget_offer, remove_offer}) {
        c.set({{3, manager}, {4, offer}});
        run(s, entry);
    }
    require(c.u8(offer + 0x10) == 0 && c.u8(secondary_rows + 4) == 0 &&
            c.u32(buyer_rows + 0x18) == 0xffffffff, "Combined offer cleanup state");
}
void original_sequence(State& s, std::uint32_t) { sequence(s, original); }
void production_sequence(State& s, std::uint32_t) { sequence(s, production); }

void restore_withdrawn_contract(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put8(pending(1) + 0x64, 1);
}

void check_negative_control() {
    active = {};
    auto s = fixture_state(mark_offer, active);
    const auto differences = compare_routine_call(*s, mark_offer, original, restore_withdrawn_contract,
        RoutineChecker::result_r3, false, true);
    require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 30006964"); }),
            "The offer contract comparison missed an altered withdrawal marker");
}
}

int cm::aot::transfer_manager::checks::offer_contracts(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check(remove_offer, {.pending_mask = 0, .real_dependencies = true}, production);
            check(forget_offer, {.real_dependencies = true}, production);
            check(mark_offer, {.pending_mask = 5, .real_dependencies = true}, production);
            check(test_offer, {.seller_restricts = true, .real_dependencies = true}, production);
            active = {.pending_mask = 0, .real_dependencies = true};
            auto s = fixture_state(test_offer, active);
            compare(*s, test_offer, original_sequence, production_sequence);
            dispatch_probe = nullptr;
            check(test_offer, {.real_dependencies = true}, production);
            std::cout << comparisons << " offer contract production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown offer contract check mode");
        check(remove_offer, {});
        check(remove_offer, {.no_list = true});
        check(remove_offer, {.extras = 0});
        check(forget_offer, {});
        check(forget_offer, {.no_list = true, .primary = false});
        check(forget_offer, {.primary = false});
        check(mark_offer, {});
        check(mark_offer, {.no_list = true});
        check(mark_offer, {.pending_mask = 4});
        check(mark_offer, {.primary = false});
        check(test_offer, {});
        check(test_offer, {.no_list = true});
        check(test_offer, {.primary = false});
        check(test_offer, {.buyer_restricts = true});
        check(test_offer, {.seller_restricts = true});
        check(test_offer, {.exempt_mask = 2, .seller_restricts = true});
        check(test_offer, {.exempt_mask = 7, .buyer_restricts = true, .seller_restricts = true});
        constexpr auto seed = 0x544f434eu;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 24; ++i) {
            const auto extras = unsigned(random() % 3), pending_mask = unsigned(random() % 8);
            const bool primary = random() & 1;
            check(mark_offer, {.extras = extras, .pending_mask = pending_mask, .primary = primary});
        }
        check_negative_control();
        std::cout << comparisons << " offer contract comparisons (seed " << seed << ") and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
