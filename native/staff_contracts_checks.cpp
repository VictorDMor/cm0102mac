#include "staff_contracts.hpp"
#include "staff_contracts_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <random>
#include <string_view>

namespace {
using namespace cm::aot;
using cm::semantic_io_checks::Effects;
using cm::semantic_io_checks::Image;
using cm::semantic_io_checks::clone;
using cm::semantic_io_checks::fixture;
using cm::semantic_io_checks::require;
using cm::semantic_io_checks::stack;
using cm::semantic_io_checks::toc;
using staff_contracts::expiry_entry;

constexpr std::array date_slots{-0x7b80, -0x79b4, -0x79a8, -0x79ac, -0x79b0, -0x79a0, -0x79a4, -0x799c};
constexpr std::array date_days{1, 180, 120, 140, 160, 80, 100, 60};
constexpr auto record = fixture + 0x400;
constexpr auto dates = fixture + 0x800;
constexpr auto dates_year = 2001;
constexpr auto normal_budget = 10000;
constexpr auto no_hole = UINT32_MAX;

std::unique_ptr<State> expiry_fixture() {
    auto s = std::make_unique<State>();
    s->registers.fill(0xa5);
    s->unique.fill(0xb7);
    s->memory.map(fixture, 0x1000);
    s->memory.map(stack - 0x100, 0x200);
    s->memory.map(toc - 0x8000, 0x1000);
    guest::Cpu c(*s);
    c.set({{1, stack}, {2, toc}, {3, record}});
    c.set_lr(return_sentinel);
    s->pc = expiry_entry;
    s->budget = normal_budget;
    c.put8(record + 0x38, 8);
    c.put16(record + 0x30, 150);
    c.put16(record + 0x32, dates_year);
    for (unsigned n = 0; n < date_slots.size(); ++n) {
        const auto address = dates + n * 8;
        c.put32(toc + std::uint32_t(date_slots[n]), address);
        c.put16(address, date_days[n]);
        c.put16(address + 2, dates_year);
        c.put32(address + 4, 0xdead0000 + n);
    }
    return s;
}

std::unique_ptr<State> state_with_hole(const State& input, std::uint32_t hole) {
    if (hole == no_hole) return clone(input);
    auto s = std::make_unique<State>();
    s->registers = input.registers;
    s->unique = input.unique;
    s->pc = input.pc;
    s->budget = input.budget;
    for (const auto& [address, length] : input.memory.mapped_ranges()) {
        const auto copy = [&](std::uint32_t begin, std::uint64_t size) {
            if (!size) return;
            s->memory.map(begin, size);
            s->memory.copy_in(begin, input.memory.copy_out(begin, size));
        };
        const auto end = address + length;
        if (hole >= address && hole < end) {
            copy(address, hole - address);
            copy(hole + 1, end - (hole + 1));
        } else {
            copy(address, length);
        }
    }
    return s;
}

struct Result {
    unsigned value, steps;
    std::vector<Effects::Access> accesses;
};
struct Counts {
    unsigned returned = 0, budgets = 0, faults = 0, negatives = 0;
};

Result compare_entry(const State& input, Counts& counts, std::uint32_t hole = no_hole,
                      bool negative = false) {
    auto original = state_with_hole(input, hole);
    auto modern = state_with_hole(input, hole);
    Effects expected, actual;
    expected.state = original.get();
    actual.state = modern.get();
    original->memory.date_observer = &expected;
    modern->memory.date_observer = &actual;
    const auto call = [](State& s, bool reference) {
        try {
            if (reference) staff_contracts_run_original(s, s.pc);
            else run_until(s, return_sentinel);
            return std::string{};
        } catch (const std::runtime_error& error) {
            return std::string(error.what());
        }
    };
    const auto old_stop = call(*original, true);
    const auto new_stop = call(*modern, false);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto test_case = counts.returned + counts.budgets + counts.faults + counts.negatives;
    const auto label = "Staff contract " + hex_address(input.pc) + " comparison " + std::to_string(test_case);
    require(old_stop == new_stop, label + " original=" + old_stop + " C++=" + new_stop);
    if (hole != no_hole) {
        require(old_stop.starts_with("Unmapped data access at "), label + " did not fault at the removed byte");
        ++counts.faults;
    } else if (old_stop == "Native instruction budget exhausted") {
        require(input.budget < normal_budget, label + " exhausted the normal budget");
        ++counts.budgets;
    } else {
        require(old_stop.empty() && modern->pc == return_sentinel, label + " did not return");
        if (!negative) ++counts.returned;
    }
    require(expected.bytes == actual.bytes, label + " changed ordered memory accesses");
    require(original->executed == modern->executed, label + " changed budget accounting");
    const auto band = unsigned(modern->r<4>(12));
    const Image image(*original);
    image.compare(*modern, label);
    if (negative) {
        modern->wr<4>(12, band ^ 1);
        bool rejected = false;
        try {
            image.compare(*modern, label);
        } catch (const std::runtime_error& error) {
            rejected = std::string_view(error.what()).starts_with(label + " register byte=");
        }
        require(rejected, label + " accepted a changed result");
        ++counts.negatives;
    }
    return {band, unsigned(modern->executed), std::move(actual.log)};
}

void date_boundaries(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    constexpr std::array<std::pair<int, int>, 24> bands{{
        {0, 14}, {1, 14}, {2, 12}, {59, 12}, {60, 13},
        {61, 12}, {79, 12}, {80, 11}, {81, 10}, {99, 10},
        {100, 9}, {101, 8}, {119, 8}, {120, 7}, {121, 6},
        {139, 6}, {140, 5}, {141, 4}, {159, 4}, {160, 3},
        {161, 2}, {179, 2}, {180, 1}, {181, 0},
    }};
    for (const auto& [day, band] : bands) {
        c.put16(record + 0x30, day);
        const auto full = compare_entry(*input, counts);
        require(full.value == unsigned(band), "Unexpected original date boundary " + std::to_string(day));
        for (unsigned cut = 0; cut < full.steps; ++cut) {
            input->budget = cut;
            compare_entry(*input, counts);
        }
        input->budget = normal_budget;
    }
    for (unsigned kind = 0; kind < 256; ++kind) {
        c.put8(record + 0x38, kind);
        compare_entry(*input, counts);
    }
}

void faults_and_mutations(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    for (const auto day : {0, 25, 60, 80, 100, 120, 140, 160, 180, 200}) {
        c.put16(record + 0x30, day);
        const auto full = compare_entry(*input, counts);
        for (const auto& access : full.accesses) {
            compare_entry(*input, counts, access.address);
        }
    }

    // Vary both year sides of each comparison. Keep the live date objects
    // mapped and aligned, and vary the second word to check exact spills.
    std::mt19937 random(0x80710);
    for (unsigned n = 0; n < 512; ++n) {
        c.put16(record + 0x30, 1 + random() % 366);
        c.put16(record + 0x32, dates_year - 1 + random() % 3);
        for (unsigned k = 0; k < date_slots.size(); ++k) {
            const auto address = dates + k * 8;
            c.put16(address, 1 + random() % 366);
            c.put16(address + 2, dates_year - 1 + random() % 3);
            c.put32(address + 4, random());
        }
        compare_entry(*input, counts);
    }
    // The contract may share memory with a date. The same load must still
    // observe a spill through a date pointer overlapping the red zone.
    c.put32(toc + std::uint32_t(date_slots[0]), record + 0x30);
    compare_entry(*input, counts);
    c.put32(toc + std::uint32_t(date_slots[0]), stack - 8);
    c.put16(stack - 8, 1);
    c.put16(stack - 6, dates_year);
    compare_entry(*input, counts);
    compare_entry(*input, counts, no_hole, true);
}

void terms_leaves(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    constexpr std::array indexed_writes{0x10374af0u, 0x10374b40u, 0x10374b60u, 0x10374b80u};
    for (const auto entry : indexed_writes) {
        input->pc = entry;
        for (unsigned selector = 0; selector < 256; ++selector) {
            // Leave high bits set to check the signed low-byte convention.
            c.set(4, 0x10203000 | selector);
            c.set(5, 0xf1e2d3c4 ^ selector);
            compare_entry(*input, counts);
        }
    }
    for (const auto entry : {0x10374ce0u, 0x10374d50u}) {
        input->pc = entry;
        for (unsigned kind = 0; kind < 256; ++kind) {
            c.put8(record + 0x38, kind);
            const auto result = compare_entry(*input, counts);
            require(result.value == unsigned((kind >> 2) == (entry == 0x10374ce0u ? 3u : 1u)),
                    "Packed contract kind");
        }
    }
    input->pc = 0x10380520;
    for (unsigned first = 0; first < 4; ++first) {
        for (unsigned second = 0; second < 4; ++second) {
            for (unsigned fourth = 0; fourth < 4; ++fourth) {
                c.put8(record + 0x1c, first);
                c.put8(record + 0x1d, second);
                c.put8(record + 0x1f, fourth);
                const auto result = compare_entry(*input, counts);
                require(result.value == unsigned(first == 2 || second == 1 || fourth == 2),
                        "Exact release-clause flag");
            }
        }
    }
    for (const auto entry : {0x1037fa50u, 0x1037fa60u, 0x1037fa70u, 0x10380700u}) {
        input->pc = entry;
        for (auto mask : {0u, 0x00000001u, 0x55555555u, 0x80000000u, UINT32_MAX}) {
            for (auto current : {0u, 0x80u, 0x55555555u, 0x80000000u, UINT32_MAX}) {
                c.set(4, mask);
                c.put32(record + 0x48, current);
                compare_entry(*input, counts);
            }
        }
    }

    const auto check_boundaries = [&](std::uint32_t entry, unsigned argument) {
        input->pc = entry;
        c.set(4, argument);
        const auto full = compare_entry(*input, counts);
        for (const auto& access : full.accesses)
            compare_entry(*input, counts, access.address);
        for (unsigned cut = 0; cut < full.steps; ++cut) {
            input->budget = cut;
            compare_entry(*input, counts);
        }
        input->budget = normal_budget;
    };
    for (const auto entry : indexed_writes)
        check_boundaries(entry, 1);
    check_boundaries(0x10374af0, 2);
    for (const auto entry : {0x10374ba0u, 0x10374ce0u, 0x10374d50u, 0x1037fa50u, 0x1037fa60u,
                             0x1037fa70u, 0x10380520u, 0x10380700u})
        check_boundaries(entry, 0);

    input->pc = 0x10374af0;
    compare_entry(*input, counts, no_hole, true);
}

unsigned generated_hooks() {
    constexpr auto entries = std::to_array<std::uint32_t>({
#define STAFF_CONTRACTS_ROUTINE(entry, ...) entry,
#include "staff_contracts_routines.inc"
#undef STAFF_CONTRACTS_ROUTINE
    });
    RoutineChecker checker([](std::uint32_t) { return true; }, 1, 1);
    checker.strict_volatile = true;
    routine_checker = &checker;
    try {
        for (const auto entry : entries) {
            auto s = expiry_fixture();
            s->pc = entry;
            s->wr<4>(16, 1);
            run_until(*s, return_sentinel);
            const auto result = checker.totals().find(entry);
            require(result != checker.totals().end() && result->second.checked == 1
                        && result->second.differences == 0 && s->pc == return_sentinel,
                    "Default generated page did not enter the staff-contract owner at " + hex_address(entry));
        }
    } catch (...) {
        routine_checker = nullptr;
        throw;
    }
    routine_checker = nullptr;
    return entries.size();
}
}

int main() {
    try {
        Counts counts;
        date_boundaries(counts);
        faults_and_mutations(counts);
        terms_leaves(counts);
        const auto hooks = generated_hooks();
        std::cout << "Staff contracts " << counts.returned << " returns, " << counts.budgets
                  << " budget stops, " << counts.faults << " partial faults, "
                  << counts.negatives << " rejected result mutations, " << hooks << " production hook comparisons\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
