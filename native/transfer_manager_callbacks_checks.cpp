#include "transfer_manager_callbacks.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::semantic_io_checks;
using namespace cm::aot::guest;

constexpr std::uint32_t priority_index = 0x104119e0, index_entry = 0x10411a10,
    ranked_transfers = 0x10411a20, linked_dates = 0x10411a90, dated = 0x10411b70,
    linked_priority = 0x10411c40, optional_index = 0x10411c80, ranked_staff = 0x10411cd0,
    sentinel_record = 0x103f9780, clear_bytes = 0x10778300, compare_names = 0x100cd6b0;
constexpr std::uint32_t staff_table = fixture + 0x800, trace = fixture + 0x1000,
    left_slot = fixture + 0x2000, right_slot = fixture + 0x2004;
constexpr std::array accepting{0x103e76b0u, 0x103ee1a0u, 0x103f8810u, 0x103fa840u};
constexpr std::array empty{0x103eae90u, 0x103fac50u, 0x103fac60u};
constexpr std::uint32_t vector_bridge = 0x10781320, vector_address = fixture + 0x2040;
constexpr std::uint32_t sort_entry = 0x10768300, sort_array = fixture + 0x2080;
std::uint32_t vector_target = 0;
bool original_vector = false, sorting = false;

void original(State&, std::uint32_t);

bool dependency(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    if (vector_target && target == vector_target) {
        if (sorting) c.put32(trace, c.u32(trace) + 1);
        if (original_vector) {
            original(s, target);
            return true;
        }
    }
    if (target == compare_names) {
        require(c.lr() == 0x10411dfc && c.r(3) == c.sp() + 0x3c && c.r(4) == c.sp() + 0x38 &&
                c.u32(c.r(3)) == staff_table + 0x70 && c.u32(c.r(4)) == staff_table + 0xe0,
                "Ranked staff name comparison boundary");
    } else if (target == clear_bytes) {
        require(c.lr() == 0x103f979c && c.r(3) == destination && c.r(4) == 0xffffffff && c.r(5) == 0x2c,
                "Clear-offer callback boundary");
    } else {
        return false;
    }
    require(c.u32(trace) == 0, "Callback called a dependency twice");
    c.put32(trace, 1);
    c.put32(trace + 4, target);
    c.put32(trace + 8, c.lr());
    for (unsigned r = 3; r <= 10; ++r) c.put32(trace + 12 + (r - 3) * 4, c.r(r));
    if (target == clear_bytes) {
        if (!vector_target || original_vector) return false;
        // The fixture delegates the real fill and permits only that external
        // dependency to consume instructions beyond the descriptor bridge.
        require(s.executed == 5, "Sentinel callback ran generated instructions before the fill");
        s.budget = s.executed + 5000;
        const auto back = c.lr();
        require(dispatch_page(s), "Missing fill dependency");
        if (s.pc != back) run_until(s, back);
        s.budget = s.executed;
        return true;
    }
    c.set(3, std::uint32_t(c.i8(staff_table + 0x70) - c.i8(staff_table + 0xe0)));
    c.set_cr(0, 8);
    c.finish();
    return true;
}

void original(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    while (s.pc != stop) {
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
}

void modern(State& s, std::uint32_t entry) {
    Cpu c(s);
    const auto l = c.r(3), r = c.r(4);
    using namespace transfer_manager;
    switch (entry) {
    case priority_index: compare_priority_and_index(c, l, r); break;
    case index_entry: compare_index(c, l, r); break;
    case ranked_transfers: compare_ranked_transfers(c, l, r); break;
    case linked_dates: compare_linked_dates(c, l, r); break;
    case dated: compare_dated_entries(c, l, r); break;
    case linked_priority: compare_linked_priorities(c, l, r); break;
    case optional_index: compare_optional_index(c, l, r); break;
    case ranked_staff: compare_ranked_staff(c, l, r); break;
    case sentinel_record: initialize_record_sentinels(c, c.r(5)); break;
    default:
        if (std::ranges::find(accepting, entry) != accepting.end()) c.set(3, 1);
        else require(std::ranges::find(empty, entry) != empty.end(), "Unknown transfer callback");
    }
    c.finish();
}

std::unique_ptr<State> make_state(std::uint32_t entry) {
    auto s = std::make_unique<State>();
    s->memory.map(fixture, 0x3000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->memory.map(toc - 0x8000, 0x1000);
    s->registers.fill(0x65);
    s->budget = 10000;
    s->pc = entry;
    Cpu c(*s);
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, std::uint8_t(field));
    for (unsigned field = 1024; field < 1028; ++field) s->wr<1>(field, 1);
    for (unsigned field = 1280; field < 1312; ++field) s->wr<1>(field, 0);
    const bool indirect = entry == linked_dates || entry == linked_priority;
    c.set({{1, stack}, {2, toc}, {3, indirect ? left_slot : left},
           {4, indirect ? right_slot : right}, {5, destination}});
    c.set_lr(return_sentinel);
    c.put32(left_slot, left);
    c.put32(right_slot, right);
    c.put32(toc - 0x7be0, fixture + 0x800 - 4);
    c.put32(fixture + 0x800 - 4, staff_table);
    c.put8(staff_table + 0x70, 'A');
    c.put8(staff_table + 0xe0, 'C');
    return s;
}

unsigned comparisons = 0;
void check(State& s, std::uint32_t entry, std::uint32_t expected, unsigned calls = 0) {
    auto control = clone(s);
    original(*control, entry);
    Cpu c(*control);
    require(c.r(3) == expected && c.u32(trace) == calls, "Unexpected original callback result at " + hex_address(entry));
    if (entry == sentinel_record) {
        for (unsigned i = 0; i < 0x2c; ++i) require(c.u8(destination + i) == 255, "Clear-offer callback bytes");
        require(c.u8(destination + 0x2c) == 0, "Clear-offer callback overrun");
    }
    const auto differences = compare_routine_call(s, entry, original, modern,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Callback comparison " + std::to_string(comparisons) + " " + hex_address(entry));
    ++comparisons;
}

void check_dates(std::int16_t left_day, std::uint16_t left_year,
                 std::int16_t right_day, std::uint16_t right_year, int rank_order = 0) {
    const auto lhs = std::pair{left_year, left_day}, rhs = std::pair{right_year, right_day};
    const int date_order = (lhs > rhs) - (lhs < rhs);
    for (auto entry : {linked_dates, dated, ranked_staff}) {
        auto s = make_state(entry);
        Cpu c(*s);
        const auto offset = entry == linked_dates ? 0x20u : 0;
        c.put16(left + offset, std::uint32_t(left_day));
        c.put16(left + offset + 2, left_year);
        c.put16(right + offset, std::uint32_t(right_day));
        c.put16(right + offset + 2, right_year);
        c.put32(right + offset + 4, 0x12233445);
        c.put32(left + 8, 1);
        c.put32(right + 8, 2);
        c.put8(left + 0x19, std::uint32_t(rank_order));
        const bool name_tie = entry == ranked_staff && rank_order == 0 && date_order == 0;
        const auto expected = entry == ranked_staff && rank_order ? rank_order : name_tie ? -2 : date_order;
        check(*s, entry, std::uint32_t(expected), name_tie);
    }
}

void vector_original(State& s, std::uint32_t) {
    original_vector = true;
    s.pc = vector_bridge;
    run_until(s, return_sentinel);
}

void vector_production(State& s, std::uint32_t) {
    s.pc = vector_bridge;
    s.budget = 5;
    run_until(s, return_sentinel);
    require(s.executed == s.budget, "Callback exhausted more than its descriptor/dependency budget");
}

void sort_original(State& s, std::uint32_t) {
    original_vector = true;
    s.pc = sort_entry;
    run_until(s, return_sentinel);
}

void sort_production(State& s, std::uint32_t) {
    s.pc = sort_entry;
    run_until(s, return_sentinel);
}

void check_sort() {
    auto s = make_state(index_entry);
    s->memory.map(toc - 0x4000, 0x1000);
    Cpu c(*s);
    constexpr auto row_size = 16;
    c.set({{3, sort_array}, {4, 5}, {5, row_size}, {6, vector_address}});
    // The aligned sorter fetches the diagnostic string pointer even when
    // the row width is valid and no diagnostic reads the string.
    c.put32(toc - 0x3c00, fixture + 0x2800);
    c.put32(vector_address, index_entry);
    c.put32(vector_address + 4, toc);
    constexpr std::array keys{5u, 2u, 2u, 7u, 1u}, sorted{1u, 2u, 2u, 5u, 7u};
    for (unsigned i = 0; i < keys.size(); ++i) {
        c.put32(sort_array + i * row_size, keys[i]);
        c.put32(sort_array + i * row_size + 4, i + 0x500);
    }
    sorting = true;
    vector_target = index_entry;
    auto control = clone(*s);
    sort_original(*control, sort_entry);
    original_vector = false;
    Cpu expected(*control);
    require(expected.u32(trace) > 1, "Original sort did not call the comparator repeatedly");
    for (unsigned i = 0; i < sorted.size(); ++i)
        require(expected.u32(sort_array + i * row_size) == sorted[i], "Original sort did not produce the expected order");
    const auto differences = compare_routine_call(*s, sort_entry, sort_original, sort_production,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Controlled transfer sort differs");
}

void check_vectors() {
    constexpr std::array vector_entries{0x103e76b0u, 0x103eae90u, 0x103f8810u, sentinel_record,
        0x103fa840u, 0x103fac50u, 0x103fac60u, priority_index, index_entry, ranked_transfers,
        linked_dates, dated, linked_priority, optional_index, ranked_staff};
    for (auto entry : vector_entries) {
        auto s = make_state(entry);
        Cpu c(*s);
        c.set(2, toc + 0x100);
        c.set(12, vector_address);
        c.put32(vector_address, entry);
        c.put32(vector_address + 4, toc);
        c.put32(left + 8, 1);
        c.put32(right + 8, 2);
        vector_target = entry;
        const auto differences = compare_routine_call(*s, vector_bridge, vector_original, vector_production,
            RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
        for (const auto& difference : differences) std::cerr << difference << '\n';
        require(differences.empty(), "Transfer descriptor route differs at " + hex_address(entry));
    }
    check_sort();
    std::cout << vector_entries.size() << " transfer descriptors and the controlled sort reached production callbacks\n";
}
}

int main(int argc, char** argv) {
    try {
        transfer_manager::install_check_coverage();
        dispatch_probe = dependency;
        if (argc == 2 && std::string_view(argv[1]) == "--vectors") {
            check_vectors();
            return 0;
        }
        require(argc == 1, "Unknown transfer callback check mode");
        for (auto entry : accepting) check(*make_state(entry), entry, 1);
        for (auto entry : empty) check(*make_state(entry), entry, left);
        check(*make_state(sentinel_record), sentinel_record, destination, 1);
        for (std::uint32_t a : {0u, 1u, 0xffffffffu, 0x80000000u})
            for (std::uint32_t b : {0u, 1u, 0xffffffffu, 0x80000000u}) {
                for (auto entry : {priority_index, index_entry, optional_index}) {
                    auto s = make_state(entry);
                    Cpu c(*s);
                    c.put32(left, a);
                    c.put32(right, b);
                    c.put32(left + 4, a + 2);
                    c.put32(right + 4, b + 4);
                    c.put32(left + 0xc, a);
                    c.put32(right + 0xc, b);
                    auto expected = a - b;
                    if (entry == optional_index && (a == 0xffffffff || b == 0xffffffff))
                        expected = std::uint32_t(int(a == 0xffffffff) - int(b == 0xffffffff));
                    if (entry == priority_index && a == b) expected = 0xfffffffe;
                    check(*s, entry, expected);
                }
            }
        for (int a : {-128, 0, 2, 127})
            for (int b : {-128, 0, 2, 127}) {
                auto s = make_state(ranked_transfers);
                Cpu c(*s);
                c.put8(left + 0x1c, std::uint32_t(a));
                c.put8(right + 0x1c, std::uint32_t(b));
                check(*s, ranked_transfers, std::uint32_t(int(a == 2) - int(b == 2)));
                c.put32(left + 0x18, std::uint32_t(a));
                c.put32(right + 0x18, std::uint32_t(b));
                check(*s, ranked_transfers, std::uint32_t((a < b) - (a > b)));

                s = make_state(linked_priority);
                Cpu link(*s);
                link.put8(left + 3, std::uint32_t(a));
                link.put8(right + 3, std::uint32_t(b));
                check(*s, linked_priority, std::uint32_t(a - b));
                link.put8(left + 4, std::uint32_t(b));
                link.put8(right + 4, std::uint32_t(a));
                check(*s, linked_priority, std::uint32_t(b - a));
            }
        check_dates(1, 2000, 1, 2000);
        check_dates(1, 2000, 2, 2000);
        check_dates(2, 2000, 1, 2000);
        check_dates(100, 2001, 200, 2000);
        check_dates(100, 2000, 200, 2001);
        check_dates(1, 2000, 1, 2000, -1);
        check_dates(1, 2000, 1, 2000, 1);
        check_dates(-32768, 2000, 32767, 2000);
        check_dates(32767, 2000, -32768, 2000);
        check_dates(1, 0, 1, 65535);
        check_dates(1, 65535, 1, 0);
        constexpr std::uint32_t seed = 0x5443424b;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 24; ++i) {
            const auto year = std::uint16_t(random());
            const auto a = std::int16_t(random()), b = std::int16_t(random());
            const auto other_year = i % 2 ? year : std::uint16_t(random());
            check_dates(a, year, b, other_year);
        }
        std::cout << comparisons << " transfer callback comparisons (seed " << seed << ")\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
