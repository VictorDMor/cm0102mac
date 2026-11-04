#include "transfer_manager_load.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t load = 0x103df790, open_file = 0x107475b0, read_file = 0x10747a80,
    close_file = 0x10747900, calloc_array = 0x10775200, allocate = 0x10775130, create_object = 0x10773520,
    init_offer = 0x10411e60, read_offer = 0x10411fa0, read_shortlist = 0x10356410, append_list = 0x10738d10;
constexpr std::uint32_t manager = fixture, request = fixture + 0xb00, file = fixture + 0xb20,
    offers = fixture + 0x1000, negotiations = fixture + 0x1100, offer = fixture + 0x1200,
    negotiation = fixture + 0x1300, shortlist = fixture + 0x1400, pairs = fixture + 0x1500,
    records = fixture + 0x2000, staff = fixture + 0x2800, clubs = fixture + 0x2900, globals = fixture + 0x3000,
    stored_lists = fixture + 0x4000, pool = fixture + 0x5000, message = fixture + 0x7000, title = fixture + 0x7100,
    body = fixture + 0x7200, effects = fixture + 0x8000, trace = fixture + 0x10000, stream = fixture + 0x20000;
constexpr auto load_frame = stack - 0x770;

struct Failure { std::uint32_t back, line, result = 0; };
constexpr std::array read_failures{
    Failure{0x103df884, 0x16a}, Failure{0x103df918, 0x172}, Failure{0x103dfb0c, 0x198}, Failure{0x103dfba0, 0x1a0},
    Failure{0x103dfdc0, 0x1d5}, Failure{0x103dfeb8, 0x1e9}, Failure{0x103dff4c, 0x1f1}, Failure{0x103dffe0, 0x1f9},
    Failure{0x103e0114, 0x20a}, Failure{0x103e0194, 0x212}, Failure{0x103e02c8, 0x227}, Failure{0x103e0360, 0x230},
    Failure{0x103e0494, 0x245}, Failure{0x103e052c, 0x24e}, Failure{0x103e05c4, 0x256}, Failure{0x103e0660, 0x261},
    Failure{0x103e0718, 0x26d}, Failure{0x103e0850, 0x283}, Failure{0x103e08e8, 0x28c}, Failure{0x103e0a20, 0x2a2},
    Failure{0x103e0ab8, 0x2ab}, Failure{0x103e0bf0, 0x2c1}, Failure{0x103e0c88, 0x2ca, 1},
    Failure{0x103e0d20, 0x2d2, 1}, Failure{0x103e0e08, 0x2e6, 1}, Failure{0x103e0ec0, 0x2f2},
    Failure{0x103e0ff8, 0x308}, Failure{0x103e1090, 0x311}, Failure{0x103e1154, 0x320},
};
constexpr std::array allocation_failures{
    Failure{0x103df9ac, 0x17e}, Failure{0x103dfc34, 0x1ac}, Failure{0x103e0068, 0x202},
    Failure{0x103e0230, 0x21e}, Failure{0x103e03fc, 0x23c}, Failure{0x103e07b8, 0x27a},
    Failure{0x103e0988, 0x299}, Failure{0x103e0b58, 0x2b8}, Failure{0x103e0f60, 0x2ff},
    Failure{0x103dfa48, 0x18b, 1}, Failure{0x103dfccc, 0x1b9, 1},
};

struct Array {
    std::uint32_t pointer, capacity, count, width, allocation, data_offset;
};
constexpr std::array arrays{
    Array{0x994, 0x998, 0x99c, 40, 0x103e0230, 0x000},
    Array{0xa50, 0xa54, 0xa58, 12, 0x103e03fc, 0x100},
    Array{0x9b4, 0x9b8, 0x9bc, 16, 0x103e07b8, 0x200},
    Array{0x9c0, 0x9c4, 0x9c8, 16, 0x103e0988, 0x300},
    Array{0x9cc, 0x9d0, 0x9d4, 16, 0x103e0b58, 0x400},
    Array{0xa6c, 0xa74, 0xa70, 16, 0x103e0f60, 0x500},
};

struct Case {
    Failure failure{};
    bool null_request = false, missing_stream = false, empty = false, legacy = false, drop_pairs_allocation = false;
};
Case active;
bool original_route = false, production_route = false, leaf_allowance = false;

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 280, "Transfer load trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 112;
    const auto add = [&](std::uint32_t word) { c.put32(out, word); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 9; ++r) add(c.r(r));
    for (unsigned r = 18; r <= 31; ++r) add(c.r(r));
    add(c.cr(0));
    add(c.s.r<1>(reg::xer_ca));
}

void take(Cpu& c, std::uint32_t address, std::uint32_t size) {
    const auto offset = c.u32(effects);
    require(offset + size <= c.u32(effects + 4), "Transfer load read beyond the fixture");
    if (size) c.s.memory.copy_in(address, c.s.memory.copy_out(stream + offset, size));
    c.put32(effects, offset + size);
}

void expect_leaf(State& s, unsigned instructions) {
    if (!production_route || original_route) return;
    s.budget = s.executed + instructions;
    leaf_allowance = true;
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    if (leaf_allowance && production_route && !original_route) {
        require(s.budget == s.executed, "Transfer load leaf charged an unexpected instruction count");
        leaf_allowance = false;
    }
    std::uint32_t answer = 0;
    switch (target) {
    case 0x103721f0:
        require(back == 0x103df7cc && a == load_frame + 0x6c8, "Transfer load local contract constructor");
        c.put32(a + 96, 0x66778899);
        break;
    case open_file:
        require(back == 0x103df7f4 && a == request && b == pool + 0x257 && d == 0 && c.r(6) == 22 && c.r(7) == 4,
                "Transfer load archive open");
        answer = active.missing_stream ? 0 : file;
        break;
    case close_file:
        require(a == load_frame + 0x40 && c.u32(a) == file && !c.u32(effects + 8), "Transfer load archive close");
        c.put32(a, 0);
        c.put32(effects + 8, 1);
        break;
    case read_file:
        require(c.r(6) == file && !c.u32(effects + 8) && b > 0 && b <= 100 && d < 8,
                "Transfer load read boundary");
        if (back == active.failure.back) {
            require(d > 0, "The failed read must request a record");
            answer = d - 1;
        } else answer = d;
        take(c, a, b * answer);
        if (back == 0x103e0d20 && answer && c.u16(a)) expect_leaf(s, 4);
        break;
    case calloc_array:
        require((back == 0x103df9ac || back == 0x103dfc34) && a == 3 && b == 4, "Transfer load offer array");
        answer = active.failure.back == back ? 0 : back == 0x103df9ac ? offers : negotiations;
        break;
    case allocate: {
        const auto row = std::ranges::find(arrays, back, &Array::allocation);
        require(row != arrays.end() && a == 2 * row->width, "Transfer load record array");
        answer = active.failure.back == back ? 0 : records + row->data_offset;
        break;
    }
    case create_object:
        switch (back) {
        case 0x103dfa48: require(a == 0x54, "Transfer offer size"); answer = offer; break;
        case 0x103dfccc: require(a == 0x54, "Transfer negotiation size"); answer = negotiation; break;
        case 0x103e0068: require(a == 0xe0, "Transfer shortlist size"); answer = shortlist; break;
        case 0x103e1124: require(a == 0x10, "Transfer pair list size"); answer = active.drop_pairs_allocation ? 0 : pairs; break;
        default: throw std::runtime_error("Unexpected transfer load object allocation");
        }
        if (active.failure.back == back) answer = 0;
        if (back == 0x103e1124 && answer) expect_leaf(s, 6);
        break;
    case init_offer:
        require((back == 0x103dfa54 && a == offer) || (back == 0x103dfcd8 && a == negotiation), "Transfer offer constructor");
        c.put32(a, 0x13579bdf);
        answer = a;
        break;
    case read_offer:
        require(b == file && ((back == 0x103dfad8 && a == offer && d == offers) ||
                (back == 0x103dfd5c && a == negotiation && !d)), "Transfer offer deserialization");
        take(c, a + 0x28, 4);
        c.put32(a + 0x30, 1);
        answer = a;
        break;
    case read_shortlist:
        require(back == 0x103e0078 && a == shortlist && b == file, "Transfer shortlist deserialization");
        take(c, a, 4);
        answer = a;
        break;
    case append_list: {
        const auto count = c.u32(effects + 12);
        require(count < 5, "Unexpected transfer list row");
        require((back == 0x103dfe6c && a == manager + 0x18 && b == load_frame + 0x3c && d == 4) ||
                (back == 0x103e06f4 && a == manager + 0x9a4 && b == load_frame + 0x198 && d == 16) ||
                (back == 0x103e0e9c && a == manager + 0xa5c && b == load_frame + 0x18c && d == 12) ||
                (back == 0x103e11e8 && a == (active.drop_pairs_allocation ? 0 : pairs) &&
                 b == load_frame + 0x6c8 && d == 100), "Transfer load list append");
        answer = stored_lists + count * 128;
        c.put32(effects + 12, count + 1);
        if (a) c.put16(a, c.u16(a) + 1);
        s.memory.copy_in(answer, s.memory.copy_out(b, d));
        break;
    }
    case 0x10739890:
        require(a == pool + 0x37 && !b && !d && c.r(6) >= load_frame + 0x1a8 && c.r(6) <= load_frame + 0x6a8 &&
                c.r(7) >= load_frame + 0x44 && c.r(7) <= load_frame + 0x184, "Transfer load diagnostic split");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        expect_leaf(s, 2);
        break;
    case 0x1077b720:
        require(a == message && (b == pool + 0x4c || b == pool + 0x6ff) && d == globals + 0x40 &&
                c.u32(c.r(6)) == 0x5452414e && c.u32(c.r(7)) == 0x43505000 &&
                c.r(8) == (active.missing_stream ? 0x163 : active.failure.line), "Transfer load diagnostic format");
        c.put32(message, c.r(8));
        c.put32(effects + 16, c.r(8));
        break;
    case 0x10727890:
        require((a == title && (b == pool + 0x2a2 || b == pool + 0xa6)) ||
                (a == body && (b == pool + 0x2b2 || b == pool + 0xbe) && d == message), "Transfer load diagnostic text");
        c.put32(a, b);
        break;
    case 0x10769b60:
        require(!d && ((a == title && b == body) || (a == pool + 0x70c && b == message)), "Transfer load diagnostic display");
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xaa000000 + r);
    c.set_cr(0, 4);
    c.set(3, answer);
    c.finish();
    return true;
}

void seed_stream(Cpu& c, const Case& test) {
    auto position = stream;
    const auto word = [&](std::uint32_t value, unsigned size) {
        for (unsigned i = size; i > 0; --i) c.put8(position++, value >> ((i - 1) * 8));
    };
    const auto pattern = [&](unsigned count, unsigned value) {
        for (unsigned i = 0; i < count; ++i) c.put8(position++, value + i);
    };
    word(test.empty ? 0 : 3, 4);
    word(test.empty ? 0 : 1, 4);
    if (!test.empty && test.failure.back != 0x103dfa48) word(2, 4);
    word(test.empty ? 0 : 3, 4);
    const bool has_negotiation = !test.empty && test.failure.back != 0x103dfa48;
    word(has_negotiation, 4);
    if (has_negotiation && test.failure.back != 0x103dfccc) word(2, 4);
    for (unsigned i = 0; i < 150; ++i) {
        const bool present = !test.empty && i == 0;
        word(present, 2);
        if (present) word(0x12345678, 4);
    }
    word(1, 1);
    word(0x00f407d2, 4);
    word(0x11223344, 4);
    word(5, 1);
    word(0x1122aabb, 4);
    word(0x114477ee, 4);
    const auto array = [&](unsigned i) {
        word(test.empty ? 0 : 2, 4);
        if (!test.empty) pattern(2 * arrays[i].width, 0x10 + i * 7);
    };
    array(0);
    array(1);
    if (!test.empty) pattern(20, 0x70);
    word(!test.empty, 2);
    if (!test.empty) pattern(16, 0x80);
    array(2);
    array(3);
    array(4);
    if (!test.empty) pattern(32, 0x90);
    word(!test.empty, 2);
    if (!test.empty) pattern(test.legacy ? 5 : 12, 0x40);
    array(5);
    word(!test.empty, 2);
    if (!test.empty) pattern(100, 0x30);
    c.put32(effects + 4, position - stream);
}

std::unique_ptr<State> fixture_state(const Case& test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0x24000);
    s->memory.map(toc - 0x8000, 0x4000);
    s->memory.map(stack - 0x3000, 0x4000);
    s->pc = load;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, test.null_request ? 0 : request}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7b50, globals);
    c.put32(toc - 0x7be4, globals + 4);
    c.put32(toc - 0x7b74, globals + 8);
    c.put32(toc - 0x7bac, globals + 12);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x79dc, title);
    c.put32(toc - 0x79e0, body);
    c.put32(toc - 0x5b20, globals + 0x40);
    c.put32(globals, test.empty ? 0 : 2);
    c.put32(globals + 4, test.empty ? 0 : 2);
    c.put32(globals + 8, test.legacy ? 0u - 0x3e002 : 0x3e003);
    c.put8(globals + 12, 1);
    c.put32(manager + 0x9a0, staff);
    c.put32(manager + 0xa4c, clubs);
    seed_stream(c, test);
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
    load_transfer_manager(c, c.r(3), c.r(4));
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    production_route = true;
    s.pc = entry;
    s.budget = s.executed;
    run_until(s, return_sentinel);
    require(!leaf_allowance, "Transfer load finished inside a leaf allowance");
    production_route = false;
}

unsigned checked = 0;
void check(const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    auto s = fixture_state(test);
    auto control = clone(*s);
    original(*control, load);
    Cpu c(*control);
    const auto expected = test.null_request || test.missing_stream ? 0 : test.failure.back ? test.failure.result : 1;
    require(c.r(3) == expected && c.u32(effects + 8) == !(test.null_request || test.missing_stream) &&
            c.u32(effects + 16) == (test.missing_stream ? 0x163 : test.failure.line) &&
            c.u8(globals + 12) == (test.failure.back != 0x103e0114), "Original transfer load result");
    if (expected && (!test.failure.back || test.failure.back == 0x103dfa48 || test.failure.back == 0x103dfccc)) {
        require(c.u32(effects) == c.u32(effects + 4), "Original transfer load byte count");
        require(c.u32(shortlist) == 0x1122aabb && c.u32(manager + 0x98c) == 0x114477ee, "Original transfer load fields");
        if (!test.empty) {
            const bool offer_found = test.failure.back != 0x103dfa48;
            const bool negotiation_found = offer_found && test.failure.back != 0x103dfccc;
            require(c.u32(offers + 8) == (offer_found ? offer : 0) &&
                    c.u32(negotiations + 4) == (negotiation_found ? negotiation : 0), "Original restored sparse offers");
            for (const auto& row : arrays)
                require(c.u32(manager + row.pointer) == records + row.data_offset &&
                        c.u32(manager + row.capacity) == 2 && c.u32(manager + row.count) == 2,
                        "Original transfer load array");
            require(c.u32(effects + 12) == 4 && c.u32(stored_lists) == 0x12345678 &&
                    c.u32(stored_lists + 2 * 128 + 8) == (test.legacy ? 0xffffffffu : 0x48494a4bu),
                    "Original transfer activity conversion");
        }
    }
    const auto differences = compare_routine_call(*s, load, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Transfer load case " + std::to_string(checked));
    ++checked;
}

void change_loaded_activity(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put32(stored_lists + 2 * 128 + 8, 0);
}

void check_negative_control() {
    active = {.legacy = true};
    auto s = fixture_state(active);
    const auto differences = compare_routine_call(*s, load, original, change_loaded_activity,
        RoutineChecker::result_r3, false, true);
    require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 30004108"); }),
            "The transfer load comparison missed an altered legacy field");
}
}

int cm::aot::transfer_manager::checks::load_section(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check({}, production);
            check({.legacy = true}, production);
            check({.failure = {0x103e0114, 0x20a}}, production);
            std::cout << checked << " transfer load production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown transfer load check mode");
        check({});
        check({.null_request = true});
        check({.missing_stream = true});
        check({.empty = true});
        check({.legacy = true});
        check({.drop_pairs_allocation = true});
        for (const auto failure : read_failures) check({.failure = failure});
        for (const auto failure : allocation_failures) check({.failure = failure});
        check_negative_control();
        std::cout << checked << " transfer load comparisons, including legacy activity and failure controls\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
