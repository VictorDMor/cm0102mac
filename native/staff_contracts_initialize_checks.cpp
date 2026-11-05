#include "staff_contracts.hpp"
#include "staff_contracts_adapter.hpp"
#include "contract_storage_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

namespace cm::staff_contracts_checks {
namespace {
using aot::guest::Cpu;
using aot::staff_contracts::initialize_entry;
using aot::staff_contracts::reference_quote_entry;

using namespace cm::semantic_io_checks;
constexpr auto contract = fixture + 0x100, person = fixture + 0x300;
constexpr auto club = fixture + 0x400, resolved = fixture + 0x700, profile = fixture + 0xa00;
constexpr auto league = fixture + 0xb00, nation = fixture + 0xe00;
constexpr auto doubles = fixture + 0x1000, singles = fixture + 0x1400;
constexpr auto current = fixture + 0x1600, reference = fixture + 0x1620, manager = fixture + 0x1640;
constexpr auto draft_delta = fixture + 0x1650;
constexpr auto release_competition = fixture + 0x1800, release_methods = fixture + 0x1860;
constexpr auto competition_slot = fixture + 0x1950, competition_table = fixture + 0x1960;
constexpr auto primary_descriptor = fixture + 0x1980, alternate_descriptor = fixture + 0x1988;
constexpr auto cursor = fixture + 0x1760, salt_slot = fixture + 0x1764, pool = fixture + 0x1770, end = fixture + 0x17c0;

bool dependency(State& s) {
    Cpu c(s);
    switch (s.pc) {
        case 0x10715880: case 0x107156f0:
            c.put32(c.r(3), 0x00a407d0);
            c.put8(c.r(3) + 4, 0x80);
            break;
        case 0x100ddf30:
            require(c.r(3) == person, "Initializer review reset staff");
            c.put8(person + 0x6a, 0x42);
            break;
        case 0x103092f0:
            require(c.r(3) == manager && c.r(4) == person && c.r(5) == club, "Initializer review rating arguments");
            c.set(3, 5);
            break;
        case 0x100ce320:
            require(c.r(3) == club && c.r(4) == c.sp() + 0x38 && c.r(5) == 1, "Initializer club lookup arguments");
            c.put8(c.r(4), c.u8(person + 0x69));
            c.set(3, c.u32(person + 0x6c));
            break;
        case 0x10084be0:
            require(c.r(3) == manager && c.r(4) == person && c.r(6) == 0 && c.r(7) == UINT32_MAX,
                    "Initializer valuation arguments");
            c.set(3, c.u32(person + 0x70));
            break;
        case reference_quote_entry:
            require(c.r(3) == contract && c.r(4) == person && c.r(6) == 0 && c.r(7) == UINT32_MAX && c.r(8) == 11
                && c.r(9) == 0, "Initializer reference quote arguments");
            c.set_f(1, 17500.25);
            break;
        case 0x10781320:
            require(c.r(3) == release_competition && (c.r(12) == primary_descriptor || c.r(12) == alternate_descriptor),
                    "Initializer competition callback arguments");
            c.put32(c.sp() + 0x14, c.r(2));
            c.set(3, c.u8(release_competition + 0x45));
            break;
        case 0x10718130:
            require(c.r(3) == current, "Initializer difference arguments");
            c.set(3, 128);
            break;
        case 0x10717dd0:
            require(c.r(4) == current, "Initializer delta arguments");
            c.put32(c.r(3), c.u32(draft_delta));
            c.put8(c.r(3) + 4, 0xa5);
            break;
        default:
            return false;
    }
    c.finish();
    return true;
}

std::unique_ptr<State> initializer_fixture() {
    auto result = std::make_unique<State>();
    State& seed = *result;
    seed.registers.fill(0xa5);
    seed.unique.fill(0xb7);
    for (unsigned bit = 1280; bit < 1312; ++bit) seed.wr<1>(bit, 0);
    seed.memory.map(fixture, 0x1a00);
    seed.memory.map(stack - 0x300, 0x400);
    seed.memory.map(toc - 0x8000, 0x1000);
    seed.memory.map(toc - 0x5000, 0x1000);
    Cpu c(seed);
    c.set({{1, stack}, {2, toc}, {3, contract}, {4, person}, {5, 12000}});
    c.set_lr(return_sentinel);
    seed.pc = initialize_entry;
    seed.budget = 10000;
    c.put32(toc - 0x4dac, doubles);
    c.put32(toc - 0x4da8, singles);
    c.put32(toc - 0x7b7c, manager + 4);
    c.put32(manager + 4, manager);
    c.put32(toc - 0x7b80, current);
    c.put32(toc - 0x7a9c, reference);
    c.put_f64(doubles + 0xc0, 4503601774854144.0);
    c.put_f64(doubles + 0xc8, 4503599627370496.0);
    for (const auto [offset, value] : std::array{std::pair{0u, 0.25f}, std::pair{4u, 0.5f},
             std::pair{8u, 20000.f}, std::pair{12u, 5000.f}, std::pair{16u, 1250.f}})
        c.put_f32(singles + offset, value);
    c.put32(current, 0x00a407d1);
    c.put8(current + 4, 0xc0);
    c.put8(contract + 0x38, 0xeb);
    c.put32(club, 15);
    c.put32(resolved, 9);
    c.put32(person + 0x70, 7500);
    c.put16(profile + 10, 9000);
    c.put16(nation + 0x2a, 7000);
    c.put8(profile + 0x11, 20);
    c.put8(person + 0x5e, 12);
    c.put8(league + 0x11f, 5);
    c.put32(person + 0x54, 0x00b007d3);
    c.put8(person + 0x58, 0x91);
    constexpr std::array slots{-0x7a1c, -0x7590, -0x7af4, -0x7a00, -0x7a28, -0x7514, -0x7650,
        -0x757c, -0x7594, -0x7574, -0x7578, -0x750c, -0x75a8, -0x7504, -0x7510, -0x79fc,
        -0x7a24, -0x7a10, -0x7a14, -0x7a18, -0x7a20};
    for (unsigned i = 0; i < slots.size(); ++i) {
        c.put32(toc + slots[i], fixture + 0x1700 + i * 4);
        c.put32(fixture + 0x1700 + i * 4, i + 1);
    }
    for (auto offset : {0x40u, 0x48u, 0x50u, 0x58u, 0x60u, 0x68u, 0x70u, 0x78u, 0x80u, 0x88u, 0x90u, 0xb8u})
        c.put_f64(doubles + offset, offset * 10);
    seed.pc = initialize_entry;
    c.put32(toc - 0x7b90, salt_slot);
    c.put32(toc - 0x7b8c, pool);
    c.put32(toc - 0x7b88, end);
    c.put32(toc - 0x7b84, cursor);
    c.put32(toc - 0x7b5c, competition_slot);
    seed.memory.map(toc - 0x34c4, 4);
    c.put32(competition_slot, competition_table);
    c.put32(competition_table + 4, release_competition);
    c.put32(release_competition, release_methods);
    c.put32(release_methods + 0xa0, primary_descriptor);
    c.put32(release_methods + 0xa4, alternate_descriptor);
    c.put32(club + 0x30, fixture + 0x1930);
    c.put32(fixture + 0x1930, 1);
    c.put32(end, pool + 4);
    c.put32(person + 0x24, club);
    c.put32(person + 0x20, resolved);
    c.put32(person + 0x6c, 0);
    c.put32(person + 0xc, 0);
    c.put32(person + 0x1c, nation);
    c.put16(person + 0x5c, 34);
    c.put8(person + 0x64, 120);
    c.put8(person + 0x67, 40);
    c.put32(person, 999);
    c.put32(reference, 0x00a307d2);
    c.put32(person + 0x4c, 0x00a307d1);
    c.put16(club + 0x1ec, 500);
    c.put32(club + 0x2c, league);
    c.put32(league + 0x68, 5);
    c.put_f64(doubles + 0x8, 10000.0);
    c.put_f64(doubles + 0x10, 1.75);
    c.put_f64(doubles + 0x18, 900000.0);
    c.put_f64(doubles + 0x20, 0.8);
    c.put_f64(doubles + 0x28, 0.5);
    c.put_f64(doubles + 0x30, 0.75);
    c.put_f64(doubles + 0x38, 0.9);
    c.put32(toc - 0x7be0, manager + 8);
    c.put32(manager + 8, person);
    c.put32(toc - 0x7b3c, manager + 4);
    c.put32(contract, 0);
    c.put16(profile + 10, 8500);
    c.put32(person + 0x14, profile);
    c.put32(draft_delta, 0x012d0001);
    c.put8(release_competition + 0x43, 2);
    c.put8(release_competition + 0x45, 1);
    c.put8(person + 0x62, 11);
    c.put16(pool, 42);
    c.put16(pool + 2, 1000);
    c.put16(pool + 4, 2000);
    c.put32(cursor, pool);
    c.put32(salt_slot, 7);
    c.put32(toc - 0x34c4, 0x12345);
    c.put_f64(doubles, 65535.0);
    return result;
}

struct ScopedServices {
    decltype(dispatch_probe) previous_probe = dispatch_probe;
    RoutineChecker* previous_checker = routine_checker;
    explicit ScopedServices(RoutineChecker* checker = nullptr) {
        dispatch_probe = dependency;
        routine_checker = checker;
    }
    ~ScopedServices() {
        dispatch_probe = previous_probe;
        routine_checker = previous_checker;
    }
};

unsigned compare_initializer(const State& seed, bool negative = false) {
    auto original = clone(seed), modern = clone(seed);
    Effects expected, actual;
    expected.state = original.get();
    actual.state = modern.get();
    original->memory.date_observer = &expected;
    modern->memory.date_observer = &actual;
    const auto execute = [](State& s, bool old) {
        RoutineChecker no_service_hooks([](std::uint32_t) { return false; }, 0, 0);
        ScopedServices services(old ? &no_service_hooks : nullptr);
        try {
            if (old) staff_contracts_run_original(s, initialize_entry);
            else staff_contracts_run_rewrite(s, initialize_entry);
            return std::string{};
        } catch (const std::runtime_error& error) {
            return std::string(error.what());
        }
    };
    const auto old_stop = execute(*original, true), new_stop = execute(*modern, false);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto label = "Initializer budget=" + std::to_string(seed.budget) + " valuation="
        + std::to_string(std::int32_t(seed.r<4>(20))) + " original PC=" + hex_address(original->pc)
        + " C++ PC=" + hex_address(modern->pc);
    require(old_stop == new_stop, label + " original=" + old_stop + " C++=" + new_stop);
    require(old_stop.empty() || old_stop == "Native instruction budget exhausted"
                || old_stop.starts_with("Unmapped data access at "), label + " unexpected stop " + old_stop);
    require(expected.bytes == actual.bytes, label + " changed ordered memory accesses");
    require(original->executed == modern->executed, label + " changed instruction accounting");
    const Image reference_image(*original);
    reference_image.compare(*modern, label);
    if (negative) {
        Cpu c(*modern);
        c.put32(contract + 12, c.u32(contract + 12) ^ 1);
        bool caught = false;
        try {
            reference_image.compare(*modern, label);
        } catch (const std::runtime_error& error) {
            caught = std::string_view(error.what()).starts_with(label + " memory mismatch");
        }
        require(caught, label + " missed a changed contract wage");
    }
    return unsigned(modern->executed);
}
}

bool initializer_hook(std::uint32_t entry) {
    if (entry != initialize_entry) return false;
    auto seed = initializer_fixture();
    RoutineChecker checker([](std::uint32_t target) { return target == initialize_entry; }, 1, 1);
    checker.strict_volatile = true;
    ScopedServices services(&checker);
    run_until(*seed, return_sentinel);
    const auto total = checker.totals().find(entry);
    require(total != checker.totals().end() && total->second.checked == 1 && !total->second.differences,
            "Default initializer page did not select its C++ owner");
    const auto& diagnostics = contract_storage_diagnostics(*seed);
    require(diagnostics.calls[2] == 1 && diagnostics.returns[2] == 1,
            "Typed initializer retains the contract storage boundary");
    auto original = initializer_fixture();
    auto& original_diagnostics = contract_storage_diagnostics(*original);
    original_diagnostics.entry_original[2] = true;
    run_until(*original, return_sentinel);
    require(original_diagnostics.original_calls[2] == 1 && original_diagnostics.calls[2] == 0
                && original_diagnostics.original_body == 0,
            "Initializer retains the contract storage original selector");
    return true;
}

void initializer_cases() {
    auto seed = initializer_fixture();
    Cpu c(*seed);
    unsigned complete = 0, budgets = 0, faults = 0;
    for (const auto registration : {0u, club}) {
        c.put32(person + 0x24, registration);
        for (const auto profile_address : {0u, profile}) {
            c.put32(person + 0x14, profile_address);
            for (const auto kind : {0, 2, 3}) {
                c.put8(club + 0x23d, kind);
                for (const auto role : {5, 11}) {
                    c.put8(person + 0x62, role);
                    for (const auto score : {1750, 2751, 6501, 8500}) {
                        c.put16(profile + 10, score);
                        for (const auto chance : {0, 1, 2, 3}) {
                            c.put8(release_competition + 0x43, chance & 1 ? 2 : 1);
                            c.put8(release_competition + 0x45, chance & 2 ? 1 : 0);
                            c.put8(person + 0x63, chance & 2 ? 20 : 8);
                            c.put8(person + 0x65, chance & 1 ? 16 : 2);
                            c.put8(profile + 0xf, chance & 1 ? 20 : 0);
                            c.put8(profile + 0x13, chance & 1 ? 18 : 0);
                            c.put8(profile + 0x14, chance & 2 ? 20 : 0);
                            c.put8(profile + 0x15, chance & 2 ? 20 : 0);
                            for (const auto valuation : {-1, 0, 12000, 560000, 25000000, 2147483647}) {
                                c.set(5, std::uint32_t(valuation));
                                for (const auto base : {0, 3800, 90000}) {
                                    c.put32(person + 4, base);
                                    c.put32(toc - 0x34c4, 0x12345 + base);
                                    c.put32(salt_slot, base & 0xffff);
                                    c.put32(cursor, chance & 1 ? pool + 4 : pool);
                                    compare_initializer(*seed);
                                    ++complete;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    seed = initializer_fixture();
    Cpu control(*seed);
    control.set(5, 560000);
    control.put8(profile + 0xf, 20);
    control.put8(profile + 0x15, 20);
    control.put8(person + 0x63, 20);
    control.put8(person + 0x65, 2);
    const auto steps = compare_initializer(*seed, true);
    ++complete;
    for (unsigned limit = 1; limit < steps; ++limit) {
        seed->budget = limit;
        compare_initializer(*seed);
        ++budgets;
    }
    seed->budget = 10000;
    control.put32(person + 0x24, 0);
    control.put32(person + 0x20, 0);
    const auto early_steps = compare_initializer(*seed);
    ++complete;
    for (unsigned limit = 1; limit < early_steps; ++limit) {
        seed->budget = limit;
        compare_initializer(*seed);
        ++budgets;
    }
    seed->budget = 10000;
    for (const auto missing : {fixture + 0x2000, fixture + 0x2100}) {
        control.set(4, missing);
        compare_initializer(*seed);
        ++faults;
    }
    std::cout << "Initializer " << complete << " returns, " << budgets << " budget limits, "
              << faults << " partial faults, and one detected record mutation\n";
}
}
