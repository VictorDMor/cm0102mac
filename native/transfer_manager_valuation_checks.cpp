#include "transfer_manager_valuation.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::semantic_io_checks;
using namespace cm::aot::guest;

constexpr std::uint32_t price = 0x103fac70, eligibility = 0x103faff0;
constexpr std::uint32_t staff = fixture + 0x100, contracts = fixture + 0x300,
    contract = fixture + 0x500, clubs = fixture + 0x800, buyer = clubs + 0x248,
    seller_nation = fixture + 0x1000, buyer_nation = fixture + 0x1100,
    globals = fixture + 0x2000, today = fixture + 0x2100, pool = fixture + 0x3000,
    message = fixture + 0x4000, trace = fixture + 0x5000;

struct Case {
    bool null_person = false, null_buyer = false, player = true, staff_record = false,
         limited = false, has_contract = true, seller_has_nation = true, buyer_has_nation = true,
         seller_domestic = false, buyer_domestic = false, seller_accepts = true, buyer_accepts = true;
    std::uint32_t mode = 0;
    std::int8_t kind = 2, age = 23;
    std::uint32_t wage = 700, days = 84, base = 5000, nation_result = 1;
    std::uint8_t kind_flags = 0;
};
Case active;

void args(Cpu& c, std::initializer_list<std::uint32_t> values) {
    unsigned r = 3;
    for (auto expected : values) {
        require(c.r(r) == expected, "Valuation call " + hex_address(c.s.pc) + " LR " +
                hex_address(c.lr()) + " argument r" + std::to_string(r));
        ++r;
    }
}

unsigned trace_call(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 12, "Valuation dependency loop");
    c.put32(trace, count + 1);
    auto cursor = trace + 4 + count * 160;
    const auto append = [&](std::uint32_t value) { c.put32(cursor, value); cursor += 4; };
    append(c.s.pc);
    append(c.lr());
    for (unsigned r = 3; r <= 10; ++r) append(c.r(r));
    for (unsigned r = 13; r < 32; ++r) append(c.r(r));
    for (unsigned field = 0; field < 8; ++field) append(c.cr(field));
    append(c.s.r<1>(reg::xer_ca));
    return count;
}

struct VersionRead final : MemoryObserver {
    explicit VersionRead(State& s) : state(s) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void read(std::uint32_t address, const std::uint8_t*, unsigned size) override {
        if (address != toc - 0x5b20) return;
        Cpu c(state);
        require(state.pc == 0x10768810 && size == 4 && (c.lr() == 0x103facd4 || c.lr() == 0x103fb048),
                "Unexpected version accessor");
        trace_call(c);
    }
    State& state;
};

bool dependency(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr();
    const auto date = c.sp() + 0x40, name = c.sp() + 0x48, extension = c.sp() + 0x38;
    std::uint32_t answer = 0;
    switch (target) {
    case price:
    case eligibility:
        require(back == return_sentinel, "Unexpected nested valuation call in the isolated fixture");
        return false;
    case 0x10715880:
        require(back == 0x103faca8 || back == 0x103fb024, "Unexpected Date call site");
        args(c, {date});
        break;
    case 0x10082f20:
        require(back == 0x103fad38 || back == 0x103faddc || back == 0x103fae80 || back == 0x103fb09c,
                "Unexpected primary-contract call site");
        args(c, {contracts, staff});
        answer = active.has_contract ? contract : 0;
        break;
    case 0x10718130:
        require(back == 0x103fad70 || back == 0x103fae14, "Unexpected date interval site");
        args(c, {contract + 0x30, c.u32(today), c.u32(today + 4)});
        answer = active.days;
        break;
    case 0x10084be0:
        require(back == 0x103fae60 || back == 0x103faf58 || back == 0x103fafc4, "Unexpected base-value site");
        args(c, {contracts, staff, clubs, 1, 0xffffffff, 0});
        answer = active.base;
        break;
    case 0x100c6a10: {
        require(back == 0x103faf74, "Unexpected age call site");
        args(c, {staff, date});
        const auto selected = active.kind == 2 ? contract + 0x30 : today;
        require(c.u32(date) == c.u32(selected) && c.u8(date + 4) == c.u8(selected + 4), "Age calculation date");
        answer = std::uint32_t(std::int32_t(active.age));
        break;
    }
    case 0x100d34c0:
        require(back == 0x103faf90 || back == 0x103fafa0, "Unexpected nation fee site");
        args(c, {back == 0x103faf90 ? buyer_nation : seller_nation});
        answer = back == 0x103faf90 ? active.buyer_accepts : active.seller_accepts;
        break;
    case 0x100d33d0:
        require(back == 0x103fb10c, "Unexpected nation eligibility site");
        args(c, {seller_nation});
        answer = active.nation_result;
        break;
    case 0x10739890:
        require(back == 0x103facd0 || back == 0x103fb044, "Unexpected assertion filename site");
        args(c, {pool + 0x37, 0, 0, name, extension});
        c.put32(name, 0x5452414e);
        c.put32(extension, 0x43505000);
        break;
    case 0x10768810: return false; // Its fast leaf route is observed at the TOC read.
    case 0x1077b720:
        require(back == 0x103facf0 || back == 0x103fb064, "Unexpected assertion format site");
        args(c, {message, pool + 0x6ff, globals + 0x40, name, extension,
                 back == 0x103facf0 ? 0x372du : 0x3797u});
        c.put32(message, c.r(8));
        break;
    case 0x10769b60:
        require(back == 0x103fad00 || back == 0x103fb074, "Unexpected assertion report site");
        args(c, {pool + 0x70c, message, 0});
        require(c.u32(message) != 0, "Assertion did not format its diagnostic");
        break;
    default:
        throw std::runtime_error("Unexpected valuation dependency " + hex_address(target));
    }
    const auto count = trace_call(c);
    if (target == 0x10715880) return false; // Execute the real date initializer.
    for (unsigned r = 3; r <= 10; ++r) c.set(r, 0x9a000000 + count * 0x100 + r);
    c.set(3, answer);
    c.set_cr(0, 2);
    s.pc = back;
    return true;
}

std::unique_ptr<State> make_state(std::uint32_t entry, Case test) {
    auto s = std::make_unique<State>();
    s->memory.map(fixture, 0x10000);
    s->memory.map(stack - 0x1000, 0x2000);
    s->memory.map(toc - 0x8000, 0x4000);
    s->registers.fill(0x71);
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, std::uint8_t(i));
    for (unsigned field = 1024; field < 1028; ++field) s->wr<1>(field, 0);
    for (unsigned field = 1280; field < 1312; ++field) s->wr<1>(field, 0);
    c.set({{1, stack}, {2, toc}, {3, globals}, {4, test.null_person ? 0 : staff},
           {5, test.null_buyer ? 0 : buyer}, {6, test.mode}});
    c.set_lr(return_sentinel);
    s->pc = entry;
    s->budget = 100000;
    c.put32(toc - 0x7b80, today);
    c.put32(toc - 0x7b7c, globals);
    c.put32(toc - 0x7be8, globals + 4);
    c.put32(toc - 0x7b78, globals + 8);
    c.put32(toc - 0x7bac, globals + 0xc);
    c.put32(toc - 0x7a68, globals + 0xd);
    c.put32(toc - 0x7514, globals + 0x10);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x5b20, globals + 0x40);
    c.put32(globals, contracts);
    c.put32(globals + 4, clubs);
    c.put8(globals + 0xc, 1);
    c.put8(globals + 0xd, test.limited);
    c.put32(globals + 0x10, 1);
    c.put32(today, 0x000407d2);
    c.put8(today + 4, 0);
    c.put32(staff + 0x14, test.player ? staff + 0x40 : 0);
    c.put32(staff + 0x1c, test.staff_record ? staff + 0x80 : 0);
    c.put32(contract + 0xc, test.wage);
    c.put32(contract + 0x30, 0x006407d4);
    c.put8(contract + 0x34, 1);
    c.put8(contract + 0x38, std::uint32_t(std::uint8_t(test.kind)) * 4 | test.kind_flags);
    c.put32(clubs + 0x2c, test.seller_has_nation ? seller_nation : 0);
    c.put32(buyer + 0x2c, test.buyer_has_nation ? buyer_nation : 0);
    c.put32(seller_nation + 0x68, test.seller_domestic ? 1 : 2);
    c.put32(buyer_nation + 0x68, test.buyer_domestic ? 1 : 3);
    return s;
}

void original(State& s, std::uint32_t entry) {
    s.pc = entry;
    while (s.pc != return_sentinel) {
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
}

void modern(State& s, std::uint32_t entry) {
    Cpu c(s);
    if (entry == price) transfer_manager::transfer_price(c, c.r(4), c.r(5), c.r(6));
    else transfer_manager::transfer_eligibility(c, c.r(4));
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    s.pc = entry;
    // The external Date owner charges one step. All other dependencies in
    // these two routing cases are scripted and charge no steps.
    s.budget = s.executed + 1;
    run_until(s, return_sentinel);
    require(s.executed == s.budget, "Production valuation did not reach its Date dependency");
}

unsigned comparisons = 0;
void check(std::uint32_t entry, Case test, std::uint32_t expected, unsigned calls, RoutineRunner rewrite = modern) {
    active = test;
    auto s = make_state(entry, test);
    auto control = clone(*s);
    VersionRead original_reads(*control), comparison_reads(*s);
    control->memory.date_observer = &original_reads;
    s->memory.date_observer = &comparison_reads;
    original(*control, entry);
    Cpu c(*control);
    require(c.r(3) == expected, "Unexpected original valuation result in case " + std::to_string(comparisons));
    require(c.u32(trace) == calls, "Unexpected original call count in case " + std::to_string(comparisons));
    const auto differences = compare_routine_call(*s, entry, original, rewrite,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& line : differences) std::cerr << line << '\n';
    require(differences.empty(), "Valuation comparison " + std::to_string(comparisons) + " " + hex_address(entry));
    ++comparisons;
}
}

int cm::aot::transfer_manager::checks::valuation(int argc, char** argv) {
    try {
        transfer_manager::install_check_coverage();
        dispatch_probe = dependency;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check(price, {}, 2500, 6, production);
            check(eligibility, {}, 1, 3, production);
            std::cout << "Production valuation and eligibility reached their external dependencies and returned\n";
            return 0;
        }
        require(argc == 1, "Unknown transfer valuation check mode");
        check(price, {.null_person = true}, 0, 5);
        check(price, {.null_buyer = true}, 0, 5);
        check(price, {.player = false, .staff_record = true, .has_contract = false}, 0, 2);
        check(price, {.player = false, .staff_record = true, .kind = 1}, 0, 2);
        check(price, {.player = false, .staff_record = true, .wage = 7}, 1000, 3);
        check(price, {.player = false, .staff_record = true}, 8400, 3);
        check(price, {.player = false, .staff_record = true, .wage = 100'000}, 1'000'000, 3);
        check(price, {.limited = true, .has_contract = false}, 0, 2);
        check(price, {.limited = true, .kind = 5}, 0, 2);
        check(price, {.limited = true}, 6700, 4);
        check(price, {.limited = true, .wage = 0xfffffff9, .base = 1}, 0xffffffd7, 4);
        check(price, {.limited = true, .mode = 0x100, .kind_flags = 3}, 6700, 4);
        check(price, {.has_contract = false}, 0, 2);
        check(price, {.kind = 5}, 0, 2);
        check(price, {.seller_has_nation = false}, 0, 2);
        check(price, {.buyer_has_nation = false}, 0, 2);
        check(price, {.seller_domestic = true, .buyer_domestic = true}, 2500, 3);
        check(price, {.seller_domestic = true, .age = 24}, 0, 3);
        check(price, {.buyer_accepts = false, .kind = -1}, 0, 4);
        check(price, {.buyer_accepts = false, .kind = -1, .kind_flags = 2}, 0, 4);
        check(price, {.seller_accepts = false, .kind = 3}, 0, 5);
        check(price, {.limited = true, .mode = 0x101}, 2500, 6);
        check(price, {.player = false}, 2500, 6);
        check(eligibility, {.null_person = true}, 0, 5);
        check(eligibility, {.has_contract = false}, 1, 2);
        check(eligibility, {.kind = 5}, 1, 2);
        check(eligibility, {.seller_has_nation = false}, 1, 2);
        check(eligibility, {.nation_result = 0}, 0, 3);
        check(eligibility, {.kind = -1, .nation_result = 0xffffffff, .kind_flags = 3}, 0xffffffff, 3);
        check(eligibility, {}, 1, 3);
        constexpr std::uint32_t seed = 0x5456414c;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 32; ++i) {
            const auto base = std::uint32_t(random());
            check(price, {.base = base}, std::uint32_t(std::int32_t(base) / 2), 6);
            const auto wage = std::uint32_t(random()), days = std::uint32_t(random() % 1001);
            const auto weekly = std::int32_t(wage * days) / 7;
            const auto sum = std::uint32_t(weekly) + base;
            check(price, {.limited = true, .wage = wage, .days = days, .base = base},
                  std::uint32_t(std::int32_t(sum) / 2), 4);
        }
        std::cout << comparisons << " transfer valuation and eligibility comparisons (seed " << seed << ")\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
