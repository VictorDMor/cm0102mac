#include "transfer_manager_candidate_clubs.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t candidate_clubs = 0x103fb840, create_list = 0x10738c00, destroy_list = 0x10738c20,
    next = 0x10738e00;
constexpr std::uint32_t manager = fixture, offer = fixture + 0x1000, offers = fixture + 0x1100,
    linked_offers = fixture + 0x1200, people = fixture + 0x2000, player = fixture + 0x2a00,
    worker = fixture + 0x2b00, international = fixture + 0x2c00, clubs = fixture + 0x3000,
    nations = fixture + 0x5000, regions = fixture + 0x6000, globals = fixture + 0x7000,
    primary = fixture + 0x7400, loan = fixture + 0x7500, shortlist = fixture + 0x7600,
    interest = fixture + 0x7700, links = fixture + 0x8000, linked_rows = fixture + 0x8100,
    listed_rows = fixture + 0x8400, constants = fixture + 0x9000, weights = fixture + 0x9200,
    pool = fixture + 0xa000, message = fixture + 0xb000, output = fixture + 0xb800, trace = fixture + 0xc000;
constexpr auto club(unsigned id) { return clubs + id * 0x248; }
constexpr auto nation(unsigned id) { return nations + id * 0x140; }
constexpr auto staff(unsigned id) { return people + id * 0x70; }
constexpr auto region(unsigned id) { return regions + id * 0x30; }

struct Case {
    unsigned flags = 0, limit = 24, type = 4, contract_type = 0, club_set = 0xffff, linked_items = 4, shortlist_items = 4,
             role = 12, staff_rating = 3700, job_rating = 3100, world_rating = 3500,
             loan_club = 3, manager_set = 0xffff, fee = 1500000, estimate = 4000, interest_band = 1,
             passport = 3, buyer_nation = 1, international_rating = 5600, contract_bits = 0;
    double statistic = 12.5;
    bool has_primary = true, has_loan = true, has_employer = true, parent_exists = true, parent_protected = false,
         has_player = true, has_worker = true, firm = true, fixed_bonus = true, has_interest = true,
         real_list = false, null_link = false, no_international = false, no_buyer_region = false,
         no_seller_nation = false, empty_manager_nation = false, different_passports = false, null_buyer = false;
};
Case active;
bool production_route = false;
std::uint32_t passport(unsigned index) { return index < 8 ? nation(index) : 0; }

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 84, "Candidate club trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 112;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 13; r <= 27; ++r) add(c.r(r));
    add(c.cr(0));
    add(c.s.r<1>(reg::xer_ca));
}

void make_list(Cpu& c, std::uint32_t list, std::uint32_t nodes, std::uint32_t payload,
               unsigned size, unsigned width) {
    c.put16(list, size);
    c.put32(list + 4, size ? nodes : 0);
    c.put32(list + 8, size ? nodes + 16 * (size - 1) : 0);
    for (unsigned i = 0; i < size; ++i) {
        c.put32(nodes + i * 16, payload + i * width);
        c.put32(nodes + i * 16 + 8, i + 1 < size ? nodes + (i + 1) * 16 : 0);
    }
}

bool service(State& s, unsigned allowed) {
    if (!production_route) return false;
    const auto back = Cpu(s).lr();
    s.budget = s.executed + allowed;
    require(dispatch_page(s), "Missing candidate club list service");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

bool observe(State& s) {
    Cpu c(s);
    const auto entry = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    std::uint32_t answer = 0;
    unsigned subsequent_native = 0;
    switch (entry) {
    case 0x10082f20:
        require(back == 0x103fb8a8 && a == globals + 0x100 && b == people, "Candidate clubs primary contract");
        answer = active.has_primary ? primary : 0;
        subsequent_native = 6;
        break;
    case create_list:
        require(back == 0x103fb910 && a == c.sp() + 0x54, "Candidate clubs temporary list");
        return service(s, 6);
    case destroy_list:
        require(back == 0x103fc6ec && a == c.sp() + 0x54 && b == 0xffffffff, "Candidate clubs list release");
        record(c);
        if (active.real_list) return service(s, 100);
        c.put16(a, 0);
        c.put32(a + 4, 0);
        c.put32(a + 8, 0);
        c.put32(a + 12, 0);
        c.set(3, a);
        c.finish();
        return true;
    case next:
        require((back == 0x103fbb68 && a == links) || (back == 0x103fbcd8 && a == c.sp() + 0x54),
                "Candidate clubs list cursor");
        record(c);
        return service(s, 8);
    case 0x102cdbf0:
        require(a == globals + 0x104 && b >= clubs && b < club(8) && (b - clubs) % 0x248 == 0,
                "Candidate club eligibility");
        answer = (active.club_set >> ((b - clubs) / 0x248)) & 1;
        break;
    case 0x10083110:
        require((back == 0x103fb994 || back == 0x103fb9b0 || back == 0x103fbb94 || back == 0x103fbbb0) &&
                a == globals + 0x100 && b == people, "Candidate clubs loan contract");
        answer = active.has_loan ? loan : 0;
        break;
    case 0x100ce320:
        require(back == 0x103fb9d0 && a == (active.has_employer ? club(3) : 0) && b == c.sp() + 0x38 &&
                d == 1, "Candidate clubs loan parent");
        c.put8(b, active.parent_protected);
        answer = active.parent_exists ? club(4) : 0;
        break;
    case 0x10362ef0:
        require(back == 0x103fbbf0 && a == shortlist && b == people && d == c.sp() + 0x54 &&
                c.r(6) == 1 && c.r(7) == 1 && c.r(8) == 1, "Candidate clubs shortlist");
        make_list(c, d, listed_rows + 0x100, listed_rows, active.shortlist_items, 8);
        break;
    case 0x1036d240:
        require(back == 0x103fbd58 && a == shortlist && b == club(1) && d == people && !c.r(6) && !c.r(7),
                "Candidate clubs recruitment interest");
        answer = active.has_interest ? interest : 0;
        break;
    case 0x100c7a20:
        require(back == 0x103fbd6c && a == active.type, "Candidate clubs staff offer type");
        answer = active.role;
        break;
    case 0x10416840:
        require((back == 0x103fbf1c || back == 0x103fc0bc) && a == offer && !b && !d &&
                !c.r(6) && !c.r(7) && c.r(8) == 1, "Candidate clubs offer value");
        answer = active.fee;
        break;
    case 0x10303560:
        require((back == 0x103fc138 || back == 0x103fc1f8) && a == globals + 0x108 && b == player &&
                d == 2 && c.r(6) == 0x12 && c.f(1) == 6.5, "Candidate clubs observed player rating");
        c.set_f(1, active.statistic);
        break;
    case 0x10715880:
        require(back >= 0x103fc24c && back <= 0x103fc26c && (back - 0x103fc24c) % 8 == 0,
                "Candidate clubs estimate date");
        return service(s, 7);
    case 0x100cdef0:
        require(back == 0x103fc2ac && a == people && b == c.sp() + 0xc4 && d == !active.has_player &&
                c.u32(b + 0x0c) == passport(active.buyer_nation) && c.u32(b + 0x24) == club(1),
                "Candidate clubs regional estimate");
        answer = active.estimate;
        break;
    case 0x10739890:
        require((back == 0x103fbb24 || back == 0x103fbc94 || back == 0x103fc30c) &&
                a == pool + 0x37 && !b && !d, "Candidate clubs capacity diagnostic");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        subsequent_native = 2;
        break;
    case 0x1077b720:
        require((back == 0x103fbb44 || back == 0x103fbcb4 || back == 0x103fc32c) &&
                a == message && b == pool + 0x6ff && d == globals + 0x10c, "Candidate clubs capacity format at " +
                hex_address(back) + " with " + hex_address(a) + ", " + hex_address(b) + ", " + hex_address(d));
        c.put32(message, c.r(8));
        break;
    case 0x10769b60:
        require((back == 0x103fbb54 || back == 0x103fbcc4 || back == 0x103fc33c) &&
                a == pool + 0x70c && b == message && !d, "Candidate clubs capacity dialog");
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xab000000 + r);
    c.set(3, answer);
    c.set_cr(0, 4);
    c.finish();
    if (production_route) s.budget = s.executed + subsequent_native;
    return true;
}

std::unique_ptr<State> make_state() {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0x13000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = candidate_clubs;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, offer}, {5, active.flags}, {6, output}, {7, active.limit}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7be0, globals);
    c.put32(toc - 0x7be4, globals + 4);
    c.put32(toc - 0x7be8, globals + 8);
    c.put32(toc - 0x7b7c, globals + 12);
    c.put32(toc - 0x7b78, globals + 16);
    c.put32(toc - 0x7bf0, globals + 20);
    c.put32(toc - 0x7b3c, globals + 24);
    c.put32(toc - 0x7a10, globals + 28);
    c.put32(toc - 0x7a1c, globals + 32);
    c.put32(toc - 0x7bac, globals + 36);
    c.put32(toc - 0x5b20, globals + 0x10c);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x4ba0, constants);
    c.put32(toc - 0x4ba4, weights);
    c.put32(globals, people);
    c.put32(globals + 4, 17);
    c.put32(globals + 8, active.null_buyer ? std::uint32_t(-0x248) : clubs);
    c.put32(globals + 12, globals + 0x100);
    c.put32(globals + 20, globals + 0x104);
    c.put32(globals + 24, globals + 0x108);
    c.put32(globals + 28, 10);
    c.put32(globals + 32, 20);
    c.put8(globals + 36, 1);
    c.put_f64(constants + 8, 4503601774854144.0);
    c.put_f64(constants + 0x168, 0.25);
    c.put_f64(constants + 0x170, 3000);
    c.put_f64(constants + 0x178, 0.0006);
    c.put_f64(constants + 0x180, 9.25);
    c.put_f64(constants + 0x188, 0.00025);
    c.put_f64(constants + 0x190, 9);
    c.put_f32(weights + 0x2c, 6.5f);
    c.put32(manager, offers);
    c.put32(manager + 0x988, shortlist);
    c.put32(offer, active.null_link ? 0 : links);
    c.put32(offer + 8, 1);
    c.put8(offer + 0x11, active.type);
    c.put32(offer + 0x20, active.fee);
    c.put32(offer + 0x28, 0);
    c.put8(offer + 0x35, active.firm);
    c.put8(offer + 0x37, active.fixed_bonus);
    c.put32(primary + 4, 2);
    c.put8(primary + 0x38, active.contract_type << 2 | active.contract_bits);
    c.put32(loan + 4, active.loan_club);
    c.put32(people + 0xc, passport(active.passport));
    c.put32(people + 0x14, active.has_player ? player : 0);
    c.put32(people + 0x1c, active.has_worker ? worker : 0);
    c.put32(people + 0x20, active.no_international ? 0 : international);
    c.put32(people + 0x24, active.has_employer ? club(3) : 0);
    c.put8(people + 0x5e, active.role);
    c.put16(player + 8, active.job_rating);
    c.put16(player + 0xa, active.staff_rating);
    c.put16(player + 0xc, active.world_rating);
    c.put16(worker + 0x28, active.job_rating);
    c.put16(worker + 0x2a, active.staff_rating);
    c.put16(worker + 0x2c, active.world_rating);
    c.put16(international + 0x1ec, active.international_rating);
    c.put8(interest + 0x28, active.interest_band << 3);
    for (unsigned i = 0; i < 8; ++i) {
        c.put32(club(i), i);
        c.put32(club(i) + 0x2c, nation(i));
        c.put32(club(i) + 0x30, region(i % 3));
        c.put32(nation(i) + 0x68, 10 + (i % 3) * 10);
        c.put32(nation(i) + 0x74, i % 3 + 1);
        c.put8(nation(i) + 0x11f, 16);
        c.put16(region(i % 3) + 0x18, 4 + i % 3);
    }
    c.put32(club(1) + 0x2c, passport(active.buyer_nation));
    if (active.no_seller_nation) c.put32(club(2) + 0x2c, 0);
    if (active.no_buyer_region) c.put32(club(1) + 0x30, 0);
    if (active.empty_manager_nation) c.put32(club(7) + 0x2c, 0);
    for (unsigned i = 1; i <= 16; ++i) {
        c.put32(staff(i) + 0xc, i % 4 ? nation((i + (active.different_passports ? 3 : 0)) % 8) : 0);
        c.put32(staff(i) + 0x24, i % 5 ? club(i % 8) : 0);
        c.put8(staff(i) + 0x6b, (active.manager_set >> (i - 1)) & 1);
    }
    constexpr std::array kinds{0, 5, 2, 15, 15, 20, 4, 3};
    constexpr std::array linked_clubs{1, 5, 6, 7, 4, 5, 6, 7};
    for (unsigned i = 0; i < kinds.size(); ++i) {
        const auto linked = linked_offers + i * 0x40;
        c.put32(offers + i * 4, i ? linked : offer);
        c.put32(linked + 8, linked_clubs[i]);
        c.put8(linked + 0x34, kinds[i]);
        c.put8(linked + 0x36, i == 3 ? 2 : 3);
        c.put32(linked_rows + i * 4, i);
        c.put32(listed_rows + i * 8 + 4, i + 1);
    }
    make_list(c, links, linked_rows + 0x100, linked_rows, active.linked_items, 4);
    return s;
}

void original(State& s, std::uint32_t entry) {
    s.pc = entry;
    while (s.pc != return_sentinel) {
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
}

void rewrite(State& s, std::uint32_t) {
    Cpu c(s);
    collect_candidate_clubs(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7));
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
void check(const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    auto s = make_state();
    auto reference = clone(*s);
    original(*reference, candidate_clubs);
    Cpu c(*reference);
    require(c.r(3) <= 8, "Original candidate club count");
    std::vector<std::uint32_t> result;
    for (unsigned i = 0; i < c.r(3); ++i) {
        const auto club = c.u32(output + i * 4);
        require(club >= clubs && club < clubs + 8 * 0x248 && (club - clubs) % 0x248 == 0 &&
                std::ranges::find(result, club) == result.end(), "Original unique candidate club list");
        result.push_back(club);
    }
    if (test.flags == 0) require(result.empty(), "Original empty candidate clubs");
    if (test.flags == 3 && test.club_set == 0xffff && test.has_primary && test.contract_type != 5)
        require(result == std::vector{club(2), club(1)}, "Original candidate club source order");
    const auto differences = compare_routine_call(*s, candidate_clubs, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Candidate club case " + std::to_string(checked) + " mask " + std::to_string(test.flags));
    ++checked;
}

void swap_clubs(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    const auto first = c.u32(output), second = c.u32(output + 4);
    c.put32(output, second);
    c.put32(output + 4, first);
}
}

int cm::aot::transfer_manager::checks::candidate_club_collection(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check({.flags = 3, .real_list = true}, production);
            check({.flags = 15}, production);
            check({.flags = 16, .world_rating = 9900, .real_list = true}, production);
            check({.flags = 16, .limit = 1, .world_rating = 9000}, production);
            check({.flags = 18, .staff_rating = 8500, .different_passports = true}, production);
            check({.flags = 80, .statistic = 20, .fixed_bonus = false, .real_list = true}, production);
            std::cout << checked << " candidate club production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown candidate club check mode");
        check({});
        check({.flags = 3});
        check({.flags = 3, .has_primary = false});
        check({.flags = 3, .contract_type = 5});
        check({.flags = 3, .club_set = 0});
        check({.flags = 4, .parent_exists = false});
        check({.flags = 4, .parent_protected = true});
        check({.flags = 4, .loan_club = 1});
        check({.flags = 4, .linked_items = 8, .has_loan = false});
        check({.flags = 4, .limit = 0, .has_loan = false});
        check({.flags = 4, .null_link = true});
        check({.flags = 8, .has_employer = false});
        check({.flags = 8, .loan_club = 1});
        check({.flags = 8, .limit = 1});
        check({.flags = 9, .shortlist_items = 7});
        check({.flags = 15});
        check({.flags = 16, .world_rating = 9999});
        check({.flags = 16, .world_rating = 9000});
        check({.flags = 16, .staff_rating = 8500});
        check({.flags = 16, .job_rating = 8500});
        check({.flags = 16, .job_rating = 8500, .world_rating = 8000});
        check({.flags = 16, .limit = 0});
        check({.flags = 16, .type = 8, .staff_rating = 9900, .has_player = false});
        check({.flags = 16, .type = 8, .has_player = false, .has_worker = false});
        check({.flags = 48, .firm = false});
        check({.flags = 48, .fixed_bonus = false});
        check({.flags = 144, .statistic = 18});
        check({.flags = 144, .statistic = 1});
        check({.flags = 80, .statistic = 1, .has_interest = false});
        check({.flags = 80, .staff_rating = 1000, .statistic = 20});
        check({.flags = 80, .has_player = false, .has_interest = false});
        check({.flags = 255});
        check({.flags = 3, .contract_type = 0x3f});
        check({.flags = 3, .contract_type = 0x3f, .contract_bits = 2});
        check({.flags = 4, .null_link = true, .null_buyer = true});
        check({.flags = 16, .role = 0, .has_employer = false});
        check({.flags = 16, .interest_band = 3});
        check({.flags = 16, .fee = 999999});
        check({.flags = 16, .type = 9, .has_player = false});
        check({.flags = 16, .type = 10, .has_player = false});
        check({.flags = 16, .staff_rating = 8500, .has_primary = false});
        check({.flags = 16, .job_rating = 8500, .passport = 1});
        check({.flags = 16, .staff_rating = 8500, .buyer_nation = 2});
        check({.flags = 16, .job_rating = 8500, .fee = 3000001});
        check({.flags = 16, .job_rating = 8500, .no_international = true});
        check({.flags = 16, .role = 22, .job_rating = 8500, .international_rating = 5500});
        check({.flags = 16, .role = 22, .job_rating = 8500, .international_rating = 5501});
        check({.flags = 16, .world_rating = 9000, .passport = 8, .has_primary = false, .empty_manager_nation = true});
        check({.flags = 16, .world_rating = 9000, .buyer_nation = 8, .no_seller_nation = true});
        check({.flags = 18, .staff_rating = 8500, .different_passports = true});
        check({.flags = 80, .staff_rating = 1000, .statistic = 1, .has_employer = false});
        check({.flags = 80, .staff_rating = 1000, .statistic = 1, .has_player = false, .has_interest = false});
        check({.flags = 80, .staff_rating = 1000, .estimate = 1000, .statistic = 1});
        check({.flags = 80, .staff_rating = 1000, .statistic = 1, .no_buyer_region = true});
        check({.flags = 144, .statistic = 18, .has_primary = false});
        constexpr auto seed = 0x5443434cu;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 20; ++i) {
            check({.flags = unsigned(random() & 0xff), .club_set = unsigned(random() & 0xff),
                   .staff_rating = unsigned(random() % 10000), .job_rating = unsigned(random() % 10000),
                   .world_rating = unsigned(random() % 10000), .manager_set = unsigned(random() & 0xffff),
                   .fixed_bonus = (i & 1) != 0});
        }
        active = {.flags = 3};
        auto s = make_state();
        const auto differences = compare_routine_call(*s, candidate_clubs, original, swap_clubs,
            RoutineChecker::result_r3, false, true);
        require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 3000b802"); }),
                "The candidate club comparison missed reordered results");
        std::cout << checked << " candidate club comparisons (seed " << seed << ") and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
