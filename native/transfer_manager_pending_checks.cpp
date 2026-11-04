#include "transfer_manager_pending.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot {
bool transfer_manager_reference(State&);
}

namespace {
using namespace cm::semantic_io_checks;
using cm::aot::guest::Cpu;

constexpr std::uint32_t mark = 0x103ea250, clear = 0x103ea270, lookup = 0x103ea290;
constexpr std::uint32_t find_pending_transfer = 0x1007f7b0, lookup_return = 0x103ea2d8;
constexpr std::uint32_t manager = fixture, person = fixture + 0x1000, status = fixture + 0x1100;
constexpr std::uint32_t contracts = fixture + 0x2000, pending = fixture + 0x2100,
                        node = fixture + 0x2200, record = fixture + 0x2300, globals = fixture + 0x3000,
                        trace = fixture + 0x4000, output = fixture + 0x5000,
                        clubs = fixture + 0x6000;

enum class Negative { none, result, argument, missing_call, extra_call, link };
Negative negative = Negative::none;
bool rewrite_running = false;

bool observe_call(State& s) {
    if (s.pc != find_pending_transfer) return false;
    Cpu c(s);
    const auto target = s.pc;
    const auto back = c.lr();
    const auto manager_argument = c.r(3);
    const auto id_argument = c.r(4);
    require(back == lookup_return && manager_argument == contracts && id_argument == c.u32(person),
            "Unexpected pending-contract call boundary");
    if (rewrite_running && negative == Negative::missing_call) return false;
    const auto count = c.u32(trace);
    require(count == 0, "Pending lookup called its dependency twice");
    c.put32(trace, rewrite_running && negative == Negative::extra_call ? 2 : 1);
    c.put32(trace + 4, target);
    c.put32(trace + 8, rewrite_running && negative == Negative::link ? back + 4 : back);
    for (unsigned r = 3; r <= 10; ++r)
        c.put32(trace + 12 + (r - 3) * 4, c.r(r) ^ (rewrite_running && negative == Negative::argument && r == 4));
    // The real contract-manager lookup executes on both sides, including its
    // list cursor updates. The probe records the call and declines.
    return false;
}

void original(State& s, std::uint32_t entry) {
    s.pc = entry;
    require(transfer_manager_reference(s), "Missing original pending-contract entry");
    while (s.pc != return_sentinel) {
        if (!transfer_manager_reference(s)) run_until(s, lookup_return);
    }
}

void rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    rewrite_running = true;
    if (entry == mark) transfer_manager::mark_pending_contract(c, c.r(3), c.r(4));
    else if (entry == clear) transfer_manager::clear_pending_contract(c, c.r(3), c.r(4));
    else if (entry == lookup) transfer_manager::find_pending_contract(c, c.r(3), c.r(4), c.r(5));
    else throw std::runtime_error("Unknown pending-contract entry");
    if (negative == Negative::result) c.set(3, c.r(3) ^ 1);
    rewrite_running = false;
    c.finish();
}

struct Case {
    std::uint32_t id = 3;
    std::uint32_t club_id = 1;
    std::uint8_t flags = 0x40;
    bool has_list = true;
    bool matching_id = true;
    bool matching_kind = true;
    bool wants_destination = true;
};

std::unique_ptr<State> make_state(Case test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0xa5);
    s->unique.fill(0x7b);
    for (unsigned field = 0; field < 8; ++field) s->wr<1>(guest::reg::cr(field), field & 7);
    for (unsigned field = 1024; field < 1028; ++field) s->wr<1>(field, 1);
    for (unsigned field = 1280; field < 1312; ++field) s->wr<1>(field, 0);
    s->memory.map(fixture, 0x10000);
    s->memory.map(stack - 0x1000, 0x2000);
    s->memory.map(toc - 0x8000, 0x2000);
    Cpu c(*s);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, person}, {5, test.wants_destination ? output : 0}});
    c.set_lr(return_sentinel);
    s->budget = 100000;
    c.put32(manager + 0x9a0, status);
    c.put32(person, test.id);
    c.put8(status + test.id * 10u, test.flags);
    c.put32(output, 0x55555555);
    c.put32(toc - 0x7b7c, globals);
    c.put32(toc - 0x7be8, globals + 4);
    c.put32(globals, contracts);
    c.put32(globals + 4, clubs);
    c.put32(contracts + 0x34, test.has_list ? pending : 0);
    c.put16(pending, 1);
    c.put32(pending + 4, node);
    c.put32(pending + 8, node);
    c.put32(node, record);
    c.put32(record, test.matching_id ? test.id : test.id + 1);
    c.put32(record + 4, test.club_id);
    c.put8(record + 0x64, test.matching_kind ? 2 : 1);
    return s;
}

void expect_lookup(State& s, Case test) {
    Cpu c(s);
    const bool marked = test.flags & 0x40;
    const bool found = marked && test.has_list && test.matching_id && test.matching_kind;
    require(c.r(3) == found, "Reference lookup result");
    require(c.u32(trace) == marked, "Reference lookup call count");
    require(c.u8(status + test.id * 10u) == (found ? test.flags : test.flags & ~0x40),
            "Reference pending marker");
    require(c.u32(output) == (found && test.wants_destination ? clubs + test.club_id * 0x248 : 0x55555555),
            "Reference destination output");
}

void compare(std::uint32_t entry, Case test, unsigned serial) {
    auto s = make_state(test);
    s->pc = entry;
    auto expectation = clone(*s);
    original(*expectation, entry);
    if (entry == lookup) expect_lookup(*expectation, test);
    else {
        Cpu c(*expectation);
        require(c.u8(status + test.id * 10u) == (entry == mark ? test.flags | 0x40 : test.flags & ~0x40),
                "Reference pending marker write");
        require(c.u32(trace) == 0, "Unexpected pending-marker dependency");
    }

    const auto differences = compare_routine_call(*s, entry, original, rewrite,
                                                 RoutineChecker::result_r3 | RoutineChecker::result_f1,
                                                 false, true);
    if (negative == Negative::none) {
        for (const auto& line : differences) std::cerr << line << '\n';
        require(differences.empty(), "Pending-contract comparison " + std::to_string(serial) + " " + hex_address(entry));
    } else {
        const std::string_view expected = negative == Negative::result ? "r3" :
            negative == Negative::argument ? "byte 30004013" :
            negative == Negative::link ? "byte 3000400b" : "byte 30004003";
        require(std::ranges::any_of(differences, [&](const auto& line) { return line.starts_with(expected); }),
                "Negative control did not detect " + std::string(expected));
    }
}

void combined(State& s, std::uint32_t, RoutineRunner route) {
    Cpu c(s);
    for (auto entry : {mark, lookup, clear}) {
        c.set({{3, manager}, {4, person}, {5, output}});
        route(s, entry);
    }
}
void original_sequence(State& s, std::uint32_t entry) { combined(s, entry, original); }
void rewrite_sequence(State& s, std::uint32_t entry) { combined(s, entry, rewrite); }
void game_sequence(State& s, std::uint32_t entry) {
    combined(s, entry, [](State& state, std::uint32_t target) {
        state.pc = target;
        run_until(state, return_sentinel);
    });
}

void production(bool force_original) {
    require(dispatch_probe == nullptr && routine_checker == nullptr, "Production mode has a checker hook");
    for (auto entry : {mark, clear, lookup}) {
        auto s = make_state({.flags = 0});
        auto expected = clone(*s);
        original(*expected, entry);
        s->pc = entry;
        if (!force_original) s->budget = 0;
        Effects effects;
        effects.state = s.get();
        s->memory.date_observer = &effects;
        run_until(*s, return_sentinel);
        s->memory.date_observer = nullptr;
        require((s->executed != 0) == force_original, "Wrong production route at " + hex_address(entry));
        for (unsigned r = 0; r < 32; ++r)
            require(s->r<4>(guest::reg::gpr(r)) == expected->r<4>(guest::reg::gpr(r)), "Production GPR differs");
        for (unsigned field = 0; field < 8; ++field)
            require(s->r<1>(guest::reg::cr(field)) == expected->r<1>(guest::reg::cr(field)), "Production CR differs");
        for (const auto& [base, size] : expected->memory.mapped_ranges())
            require(s->memory.copy_out(base, size) == expected->memory.copy_out(base, size), "Production memory differs");
        require(!effects.log.empty(), "Production mode did not exercise observation");
    }
    auto s = make_state({.flags = 0});
    const auto differences = compare_routine_call(*s, mark, original_sequence, game_sequence,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& line : differences) std::cerr << line << '\n';
    require(differences.empty(), "Production pending-contract sequence differs");
    std::cout << "All three pending-contract entries reached the "
              << (force_original ? "forced original" : "normal handwritten") << " route\n";
}
}

int main(int argc, char** argv) {
    try {
        transfer_manager::install_check_coverage();
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            production(false);
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--forced-original") {
            production(true);
            return 0;
        }
        require(argc == 1, "Unknown pending-contract check mode");
        dispatch_probe = observe_call;
        unsigned serial = 0;
        for (std::uint8_t flags : {0, 0x40, 0xa5, 0xff}) {
            const Case test{.flags = flags};
            compare(mark, test, serial++);
            compare(clear, test, serial++);
            compare(lookup, test, serial++);
        }
        compare(lookup, {.has_list = false}, serial++);
        compare(lookup, {.matching_id = false}, serial++);
        compare(lookup, {.matching_kind = false}, serial++);
        compare(lookup, {.wants_destination = false}, serial++);
        constexpr auto seed = 0x54524e53u;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 32; ++i) {
            const Case test{.id = std::uint32_t(random() % 16), .club_id = std::uint32_t(random() % 24),
                            .flags = std::uint8_t(random()), .has_list = bool(random() & 1),
                            .matching_id = bool(random() & 1), .matching_kind = bool(random() & 1),
                            .wants_destination = bool(random() & 1)};
            for (auto entry : {mark, clear, lookup}) compare(entry, test, serial++);
        }
        auto s = make_state({.flags = 0});
        const auto combined_differences = compare_routine_call(*s, mark, original_sequence, rewrite_sequence,
            RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
        require(combined_differences.empty(), "Combined pending-contract sequence differs");
        for (auto defect : {Negative::result, Negative::argument, Negative::missing_call, Negative::extra_call, Negative::link}) {
            negative = defect;
            compare(lookup, {}, serial);
        }
        std::cout << serial << " pending-contract comparisons (seed " << seed
                  << "), combined sequence and five negative controls passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
