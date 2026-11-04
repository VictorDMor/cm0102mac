#include "transfer_manager_permissions.hpp"
#include "transfer_manager_callbacks.hpp"
#include "transfer_manager_collections.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::semantic_io_checks;
using namespace cm::aot::guest;

constexpr std::uint32_t regional = 0x103ecef0, nation_record = 0x103f8380, visible = 0x103f5e50,
    collect = 0x103eaea0, filter = 0x103fb390, compare_id = 0x10411c80;
constexpr std::uint32_t manager = fixture, people = fixture + 0x1000, person = people + 0x70,
    clubs = fixture + 0x2000, buyer = clubs + 0x248, nation = fixture + 0x2800,
    globals = fixture + 0x3000, contracts = globals + 0x100, primary = globals + 0x200,
    pending = globals + 0x400, approval_table = fixture + 0x4000, comparator = fixture + 0x4100,
    status_table = fixture + 0x5000, pool = fixture + 0x6000, message = fixture + 0x8000,
    viewers = fixture + 0x9000, viewer = fixture + 0xc000, offers = fixture + 0xd000,
    row = fixture + 0xd100, node = fixture + 0xd200, country_rows = fixture + 0xe000, trace = fixture + 0xf000;

struct Case {
    std::uint32_t mode = 0, staff_count = 8;
    std::int8_t kind = 2, pending_kind = 5;
    std::uint8_t flags = 0x20, nation_type = 14, contract_flag_bits = 0;
    int country_record_index = 0;
    bool null_person = false, null_club = false, has_nation = true, exempt = false,
         has_primary = true, approval_found = true, has_viewer = true, has_offer = true,
         offer_open = false, controls_buyer = false, controls_seller = false, related = false,
         managed_club = true, has_pending = true, second_contract_gone = false;
};
Case active;
bool original_route = false, production_route = false;

bool owned_dependency(std::uint32_t entry) { return entry == collect || entry == filter || entry == compare_id; }
void original(State&, std::uint32_t);
void modern(State&, std::uint32_t);

void trace_call(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 24, "Permission trace overflow");
    c.put32(trace, count + 1);
    auto cursor = trace + 4 + count * 128;
    const auto add = [&](std::uint32_t value) { c.put32(cursor, value); cursor += 4; };
    add(c.s.pc);
    add(c.lr());
    for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 21; r <= 31; ++r) add(c.r(r));
    add(c.cr(0));
    add(c.s.r<1>(reg::xer_ca));
}

struct LeafTrace final : MemoryObserver {
    explicit LeafTrace(State& state) : s(state) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t address, const std::uint8_t*, std::size_t) override {
        if (s.pc != 0x10738c00) return;
        Cpu c(s);
        if (address == c.r(3) + 4) {
            require(c.lr() == 0x103f5e88 && c.r(3) == c.sp() + 0x38, "Visibility list constructor");
            trace_call(c);
        }
    }
    void read(std::uint32_t address, const std::uint8_t*, unsigned size) override {
        if (address != toc - 0x5b20) return;
        Cpu c(s);
        require(size == 4 && s.pc == 0x10768810 && c.lr() == 0x103ecf54, "Approval diagnostic version");
        trace_call(c);
    }
    State& s;
};

bool observe(State& s) {
    Cpu c(s);
    const auto entry = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    constexpr auto list = stack - 0x80 + 0x38;
    if (owned_dependency(entry)) {
        if (entry == collect)
            require(a == manager && b == person && d == list && c.r(6) == 0 && back == 0x103f5f5c,
                    "Visibility collection arguments");
        else if (entry == filter)
            require(a == manager && b == row, "Visibility filter arguments");
        else
            require(a == stack - 0x90 + 0x40 && b >= approval_table && b < approval_table + 48 &&
                    (b - approval_table) % 16 == 0, "Approval comparator arguments");
        trace_call(c);
        if (original_route) original(s, entry);
        else if (production_route) return false;
        else modern(s, entry);
        return true;
    }
    std::uint32_t answer = 0;
    switch (entry) {
    case 0x100ca6a0:
        require(back == 0x103ecfac && a == person && b == 0, "National exemption arguments");
        answer = active.exempt;
        break;
    case 0x10082f20:
        require(a == contracts && b == person && (back == 0x103ecfd4 || back == 0x103f5fdc || back == 0x103f604c),
                "Permission contract lookup");
        answer = active.has_primary && !(active.second_contract_gone && back == 0x103f604c) ? primary : 0;
        break;
    case 0x10775600:
        require(back == 0x103ed060 && a == stack - 0x90 + 0x40 && c.u32(a) == 1 &&
                b == approval_table && d == 3 && c.r(6) == 16 && c.r(7) == comparator,
                "Approval search contract");
        trace_call(c);
        return false; // The real bsearch invokes the original or handwritten comparator.
    case 0x1030fbf0:
        require(back == 0x103f5f44 && a == globals + 0x500 && b == person && d == viewer,
                "Viewing relationship arguments");
        answer = active.related;
        break;
    case 0x1014c490:
        require(b == 0 && d == 1 && (back == 0x103f5fa4 ? a == buyer :
                back == 0x103f60a4 && a == (active.second_contract_gone ? 0 : clubs)),
                "Viewing club control arguments");
        answer = a == buyer ? active.controls_buyer : a && active.controls_seller;
        break;
    case 0x1007f720:
        require(back == 0x103f612c && a == contracts && b == 1 && d == 8, "Viewing pending contract arguments");
        answer = active.has_pending ? pending : 0;
        break;
    case 0x10738d10:
        require(a == list && b == row + 0x28 && d == 4 && c.u32(b) == 0, "Visibility offer-list append");
        c.put16(list, 1);
        c.put32(list + 4, node);
        c.put32(list + 8, node);
        c.put32(node, node + 0x10);
        c.put32(node + 0x10, 0);
        answer = node + 0x10;
        break;
    case 0x10738e00:
        require(a == list && back == 0x103f60fc, "Visibility iterator arguments");
        trace_call(c);
        return false;
    case 0x10738c20:
        require(a == list && b == 0xffffffff, "Visibility list destructor arguments");
        c.put32(a + 4, 0);
        answer = a;
        break;
    case 0x10739890:
        require(back == 0x103ecf50 && a == pool + 0x37 && b == 0 && d == 0 &&
                c.r(6) == c.sp() + 0x50 && c.r(7) == c.sp() + 0x38, "Approval diagnostic split");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        break;
    case 0x1077b720:
        require(back == 0x103ecf70 && a == message && b == pool + 0x6ff && d == globals + 0x40 &&
                c.r(6) == c.sp() + 0x50 && c.r(7) == c.sp() + 0x38 && c.r(8) == 0x1c22,
                "Approval diagnostic format");
        c.put32(a, c.r(8));
        break;
    case 0x10769b60:
        require(back == 0x103ecf80 && a == pool + 0x70c && b == message && d == 0 &&
                c.u32(message) == 0x1c22, "Approval diagnostic display");
        break;
    default: return false;
    }
    trace_call(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xa8000000 + r);
    c.set_cr(0, 4);
    c.set(3, answer);
    c.finish();
    return true;
}

std::unique_ptr<State> make_state(std::uint32_t entry, Case test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0x10000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->memory.map(toc - 0x8000, 0x4000);
    if (test.null_person) s->memory.map(0, 4);
    s->budget = 100000;
    s->pc = entry;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, std::uint8_t(i));
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, test.null_person ? 0 : person}, {5, test.mode}, {6, test.mode}});
    if (entry == nation_record) c.set(4, buyer);
    if (entry == regional) c.set(5, test.null_club ? 0 : buyer);
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7be4, globals);
    c.put32(toc - 0x7be0, globals + 4);
    c.put32(toc - 0x7be8, globals + 8);
    c.put32(toc - 0x7b7c, globals + 12);
    c.put32(toc - 0x7b78, globals + 16);
    c.put32(toc - 0x7bb0, viewers);
    c.put32(toc - 0x7b1c, globals + 20);
    c.put32(toc - 0x7bac, globals + 24);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x5b20, globals + 0x40);
    c.put32(toc - 0x4ba8, comparator);
    c.put32(toc - 0x5020, pool);
    c.put32(globals, test.staff_count);
    c.put32(globals + 4, people);
    c.put32(globals + 8, clubs);
    c.put32(globals + 12, contracts);
    c.put32(globals + 20, globals + 0x500);
    c.put8(globals + 24, 1);
    c.put32(person, 1);
    if (test.null_person) c.put32(0, 1);
    c.put32(clubs, 7);
    c.put32(buyer, 8);
    c.put32(buyer + 0x2c, test.has_nation ? nation : 0);
    c.put8(nation + 0x118, test.nation_type);
    c.put8(primary + 0x38, std::uint32_t(std::uint8_t(test.kind)) * 4 + test.contract_flag_bits);
    c.put8(pending + 0x38, std::uint32_t(std::uint8_t(test.pending_kind)) * 4);
    c.put32(manager, offers);
    c.put32(manager + 4, test.has_offer ? 1 : 0);
    c.put32(manager + 0x9a0, status_table);
    c.put32(manager + 0xa6c, approval_table);
    c.put32(manager + 0xa70, 3);
    c.put8(status_table + 10, test.flags);
    c.put32(comparator, compare_id);
    c.put32(comparator + 4, toc);
    c.put32(approval_table, 0);
    c.put32(approval_table + 16, test.approval_found ? 1 : 2);
    c.put32(approval_table + 32, 4);
    for (int i = 0; i < 27; ++i) {
        const auto at = country_rows + std::uint32_t(i) * 16;
        c.put32(manager + 0x9e0 + std::uint32_t(i) * 4, at);
        c.put32(at + 0xc, test.country_record_index == i ? nation : nation + 0x100);
    }
    c.put32(viewers + 4, test.has_viewer ? viewer : 0);
    c.put32(viewer + 0x24, test.managed_club ? buyer : 0);
    c.put32(offers, row);
    c.put32(row + 8, 1);
    c.put32(row + 0xc, 1);
    c.put8(row + 0x35, test.offer_open);
    return s;
}

void original(State& s, std::uint32_t entry) {
    const auto previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    require(transfer_manager_reference(s), "Missing permission reference entry");
    while (s.pc != stop) {
        if (owned_dependency(s.pc) && !dispatch_probe) original(s, s.pc);
        else if (owned_dependency(s.pc) || !transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void modern(State& s, std::uint32_t entry) {
    Cpu c(s);
    const auto a = c.r(3), b = c.r(4), d = c.r(5), e = c.r(6);
    using namespace transfer_manager;
    switch (entry) {
    case regional: requires_regional_transfer_approval(c, a, b, d, e); break;
    case nation_record: find_nation_transfer_record(c, a, b); break;
    case visible: has_visible_offer(c, a, b, d); break;
    case collect: collect_staff_offers(c, a, b, d, e); break;
    case filter: offer_is_filtered(c, b); break;
    case compare_id: compare_optional_index(c, a, b); break;
    default: throw std::runtime_error("Unknown transfer permission entry");
    }
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    production_route = true;
    s.pc = entry;
    // This route uses only scripted boundaries or the real six-step list
    // initializer. Any generated transfer-manager instructions exceed it.
    s.budget = entry == visible ? 6 : 0;
    run_until(s, return_sentinel);
    require(s.executed == s.budget, "Wrong normal permission route");
}

void direct_production(State& s, std::uint32_t entry) {
    require(!dispatch_probe && !routine_checker, "The direct permission route has checker hooks");
    s.pc = entry;
    run_until(s, return_sentinel);
}

unsigned comparisons = 0;
void check(std::uint32_t entry, Case test, std::uint32_t expected, RoutineRunner selected = modern) {
    active = test;
    auto s = make_state(entry, test);
    auto control = clone(*s);
    LeafTrace leaves(*control), child_leaves(*s);
    control->memory.date_observer = &leaves;
    s->memory.date_observer = &child_leaves;
    original(*control, entry);
    Cpu c(*control);
    require(c.r(3) == expected && c.u8(globals + 24) == !(entry == regional && test.null_person),
            "Unexpected original permission result in case " + std::to_string(comparisons));
    if (entry == regional && !std::int8_t(test.mode) && !test.null_person && !test.null_club && test.has_nation &&
        test.nation_type == 14 && !test.exempt && test.has_primary && test.kind == 2 &&
        test.staff_count > 1 && (test.flags & 0x20)) {
        bool compared_id = false;
        for (unsigned n = 0; n < c.u32(trace); ++n)
            compared_id |= c.u32(trace + 4 + n * 128) == compare_id;
        require(compared_id, "The real approval search did not reach its comparator");
    }
    const auto differences = compare_routine_call(*s, entry, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& line : differences) std::cerr << line << '\n';
    require(differences.empty(), "Transfer permission comparison " + std::to_string(comparisons) + " " + hex_address(entry));
    ++comparisons;
}
}

int cm::aot::transfer_manager::checks::permissions(int argc, char** argv) {
    try {
        transfer_manager::install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check(regional, {.mode = 1}, 1, production);
            check(nation_record, {.country_record_index = 26}, country_rows + 26 * 16, production);
            check(visible, {.flags = 1}, 1, production);
            dispatch_probe = nullptr;
            // A viewer outside the managed-staff band takes the real
            // relationship service's early return. No offers need allocation.
            check(visible, {.mode = 1, .staff_count = 32, .has_offer = false, .managed_club = false},
                  0, direct_production);
            std::cout << comparisons << " transfer permission production routes returned\n";
            return 0;
        }
        require(argc == 1, "Unknown transfer permission check mode");
        check(regional, {.null_club = true}, 0);
        check(regional, {.has_nation = false}, 0);
        check(regional, {.null_person = true}, 0);
        check(regional, {.nation_type = 13}, 0);
        check(regional, {.exempt = true}, 0);
        check(regional, {.mode = 1}, 1);
        check(regional, {.has_primary = false}, 1);
        check(regional, {.kind = 5}, 1);
        check(regional, {.kind = -1}, 1);
        check(regional, {.kind = -1, .contract_flag_bits = 2}, 1);
        check(regional, {.staff_count = 1}, 1);
        check(regional, {.flags = 0}, 1);
        check(regional, {.mode = 0x100}, 0);
        check(regional, {.approval_found = false}, 1);
        for (int index : {-1, 0, 13, 26}) {
            check(nation_record, {.country_record_index = index}, index < 0 ? 0 : country_rows + std::uint32_t(index) * 16);
        }
        check(visible, {.staff_count = 1}, 0);
        check(visible, {.flags = 1}, 1);
        check(visible, {}, 0);
        check(visible, {.flags = 1, .null_person = true}, 1);
        check(visible, {.mode = 1, .flags = 1, .has_viewer = false}, 1);
        check(visible, {.mode = 1, .has_viewer = false}, 0);
        check(visible, {.mode = 1, .offer_open = true}, 1);
        check(visible, {.mode = 1, .controls_buyer = true}, 1);
        check(visible, {.mode = 1, .controls_seller = true}, 1);
        check(visible, {.mode = 1, .related = true}, 1);
        check(visible, {.mode = 1, .has_primary = false, .has_pending = false}, 0);
        check(visible, {.mode = 1, .kind = 5, .has_pending = false}, 0);
        check(visible, {.mode = 1, .pending_kind = 2}, 0);
        check(visible, {.mode = 1, .has_offer = false, .managed_club = false}, 0);
        check(visible, {.mode = 1, .has_offer = false}, 1);
        check(visible, {.mode = 1, .has_pending = false, .second_contract_gone = true}, 0);
        constexpr std::uint32_t seed = 0x54504552;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 24; ++i) {
            const auto flags = std::uint8_t(random());
            check(visible, {.mode = 0x100, .flags = flags}, flags & 1);
        }
        std::cout << comparisons << " transfer permission comparisons (seed " << seed << ")\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
