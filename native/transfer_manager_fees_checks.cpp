#include "transfer_manager_fees.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

struct Table {
    FeeKind kind;
    std::uint32_t append, remove, find, offset, key, byte, flag, diagnostic, label, compare;
};
constexpr auto sell_on_compare = 0x104119e0, appearance_compare = 0x10411a10;
constexpr std::array tables{
    Table{FeeKind::sell_on, 0x103f1e60, 0x103f29f0, 0x103fb400, 0x9b4, 12, 0, 8,
          0x2552, 0x1a3c, sell_on_compare},
    Table{FeeKind::first_team_appearance, 0x103f1fb0, 0x103f2aa0, 0x103fb510, 0x9c0, 0, 8, 2,
          0x2580, 0x1a7f, appearance_compare},
    Table{FeeKind::international_appearance, 0x103f2100, 0x103f2b50, 0x103fb5a0, 0x9cc, 0, 8, 4,
          0x25ae, 0x1ac9, appearance_compare},
};
constexpr std::uint32_t manager = fixture, staff = fixture + 0x1000, flags = fixture + 0x1100,
    rows = fixture + 0x2000, expanded = fixture + 0x2800, input = fixture + 0x3000,
    globals = fixture + 0x3100, ledger = fixture + 0x3200, primary = fixture + 0x3300,
    clubs = fixture + 0x4000, club = clubs + 0x248, vector = fixture + 0x4800,
    trace = fixture + 0x5000, pool = fixture + 0x7000, message = fixture + 0x9000,
    title = fixture + 0x9100, body = fixture + 0x9200;
constexpr std::uint32_t sort_rows = 0x10768300, search_rows = 0x10775600, primary_lookup = 0x10082f20;

struct Case {
    unsigned count = 3, capacity = 8, id = 3, input_id = 2, club_id = 7;
    bool flagged = true, has_array = true, allocation_fails = false, contract_found = true,
         second_contract_gone = false;
};
Case active;
const Table* table;
bool original_route = false, production_route = false;

void original(State&, std::uint32_t);
void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 72, "Transfer fee trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 64;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 9; ++r) add(c.r(r));
    add(c.cr(0));
}

unsigned calls(Cpu& c, std::uint32_t target) {
    unsigned count = 0;
    for (unsigned i = 0; i < c.u32(trace); ++i) count += c.u32(trace + 4 + i * 64) == target;
    return count;
}

bool real_collection_service(State& s, std::uint32_t back) {
    if (!production_route || original_route) return false;
    s.budget = s.executed + 15000;
    require(dispatch_page(s), "Missing transfer fee collection service");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    const auto entry = table->append, registry = manager + table->offset;
    std::uint32_t answer = 0;
    switch (target) {
    case sell_on_compare:
    case appearance_compare:
        require(target == table->compare, "Wrong transfer fee comparator");
        record(c);
        if (!original_route) return false;
        original(s, target);
        return true;
    case sort_rows:
        require(back == entry + 0x134 && a == c.u32(registry) && b == c.u32(registry + 8) &&
                d == 16 && c.r(6) == vector, "Transfer fee sort");
        record(c);
        return real_collection_service(s, back);
    case search_rows:
        require((back == 0x103fb4c8 || back == table->find + 0x50) && a == c.sp() + 0x38 &&
                b == c.u32(registry) && d == c.u32(registry + 8) && c.r(6) == 16 && c.r(7) == vector &&
                c.u32(a + table->key) == active.id, "Transfer fee search");
        if (table->kind == FeeKind::sell_on)
            require(c.u32(a + 4) == active.club_id, "Sell-on employer search key");
        record(c);
        return real_collection_service(s, back);
    case primary_lookup:
        require(a == ledger && b == staff && (back == 0x103fb444 || back == 0x103fb484),
                "Sell-on primary contract lookup");
        answer = active.contract_found && !(active.second_contract_gone && back == 0x103fb484) ? primary : 0;
        break;
    case 0x10775130:
        require(back == entry + 0x58 && !active.has_array && a == (active.capacity + 50) * 16,
                "Transfer fee allocation");
        answer = active.allocation_fails ? 0 : expanded;
        break;
    case 0x107751b0:
        require(back == entry + 0x6c && active.has_array && a == rows && b == (active.capacity + 50) * 16,
                "Transfer fee reallocation");
        if (!active.allocation_fails) {
            for (unsigned offset = 0; offset < (active.count + 1) * 16; offset += 4)
                c.put32(expanded + offset, c.u32(rows + offset));
            answer = expanded;
        }
        break;
    case 0x10739890:
        require(back == entry + 0x94 && a == pool + 0x37 && b == 0 && d == 0 &&
                c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38, "Transfer fee diagnostic split");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        break;
    case 0x1077b720:
        require(back == entry + 0xb8 && a == message && b == pool + 0x4c && d == globals + 0x40 &&
                c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38 &&
                c.r(8) == table->diagnostic && c.r(9) == pool + table->label, "Transfer fee diagnostic format");
        c.put32(a, c.r(8));
        break;
    case 0x10727890:
        require((back == entry + 0xc4 && a == title && b == pool + 0xa6) ||
                (back == entry + 0xd4 && a == body && b == pool + 0xbe && d == message),
                "Transfer fee diagnostic text");
        c.put32(a, b);
        break;
    case 0x10769b60:
        require(back == entry + 0xe4 && a == title && b == body && d == 0 &&
                c.u32(message) == table->diagnostic, "Transfer fee diagnostic display");
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xa2000000 + r);
    c.set_cr(0, 4);
    c.set(3, answer);
    c.finish();
    return true;
}

void write_row(Cpu& c, std::uint32_t row, unsigned id, unsigned stamp) {
    c.put32(row, 0x39000000 | stamp);
    c.put32(row + 4, 7);
    c.put32(row + 8, 0x16000000 | stamp);
    c.put32(row + 12, 0x11220000 | stamp);
    c.put32(row + table->key, id);
}

std::unique_ptr<State> fixture_state(std::uint32_t entry, const Case& test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0xa000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    if (test.second_contract_gone) s->memory.map(0, 4);
    s->pc = entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, entry == table->append ? input : staff}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7be8, globals);
    c.put32(toc - 0x7b7c, globals + 4);
    c.put32(toc - 0x7b78, globals + 8);
    c.put32(toc - 0x5b20, globals + 0x40);
    c.put32(toc - 0x4bb0, vector);
    c.put32(toc - 0x4bb4, vector);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x79dc, title);
    c.put32(toc - 0x79e0, body);
    c.put32(toc - 0x3c00, pool);
    c.put32(globals, clubs);
    c.put32(globals + 4, ledger);
    c.put32(globals + 8, 0xbad00000);
    c.put32(primary + 4, 1);
    c.put32(club, test.club_id);
    c.put32(vector, table->compare);
    c.put32(vector + 4, toc);
    if (test.second_contract_gone) c.put32(0, test.club_id);
    c.put32(staff, test.id);
    c.put8(flags + test.id * 10, 0x90 | (test.flagged ? table->flag : 0));
    c.put32(manager + 0x9a0, flags);
    c.put32(manager + table->offset, test.has_array ? rows : 0);
    c.put32(manager + table->offset + 4, test.capacity);
    c.put32(manager + table->offset + 8, test.count);
    for (unsigned i = 0; i <= test.count; ++i) {
        write_row(c, rows + i * 16, 1 + i * 2, 0xa0 + i);
        c.put32(expanded + i * 16 + table->byte, 0xfedcba98);
    }
    write_row(c, input, test.input_id, 0x1234);
    return s;
}

void original(State& s, std::uint32_t entry) {
    const bool previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    require(transfer_manager_reference(s), "Missing transfer fee reference");
    while (s.pc != stop) {
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    const auto manager = c.r(3), row = c.r(4);
    if (entry == table->append) append_transfer_fee(c, table->kind, manager, row);
    else if (entry == table->remove) remove_transfer_fee(c, table->kind, manager, row);
    else if (table->kind == FeeKind::sell_on) find_sell_on_fee(c, manager, row);
    else find_appearance_fee(c, table->kind, manager, row);
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
void compare(State& s, std::uint32_t entry, RoutineRunner first, RoutineRunner second) {
    const auto differences = compare_routine_call(s, entry, first, second,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Transfer fee case " + std::to_string(checked) + " at " + hex_address(entry));
    ++checked;
}

void check(std::uint32_t entry, const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    auto s = fixture_state(entry, test);
    auto control = clone(*s);
    original(*control, entry);
    Cpu c(*control);
    const auto registry = manager + table->offset, flag = flags + test.id * 10;
    const bool has_row = (test.id & 1) && test.id / 2 < test.count;
    if (entry == table->append) {
        const bool grows = test.count == test.capacity;
        const auto storage = grows ? (test.allocation_fails ? 0 : expanded) : rows;
        require(c.u32(registry) == storage && c.u32(registry + 4) == test.capacity + (grows ? 50 : 0),
                "Original transfer fee allocation result");
        if (storage) {
            require(c.u32(registry + 8) == test.count + 1 && calls(c, sort_rows) == 1, "Original transfer fee append");
            bool found = false;
            for (unsigned i = 0; i <= test.count; ++i) {
                const auto row = storage + i * 16;
                const auto id = c.u32(row + table->key);
                if (i) require(c.u32(row - 16 + table->key) <= id, "Original transfer fee sort order");
                if (id == test.input_id) {
                    const auto padding = test.has_array ? 0xa0 + test.count : 0xdcba98;
                    found |= (c.u32(row + table->byte) & 0xffffff) == padding;
                }
            }
            require(found, "Original transfer fee padding or inserted record");
        } else {
            require(c.u32(registry + 8) == test.count && calls(c, 0x10769b60) == 1,
                    "Original transfer fee allocation diagnostic");
        }
    } else if (entry == table->remove) {
        require(c.u8(flag) == 0x90 && c.u32(registry + 8) == test.count - has_row, "Original transfer fee removal");
        if (has_row && test.id / 2 + 1 < test.count) {
            const auto row = rows + test.id / 2 * 16;
            require(c.u32(row + table->key) == test.id + 2 &&
                    (c.u32(row + table->byte) & 0xffffff) == 0xa0 + test.id / 2,
                    "Original transfer fee compaction");
        }
    } else {
        const auto sell_on = table->kind == FeeKind::sell_on;
        const bool searches = test.flagged && (!sell_on || test.contract_found);
        const bool found = searches && has_row && (!sell_on || test.club_id == 7);
        const bool still_flagged = test.flagged && (!searches || found);
        require(c.r(3) == (found ? rows + test.id / 2 * 16 : 0) && calls(c, search_rows) == searches &&
                c.u8(flag) == (0x90 | (still_flagged ? table->flag : 0)), "Original transfer fee lookup result");
        if (found) require(calls(c, table->compare) > 0, "Original transfer fee lookup skipped its comparator");
    }
    compare(*s, entry, original, selected);
}

void sequence(State& s, RoutineRunner run) {
    Cpu c(s);
    run(s, table->append);
    c.set({{3, manager}, {4, staff}});
    run(s, table->find);
    require(c.r(3) == rows + 16, "Combined transfer fee search did not find the added row");
    c.set({{3, manager}, {4, staff}});
    run(s, table->remove);
    c.set({{3, manager}, {4, staff}});
    run(s, table->find);
    require(c.r(3) == 0 && c.u32(manager + table->offset + 8) == active.count, "Combined transfer fee removal state");
}
void original_sequence(State& s, std::uint32_t) { sequence(s, original); }
void production_sequence(State& s, std::uint32_t) { sequence(s, production); }

void retain_removed_flag(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    const auto address = flags + active.id * 10;
    c.put8(address, c.u8(address) | table->flag);
}

void check_negative_control() {
    table = &tables.front();
    active = {};
    auto s = fixture_state(table->remove, active);
    const auto differences = compare_routine_call(*s, table->remove, original, retain_removed_flag,
        RoutineChecker::result_r3, false, true);
    require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 3000111e"); }),
            "The transfer fee comparison missed an incorrectly retained flag");
}
}

int cm::aot::transfer_manager::checks::fees(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            for (const auto& current : tables) {
                table = &current;
                active = {.id = 2};
                auto s = fixture_state(table->append, active);
                compare(*s, table->append, original_sequence, production_sequence);
            }
            std::cout << checked << " transfer fee production sequences\n";
            return 0;
        }
        require(argc == 1, "Unknown transfer fee check mode");
        constexpr auto seed = 0x54464545u;
        std::mt19937 random(seed);
        for (const auto& current : tables) {
            table = &current;
            check(table->append, {});
            check(table->append, {.count = 3, .capacity = 3});
            check(table->append, {.count = 0, .capacity = 0, .has_array = false});
            check(table->append, {.count = 0, .capacity = 0, .has_array = false, .allocation_fails = true});
            check(table->append, {.count = 3, .capacity = 3, .allocation_fails = true});
            check(table->remove, {});
            check(table->remove, {.count = 0});
            check(table->remove, {.id = 5, .flagged = false});
            check(table->find, {});
            check(table->find, {.id = 2});
            check(table->find, {.flagged = false});
            if (table->kind == FeeKind::sell_on) {
                check(table->find, {.club_id = 8});
                check(table->find, {.contract_found = false});
                check(table->find, {.second_contract_gone = true});
            }
            for (unsigned i = 0; i < 15; ++i) {
                const auto count = unsigned(random() % 5), id = unsigned(random() % 10);
                const bool flagged = random() & 1;
                check(table->remove, {.count = count, .id = id, .flagged = flagged});
            }
        }
        check_negative_control();
        std::cout << checked << " transfer fee comparisons (seed " << seed << ") and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
