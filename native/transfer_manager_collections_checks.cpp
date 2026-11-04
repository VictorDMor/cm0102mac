#include "transfer_manager_collections.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::semantic_io_checks;
using namespace cm::aot::guest;

constexpr std::uint32_t staff_offers = 0x103eaea0, primary_club = 0x103eb020, secondary_club = 0x103eb1f0,
    club_offers = 0x103eb360, primary_slot = 0x103eb440, secondary_slot = 0x103eb470, offer_filter = 0x103fb390;
constexpr std::uint32_t manager = fixture, person = fixture + 0x1000, clubs = fixture + 0x2000,
    globals = fixture + 0x3000, records = fixture + 0x4000, staff_flags = fixture + 0x5000,
    primary = fixture + 0x6000, secondary = fixture + 0x6100,
    offer_chain = fixture + 0x7000, members = fixture + 0x7100, nodes = fixture + 0x7200,
    output = fixture + 0x8000, output_items = fixture + 0x8100, trace = fixture + 0x9000;
constexpr std::uint32_t contracts = globals + 0x100, contract = globals + 0x200;
constexpr std::uint32_t lookup_contract = 0x10082f20, next_item = 0x10738e00, append_item = 0x10738d10;
bool original_route = false, trace_calls = true;

struct Case {
    std::uint32_t staff_id = 1, staff_count = 8, mode = 0;
    std::uint8_t cached_count = 1, status = 0, stage = 0;
    bool primary_present = true, primary_staff = true, correct_club = true, has_contract = true,
         linked_offers = true, linked_members = true, member_present = true, wants_output = true,
         null_person = false, empty_primary = false, contract_disappears = false;
};
Case active;

void original(State&, std::uint32_t);

bool observe(State& s) {
    const auto target = s.pc;
    if (target != offer_filter && target != lookup_contract && target != next_item && target != append_item)
        return false;
    Cpu c(s);
    const auto back = c.lr(), first = c.r(3), second = c.r(4);
    if (target == offer_filter) {
        require(first == manager && second >= records && second < records + 0x600,
                "Offer filter received an invalid record");
    } else if (target == lookup_contract) {
        require(first == contracts && second == (active.null_person ? 0 : person),
                "Collection contract lookup arguments");
    } else if (target == next_item) {
        require(first == offer_chain || first == members, "Collection iterator received a different list");
    } else {
        require(first == output && c.r(5) == 4, "Collection append destination or width");
        require((second >= nodes && second < nodes + 0x200) ||
                (second >= records && second < records + 0x600 && (second & 0xff) == 0x28) ||
                second == c.sp() + 0x38, "Collection append source");
    }
    if (trace_calls) {
        const auto count = c.u32(trace);
        require(count < 48, "Too many collection dependency calls");
        c.put32(trace, count + 1);
        auto cursor = trace + 4 + count * 128;
        const auto add = [&](std::uint32_t word) { c.put32(cursor, word); cursor += 4; };
        add(target);
        add(back);
        for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
        for (unsigned r = 24; r <= 31; ++r) add(c.r(r));
        add(c.cr(0));
    }
    if (target == next_item) return false; // The real cursor advances.
    if (target == offer_filter) {
        if (original_route) original(s, target);
        else {
            transfer_manager::offer_is_filtered(c, second);
            c.finish();
        }
        return true;
    }
    if (target == lookup_contract) {
        const bool found = active.has_contract && !(active.contract_disappears && back == 0x103eb0dc);
        c.set({{3, found ? contract : 0}, {4, 0x9a000004}, {5, 0x9a000005}});
        c.set_cr(0, 4);
    } else {
        const auto count = c.u32(output);
        require(count < 8, "Collection fixture result list overflow");
        c.put32(output_items + count * 4, c.u32(second));
        c.put32(output, count + 1);
        c.set({{3, output}, {4, 0x9a000014}, {5, 0x9a000015}});
        c.set_cr(0, 8);
    }
    c.finish();
    return true;
}

void original(State& s, std::uint32_t entry) {
    const auto previous = original_route;
    original_route = true;
    const auto stop = Cpu(s).lr();
    s.pc = entry;
    require(transfer_manager_reference(s), "Missing transfer collection reference entry");
    while (s.pc != stop) {
        if (s.pc == offer_filter && s.pc != entry && !dispatch_probe) original(s, offer_filter);
        else if (s.pc == offer_filter && s.pc != entry) run_until(s, Cpu(s).lr());
        else if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void modern(State& s, std::uint32_t entry) {
    Cpu c(s);
    const auto a = c.r(3), b = c.r(4), d = c.r(5);
    using namespace transfer_manager;
    switch (entry) {
    case staff_offers: collect_staff_offers(c, a, b, d, c.r(6)); break;
    case primary_club: collect_primary_club_offers(c, a, b, d); break;
    case secondary_club: collect_secondary_club_offers(c, a, b, d); break;
    case club_offers: collect_offers_from_club(c, a, b, d); break;
    case primary_slot: primary_offer_slot(c, a, std::int32_t(b)); break;
    case secondary_slot: secondary_offer_slot(c, a, std::int32_t(b)); break;
    case offer_filter: offer_is_filtered(c, b); break;
    default: throw std::runtime_error("Unknown transfer collection entry");
    }
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    s.pc = entry;
    s.budget = 0;
    run_until(s, return_sentinel);
    require(s.executed == 0, "Collection used generated instructions on the direct path");
}

void make_list(Cpu& c, std::uint32_t list, std::uint32_t first,
               std::initializer_list<std::uint32_t> items) {
    c.put32(list + 4, first);
    unsigned index = 0;
    for (auto item : items) {
        const auto node = first + index * 0x10;
        c.put32(node, node + 0xc);
        c.put32(node + 8, ++index < items.size() ? node + 0x10 : 0);
        c.put32(node + 0xc, item);
    }
}

std::unique_ptr<State> make_state(std::uint32_t entry, Case test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x69);
    s->memory.map(fixture, 0x10000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->memory.map(toc - 0x8000, 0x1000);
    if (test.null_person || test.contract_disappears) s->memory.map(0, 4);
    s->pc = entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, std::uint8_t(i));
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    const auto subject = entry == club_offers ? clubs : entry == offer_filter ? records :
                         test.null_person ? 0 : person;
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, subject}, {5, test.wants_output ? output : 0}, {6, test.mode}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7be4, globals);
    c.put32(toc - 0x7b7c, globals + 4);
    c.put32(toc - 0x7b78, globals + 8);
    c.put32(toc - 0x7be8, globals + 0xc);
    c.put32(globals, test.staff_count);
    c.put32(globals + 4, contracts);
    c.put32(globals + 0xc, clubs);
    c.put32(person, test.staff_id);
    if (test.null_person) c.put32(0, test.staff_id);
    c.put32(clubs, 8);
    c.put32(manager, primary);
    c.put32(manager + 4, test.empty_primary ? 0 : 4);
    c.put32(manager + 0xc, secondary);
    c.put32(manager + 0x10, 2);
    c.put32(manager + 0x9a0, staff_flags);
    c.put8(staff_flags + test.staff_id * 10 + 2, test.cached_count);
    c.put32(primary, test.primary_present ? records : 0);
    c.put32(primary + 4, records + 0x100);
    c.put32(primary + 8, records + 0x200);
    c.put32(secondary + 4, records + 0x300);
    c.put32(records, test.linked_offers ? offer_chain : 0);
    c.put32(records + 4, test.linked_members ? members : 0);
    c.put32(records + 8, test.correct_club ? 8 : 9);
    c.put32(records + 0xc, test.primary_staff ? test.staff_id : test.staff_id + 1);
    c.put8(records + 0x34, test.status);
    c.put8(records + 0x36, test.stage);
    c.put32(records + 0x100 + 8, 9);
    c.put32(records + 0x100 + 0xc, test.staff_id + 1);
    c.put32(records + 0x200 + 8, 8);
    c.put32(records + 0x200 + 0xc, test.staff_id);
    c.put32(records + 0x200 + 0x28, 2);
    c.put8(records + 0x200 + 0x34, 4);
    c.put32(records + 0x300 + 4, test.linked_members ? members : 0);
    c.put32(records + 0x300 + 8, test.correct_club ? 8 : 9);
    make_list(c, offer_chain, nodes, {1, 3, 2, 0});
    make_list(c, members, nodes + 0x100, {test.staff_id + 1, test.member_present ? test.staff_id : test.staff_id + 2});
    return s;
}

unsigned comparisons = 0;
void check(std::uint32_t entry, Case test, std::uint32_t expected, RoutineRunner selected = modern,
           std::int32_t index = 0) {
    active = test;
    auto s = make_state(entry, test);
    if (entry == primary_slot || entry == secondary_slot) Cpu(*s).set(4, std::uint32_t(index));
    auto control = clone(*s);
    original(*control, entry);
    Cpu c(*control);
    require(c.r(3) == expected, "Unexpected original collection result at " + hex_address(entry) +
            " case " + std::to_string(comparisons) + " wanted " + hex_address(expected) +
            " got " + hex_address(c.r(3)));
    if (entry == staff_offers || entry == primary_club || entry == secondary_club || entry == club_offers)
        require(c.u32(output) == (test.wants_output ? expected : 0), "Unexpected original list output count");
    const auto differences = compare_routine_call(*s, entry, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Transfer collection comparison " + std::to_string(comparisons) + " " + hex_address(entry));
    ++comparisons;
}
}

int cm::aot::transfer_manager::checks::collections(int argc, char** argv) {
    try {
        transfer_manager::install_check_coverage();
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            trace_calls = false;
            check(staff_offers, {.linked_offers = false, .wants_output = false}, 1, production);
            check(staff_offers, {.status = 4, .linked_offers = false, .wants_output = false}, 0, production);
            std::cout << "The normal collection path returned through its direct C++ filter call\n";
            return 0;
        }
        require(argc == 1, "Unknown transfer collection check mode");
        dispatch_probe = observe;
        for (auto i : {-1, 0, 1, 2, 3, 4, INT32_MAX}) {
            check(primary_slot, {}, primary + (i < 4 ? std::uint32_t(i) * 4 : 0), modern, i);
            check(secondary_slot, {}, secondary + (i < 2 ? std::uint32_t(i) * 4 : 0), modern, i);
        }
        for (std::uint8_t kind : {0, 1, 2, 3, 4, 14, 15, 16, 17, 19, 20, 21, 255})
            for (std::uint8_t stage : {0, 2, 3, 255})
                check(offer_filter, {.status = kind, .stage = stage},
                      kind == 2 || kind == 3 || kind == 4 || kind == 16 || kind == 20 || (kind == 15 && stage >= 3));

        check(staff_offers, {}, 1);
        check(staff_offers, {.mode = 0x102}, 2);
        check(staff_offers, {.staff_count = 1}, 0);
        check(staff_offers, {.linked_offers = false}, 1);
        check(staff_offers, {.wants_output = false}, 1);
        check(staff_offers, {.status = 4, .linked_offers = false}, 0);
        check(staff_offers, {.mode = 1, .status = 4, .linked_offers = false}, 1);
        check(staff_offers, {.linked_offers = false, .wants_output = false}, 1);
        check(staff_offers, {.primary_present = false}, 0);
        check(staff_offers, {.primary_staff = false}, 0);
        check(staff_offers, {.empty_primary = true}, 0);
        check(staff_offers, {.null_person = true}, 1);

        check(primary_club, {}, 1);
        check(primary_club, {.staff_count = 1}, 0);
        check(primary_club, {.cached_count = 0}, 0);
        check(primary_club, {.cached_count = 2}, 1);
        check(primary_club, {.has_contract = false}, 0);
        check(primary_club, {.correct_club = false}, 0);
        check(primary_club, {.linked_members = false}, 0);
        check(primary_club, {.member_present = false}, 0);
        check(primary_club, {.wants_output = false}, 1);
        check(primary_club, {.cached_count = 0, .null_person = true}, 0);
        check(primary_club, {.contract_disappears = true}, 0);

        check(secondary_club, {}, 1);
        check(secondary_club, {.staff_count = 1}, 0);
        check(secondary_club, {.has_contract = false}, 0);
        check(secondary_club, {.primary_present = false}, 0);
        check(secondary_club, {.correct_club = false}, 0);
        check(secondary_club, {.status = 4}, 0);
        check(secondary_club, {.linked_members = false}, 0);
        check(secondary_club, {.member_present = false}, 0);
        check(secondary_club, {.wants_output = false}, 1);
        check(secondary_club, {.has_contract = false, .null_person = true}, 0);

        check(club_offers, {}, 1);
        check(club_offers, {.wants_output = false}, 1);
        check(club_offers, {.primary_present = false}, 0);
        check(club_offers, {.correct_club = false}, 0);
        check(club_offers, {.status = 4}, 0);

        constexpr std::uint32_t seed = 0x54434f4c;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 24; ++i) {
            const auto id = std::uint32_t(random() % 16);
            const bool want_output = random() & 1;
            const bool member = random() & 1;
            const Case test{.staff_id = id, .staff_count = 20, .member_present = member,
                            .wants_output = want_output};
            check(primary_club, test, member);
            check(secondary_club, test, member);
        }
        std::cout << comparisons << " transfer collection comparisons (seed " << seed << ")\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
