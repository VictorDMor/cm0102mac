#include "transfer_manager_rejection.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::semantic_io_checks;
using namespace cm::aot::guest;

constexpr std::uint32_t from_status = 0x103ea350, allowed = 0x103ea440, describe = 0x103ea600, to_status = 0x103ea760;
constexpr std::uint32_t people = fixture + 0x1000, staff = people + 0x70, clubs = fixture + 0x2000,
    buyer = clubs + 0x248, primary = fixture + 0x3000,
    globals = fixture + 0x4000, contracts = globals + 0x100, offer = fixture + 0x5000,
    pool = fixture + 0x6000, message = fixture + 0x8000, output = fixture + 0x9000, trace = fixture + 0xa000;

struct Case {
    std::uint32_t reason = 0;
    std::int32_t capacity = 50;
    std::int16_t age = 23;
    std::uint8_t status = 0;
    bool primary_contract = true, same_division = true,
         buyer_controlled = false, seller_controlled = false, rival_service = false, null_buyer = false;
    int named_rival = -1;
};
Case active;

struct Message {
    std::uint16_t reason;
    std::uint32_t source, back;
};
constexpr std::array messages{
    Message{0, 0x18ae, 0x103ea6e4}, Message{10, 0x19a0, 0x103ea734},
    Message{0x200, 0x18e9, 0x103ea6f4}, Message{0x400, 0x1912, 0x103ea704},
    Message{0x800, 0x19c2, 0x103ea744}, Message{0x1000, 0x196d, 0x103ea724},
    Message{0x2000, 0x1940, 0x103ea714},
};

void record_call(Cpu& c) {
    const auto calls = c.u32(trace);
    require(calls < 16, "Transfer reason trace overflow");
    c.put32(trace, calls + 1);
    auto cursor = trace + 4 + calls * 128;
    const auto add = [&](std::uint32_t value) { c.put32(cursor, value); cursor += 4; };
    add(c.s.pc);
    add(c.lr());
    for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 27; r <= 31; ++r) add(c.r(r));
    add(c.cr(0));
}

struct VersionRead final : MemoryObserver {
    explicit VersionRead(State& s) : state(s) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void read(std::uint32_t address, const std::uint8_t*, unsigned) override {
        if (address != toc - 0x5b20) return;
        Cpu c(state);
        require(state.pc == 0x10768810 && c.lr() == 0x103ea640, "Reason diagnostic version site");
        record_call(c);
    }
    State& state;
};

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    std::uint32_t answer = 0;
    switch (target) {
    case 0x10082f20:
        require(a == contracts && b == staff &&
                (back == 0x103ea390 || back == 0x103ea484 || back == 0x103ea794),
                "Reason primary contract arguments");
        answer = active.primary_contract ? primary : 0;
        break;
    case 0x10082f90:
        require(a == contracts && b == staff && d == (active.primary_contract ? clubs : 0) &&
                (back == 0x103ea3c4 || back == 0x103ea7c8), "Reason matching contract arguments");
        answer = active.primary_contract ? primary : 0;
        break;
    case 0x1014c2e0:
        require((back == 0x103ea53c ? a == clubs : back == 0x103ea55c && a == buyer) &&
                b == 0 && d == 1 && c.r(6) == 0 && c.r(7) == 0, "Rival control arguments");
        answer = a == clubs ? active.seller_controlled : active.buyer_controlled;
        break;
    case 0x10464160:
        require(back == 0x103ea578 && a == buyer && b == clubs, "Rival service arguments");
        answer = active.rival_service;
        break;
    case 0x10727890: {
        const auto selected = std::ranges::find(messages, std::uint16_t(active.reason), &Message::reason);
        require(selected != messages.end() && a == output && b == pool + selected->source &&
                back == selected->back && d == (selected->reason == 0 ? pool + 0x18e8 : output),
                "Reason text ABI for " + hex_address(active.reason) + " at " + hex_address(back) +
                " r3=" + hex_address(a) + " r4=" + hex_address(b) + " r5=" + hex_address(d));
        c.put32(output, b);
        c.put8(output + 4, 0);
        answer = output;
        break;
    }
    case 0x10739890:
        require(back == 0x103ea63c && a == pool + 0x37 && b == 0 && d == 0 &&
                c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38, "Reason diagnostic split ABI");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        break;
    case 0x1077b720:
        require(back == 0x103ea65c && a == message && b == pool + 0x6ff && d == globals + 0x40 &&
                c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38 && c.r(8) == 0x163c,
                "Reason diagnostic formatting ABI");
        c.put32(message, c.r(8));
        break;
    case 0x10769b60:
        require(back == 0x103ea66c && a == pool + 0x70c && b == message && d == 0 && c.u32(message) == 0x163c,
                "Reason diagnostic display ABI");
        break;
    default: return false;
    }
    record_call(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xa8000000 + r);
    c.set_cr(0, 4);
    c.set(3, answer);
    c.finish();
    return true;
}

std::unique_ptr<State> make_state(std::uint32_t entry, Case test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0xc000);
    s->memory.map(stack - 0x1000, 0x2000);
    s->memory.map(toc - 0x8000, 0x4000);
    s->budget = 100000;
    s->pc = entry;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, std::uint8_t(i));
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, globals}, {4, offer}, {5, 0x12345678}});
    if (entry == allowed) c.set({{4, test.reason}, {5, offer}});
    else if (entry == describe) c.set({{4, test.reason}, {5, output}, {6, std::uint32_t(test.capacity)}});
    else if (entry == to_status) c.set({{4, staff}, {5, test.reason}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7be0, globals);
    c.put32(toc - 0x7b7c, globals + 4);
    c.put32(toc - 0x7b78, globals + 8);
    c.put32(toc - 0x7be8, globals + 12);
    c.put32(toc - 0x7bac, globals + 16);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x5b20, globals + 0x40);
    c.put32(globals, people);
    c.put32(globals + 4, contracts);
    c.put32(globals + 12, test.null_buyer ? 0 : clubs);
    c.put8(globals + 16, 1);
    c.put32(offer + 8, test.null_buyer ? 0 : 1);
    c.put32(offer + 0xc, 1);
    c.put32(primary + 4, test.null_buyer ? 1 : 0);
    c.put8(primary + 0x60, 0xb0 | test.status);
    c.put16(staff + 0x5c, std::uint32_t(test.age));
    c.put32(clubs + 0x30, clubs + 0x800);
    c.put32(buyer + 0x30, clubs + (test.same_division ? 0x800 : 0x900));
    if (test.named_rival >= 0) {
        const auto first = test.named_rival < 3 ? buyer : clubs;
        const auto second = test.named_rival < 3 ? clubs : buyer;
        c.put32(first + 0x74 + std::uint32_t(test.named_rival % 3) * 4, second);
    }
    return s;
}

void original(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    require(transfer_manager_reference(s), "Missing transfer reason reference");
    while (s.pc != stop)
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
}

void modern(State& s, std::uint32_t entry) {
    Cpu c(s);
    const auto a = c.r(4), b = c.r(5);
    using namespace transfer_manager;
    switch (entry) {
    case from_status: rejection_reason_from_squad_status(c, a); break;
    case allowed: rejection_reason_allowed(c, a, b); break;
    case describe: describe_rejection_reason(c, a, b, std::int32_t(c.r(6))); break;
    case to_status: squad_status_for_rejection_reason(c, a, b); break;
    default: throw std::runtime_error("Unknown transfer rejection entry");
    }
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    s.pc = entry;
    s.budget = 0;
    run_until(s, return_sentinel);
    require(s.executed == 0, "The rejection policy ran original instructions on the scripted normal route");
}

void sequence(State& s, RoutineRunner route) {
    Cpu c(s);
    route(s, from_status);
    const auto reason = c.r(3);
    require(reason == 0x2000, "Expected a prospect's rejection reason");
    c.set({{3, globals}, {4, reason}, {5, offer}});
    route(s, allowed);
    require(c.r(3) == 1, "Rejection reason should be available for this prospect");
    c.set({{3, globals}, {4, reason}, {5, output}, {6, 50}});
    route(s, describe);
    require(c.u32(output) == pool + 0x1940, "Wrong reason published to the rejection dialog");
    c.set({{3, globals}, {4, staff}, {5, reason}});
    route(s, to_status);
    require(c.r(3) == 5, "Rejection reason should select the prospect squad status");
}

void check_sequence() {
    active = {.reason = 0x2000, .status = 5};
    auto s = make_state(from_status, active);
    const auto differences = compare_routine_call(*s, from_status,
        [](State& state, std::uint32_t) { sequence(state, original); },
        [](State& state, std::uint32_t) { sequence(state, production); },
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& line : differences) std::cerr << line << '\n';
    require(differences.empty(), "Combined transfer rejection route differs");
}

unsigned comparisons = 0;
void check(std::uint32_t entry, Case test, std::uint32_t expected, std::uint32_t text = 0) {
    active = test;
    auto s = make_state(entry, test);
    auto control = clone(*s);
    VersionRead original_reads(*control), child_reads(*s);
    control->memory.date_observer = &original_reads;
    s->memory.date_observer = &child_reads;
    original(*control, entry);
    Cpu c(*control);
    require(c.r(3) == expected && c.u32(output) == text && c.u8(globals + 16) == (test.capacity >= 50),
            "Unexpected original rejection case " + std::to_string(comparisons));
    const auto differences = compare_routine_call(*s, entry, original, modern,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& line : differences) std::cerr << line << '\n';
    require(differences.empty(), "Transfer rejection comparison " + std::to_string(comparisons) + " " + hex_address(entry));
    ++comparisons;
}
}

int cm::aot::transfer_manager::checks::rejection(int argc, char** argv) {
    try {
        transfer_manager::install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check_sequence();
            std::cout << "The four production rejection routines agree in the controlled sequence\n";
            return 0;
        }
        require(argc == 1, "Unknown transfer rejection check mode");
        for (std::uint8_t status = 0; status < 16; ++status)
            check(from_status, {.status = status}, status == 1 ? 0x200 : status == 2 || status == 3 ? 0x400 : status == 5 ? 0x2000 : 0);
        check(from_status, {.primary_contract = false}, 0);
        check(to_status, {.reason = 0x2000, .primary_contract = false}, 0);
        for (std::uint32_t reason : {0u, 0x200u, 0x400u, 0x800u, 0x1000u, 0x2000u, 0x80000400u, 0xffffu})
            check(to_status, {.reason = reason, .status = 7},
                  std::uint16_t(reason) == 0x200 ? 1 : std::uint16_t(reason) == 0x400 ? 2 : std::uint16_t(reason) == 0x2000 ? 5 : 7);
        check(allowed, {}, 1);
        check(allowed, {.reason = 0x2000, .age = 24}, 0);
        check(allowed, {.reason = 0x2000, .age = 23}, 1);
        check(allowed, {.reason = 0x1000, .same_division = false}, 0);
        check(allowed, {.reason = 0x1000}, 1);
        check(allowed, {.reason = 0x1000, .primary_contract = false}, 1);
        check(allowed, {.reason = 0x800}, 0);
        check(allowed, {.reason = 0x800, .seller_controlled = true, .rival_service = true}, 1);
        check(allowed, {.reason = 0x800, .buyer_controlled = true, .seller_controlled = true}, 1);
        check(allowed, {.reason = 0x800, .rival_service = true}, 1);
        check(allowed, {.reason = 0x800, .primary_contract = false}, 1);
        check(allowed, {.reason = 0x800, .null_buyer = true}, 1);
        for (int i = 0; i < 6; ++i) check(allowed, {.reason = 0x800, .named_rival = i}, 1);
        for (const auto& message : messages)
            check(describe, {.reason = message.reason}, output, pool + message.source);
        for (std::uint32_t reason : {1u, 11u, 0x201u, 0x801u, 0x1001u, 0x2001u, 0xffffu})
            check(describe, {.reason = reason}, globals);
        check(describe, {.reason = 0x800, .capacity = 49}, globals + 16);
        constexpr std::uint32_t seed = 0x5452454a;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 20; ++i) {
            const auto& message = messages[random() % messages.size()];
            const auto upper_bits = std::uint32_t(random()) & 0xffff0000;
            check(describe, {.reason = upper_bits | message.reason}, output, pool + message.source);
        }
        std::cout << comparisons << " transfer rejection comparisons (seed " << seed << ")\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
