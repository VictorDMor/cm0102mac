#include "transfer_manager_lifecycle.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t destroy = 0x103df570, free_array = 0x10775170, destroy_offer = 0x10411f10,
    destroy_nation = 0x105c41c0, destroy_shortlist = 0x103567b0, destroy_list = 0x10738c20,
    destroy_lists = 0x10773420, release_object = 0x107735a0;
constexpr std::uint32_t manager = fixture, objects = fixture + 0x1000, first = fixture + 0x3000,
    second = fixture + 0x3100, vector = fixture + 0x3400, trace = fixture + 0x5000;
constexpr std::array<std::uint32_t, 11> allocated_fields{0x994, 0x988, 0x9a0, 0xa4c, 0x9b4, 0x9c0,
                                                       0x9cc, 0xa50, 0x9d8, 0xa6c, 0xa78};
constexpr auto object(unsigned index) { return objects + index * 0x40; }

struct Case {
    std::uint32_t delete_flag = 0, fields = 0x7ff, nations = (1u << 26) | 1u;
    unsigned offers = 3, negotiations = 2;
    bool null_manager = false, no_offer_array = false, no_negotiation_array = false;
};
Case active;
bool original_route = false, production_route = false, inside_list_array = false;

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 220, "Transfer destruction trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 64;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 6; ++r) add(c.r(r));
    for (unsigned r = 28; r <= 31; ++r) add(c.r(r));
    add(c.cr(0));
}

unsigned calls(Cpu& c, std::uint32_t target) {
    unsigned count = 0;
    for (unsigned i = 0; i < c.u32(trace); ++i) count += c.u32(trace + 4 + i * 64) == target;
    return count;
}

bool real_service(State& s, std::uint32_t target, std::uint32_t back) {
    if (!production_route || original_route || inside_list_array) return false;
    inside_list_array = target == destroy_lists;
    s.budget = s.executed + 15000;
    require(dispatch_page(s), "Missing real transfer list destruction");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    inside_list_array = false;
    return true;
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4);
    switch (target) {
    case destroy_offer:
        require((back == 0x103df5c8 || back == 0x103df618) && b == 1 &&
                a >= object(12) && a < object(20), "Transfer offer destruction arguments");
        break;
    case destroy_shortlist:
        require(back == 0x103df658 && a == object(1) && b == 1, "Shortlist destruction arguments");
        break;
    case destroy_nation:
        require(back == 0x103df6e0 && b == 1 && a >= object(20) && a < object(47),
                "Nation destruction arguments");
        break;
    case free_array:
        require(a == first || a == second || (a >= objects && a < object(11) && (a - objects) % 0x40 == 0),
                "Transfer array release arguments");
        break;
    case destroy_lists:
        require(back == 0x103df754 && a == manager + 0x18 && b == vector && c.r(5) == 16 && c.r(6) == 150,
                "Embedded list-array destruction");
        record(c);
        return real_service(s, target, back);
    case destroy_list:
        if (back == 0x1077346c) {
            const auto completed = calls(c, destroy_list) - (2 + ((active.fields >> 10) & 1u));
            require(completed < 150 && a == manager + 0x18 + (149 - completed) * 16 && b == 0xffffffff,
                    "Embedded list element and order");
        } else {
            require((back == 0x103df728 && a == object(10) && b == 1) ||
                    (back == 0x103df734 && a == manager + 0xa5c && b == 0xffffffff) ||
                    (back == 0x103df740 && a == manager + 0x9a4 && b == 0xffffffff), "Transfer list destruction");
        }
        record(c);
        return real_service(s, target, back);
    case release_object:
        require((back == 0x103df764 && a == manager && std::int16_t(active.delete_flag) > 0) ||
                (back == 0x10738c74 && a == object(10)), "Transfer object deletion arguments");
        break;
    default:
        return false;
    }
    record(c);
    if (a != manager) c.put8(a, 0xa1);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xa1000000 + r);
    c.set(3, 0);
    c.set_cr(0, 4);
    c.finish();
    return true;
}

std::unique_ptr<State> fixture_state(const Case& test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0x9000);
    s->memory.map(toc - 0x7000, 0x1000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = destroy;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, test.null_manager ? 0 : manager}, {4, test.delete_flag}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x6314, vector);
    c.put32(vector, destroy_list);
    c.put32(vector + 4, toc);
    for (unsigned i = 0; i < allocated_fields.size(); ++i)
        if (test.fields & (1u << i)) c.put32(manager + allocated_fields[i], object(i));
    for (unsigned i = 0; i < 27; ++i)
        if (test.nations & (1u << i)) c.put32(manager + 0x9e0 + i * 4, object(20 + i));
    if (!test.no_offer_array) c.put32(manager, first);
    if (!test.no_negotiation_array) c.put32(manager + 0xc, second);
    c.put32(manager + 4, test.offers);
    c.put32(manager + 0x10, test.negotiations);
    for (unsigned i = 0; i < test.offers; ++i)
        if (!(i & 1)) c.put32(first + i * 4, object(12 + i));
    for (unsigned i = 0; i < test.negotiations; ++i)
        if (i & 1) c.put32(second + i * 4, object(16 + i));
    return s;
}

void original(State& s, std::uint32_t entry) {
    original_route = true;
    s.pc = entry;
    while (s.pc != return_sentinel) {
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = false;
}

void rewrite(State& s, std::uint32_t) {
    Cpu c(s);
    destroy_transfer_manager(c, c.r(3), c.r(4));
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
    auto s = fixture_state(test);
    auto control = clone(*s);
    original(*control, destroy);
    Cpu c(*control);
    require(c.r(3) == (test.null_manager ? 0 : manager), "Original transfer destruction return");
    if (test.null_manager) require(c.u32(trace) == 0, "Original null transfer destruction called a dependency");
    else {
        const unsigned offers = test.no_offer_array ? 0 : (test.offers + 1) / 2;
        const unsigned negotiations = test.no_negotiation_array ? 0 : test.negotiations / 2;
        const unsigned pairs = (test.fields >> 10) & 1;
        require(calls(c, destroy_offer) == offers + negotiations &&
                calls(c, destroy_nation) == unsigned(std::popcount(test.nations)) &&
                calls(c, destroy_list) == 152 + pairs &&
                calls(c, release_object) == pairs + (std::int16_t(test.delete_flag) > 0), "Original transfer destruction counts");
    }
    const auto differences = compare_routine_call(*s, destroy, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Transfer manager destruction case " + std::to_string(checked));
    ++checked;
}

void omit_array_release(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put8(object(0), 0);
}

void check_negative_control() {
    active = {};
    auto s = fixture_state(active);
    const auto differences = compare_routine_call(*s, destroy, original, omit_array_release,
        RoutineChecker::result_r3, false, true);
    require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 30001000"); }),
            "The destruction comparison missed an unreleased history array");
}
}

int cm::aot::transfer_manager::checks::lifecycle(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check({.delete_flag = 1}, production);
            check({.fields = 0, .nations = 0, .no_offer_array = true, .no_negotiation_array = true}, production);
            check({.null_manager = true}, production);
            std::cout << checked << " transfer lifecycle production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown transfer lifecycle check mode");
        check({.null_manager = true});
        check({});
        check({.delete_flag = 0x10001});
        check({.delete_flag = 0x1ffff, .fields = 0, .nations = 0, .no_offer_array = true, .no_negotiation_array = true});
        check({.offers = 0, .negotiations = 0});
        constexpr auto seed = 0x544c4946u;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 16; ++i) {
            const auto fields = std::uint32_t(random() & 0x7ff), nations = std::uint32_t(random() & 0x07ffffff);
            const auto offers = unsigned(random() % 4), negotiations = unsigned(random() % 4);
            check({.fields = fields, .nations = nations, .offers = offers, .negotiations = negotiations});
        }
        check_negative_control();
        std::cout << checked << " transfer lifecycle comparisons (seed " << seed << ") and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
