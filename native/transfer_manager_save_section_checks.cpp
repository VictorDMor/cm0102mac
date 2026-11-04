#include "transfer_manager_save.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <ranges>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t save = 0x103e1220, open_archive = 0x107475b0, close_archive = 0x10747900,
    write_archive = 0x10747cc0, next_list = 0x10738e00, save_offer = 0x104123e0, save_shortlist = 0x10358b30;
constexpr std::uint32_t manager = fixture, request = fixture + 0xb00, file = fixture + 0xb20,
    offers = fixture + 0x1000, negotiations = fixture + 0x1100, offer_one = fixture + 0x1200,
    offer_two = fixture + 0x1300, negotiation = fixture + 0x1400, shortlist = fixture + 0x1500,
    data = fixture + 0x2000, globals = fixture + 0x3000, lists = fixture + 0x4000,
    list_rows = fixture + 0x4400, nodes = fixture + 0x4800, pool = fixture + 0x5000,
    message = fixture + 0x7000, title = fixture + 0x7100, body = fixture + 0x7200,
    effects = fixture + 0x8000, trace = fixture + 0x10000, stream = fixture + 0x20000;
constexpr auto save_frame = stack - 0x520;

struct Failure { std::uint32_t back, line; };
constexpr std::array failures{
    Failure{0x103e1334, 0x35a}, Failure{0x103e1408, 0x36a}, Failure{0x103e1518, 0x38a},
    Failure{0x103e15a8, 0x392}, Failure{0x103e1698, 0x3b3}, Failure{0x103e177c, 0x3c4},
    Failure{0x103e180c, 0x3cc}, Failure{0x103e189c, 0x3d4}, Failure{0x103e1938, 0x3df},
    Failure{0x103e19c8, 0x3e7}, Failure{0x103e1a58, 0x3f0}, Failure{0x103e1aec, 0x3f8},
    Failure{0x103e1b7c, 0x401}, Failure{0x103e1c10, 0x409}, Failure{0x103e1cac, 0x412},
    Failure{0x103e1d48, 0x41f}, Failure{0x103e1dec, 0x428}, Failure{0x103e1e7c, 0x431},
    Failure{0x103e1f10, 0x439}, Failure{0x103e1fa0, 0x442}, Failure{0x103e2034, 0x44a},
    Failure{0x103e20c4, 0x453}, Failure{0x103e2158, 0x45b}, Failure{0x103e21f4, 0x464},
    Failure{0x103e228c, 0x470}, Failure{0x103e2330, 0x479}, Failure{0x103e23c0, 0x482},
    Failure{0x103e2470, 0x48b}, Failure{0x103e2518, 0x49a},
};
constexpr std::uint32_t unchecked_group_row = 0x103e1734;

struct Case {
    Failure failure{};
    bool missing_stream = false, zero_counts = false, offers_all_holes = false, has_pairs = true;
};
Case active;
bool original_route = false, production_route = false;

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 400, "Transfer save trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 112;
    const auto add = [&](std::uint32_t word) { c.put32(out, word); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 9; ++r) add(c.r(r));
    for (unsigned r = 20; r <= 31; ++r) add(c.r(r));
    add(c.cr(0));
    add(c.s.r<1>(reg::xer_ca));
    add(c.s.r<4>(reg::ctr));
}

void push(Cpu& c, std::uint32_t address, unsigned size) {
    const auto count = c.u32(effects);
    require(count + size < 0x2000, "Transfer save buffer exhausted");
    if (size) c.s.memory.copy_in(stream + count, c.s.memory.copy_out(address, size));
    c.put32(effects, count + size);
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    std::uint32_t answer = 0;
    switch (target) {
    case open_archive:
        require(back == 0x103e1264 && a == request && b == pool + 0x257 && d == 1 && c.r(6) == 22 && c.r(7) == 4,
                "Transfer save archive open");
        answer = active.missing_stream ? 0 : file;
        break;
    case close_archive:
        require(a == save_frame + 0x3c && c.u32(a) == file && !c.u32(effects + 4), "Transfer save archive close");
        c.put32(a, 0);
        c.put32(effects + 4, 1);
        break;
    case write_archive:
        require(c.r(6) == file && !c.u32(effects + 4) && b > 0 && b <= 100 && d < 8,
                "Transfer save write boundary");
        if (back == active.failure.back) {
            require(d > 0, "The injected short write must request a record");
            answer = d - 1;
        } else answer = d;
        push(c, a, b * answer);
        c.put32(effects + 8, c.u32(effects + 8) + 1);
        break;
    case next_list:
        require((back == 0x103e173c && a >= manager + 0x18 && a < manager + 0x978) ||
                (back == 0x103e1dd0 && a == manager + 0x9a4) || (back == 0x103e2314 && a == manager + 0xa5c) ||
                (back == 0x103e25a0 && a == lists), "Transfer save list iterator");
        record(c);
        if (!production_route || original_route) return false;
        s.budget = s.executed + 8;
        require(dispatch_page(s), "Missing transfer save list iterator");
        if (s.pc != back) run_until(s, back);
        s.budget = s.executed;
        return true;
    case save_offer:
        require(b == file && ((back == 0x103e14ac && (a == offer_one || a == offer_two)) ||
                (back == 0x103e1648 && a == negotiation)), "Offer serialization dependency");
        push(c, a, 4);
        break;
    case save_shortlist:
        require(back == 0x103e1924 && a == shortlist && b == file, "Shortlist serialization dependency");
        push(c, a, 4);
        break;
    case 0x10739890:
        require(a == pool + 0x37 && !b && !d && c.r(6) >= save_frame + 0x130 && c.r(6) <= save_frame + 0x4d0 &&
                c.r(7) >= save_frame + 0x40 && c.r(7) <= save_frame + 0x128, "Transfer save diagnostic source split");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        if (production_route && !original_route) s.budget = s.executed + 2;
        break;
    case 0x1077b720:
        require(a == message && b == pool + 0x4c && d == globals + 0x40 && c.u32(c.r(6)) == 0x5452414e &&
                c.u32(c.r(7)) == 0x43505000 && c.r(8) == (active.missing_stream ? 0x346 : active.failure.line),
                "Transfer save diagnostic format");
        if (production_route && !original_route)
            require(s.executed == s.budget, "Save diagnostic did not use the two-step version leaf");
        c.put32(message, c.r(8));
        c.put32(effects + 12, c.r(8));
        break;
    case 0x10727890:
        require((a == title && b == pool + 0xebf) || (a == body && b == pool + 0xecf && d == message),
                "Transfer save diagnostic text");
        c.put32(a, b);
        break;
    case 0x10769b60:
        require(a == title && b == body && !d, "Transfer save diagnostic display");
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xa9000000 + r);
    c.set_cr(0, 4);
    c.set(3, answer);
    c.finish();
    return true;
}

void make_list(Cpu& c, std::uint32_t address, unsigned index) {
    const auto node = nodes + index * 16;
    c.put16(address, 1);
    c.put32(address + 4, node);
    c.put32(address + 8, node);
    c.put32(address + 0xc, node + 8);
    c.put32(node, list_rows + index * 128);
}

struct Array { std::uint32_t pointer, count, offset, width; };
constexpr std::array arrays{
    Array{0x994, 0x99c, 0x000, 40}, Array{0xa50, 0xa58, 0x100, 12},
    Array{0x9b4, 0x9bc, 0x300, 16}, Array{0x9c0, 0x9c8, 0x400, 16},
    Array{0x9cc, 0x9d4, 0x500, 16}, Array{0xa6c, 0xa70, 0x700, 16},
};

std::unique_ptr<State> fixture_state(const Case& test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0x24000);
    s->memory.map(toc - 0x8000, 0x4000);
    s->memory.map(stack - 0x3000, 0x4000);
    s->pc = save;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, request}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7b50, globals);
    c.put32(toc - 0x7be4, globals + 4);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x79dc, title);
    c.put32(toc - 0x79e0, body);
    c.put32(toc - 0x5b20, globals + 0x40);
    const auto count = test.zero_counts ? 0 : 2;
    c.put32(globals, count);
    c.put32(globals + 4, count);
    c.put32(manager, offers);
    c.put32(manager + 4, test.zero_counts ? 0 : 5);
    c.put32(manager + 8, 99);
    c.put32(manager + 0xc, negotiations);
    c.put32(manager + 0x10, test.zero_counts ? 0 : 3);
    c.put32(manager + 0x14, test.zero_counts ? 0 : 1);
    if (!test.offers_all_holes && !test.zero_counts) {
        c.put32(offers, offer_one);
        c.put32(offers + 8, offer_two);
        c.put32(negotiations + 4, negotiation);
    }
    c.put32(offer_one, 0xaabbcc01);
    c.put32(offer_two, 0xaabbcc02);
    c.put32(negotiation, 0xaabbcc03);
    c.put32(shortlist, 0xaabbcc04);
    c.put32(manager + 0x988, shortlist);
    c.put8(manager + 0x978, 1);
    c.put32(manager + 0x97c, 0x00f407d2);
    c.put32(manager + 0x980, 0x11223344);
    c.put8(manager + 0x984, 5);
    c.put32(manager + 0x98c, 0x114477ee);
    for (const auto& row : arrays) {
        c.put32(manager + row.pointer, data + row.offset);
        c.put32(manager + row.count, count);
    }
    c.put32(manager + 0x9a0, data + 0x200);
    c.put32(manager + 0xa4c, data + 0x600);
    for (unsigned i = 0; i < 0x800; ++i) c.put8(data + i, 9 * (i >> 8) + i);
    for (unsigned i = 0; i < 0x200; ++i) c.put8(list_rows + i, 17 + i * 3);
    if (!test.zero_counts) {
        make_list(c, manager + 0x18, 0);
        make_list(c, manager + 0x9a4, 1);
        make_list(c, manager + 0xa5c, 2);
        if (test.has_pairs) make_list(c, lists, 3);
    }
    if (test.has_pairs) c.put32(manager + 0xa78, lists);
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
    save_transfer_manager(c, c.r(3), c.r(4));
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    production_route = true;
    s.pc = entry;
    s.budget = s.executed;
    run_until(s, return_sentinel);
    production_route = false;
}

std::vector<std::uint8_t> expected_bytes(State& start, const Case& test) {
    std::vector<std::uint8_t> bytes;
    const auto word = [&](std::uint32_t value, unsigned size) {
        for (unsigned i = size; i > 0; --i) bytes.push_back(std::uint8_t(value >> ((i - 1) * 8)));
    };
    const auto raw = [&](std::uint32_t address, unsigned size) {
        if (!size) return;
        const auto chunk = start.memory.copy_out(address, size);
        bytes.insert(bytes.end(), chunk.begin(), chunk.end());
    };
    const bool has_offers = !test.offers_all_holes && !test.zero_counts;
    word(test.zero_counts ? 0 : has_offers ? 3 : 5, 4);
    word(has_offers ? 2 : 0, 4);
    if (has_offers) { raw(offer_one, 4); raw(offer_two, 4); }
    word(test.zero_counts ? 0 : has_offers ? 2 : 3, 4);
    word(test.zero_counts ? 0 : 1, 4);
    if (has_offers) raw(negotiation, 4);
    for (unsigned list = 0; list < 150; ++list) {
        const bool has_item = !test.zero_counts && list == 0;
        word(has_item, 2);
        if (has_item && test.failure.back != unchecked_group_row) raw(list_rows, 4);
    }
    raw(manager + 0x978, 1);
    raw(manager + 0x97c, 8);
    raw(manager + 0x984, 1);
    raw(shortlist, 4);
    raw(manager + 0x98c, 4);
    const auto array = [&](unsigned index) {
        const auto& row = arrays[index];
        const unsigned count = test.zero_counts ? 0 : 2;
        word(count, 4);
        raw(data + row.offset, count * row.width);
    };
    array(0);
    array(1);
    raw(data + 0x200, test.zero_counts ? 0 : 20);
    word(!test.zero_counts, 2);
    if (!test.zero_counts) raw(list_rows + 128, 16);
    array(2);
    array(3);
    array(4);
    raw(data + 0x600, test.zero_counts ? 0 : 32);
    word(!test.zero_counts, 2);
    if (!test.zero_counts) raw(list_rows + 256, 12);
    array(5);
    word(test.has_pairs && !test.zero_counts, 2);
    if (test.has_pairs && !test.zero_counts) raw(list_rows + 384, 100);
    return bytes;
}

unsigned checked = 0;
void check(const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    auto s = fixture_state(test);
    auto control = clone(*s);
    original(*control, save);
    Cpu c(*control);
    const auto length = c.u32(effects), line = test.missing_stream ? 0x346 : test.failure.line;
    require(c.u32(effects + 12) == line && c.u32(effects + 4) == !test.missing_stream,
            "Original transfer save completion or failure state");
    const auto bytes = expected_bytes(*s, test);
    if (!line) require(length == bytes.size() && c.s.memory.copy_out(stream, length) == bytes,
                       "Original transfer section byte layout");
    else require(length < bytes.size() && std::ranges::equal(c.s.memory.copy_out(stream, length), bytes | std::views::take(length)),
                 "Original transfer section prefix after a failed write");
    const auto differences = compare_routine_call(*s, save, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Transfer save case " + std::to_string(checked));
    ++checked;
}

void change_serialized_count(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put32(stream, 0);
}

void check_negative_control() {
    active = {};
    auto s = fixture_state(active);
    const auto differences = compare_routine_call(*s, save, original, change_serialized_count,
        RoutineChecker::result_r3, false, true);
    require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 30020003"); }),
            "The transfer save comparison missed an altered serialized count");
}
}

int cm::aot::transfer_manager::checks::save_section(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check({}, production);
            check({.zero_counts = true, .has_pairs = false}, production);
            check({.failure = failures.back()}, production);
            std::cout << checked << " transfer save production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown transfer save check mode");
        check({});
        check({.zero_counts = true});
        check({.offers_all_holes = true, .has_pairs = false});
        check({.missing_stream = true});
        for (const auto failure : failures) check({.failure = failure});
        check({.failure = {unchecked_group_row, 0}});
        check_negative_control();
        std::cout << checked << " transfer save comparisons, including every checked write failure and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
