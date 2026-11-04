#include "transfer_manager_nations.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t initialize = 0x103fd420, allocate_object = 0x10773520, allocate_index = 0x10775130,
    sort = 0x10768300, comparator = 0x10411c40;
constexpr std::uint32_t manager = fixture, objects = fixture + 0x1000, borders = fixture + 0x2000,
    globals = fixture + 0x3000, date_index = fixture + 0x4000, vector = fixture + 0x4200,
    trace = fixture + 0x5000, effects = fixture + 0x7000, pool = fixture + 0x8000,
    message = fixture + 0xb000, title = fixture + 0xb100, body = fixture + 0xb200;
constexpr auto object(unsigned id) { return objects + id * 0x20; }
constexpr auto dates(unsigned id) { return borders + id * 0x30; }
constexpr auto id_at(unsigned order) { return order == 16 ? 17 : order == 17 ? 16 : order; }
constexpr std::array constructors{
    0x105bf320u, 0x105bd570u, 0x106bcb30u, 0x105bdc00u, 0x105be6d0u, 0x106bce80u, 0x105bea70u, 0x105beb80u,
    0x106bd110u, 0x105bf550u, 0x105c0300u, 0x106bd5c0u, 0x105c0d70u, 0x106bd8f0u, 0x105c10a0u, 0x105c1d00u,
    0x105c2340u, 0x106bdfb0u, 0x106be3a0u, 0x105c2a40u, 0x106be640u, 0x105c3260u, 0x105c37d0u, 0x105c4080u,
    0x106be9d0u, 0x105e0240u, 0x106bedf0u,
};
constexpr std::array slots{
    -0x7a70, -0x7508, -0x7650, -0x7510, -0x7b68, -0x7590, -0x750c, -0x7a10, -0x7594, -0x7a00,
    -0x7af4, -0x7598, -0x7a28, -0x7574, -0x7a14, -0x7a24, -0x7504, -0x7578, -0x75a8, -0x79fc,
    -0x758c, -0x7a1c, -0x7a18, -0x7514, -0x75b0, -0x7a20, -0x757c,
};

struct Case {
    std::uint32_t missing = 0, rows = 1 | (1u << 17) | (1u << 26);
    bool index_fails = false;
};
Case active;
bool original_route = false, production_route = false;
unsigned rows_for(unsigned id) { return active.rows & (1u << id) ? 1 + (id & 1) : 0; }

void original(State&, std::uint32_t);

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 120, "Nation transfer trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 64;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 9; ++r) add(c.r(r));
    add(c.r(25));
    add(c.r(30));
    add(c.cr(0));
}

unsigned calls(Cpu& c, std::uint32_t target) {
    unsigned count = 0;
    for (unsigned i = 0; i < c.u32(trace); ++i) count += c.u32(trace + 4 + i * 64) == target;
    return count;
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    const auto order = c.u32(effects);
    std::uint32_t answer = 0;
    if (order && order <= constructors.size() && target == constructors[order - 1]) {
        const auto id = id_at(order - 1), nation = 500 + id;
        require(back == 0x103fd474 + (order - 1) * 0x30 && a == object(id) && b == id && c.r(8) == 0 &&
                (id == 0 ? d == 0xffffffff && c.r(6) == nation && c.r(7) == 2 :
                           d == nation && c.r(6) == 0xffffffff && c.r(7) == 1),
                "Nation transfer object constructor arguments and order");
        c.put32(a + 4, dates(id));
        c.put8(a + 8, rows_for(id));
        c.put32(a + 0xc, nation);
        answer = a;
    } else {
        switch (target) {
        case allocate_object:
            require(order < constructors.size() && back == 0x103fd450 + order * 0x30 && a == 0x18,
                    "Nation transfer object allocation order");
            c.put32(effects, order + 1);
            if (!(active.missing & (1u << id_at(order)))) answer = object(id_at(order));
            break;
        case allocate_index:
            require(back == 0x103fd9b0 && a == c.u32(manager + 0x9dc) * 4 && order == 27,
                    "Nation transfer date index allocation");
            answer = active.index_fails ? 0 : date_index;
            break;
        case sort:
            require(back == 0x103fdacc && a == date_index && b == c.u32(manager + 0x9dc) && d == 4 &&
                    c.r(6) == vector, "Nation transfer date sort");
            record(c);
            if (!production_route || original_route) return false;
            s.budget = s.executed + 15000;
            require(dispatch_page(s), "Missing real nation date sort");
            if (s.pc != back) run_until(s, back);
            s.budget = s.executed;
            return true;
        case comparator:
            record(c);
            if (!original_route) return false;
            original(s, target);
            return true;
        case 0x10739890:
            require(back == 0x103fd9d8 && a == pool + 0x37 && !b && !d &&
                    c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38, "Nation transfer diagnostic split");
            c.put32(c.r(6), 0x5452414e);
            c.put32(c.r(7), 0x43505000);
            if (production_route && !original_route) s.budget = s.executed + 2;
            break;
        case 0x1077b720:
            require(back == 0x103fd9fc && a == message && b == pool + 0x4c && d == globals + 0x100 &&
                    c.r(8) == 0x3bdd && c.r(9) == pool + 0x23f2, "Nation transfer diagnostic format");
            if (production_route && !original_route) require(s.executed == s.budget, "Nation transfer version leaf");
            c.put32(a, c.r(8));
            break;
        case 0x10727890:
            require((back == 0x103fda08 && a == title && b == pool + 0xa6) ||
                    (back == 0x103fda18 && a == body && b == pool + 0xbe && d == message), "Nation transfer diagnostic text");
            break;
        case 0x10769b60:
            require(back == 0x103fda28 && a == title && b == body && !d && c.u32(message) == 0x3bdd,
                    "Nation transfer diagnostic display");
            break;
        default:
            return false;
        }
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xac000000 + r);
    c.set_cr(0, 4);
    c.set(3, answer);
    c.finish();
    return true;
}

std::unique_ptr<State> fixture_state() {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0xc000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = initialize;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x79dc, title);
    c.put32(toc - 0x79e0, body);
    c.put32(toc - 0x5b20, globals + 0x100);
    c.put32(toc - 0x4bcc, vector);
    c.put32(toc - 0x3c00, pool);
    c.put32(vector, comparator);
    c.put32(vector + 4, toc);
    for (unsigned order = 0; order < slots.size(); ++order) {
        const auto id = id_at(order);
        c.put32(toc + std::uint32_t(slots[order]), globals + id * 4);
        c.put32(globals + id * 4, 500 + id);
        for (unsigned date = 0; date < 4; ++date) {
            c.put8(dates(id) + date * 6 + 4, 3 - (id + date) % 7);
            c.put8(dates(id) + date * 6 + 3, id - date);
        }
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

void rewrite(State& s, std::uint32_t) {
    Cpu c(s);
    initialize_nation_transfer_records(c, c.r(3));
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
    auto s = fixture_state();
    auto control = clone(*s);
    original(*control, initialize);
    Cpu c(*control);
    std::vector<std::uint32_t> pointers;
    for (unsigned id = 0; id < 27; ++id) {
        const bool present = !(test.missing & (1u << id));
        require(c.u32(manager + 0x9e0 + id * 4) == (present ? object(id) : 0), "Original nation object table");
        if (present) for (unsigned j = 0; j < rows_for(id) * 2; ++j) pointers.push_back(dates(id) + j * 6);
    }
    require(c.u32(manager + 0x9dc) == pointers.size() &&
            c.u32(manager + 0x9d8) == (test.index_fails ? 0 : date_index), "Original nation date index");
    if (!test.index_fails) {
        const auto key = [&](std::uint32_t pointer) { return std::pair(c.i8(pointer + 4), c.i8(pointer + 3)); };
        require(calls(c, sort) == 1 && (pointers.empty() || calls(c, comparator) > 0), "Original nation sort route");
        std::vector<std::uint32_t> actual;
        for (unsigned i = 0; i < pointers.size(); ++i) actual.push_back(c.u32(date_index + i * 4));
        require(std::ranges::is_sorted(actual, {}, key) && std::ranges::is_permutation(pointers, actual),
                "Original nation date pointer sort result");
    } else {
        require(calls(c, sort) == 0 && calls(c, 0x10769b60) == 1, "Original nation allocation failure");
    }
    const auto differences = compare_routine_call(*s, initialize, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Nation transfer case " + std::to_string(checked));
    ++checked;
}

void alter_nation_slot(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put32(manager + 0x9e0 + 16 * 4, 0);
}

void check_negative_control() {
    active = {};
    auto s = fixture_state();
    const auto differences = compare_routine_call(*s, initialize, original, alter_nation_slot,
        RoutineChecker::result_r3, false, true);
    require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 30000a20"); }),
            "The nation comparison missed a missing country slot");
}
}

int cm::aot::transfer_manager::checks::nations(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check({}, production);
            check({.missing = (1u << 16) | (1u << 17), .rows = 0}, production);
            check({.index_fails = true}, production);
            std::cout << checked << " nation transfer production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown nation transfer check mode");
        check({});
        check({.rows = 0});
        check({.missing = (1u << 27) - 1});
        check({.index_fails = true});
        constexpr auto seed = 0x544e4154u;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 24; ++i) {
            const auto missing = std::uint32_t(random() & 0x07ffffff);
            const auto rows = (1u << (random() % 27)) | (1u << (random() % 27)) | (1u << (random() % 27));
            check({.missing = missing, .rows = std::uint32_t(rows)});
        }
        check_negative_control();
        std::cout << checked << " nation transfer comparisons (seed " << seed << ") and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
