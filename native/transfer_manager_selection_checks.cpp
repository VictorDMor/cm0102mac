#include "transfer_manager_selection.hpp"
#include "transfer_manager_collections.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::semantic_io_checks;
using namespace cm::aot::guest;

constexpr std::uint32_t append_registration = 0x103e6290, count_registrations = 0x103e6360,
    has_registration = 0x103e6470, best_offer = 0x103f4240, special_offer = 0x103f43b0,
    division_gate = 0x10411e10, staff_offers = 0x103eaea0, club_offers = 0x103eb020,
    filter = 0x103fb390;
constexpr std::uint32_t manager = fixture, people = fixture + 0x1000, person = people + 0x70,
    clubs = fixture + 0x2000, peer = clubs + 0x248, globals = fixture + 0x3000,
    records = fixture + 0x4000, statuses = fixture + 0x5000, slots = fixture + 0x6000,
    chain = fixture + 0x7000, membership = fixture + 0x7100, nodes = fixture + 0x8000,
    details = fixture + 0x9000, output = fixture + 0x9100, pool = fixture + 0xa000,
    trace = fixture + 0xb000, scratch_nodes = fixture + 0xe000, registration = manager + 0xa5c;
constexpr std::uint32_t contract_manager = globals + 0x100, contract = globals + 0x200,
    nation = globals + 0x300, division = globals + 0x400, message = globals + 0x800;

struct Case {
    bool registered = false, null_club = false, has_peer = true, has_registration_rows = true,
         cached = true, linked = true, matches = true, alternative_club = false, has_detail = true,
         wants_detail = true, restricted_nation = true, has_nation = true, has_division = true,
         permitted_division = true, club_mismatch = false, via_club = false,
         empty_offers = false, first_slot = true;
    std::uint8_t first_state = 2, second_state = 2, third_state = 2, cached_count = 1;
    std::uint32_t staff_id = 1, first_value = 10, second_value = 20, third_value = 20;
};
Case active;
bool original_route = false, production_route = false;

bool owned_callee(std::uint32_t entry) { return entry == staff_offers || entry == club_offers || entry == filter; }
void original(State&, std::uint32_t);
void modern(State&, std::uint32_t);

void trace_call(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 63, "Selection trace overflow");
    c.put32(trace, count + 1);
    auto cursor = trace + 4 + count * 128;
    const auto add = [&](std::uint32_t value) { c.put32(cursor, value); cursor += 4; };
    add(c.s.pc);
    add(c.lr());
    for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 24; r <= 31; ++r) add(c.r(r));
    add(c.cr(0));
}

void append_list_node(Cpu& c, std::uint32_t list, std::uint32_t node,
                      std::uint32_t source, std::uint32_t width) {
    const auto tail = c.u32(list + 8);
    c.put32(node, node + 0x10);
    c.put32(node + 8, 0);
    c.put32(node + 0xc, tail);
    for (unsigned i = 0; i < width; ++i) c.put8(node + 0x10 + i, c.u8(source + i));
    if (tail) c.put32(tail + 8, node);
    else c.put32(list + 4, node);
    c.put32(list + 8, node);
    c.put16(list, c.u16(list) + 1);
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4);
    if (owned_callee(target)) {
        require(a == manager && (target == filter ? b >= records && b < records + 0x300 : b == person),
                "Owned selection call arguments");
        trace_call(c);
        if (original_route) original(s, target);
        else if (production_route) return false;
        else modern(s, target);
        return true;
    }
    std::uint32_t answer = 0;
    switch (target) {
    case 0x10083110:
        require(back == 0x103e62c4 && a == contract_manager && b == person && c.r(7) == globals,
                "Registration contract check arguments");
        answer = active.registered ? contract : 0;
        break;
    case 0x100ce320:
        require(back == 0x103e63f8 && a == clubs && b == 0 && c.r(5) == 1, "Associated-club arguments");
        answer = active.has_peer ? peer : 0;
        break;
    case 0x10082f20:
        require((back == 0x103eb09c || back == 0x103eb0dc) && a == contract_manager && b == person,
                "Special-offer contract lookup arguments");
        answer = contract;
        break;
    case 0x10738e00:
        require(a == registration || a == chain || a == membership || a == stack - 0x60 + 0x38,
                "Selection iterated an unexpected list");
        trace_call(c);
        if (production_route && !original_route) {
            require(s.executed == s.budget, "Selection consumed budget before its list dependency");
            s.budget = s.executed + 100;
            require(dispatch_page(s), "Missing real list iterator");
            if (s.pc != back) run_until(s, back);
            s.budget = s.executed;
            return true;
        }
        return false;
    case 0x10738d10: {
        const auto width = c.r(5);
        if (a == registration) {
            require(back == 0x103e6344 && b == stack - 0x90 + 0x40 && width == 12 &&
                    c.u32(b) == active.staff_id && c.u8(b + 4) == 0x7f && c.u32(b + 8) == 0x12345678,
                    "Registration publication ABI and row");
        } else {
            require(a == stack - 0x60 + 0x38 && width == 4 && c.u32(b) < 3, "Special-offer list append");
        }
        auto index = c.u32(globals + 0x18);
        require(index < 12, "Selection fixture list overflow");
        const auto node = scratch_nodes + index * 0x20;
        c.put32(globals + 0x18, index + 1);
        append_list_node(c, a, node, b, width);
        answer = node + 0x10;
        break;
    }
    case 0x10738ca0:
    case 0x10738c20:
        require(a == stack - 0x60 + 0x38 && (target == 0x10738ca0 ? back == 0x103f4480 : b == 0xffffffff),
                "Special-offer list reset or destruction ABI");
        c.put32(a + 4, 0);
        if (target == 0x10738ca0) {
            c.put16(a, 0);
            c.put32(a + 8, 0);
            c.put32(a + 0xc, 0);
        }
        answer = a;
        break;
    case 0x10416710:
        require(back == 0x103f4314 && a >= records && a < records + 0x300 &&
                b == 0 && c.r(5) == 0 && c.r(6) == 0 && c.r(7) == 1, "Qualifying offer valuation ABI");
        answer = c.u32(a + 0x20);
        break;
    case 0x10739890:
        require(a == pool + 0x37 && b == 0 && c.r(5) == 0 &&
                c.r(6) == c.sp() + (back == 0x103e62e4 ? 0x4c : 0x40) && c.r(7) == c.sp() + 0x38,
                "Registration diagnostic split");
        c.put32(c.r(6), 0x53544146);
        c.put32(c.r(7), 0x43505000);
        break;
    case 0x1077b720:
        require(a == message && b == pool + 0x6ff && c.r(5) == globals + 0x800 &&
                c.r(8) == (back == 0x103e6304 ? 0xbe6u : 0xc04u), "Registration diagnostic format");
        c.put32(a, c.r(8));
        break;
    case 0x10769b60:
        require(a == pool + 0x70c && b == message && c.r(5) == 0 && c.u32(message) != 0,
                "Registration diagnostic display");
        break;
    default:
        return false;
    }
    trace_call(c);
    c.set({{3, answer}, {4, 0x99000004}, {5, 0x99000005}});
    c.set_cr(0, 4);
    c.finish();
    return true;
}

struct LeafReads final : MemoryObserver {
    explicit LeafReads(State& s) : state(s) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t address, const std::uint8_t*, std::size_t) override {
        if (state.pc == 0x10738c00) {
            Cpu c(state);
            if (address == c.r(3) + 4) {
                require(c.lr() == 0x103f43e0 && c.r(3) == c.sp() + 0x38, "Candidate list constructor ABI");
                trace_call(c);
            }
        }
    }
    void cleared(std::uint32_t, std::uint64_t) override {}
    void read(std::uint32_t address, const std::uint8_t*, unsigned) override {
        if (address != toc - 0x5b20) return;
        Cpu c(state);
        require(state.pc == 0x10768810 && (c.lr() == 0x103e62e8 || c.lr() == 0x103e63a8),
                "Registration diagnostic version accessor");
        trace_call(c);
    }
    State& state;
};

void original(State& s, std::uint32_t entry) {
    const bool previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    require(transfer_manager_reference(s), "Missing transfer selection reference");
    while (s.pc != stop) {
        if (owned_callee(s.pc) && !dispatch_probe) original(s, s.pc);
        else if (owned_callee(s.pc) || !transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void modern(State& s, std::uint32_t entry) {
    Cpu c(s);
    const auto a = c.r(3), b = c.r(4), d = c.r(5);
    using namespace transfer_manager;
    switch (entry) {
    case append_registration: append_staff_registration(c, a, b, d, c.r(6)); break;
    case count_registrations: count_staff_registrations_for_club(c, a, b); break;
    case has_registration: has_staff_registration(c, a, b); break;
    case best_offer: best_qualifying_offer(c, a, b, d); break;
    case special_offer: find_special_offer(c, a, b, d); break;
    case division_gate: club_passes_division_gate(c, a); break;
    case staff_offers: collect_staff_offers(c, a, b, d, c.r(6)); break;
    case club_offers: collect_primary_club_offers(c, a, b, d); break;
    case filter: offer_is_filtered(c, b); break;
    default: throw std::runtime_error("Unknown selection check entry");
    }
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    production_route = true;
    s.pc = entry;
    s.budget = entry == special_offer ? 6 : 0; // The real list constructor is a six-instruction leaf.
    run_until(s, return_sentinel);
    require(s.executed == s.budget, "Unexpected original selection steps in production");
}

void direct_production(State& s, std::uint32_t entry) {
    require(dispatch_probe == nullptr && routine_checker == nullptr, "Direct selection route has a checker hook");
    s.pc = entry;
    run_until(s, return_sentinel);
}

std::unique_ptr<State> make_state(std::uint32_t entry, Case test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x69);
    s->memory.map(fixture, 0x10000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->memory.map(toc - 0x8000, 0x4000);
    s->pc = entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, std::uint8_t(i));
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, entry == division_gate ? clubs : manager},
           {4, entry == count_registrations ? (test.null_club ? 0 : clubs) : person},
           {5, entry == best_offer ? clubs : entry == append_registration ? 0x17f : test.wants_detail ? output : 0},
           {6, 0x12345678}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7b7c, globals);
    c.put32(toc - 0x7b78, globals + 4);
    c.put32(toc - 0x7be8, globals + 8);
    c.put32(toc - 0x7be0, globals + 0xc);
    c.put32(toc - 0x7be4, globals + 0x10);
    c.put32(toc - 0x7bac, globals + 0x20);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x5b20, message);
    c.put32(toc - 0x75b0, globals + 0x30);
    c.put32(toc - 0x6a74, globals + 0x34);
    c.put32(globals, contract_manager);
    c.put32(globals + 8, clubs);
    c.put32(globals + 0xc, people);
    c.put32(globals + 0x10, 8);
    c.put8(globals + 0x20, 1);
    c.put32(globals + 0x30, 18);
    c.put32(globals + 0x34, 24);
    c.put32(person, test.staff_id);
    c.put32(clubs, 7);
    c.put32(clubs + 0x2c, test.has_nation ? nation : 0);
    c.put32(clubs + 0x30, test.has_division ? division : 0);
    c.put32(nation + 0x68, test.restricted_nation ? 18 : 19);
    c.put32(division, test.permitted_division ? 24 : 25);
    c.put32(people + 0x70 + 0x24, clubs);
    c.put32(people + 0xe0 + 0x24, peer);
    c.put32(manager, slots);
    c.put32(manager + 4, test.empty_offers ? 0 : 3);
    c.put32(manager + 0x9a0, statuses);
    c.put32(manager + 0xa50, details);
    c.put32(manager + 0xa58, 2);
    c.put32(details + 8, 99);
    c.put32(details + 12 + 8, test.has_detail ? std::uint32_t(test.via_club) : 98);
    c.put32(output, 0xffffffff);
    c.put8(statuses + test.staff_id * 10, test.cached ? 0x10 : 0);
    c.put8(statuses + test.staff_id * 10 + 1, test.cached ? 1 : 0);
    c.put8(statuses + test.staff_id * 10 + 2, test.cached_count);
    if (test.has_registration_rows) {
        for (unsigned i = 1; i <= 3; ++i) {
            c.put32(fixture + 0xff00, i);
            append_list_node(c, registration, nodes + i * 0x20, fixture + 0xff00, 4);
        }
    }
    const std::array states{test.first_state, test.second_state, test.third_state};
    const std::array values{test.first_value, test.second_value, test.third_value};
    for (unsigned i = 0; i < 3; ++i) {
        const auto row = records + i * 0x100;
        c.put32(slots + i * 4, i || test.first_slot ? row : 0);
        c.put32(row + 8, test.matches && !test.club_mismatch ? 7 : 8);
        c.put32(row + 0xc, i == 0 && test.matches ? test.staff_id : test.staff_id + 1);
        c.put32(row + 0x28, i);
        c.put32(row + 0x20, values[i]);
        c.put8(row + 0x34, states[i]);
        c.put8(row + 0x35, test.alternative_club);
        c.put32(fixture + 0xff00, i);
        append_list_node(c, chain, nodes + 0x100 + i * 0x20, fixture + 0xff00, 4);
    }
    if (test.linked) c.put32(records, chain);
    c.put32(records + 4, membership);
    if (test.via_club) c.put32(records + 0x100 + 4, membership);
    c.put32(fixture + 0xff00, test.staff_id);
    append_list_node(c, membership, nodes + 0x200, fixture + 0xff00, 4);
    return s;
}

unsigned comparisons = 0;
void check(std::uint32_t entry, Case test, std::uint32_t expected, std::uint32_t detail = 0xffffffff,
           RoutineRunner selected = modern) {
    active = test;
    auto s = make_state(entry, test);
    auto control = clone(*s);
    LeafReads original_leaves(*control), child_leaves(*s);
    control->memory.date_observer = &original_leaves;
    s->memory.date_observer = &child_leaves;
    original(*control, entry);
    Cpu c(*control);
    require(c.r(3) == expected && c.u32(output) == detail, "Unexpected original selection result at " +
            hex_address(entry) + " case " + std::to_string(comparisons) + " result " + hex_address(c.r(3)));
    if (entry == append_registration) {
        const auto expected_count = (test.has_registration_rows ? 3 : 0) + !test.registered;
        require(c.u16(registration) == expected_count && c.u8(globals + 0x20) == !test.registered,
                "Registration publication or assertion result");
    }
    const auto differences = compare_routine_call(*s, entry, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Transfer selection comparison " + std::to_string(comparisons) + " " + hex_address(entry));
    ++comparisons;
}
}

int cm::aot::transfer_manager::checks::selection(int argc, char** argv) {
    try {
        transfer_manager::install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check(append_registration, {}, scratch_nodes + 0x10, 0xffffffff, production);
            check(count_registrations, {}, 2, 0xffffffff, production);
            check(has_registration, {}, 1, 0xffffffff, production);
            check(best_offer, {}, records + 0x100, 0xffffffff, production);
            check(special_offer, {.via_club = true, .second_state = 19}, 1, details + 12, production);
            dispatch_probe = nullptr;
            check(division_gate, {}, 1, 0xffffffff, production);
            check(special_offer, {.empty_offers = true, .cached_count = 0}, 0, 0xffffffff, direct_production);
            std::cout << comparisons << " transfer selection production routes returned\n";
            return 0;
        }
        require(argc == 1, "Unknown transfer selection check mode");
        check(append_registration, {}, scratch_nodes + 0x10);
        check(append_registration, {.registered = true}, globals + 0x20);
        check(append_registration, {.has_registration_rows = false}, scratch_nodes + 0x10);
        check(count_registrations, {}, 2);
        check(count_registrations, {.has_peer = false}, 1);
        check(count_registrations, {.null_club = true}, 0);
        check(count_registrations, {.has_registration_rows = false}, 0);
        check(has_registration, {}, 1);
        check(has_registration, {.staff_id = 3}, 1);
        check(has_registration, {.staff_id = 4}, 0);
        check(has_registration, {.has_registration_rows = false}, 0);

        check(best_offer, {.cached = false}, 0);
        check(best_offer, {.linked = false}, records);
        check(best_offer, {.linked = false, .first_state = 4}, 0);
        check(best_offer, {.linked = false, .club_mismatch = true}, 0);
        check(best_offer, {.linked = false, .alternative_club = true, .club_mismatch = true}, records);
        check(best_offer, {.matches = false}, 0);
        check(best_offer, {.first_slot = false}, 0);
        check(best_offer, {}, records + 0x100);
        check(best_offer, {.second_state = 4, .third_state = 3}, records);
        check(best_offer, {.first_value = 0xffffffff, .second_value = 0xffffffff, .third_value = 0xffffffff}, 0);

        check(special_offer, {.cached = false}, 0);
        check(special_offer, {}, 0);
        check(special_offer, {.first_state = 0}, 0);
        check(special_offer, {.first_state = 19}, 1, details + 12);
        check(special_offer, {.has_detail = false, .first_state = 19}, 1);
        check(special_offer, {.wants_detail = false, .first_state = 19}, 1);
        check(special_offer, {.second_state = 19}, 0);
        check(special_offer, {.via_club = true, .second_state = 19}, 1, details + 12);
        check(special_offer, {.via_club = true, .first_state = 0, .second_state = 19, .cached_count = 2},
              1, details + 12);

        check(division_gate, {}, 1);
        check(division_gate, {.has_nation = false}, 1);
        check(division_gate, {.restricted_nation = false}, 1);
        check(division_gate, {.has_division = false}, 0);
        check(division_gate, {.permitted_division = false}, 0);
        constexpr std::uint32_t seed = 0x5453454c;
        std::mt19937 random(seed);
        for (unsigned n = 0; n < 24; ++n) {
            const auto a = std::uint32_t(random() % 100), b = std::uint32_t(random() % 100),
                       d = std::uint32_t(random() % 100);
            const auto winner = a >= b && a >= d ? records : b >= d ? records + 0x100 : records + 0x200;
            check(best_offer, {.first_value = a, .second_value = b, .third_value = d}, winner);
        }
        std::cout << comparisons << " transfer selection comparisons (seed " << seed << ")\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
