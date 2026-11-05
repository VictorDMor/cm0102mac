#include "staff_contracts.hpp"
#include "staff_contracts_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

namespace cm::staff_contracts_checks {
namespace {
using namespace cm::semantic_io_checks;
using aot::guest::Cpu;
using aot::staff_contracts::proposal_entry;
using aot::staff_contracts::reference_quote_entry;
constexpr auto record = fixture + 0x100, person = fixture + 0x200;
constexpr auto club = fixture + 0x300, other_club = fixture + 0x600, profile = fixture + 0x900;
constexpr auto nation = fixture + 0xa00, league = fixture + 0xb00;
constexpr auto primary = fixture + 0xd00, existing = fixture + 0xe00;
constexpr auto manager = fixture + 0xf00, cache = fixture + 0xf04;
constexpr auto club_slot = fixture + 0xf08, population = fixture + 0xf0c, today = fixture + 0xf10;
constexpr auto primary_result = fixture + 0xf20, other_result = fixture + 0xf24, club_result = fixture + 0xf28;
constexpr auto selector_result = fixture + 0xfa0, valuation_input = fixture + 0xf30, staff_service = fixture + 0x1000;
constexpr auto doubles = fixture + 0x2000, singles = fixture + 0x2400;
constexpr auto metadata_slot = fixture + 0xf34, metadata = fixture + 0x3000;
constexpr auto region_slot = fixture + 0xfb0, competition = fixture + 0xfc0;
constexpr auto wage_service = fixture + 0x1c00;
constexpr auto competition_slot = fixture + 0x1e00, competition_table = fixture + 0x1e10;
constexpr auto competition_entry = fixture + 0x1e40, competition_vtable = fixture + 0x1ec0;
struct Call {
    std::uint32_t target, link;
    std::array<std::uint32_t, 8> arguments;
    bool operator==(const Call&) const = default;
};
std::vector<Call>* calls = nullptr;

constexpr auto random_cursor = fixture + 0x1d00, random_last = random_cursor + 4;
constexpr auto random_salt = random_cursor + 8, random_draws = random_cursor + 0x10;
constexpr auto duration_table = fixture + 0x2200, league_configuration = duration_table + 0x40;
constexpr auto transfer_entry = fixture + 0x1d40;

bool dependency(State& s) {
    Cpu c(s);
    const auto target = s.pc, link = c.lr();
    const std::array arguments{c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(12)};
    switch (target) {
        case 0x103eb440:
            require(c.r(3) == staff_service && c.r(4) == 2, "Proposal transfer index arguments");
            c.set(3, c.u32(transfer_entry + 4) ? 0 : transfer_entry);
            break;
        case 0x103f8cc0:
        case 0x103f9200:
            require(c.r(3) == staff_service && (c.r(4) == club || c.r(4) == other_club)
                && c.r(5) == record + 0x28, "Proposal transfer date arguments");
            c.put32(c.r(5), 0x005500b4);
            break;
        case 0x1077b770:
            c.set(3, 0x9a3);
            break;
        case 0x10089e40:
            require(c.r(3) == fixture && (c.r(4) == club || c.r(4) == other_club), "Proposal month arguments");
            c.set(3, 6);
            break;
        case 0x10191fd0:
            require(c.r(3) == club || c.r(3) == other_club, "Proposal league configuration argument");
            c.set(3, c.u8(c.r(3) + 0x240));
            break;
        case 0x107156f0:
            require(c.r(3) == c.sp() + 0x44 && c.r(4) > 0 && c.r(4) <= 28
                && c.r(5) == 6 && c.r(6) == 1998 && c.r(7) == UINT32_MAX, "Proposal provisional date arguments");
            c.put16(c.r(3), 120 + c.r(4));
            c.put16(c.r(3) + 2, c.r(6));
            break;
        case 0x107156b0:
            require(c.r(3) == c.sp() + 0x4c && c.r(4) >= 121 && c.r(4) <= 148
                && c.r(5) > 1990 && c.r(5) < 2040, "Proposal expiry date arguments");
            c.put16(c.r(3), c.r(4));
            c.put16(c.r(3) + 2, c.r(5));
            c.put8(c.r(3) + 4, 0x80);
            break;
        case 0x10781320:
            require(c.r(3) == competition_entry && (c.r(12) == 0xa0 || c.r(12) == 0xa4),
                    "Proposal competition callback arguments");
            c.put32(c.sp() + 0x14, c.r(2));
            c.set(3, c.u8(competition_entry + 0x44 + (c.r(12) == 0xa4)));
            break;
        case 0x10739890:
            require(c.r(3) == singles && c.r(6) == c.sp() + 0x64 && c.r(7) == c.sp() + 0x54,
                    "Proposal transfer diagnostic parameters");
            c.put32(c.r(6), 0);
            c.put32(c.r(7), 0);
            break;
        case 0x10769b60:
            require(c.r(3) == singles + 0x21 && c.r(4) == fixture + 0xf50 && c.r(5) == 0,
                    "Proposal transfer alert arguments");
            break;
        case 0x1077b720:
            require(c.r(3) == fixture + 0xf50 && (c.r(4) == singles + 0x1ea
                || (c.r(4) == singles + 0x14 && c.r(8) == 0xc31)), "Proposal diagnostic arguments");
            c.put32(c.r(3), c.r(5));
            break;
        case 0x1014c2e0:
            require(c.r(4) == 0 && c.r(5) == 1 && c.r(6) == 0 && c.r(7) == 0, "Proposal wage cap callback arguments");
            c.set(3, c.u32(selector_result + 0x60) & 1);
            break;
        case 0x100f2680:
            require(c.r(3) == wage_service && c.r(4) == person, "Proposal comparative wage lookup arguments");
            c.set(3, c.u32(selector_result + 0x64));
            break;
        case 0x10366470:
            require(c.r(3) == staff_service && c.r(4) == person && c.r(6) == 0, "Proposal staff interest arguments");
            c.set_f(1, double(c.u32(selector_result + 0x54)) / 100.0);
            break;
        case 0x10369080:
            require(c.r(3) == staff_service && c.r(4) == person && c.r(6) == 0 && c.r(7) == 0,
                    "Proposal interest arguments");
            c.set_f(1, double(c.u32(selector_result + 0x54)) / 100.0);
            break;
        case 0x10103510:
            require(c.r(3) == fixture, "Proposal wage service manager");
            c.set(3, c.r(4) == club || c.r(4) == other_club ? wage_service : 0);
            break;
        case 0x100de260:
            c.set(3, c.r(3) ? c.u8(c.r(3) + 0x240) : 0);
            break;
        case 0x10718130:
            require(c.r(3) == existing + 0x30 || c.r(3) == 0x30, "Proposal remaining days argument");
            c.set(3, c.r(3) == 0x30 ? 1000 : c.u32(selector_result + 8));
            break;
        case 0x103f43b0:
        case 0x100d3150:
            c.set(3, c.u8(selector_result + 12));
            break;
        case reference_quote_entry:
            require(c.r(3) == record && c.r(4) == person && c.r(6) <= 1 && c.r(8) <= 11 && c.r(9) <= 3,
                    "Proposal reference quote arguments "
                        + hex_address(c.r(3)) + "/" + hex_address(c.r(4))
                        + " incumbent=" + std::to_string(c.r(6)) + " role=" + std::to_string(c.r(8))
                        + " adjustment=" + std::to_string(c.r(9)));
            c.set_f(1, double(c.u32(selector_result + 0x50)) + c.r(9) * 100.0 + c.r(6) * 50.0);
            break;
        case 0x10715880:
            c.put32(c.r(3), 0x00a407d0);
            c.put8(c.r(3) + 4, 0x80);
            break;
        case 0x10082f20:
            require(c.r(3) == fixture && c.r(4) == person, "Proposal primary record argument order");
            c.set(3, c.u32(primary_result));
            break;
        case 0x10082f90:
            require(c.r(3) == fixture && c.r(4) == person && c.r(5) == club, "Proposal existing contract argument order");
            c.set(3, c.u32(other_result));
            break;
        case 0x100ce320:
            require(c.r(3) == club && c.r(4) == c.sp() + 0x38 && c.r(5) == 1, "Proposal related club argument order");
            c.put8(c.r(4), c.u8(club_result + 8));
            c.set(3, c.u32(club_result));
            break;
        case 0x1007c140:
            require(c.r(3) == fixture && c.r(4) == person && (c.r(5) == existing || c.r(5) == 0), "Proposal role lookup argument order");
            c.set(3, c.u8(selector_result));
            break;
        case 0x1036d510:
            require(c.r(3) == staff_service && c.r(4) == person && c.r(6) == 0, "Proposal selection argument order");
            c.set(3, c.u8(selector_result));
            break;
        case 0x10084be0:
            require(c.r(3) == fixture && c.r(4) == person && c.r(6) == 1 && c.r(7) == UINT32_MAX,
                    "Proposal valuation argument order");
            c.set(3, c.u32(valuation_input));
            break;
        case 0x100c7a20:
            c.set(3, c.u8(selector_result + 1));
            break;
        default:
            return false;
    }
    if (calls) calls->push_back({target, link, arguments});
    c.finish();
    return true;
}

struct ScopedServices {
    decltype(dispatch_probe) previous_probe = dispatch_probe;
    decltype(calls) previous_calls = calls;
    RoutineChecker* previous_checker = routine_checker;
    explicit ScopedServices(std::vector<Call>* trace = nullptr, RoutineChecker* checker = nullptr) {
        dispatch_probe = dependency;
        calls = trace;
        routine_checker = checker;
    }
    ~ScopedServices() {
        dispatch_probe = previous_probe;
        calls = previous_calls;
        routine_checker = previous_checker;
    }
};

struct Comparison {
    unsigned executed;
    unsigned competition_checks;
    bool faulted;
};

Comparison compare_proposal(const State& seed, bool reject_changed_wage = false) {
    auto modern = clone(seed), original = clone(seed);
    Effects actual, expected;
    actual.state = modern.get();
    expected.state = original.get();
    modern->memory.date_observer = &actual;
    original->memory.date_observer = &expected;
    std::vector<Call> original_calls, modern_calls;
    const auto execute = [](State& s, bool reference, std::vector<Call>& trace) {
        // The original must reach the fixture's external services through
        // dispatch, even when a generated direct edge exists for that site.
        RoutineChecker no_hooks([](std::uint32_t) { return false; }, 0, 0);
        ScopedServices services(&trace, reference ? &no_hooks : nullptr);
        try {
            if (reference) staff_contracts_run_original(s, proposal_entry);
            else staff_contracts_run_rewrite(s, proposal_entry);
            return std::string{};
        } catch (const std::runtime_error& error) {
            return std::string(error.what());
        }
    };
    const auto old_stop = execute(*original, true, original_calls);
    const auto new_stop = execute(*modern, false, modern_calls);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto label = "Proposal original PC=" + hex_address(original->pc) + " C++ PC=" + hex_address(modern->pc)
        + " budget=" + std::to_string(seed.budget);
    require(old_stop == new_stop, label + " original=" + old_stop + " C++=" + new_stop);
    require(old_stop.empty() || old_stop == "Native instruction budget exhausted"
                || old_stop.starts_with("Unmapped data access at "), label + " unexpected stop " + old_stop);
    require(original_calls == modern_calls, label + " changed dependency arguments or order");
    require(expected.bytes == actual.bytes, label + " changed ordered memory accesses");
    require(original->executed == modern->executed, label + " changed instruction accounting");
    const Image reference(*original);
    reference.compare(*modern, label);
    if (reject_changed_wage) {
        Cpu c(*modern);
        c.put32(record + 12, c.u32(record + 12) ^ 1u);
        bool caught = false;
        try {
            reference.compare(*modern, label);
        } catch (const std::runtime_error& error) {
            caught = std::string_view(error.what()).starts_with(label + " memory mismatch");
        }
        require(caught, label + " missed a changed proposal wage");
    }
    const auto competition = std::ranges::count_if(modern_calls, [](const Call& call) { return call.target == 0x10781320; });
    return {unsigned(modern->executed), unsigned(competition), old_stop.starts_with("Unmapped data access at ")};
}

std::unique_ptr<State> proposal_fixture() {
    auto result = std::make_unique<State>();
    auto& seed = *result;
    seed.registers.fill(0xa5);
    seed.unique.fill(0xb7);
    for (unsigned bit = 1280; bit < 1312; ++bit) seed.wr<1>(bit, 0);
    seed.memory.map(fixture, 0x2900);
    seed.memory.map(metadata, 0x1600);
    seed.memory.map(stack - 0x400, 0x500);
    seed.memory.map(toc - 0x8000, 0x1000);
    seed.memory.map(toc - 0x5000, 0x1000);
    Cpu c(seed);
    c.set({{1, stack}, {2, toc}, {3, record}, {4, person}, {5, 0}, {6, 2}, {7, UINT32_MAX},
           {8, 11}, {9, 1}, {10, selector_result + 0x5c}});
    c.set_lr(return_sentinel);
    seed.pc = proposal_entry;
    seed.budget = 10000;
    c.put32(toc - 0x7b9c, metadata_slot);
    c.put32(toc - 0x7ab8, manager + 0x48);
    c.put32(manager + 0x48, fixture);
    c.put32(wage_service + 0x10, 15000);
    c.put32(toc - 0x7a10, region_slot);
    c.put32(toc - 0x7a14, region_slot + 4);
    c.put32(toc - 0x7a18, region_slot + 8);
    c.put32(region_slot, 1);
    c.put32(region_slot + 4, 2);
    c.put32(region_slot + 8, 3);
    c.put_f32(singles + 0x14, 85000.0f);
    c.put_f64(doubles + 0xd8, 0.4);
    for (auto offset : {0x10u, 0x78u, 0xe8u, 0xf0u, 0xf8u, 0x100u, 0x108u})
        c.put_f64(doubles + offset, double(offset % 13) / 15.0);
    c.put_f64(doubles + 0xe0, 1.2);
    for (auto offset : {0x18u, 0x1cu, 0x20u, 0x24u})
        c.put_f32(singles + offset, float(offset) / 15.0f);
    c.put32(metadata_slot, metadata);
    c.put32(toc - 0x7b5c, competition_slot);
    c.put32(competition_slot, competition_table);
    c.put32(competition_table, competition_entry);
    c.put32(competition_entry, competition_vtable);
    c.put32(competition_vtable + 0xa0, 0xa0);
    c.put32(competition_vtable + 0xa4, 0xa4);
    c.put_f32(singles + 0x94, 20.0f);
    c.put_f32(singles + 0x98, 0.12f);
    c.put_f32(singles + 0x9c, 25.0f);
    c.put_f64(doubles + 0x190, 2500000.0);
    c.put_f64(doubles + 0x198, 1.4);
    c.put_f64(doubles + 0x8, 15000.0);
    c.put_f64(doubles + 0x20, 0.5);
    c.put_f64(doubles + 0x38, 1.1);
    c.put32(toc - 0x4da4, singles);
    c.put32(toc - 0x7bb8, fixture + 0xf50);
    c.put32(toc - 0x7ba8, fixture + 0xf50);
    seed.memory.map(toc - 0x5b20, 4);
    c.put32(toc - 0x5b20, 0x1234);
    c.put32(toc - 0x7bac, transfer_entry + 8);
    c.put8(transfer_entry + 8, 1);
    c.put32(toc - 0x7b7c, manager);
    c.put32(manager, fixture);
    c.put32(toc - 0x7b78, cache);
    c.put32(toc - 0x7be8, club_slot);
    c.put32(club_slot, club);
    c.put32(toc - 0x7be4, population);
    c.put32(population, 1000);
    c.put32(toc - 0x7b80, today);
    c.put16(today, 77);
    c.put16(today + 2, 2002);
    c.put8(today + 4, 0x80);
    c.put32(toc - 0x7b84, random_cursor);
    c.put32(toc - 0x7b88, random_last);
    c.put32(toc - 0x7b8c, random_draws);
    c.put32(toc - 0x7b90, random_salt);
    c.put32(toc - 0x7b44, league_configuration);
    c.put32(toc - 0x4db4, duration_table);
    c.put32(toc - 0x7990, random_draws + 0xc);
    c.put8(random_draws + 0xc, 5);
    c.put32(random_last, random_draws + 4);
    c.put16(random_draws, 77);
    c.put16(random_draws + 2, 453);
    c.put32(random_salt, 400);
    c.put32(toc - 0x7594, region_slot);
    c.put32(toc - 0x75b0, region_slot + 4);
    c.put32(transfer_entry, club);
    const std::array<std::uint32_t, 8> role_adjustments{0x10378338, 0x103782c0, 0x10378338, 0x103782e0,
        0x103782d0, 0x103782e0, 0x10378314, 0x10378334};
    for (unsigned i = 0; i < role_adjustments.size(); ++i)
        c.put32(duration_table + 4 * i, role_adjustments[i]);
    c.put_f32(singles + 0xa0, 2.5f);
    c.put_f32(singles + 0xa4, 40000.0f);
    c.put_f32(singles + 0xa8, 65000.0f);
    c.put_f32(singles + 0xac, 85000.0f);
    c.put_f32(singles + 0xb0, 20.0f);
    c.put_f32(singles + 0xb4, 50.0f);
    c.put_f32(singles + 0xb8, 45000.0f);
    c.put_f32(singles + 0xbc, 65000.0f);
    c.put_f32(singles + 0xc0, 0.0f);
    c.put_f32(singles + 0xc4, 25.0f);
    c.put_f32(singles + 0xc8, 30.0f);
    c.put_f32(singles + 0xcc, 35.0f);
    c.put32(toc - 0x4dac, doubles);
    c.put32(toc - 0x4da8, singles);
    c.put32(toc - 0x7b40, manager + 0x38);
    c.put32(manager + 0x38, staff_service);
    c.put32(staff_service + 0x988, staff_service);
    c.put_f64(doubles + 0xc0, 4503601774854144.0);
    c.put_f64(doubles + 0xc8, 4503599627370496.0);
    for (auto offset : {0x28u, 0x30u, 0x48u, 0x88u, 0xd0u})
        c.put_f64(doubles + offset, double(offset) / 200.0);
    c.put_f64(doubles + 0x110, 1.0);
    for (unsigned offset = 0x118; offset <= 0x160; offset += 8)
        c.put_f64(doubles + offset, 0.8 + double(offset % 23) / 10.0);
    for (unsigned offset = 0x28; offset <= 0x54; offset += 4)
        c.put_f32(singles + offset, 1.0f + float(offset % 11) / 10.0f);
    c.put_f32(singles + 0x58, 0.93f);
    c.put_f32(singles + 0x5c, 1.03f);
    c.put_f32(singles + 0x60, 120000.0f);
    c.put_f32(singles + 0x64, 100000.0f);
    c.put_f32(singles + 0x68, 95000.0f);
    c.put_f32(singles + 0x6c, 110000.0f);
    c.put_f64(doubles + 0x168, 0.2);
    c.put_f64(doubles + 0x170, 0.1);
    c.put_f64(doubles + 0x178, 40000.0);
    c.put_f64(doubles + 0x180, 100.0);
    c.put_f64(doubles + 0x188, 1.0);
    for (unsigned offset = 0x70; offset <= 0x90; offset += 4)
        c.put_f32(singles + offset, 70000.0f + float(offset) * 100.0f);
    c.put32(club + 0x2c, league);
    c.put32(other_club + 0x2c, league);
    c.put_f32(singles, 0.25f);
    c.put_f32(singles + 4, 0.5f);
    c.put32(person + 0x20, other_club);
    c.put32(person, 42);
    c.put32(club, 55);
    c.put32(other_club, 70);
    c.put32(primary + 4, 0);
    c.put16(person + 0x5c, 25);
    c.put8(person + 0x5e, 12);
    c.put8(league + 0x11f, 3);
    c.put8(record + 0x38, 0xa5);
    c.put8(record + 0x60, 0xf2);
    c.put32(person + 0x14, profile);
    c.put32(person + 0x24, club);
    c.put32(random_cursor, random_draws);
    c.put8(person + 0x62, 11);
    c.put16(profile + 10, 5500);
    c.put16(profile + 12, 5500);
    return result;
}
}

bool proposal_hook(std::uint32_t entry) {
    if (entry != proposal_entry) return false;
    auto seed = proposal_fixture();
    RoutineChecker checker([](std::uint32_t target) { return target == proposal_entry; }, 1, 1);
    checker.strict_volatile = true;
    ScopedServices services(nullptr, &checker);
    run_until(*seed, return_sentinel);
    const auto total = checker.totals().find(entry);
    require(total != checker.totals().end() && total->second.checked == 1 && !total->second.differences,
            "Default proposal page did not select its C++ owner");
    return true;
}

void proposal_cases() {
    auto input = proposal_fixture();
    State& seed = *input;
    Cpu c(seed);
    unsigned competitions = 0;
    unsigned cases = 0;
    for (const auto destination : {0u, club, other_club})
        for (const auto first : {0u, primary})
            for (const auto alternate : {0u, existing})
                for (const auto staff_profile : {0u, profile})
                    for (const auto staff_nation : {0u, nation})
                        for (const auto role : {0u, 1u, 3u, UINT32_MAX}) {
                            c.set({{5, destination}, {7, role}});
                            c.put32(primary_result, first);
                            c.put32(other_result, alternate);
                            c.put32(person + 0x14, staff_profile);
                            c.put32(person + 0x1c, staff_nation);
                            for (unsigned index = 0; index < 12; ++index) {
                                const auto value = int(index) * 500 - 1500;
                                c.put16(club + 0x1ec, value);
                                c.put16(other_club + 0x1ec, value + 1750);
                                c.put16(profile + 12, destination && index > 7 ? 8500 : value - 250);
                                c.put8(existing + 0x44, index > 5 ? 60 : 10);
                                c.put8(existing + 0x46, index % 3);
                                c.put8(club + 0x240, index & 1);
                                c.put8(other_club + 0x240, index & 1);
                                c.put32(existing + 12, index % 3 == 1 ? 43000 : 25000);
                                c.put32(selector_result + 0x50, index > 5 ? 140000 : 2500);
                                c.put32(league + 0x68, index % 5);
                                c.put32(club + 0x30, index & 4 ? competition : 0);
                                c.put32(other_club + 0x30, index & 4 ? competition : 0);
                                c.put16(competition + 0x18, 16);
                                c.put16(profile + 10, value + 500);
                                c.put16(nation + 0x2a, value);
                                c.put8(club + 0x23d, index % 3);
                                c.put8(other_club + 0x23d, index % 3);
                                const std::uint32_t condition_cases[] = {0, 0, 0x10, 2, 1, 0x8000, 0, 4, 0, 0x100, 0x01000000, 0};
                                c.put32(existing + 0x50, condition_cases[index]);
                                c.set(10, index & 2 ? selector_result + 0x5c : 0);
                                c.put32(selector_result + 0x54, (index + 1) * 14);
                                c.put32(selector_result + 0x60, index);
                                c.put32(selector_result + 0x64, index * 4000 + 1000);
                                const unsigned categories[] = {6, 8, 9, 10, 11, 1};
                                c.set(8, categories[index % 6]);
                                c.put8(primary + 0x38, index & 4 ? 20 : 4);
                                c.put8(selector_result, index % 5);
                                c.put8(selector_result + 1, index & 1);
                                c.set(6, index & 1 ? 2 : UINT32_MAX);
                                c.put32(random_cursor, random_draws + (index & 2 ? 4 : 0));
                                c.put8(league_configuration + 0x64, index & 1);
                                c.put16(person + 0x54, 150 + index * 5);
                                c.put16(person + 0x56, 2000 + index);
                                c.put8(person + 0x58, 0x80);
                                c.put8(profile + 0x13, index & 2 ? 16 : 12);
                                c.put8(profile + 0x14, index & 4 ? 17 : 9);
                                c.put8(profile + 0x15, index & 8 ? 18 : 10);
                                c.put32(valuation_input, index & 2 ? UINT32_MAX - 5000 : index & 8 ? 2000000 : 90000);
                                c.put8(competition_entry + 0x43, index & 8 ? 2 : 0);
                                c.put8(competition_entry + 0x44, index & 1);
                                c.put8(competition_entry + 0x45, index & 4);
                                for (auto clause = 0x1cu; clause <= 0x20; ++clause)
                                    c.put8(existing + clause, (index + clause) & 1);
                                c.put32(person + 0x24, destination && index & 4 ? 0 : club);
                                c.put16(person + 0x5c, 17 + index);
                                c.put8(person + 0x66, index);
                                c.put8(person + 0x65, index > 8 ? 45 : index);
                                c.put8(person + 0x63, index > 9 ? 60 : index * 3);
                                c.put8(person + 0x62, index & 2 ? 0 : 11);
                                c.put8(person + 0x67, index * 2);
                                c.put8(profile + 15, index == 9 ? 20 : 0);
                                c.put8(profile + 17, index == 7 ? 20 : 0);
                                c.put8(profile + 21, index == 11 ? 20 : 0);
                                c.put32(selector_result + 8, index > 8 ? 12 : index * 40);
                                c.put8(selector_result + 12, index & 4);
                                c.put8(metadata + 42 * 100 + 12, index & 8 ? 0x20 : 0);
                                c.put8(league + 0x11f, index + 1);
                                c.put32(club_result, index & 2 ? other_club : 0);
                                c.put8(club_result + 8, index & 8);
                                c.put32(person + 0xc, index & 1 ? league : 0);
                                if (destination == 0 && (first == 0 || (index & 4)) && index % 6 < 4
                                    && (staff_profile == 0 || !(index & 1))) continue;
                                try { competitions += compare_proposal(seed).competition_checks; }
                                catch (const std::exception& error) {
                                    throw std::runtime_error("Proposal fixture " + std::to_string(cases) + ": " + error.what());
                                }
                                ++cases;
                            }
                        }
    input = proposal_fixture();
    auto& dated = *input;
    Cpu terms(dated);
    terms.set({{5, club}, {6, UINT32_MAX}});
    terms.put32(primary_result, primary);
    terms.put32(other_result, existing);
    terms.put8(selector_result + 1, 1);
    terms.put16(club + 0x1ec, 500);
    terms.put32(person + 0x1c, nation);
    terms.put8(competition_entry + 0x43, 2);
    terms.put8(competition_entry + 0x44, 1);
    terms.put8(competition_entry + 0x45, 1);
    terms.put32(club + 0x30, competition);
    terms.put32(valuation_input, 1800000);
    for (unsigned role = 0; role < 8; ++role) {
        terms.set(7, role);
        for (const auto age : {17, 19, 20, 21, 24, 27, 29, 30, 31, 32, 34, 37}) {
            terms.put16(person + 0x5c, age);
            terms.put8(person + 0x63, age > 32 ? 45 : 18);
            terms.put8(person + 0x64, 20);
            terms.put8(person + 0x65, age & 1 ? 19 : 2);
            terms.put8(person + 0x67, 20);
            terms.put8(profile + 0xf, age & 1 ? 20 : 2);
            terms.put8(profile + 0x11, age & 2 ? 19 : 0);
            terms.put8(profile + 0x15, age & 4 ? 20 : 8);
            for (const auto rating : {800, 2600, 3900, 8000}) {
                terms.put16(profile + 10, rating);
                terms.put16(profile + 12, rating);
                try { competitions += compare_proposal(dated).competition_checks; }
                catch (const std::exception& error) {
                    throw std::runtime_error("Proposal dated role=" + std::to_string(role)
                        + " age=" + std::to_string(age) + " rating=" + std::to_string(rating) + ": " + error.what());
                }
                ++cases;
            }
        }
    }

    const auto steps = compare_proposal(dated, true).executed;
    ++cases;
    unsigned budgets = 0;
    for (unsigned limit = 1; limit < steps; limit += 7) {
        dated.budget = limit;
        compare_proposal(dated);
        ++budgets;
    }
    dated.budget = 10000;
    terms.set(3, fixture + 0x6000);
    require(compare_proposal(dated).faulted, "Proposal contract destination must fault");
    terms.set(3, record);
    terms.put32(toc - 0x7b84, fixture + 0x6000);
    require(compare_proposal(dated).faulted, "Proposal RNG cursor must fault");
    terms.put32(toc - 0x7b84, random_cursor);

    terms.set({{5, other_club}, {6, 2}, {7, 1}});
    terms.put32(transfer_entry + 4, 1);
    require(compare_proposal(dated).faulted, "Missing transfer faults on an unmapped null entry");
    dated.memory.map(0, 4);
    require(!compare_proposal(dated).faulted, "Mapped null transfer diagnostic must return");
    ++cases;
    terms.put32(transfer_entry + 4, 0);
    terms.put16(club + 0x1ec, 4500);
    terms.put16(other_club + 0x1ec, 500);
    terms.put16(person + 0x5c, 25);
    terms.put8(person + 0x65, 2);
    terms.put8(person + 0x63, 16);
    terms.put8(person + 0x66, 12);
    terms.put8(existing + 0x44, 65);
    terms.put8(existing + 0x46, 3);
    terms.put32(existing + 0x50, 0x100);
    for (const auto role : {1u, 3u})
        for (const auto wage : {48000u, 900000u})
            for (const auto region : {1u, 2u, 3u, 4u})
                for (const auto rating : {6300, 8000}) {
                    terms.set(7, role);
                    terms.put32(existing + 12, wage);
                    terms.put32(league + 0x68, region);
                    terms.put16(profile + 10, 8200);
                    terms.put16(profile + 12, rating);
                    terms.put8(profile + 0xf, 3);
                    terms.put8(profile + 0x15, region & 1 ? 20 : 12);
                    terms.put32(selector_result + 0x50, region & 1 ? 125000 : 25000);
                    competitions += compare_proposal(dated).competition_checks;
                    ++cases;
                }
    require(competitions > 0, "Proposal competition fixtures did not reach their callbacks");
    std::cout << "Proposal " << cases << " returns, " << budgets << " budget limits, 3 partial faults, "
              << competitions << " competition checks and one detected wage mutation\n";
}
}
