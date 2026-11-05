#include "staff_contracts.hpp"
#include "staff_contracts_adapter.hpp"
#include "shortlist_quotation_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <bit>
#include <cmath>

namespace cm::staff_contracts_checks {
namespace {
using aot::guest::Cpu;
using aot::staff_contracts::country_quote_entry;
using aot::staff_contracts::reference_quote_entry;
constexpr std::array quote_entries{country_quote_entry, reference_quote_entry};

using cm::semantic_io_checks::Effects;
using cm::semantic_io_checks::Image;
using cm::semantic_io_checks::clone;
using cm::semantic_io_checks::fixture;
using cm::semantic_io_checks::require;
using cm::semantic_io_checks::stack;
using cm::semantic_io_checks::toc;

constexpr auto manager = fixture + 0x30, cache = fixture + 0x34, denominator = fixture + 0x38;
constexpr auto clubs_slot = fixture + 0x3c, player = fixture + 0x100, club = fixture + 0x400;
constexpr auto old_club = fixture + 0x700, profile = fixture + 0xa00, nation = fixture + 0xb00;
constexpr auto contract = fixture + 0xc00, doubles = fixture + 0x1400, singles = fixture + 0x1800;
constexpr auto tables = fixture + 0xf00, seed_contract = fixture + 0xf80;
constexpr auto market_mode = fixture + 0xf84, destination_league = fixture + 0x1000, home_league = fixture + 0x1200;

std::unique_ptr<State> wage_quote_fixture() {
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
    c.set({{1, stack}, {2, toc}, {3, contract}, {4, player}, {6, 0}, {7, 3}, {8, 11}, {9, 0}});
    c.set_lr(return_sentinel);
    seed.budget = 10000;
    c.put32(toc - 0x7b7c, manager);
    c.put32(manager, 0x31003000);
    c.put32(toc - 0x7b78, cache);
    c.put32(toc - 0x7a68, market_mode);
    c.put32(toc - 0x7be4, denominator);
    c.put32(toc - 0x7be8, clubs_slot);
    c.put32(clubs_slot, old_club);
    c.put32(toc - 0x4dac, doubles);
    constexpr auto conversion_thresholds = fixture + 0x1380;
    seed.memory.map(toc - 0x3b7c, 4);
    c.put32(toc - 0x3b7c, conversion_thresholds);
    c.put_f64(conversion_thresholds, 0.0);
    c.put_f64(conversion_thresholds + 8, 4294967296.0);
    c.put_f64(conversion_thresholds + 16, 2147483648.0);
    c.put32(toc - 0x4da8, singles);
    c.put_f64(doubles + 0xc0, 4503601774854144.0);
    c.put_f64(doubles + 0x28, 0.5);
    c.put_f64(doubles + 0x30, 1.2);
    c.put_f64(doubles + 0x48, 0.75);
    c.put_f64(doubles + 0x58, 2.5);
    c.put_f64(doubles + 0x148, 1.3);
    c.put_f64(doubles + 0xc8, 4503599627370496.0);
    c.put_f64(doubles + 0xa0, 0.65);
    c.put_f64(doubles + 0xb0, 3250.0);
    c.put_f64(doubles + 0x150, 0.75);
    c.put_f64(doubles + 0x158, 0.8);
    c.put_f64(doubles + 0x198, 0.7);
    c.put_f64(doubles + 0x1b0, 0.85);
    c.put_f64(doubles + 0x1b8, 0.9);
    c.put_f64(doubles + 0x1c0, 2000000.0);
    c.put_f64(doubles + 0x1a8, 2000000.0);
    for (auto offset : {0x14u, 0x2cu, 0x40u, 0x58u, 0x80u, 0xa4u, 0xa8u, 0xd0u, 0xd4u, 0xd8u, 0xdcu, 0xe0u, 0xe4u, 0x10cu, 0x110u, 0x114u})
        c.put32(singles + offset, std::bit_cast<std::uint32_t>(float(offset) / 2.0f + 10.0f));
    c.put32(toc - 0x4db8, tables);
    c.put32(toc - 0x4dbc, tables + 0x40);
    constexpr std::array country{0x10379024u, 0x1037918cu, 0x10379378u, 0x1037955cu, 0x10379718u, 0x10379884u,
                                  0x103799f0u, 0x103799f0u, 0x103799f0u, 0x1037a3e4u, 0x103799f0u};
    constexpr std::array reference{0x1037b974u, 0x1037badcu, 0x1037bcc8u, 0x1037beacu, 0x1037c068u, 0x1037c1d4u,
                                    0x1037c340u, 0x1037c340u, 0x1037c340u, 0x1037cfe8u, 0x1037c340u};
    for (unsigned n = 0; n < country.size(); ++n) {
        c.put32(tables + n * 4, country[n]);
        c.put32(tables + 0x40 + n * 4, reference[n]);
    }
    c.set(5, club);
    c.put32(player + 0x14, profile);
    c.put8(player + 0x60, 11);
    c.put16(profile + 6, 70);
    c.put8(club + 0x240, 1);
    c.put32(club + 0x2c, destination_league);
    c.put8(destination_league + 0x118, 0x81);
    c.put32(toc - 0x7b80, fixture + 0x1360);
    c.put32(toc - 0x7bb8, fixture + 0x1370);
    c.put32(toc - 0x4da4, singles);
    seed.memory.map(toc - 0x34c4, 4);
    c.put32(toc - 0x34c4, 999);
    c.put32(toc - 0x7a10, fixture + 0x13a0);
    c.put32(toc - 0x7a14, fixture + 0x13a4);
    c.put32(toc - 0x7a18, fixture + 0x13a8);
    c.put32(fixture + 0x13a0, 1);
    c.put32(fixture + 0x13a4, 2);
    c.put32(fixture + 0x13a8, 3);
    c.put_f64(doubles + 0x88, 0.5);
    c.put_f64(doubles + 0x238, 0.9);
    c.put_f64(doubles + 0x1d8, 1.1);
    c.put_f64(doubles + 0x108, 1.5);
    for (auto offset : {0xe0u, 0x1c8u, 0x1d0u})
        c.put_f64(doubles + offset, 0.0003);
    for (auto offset : {0x78u, 0xd8u, 0x110u, 0x118u, 0x130u, 0x1e0u, 0x1e8u, 0x1f0u, 0x1f8u, 0x200u, 0x208u})
        c.put_f64(doubles + offset, double(offset % 300) / 200.0);
    for (auto offset : {0x18u, 0x6cu, 0x7cu, 0xecu, 0xf4u, 0x100u, 0x11cu, 0x120u, 0x124u, 0x128u, 0x12cu, 0x15cu, 0x160u, 0x164u})
        c.put32(singles + offset, std::bit_cast<std::uint32_t>(float(offset) * 0.01f));
    c.put32(singles + 0x98, std::bit_cast<std::uint32_t>(0.9f));
    c.put32(singles + 0x54, std::bit_cast<std::uint32_t>(1.1f));
    c.put32(singles, std::bit_cast<std::uint32_t>(0.85f));
    c.put32(singles + 0x4c, std::bit_cast<std::uint32_t>(0.85f));
    c.put32(singles + 0x118, std::bit_cast<std::uint32_t>(0.65f));
    c.put8(destination_league + 0x11f, 8);
    c.put8(home_league + 0x118, 0x83);
    c.put8(market_mode, 1);
    constexpr std::array market_slots{-0x7a24, -0x7a20, -0x7a1c, -0x7af4, -0x7a00, -0x7a28,
        -0x7514, -0x757c, -0x7594, -0x7574, -0x7578, -0x7650, -0x750c, -0x7504, -0x75a8,
        -0x7510, -0x79fc, -0x7590, -0x7b68, -0x7508, -0x6778};
    seed.memory.map(toc - 0x6778, 4);
    for (unsigned i = 0; i < market_slots.size(); ++i) {
        c.put32(toc + market_slots[i], fixture + 0x1900 + i * 4);
        c.put32(fixture + 0x1900 + i * 4, i + 10);
    }
    for (auto offset : {0x40u, 0x60u, 0x68u, 0x80u, 0x90u, 0x98u, 0xb8u, 0x210u, 0x220u, 0x228u, 0x230u, 0x240u})
        c.put_f64(doubles + offset, double(offset % 300) + 20.0);
    for (auto offset : {0xf8u, 0x170u, 0x218u})
        c.put_f64(doubles + offset, double(offset % 300) / 200.0);
    c.put32(player + 0xc, destination_league);
    c.put32(player + 0x24, club);

    c.put32(toc - 0x7ab8, fixture + 0x1358);
    c.put32(fixture + 0x1358, manager);
    c.put32(fixture + 0x1350, club);
    c.put32(fixture + 0x1354, 8000);
    c.put8(player + 0x63, 15);
    c.put16(profile + 10, 7500);
    c.put8(contract + 0x60, 7);
    c.put8(contract + 0x38, 4);
    c.put32(contract + 12, 500);
    c.put16(club + 0x1ec, 7450);
    c.put16(profile + 12, 7500);
    c.put16(nation + 0x2c, 6500);
    c.put16(nation + 0x2a, 6500);
    c.put32(seed_contract, contract);
    c.put16(player + 0x5c, 25);
    c.put8(player + 0x62, 11);
    c.put8(player + 0x65, 1);
    return result;
}

struct QuoteCall {
    std::uint32_t target, link, stack, toc;
    std::array<std::uint32_t, 6> arguments;
    std::uint64_t first_float, second_float;
    bool operator==(const QuoteCall&) const = default;
};

std::vector<QuoteCall>* active_trace = nullptr;

bool primary_dependency(State& s) {
    Cpu c(s);
    const auto supported = s.pc == 0x10082f90 || s.pc == 0x10103510 || s.pc == 0x100f2680
        || s.pc == 0x10780fc0 || s.pc == 0x1077b720 || s.pc == 0x10718130 || s.pc == 0x100d9440
        || s.pc == 0x100de260 || s.pc == 0x10082f20;
    if (!supported) return false;
    if (active_trace) {
        QuoteCall call{s.pc, c.lr(), c.sp(), c.r(2), {}, s.r<8>(aot::guest::reg::fpr(1)), s.r<8>(aot::guest::reg::fpr(2))};
        for (unsigned i = 0; i < call.arguments.size(); ++i) call.arguments[i] = c.r(3 + i);
        active_trace->push_back(call);
    }
    if (s.pc == 0x10082f90 || s.pc == 0x10103510 || s.pc == 0x100f2680) {
        if (s.pc == 0x10082f90) {
            require(c.r(3) == 0x31003000 && c.r(4) == player && c.r(5) == old_club, "Quote current-club lookup arguments");
            c.set(3, c.u32(seed_contract));
        } else if (s.pc == 0x10103510) {
            require(c.r(3) == manager && c.r(4) == club, "Quote wage service lookup arguments");
            c.set(3, c.u32(fixture + 0x1350));
        } else {
            require(c.r(3) == club && c.r(4) == player, "Quote reference wage lookup arguments");
            c.set(3, c.u32(fixture + 0x1354));
        }
        c.finish();
        return true;
    }
    if (s.pc == 0x10780fc0) {
        c.put32(c.sp() + 0x14, c.r(2));
        c.set_f(1, std::pow(c.f(1), c.f(2)));
        c.finish();
        return true;
    }
    if (s.pc == 0x1077b720) {
        require(c.r(3) == fixture + 0x1370 && c.r(4) == singles + 0x233, "Quote diagnostic argument order");
        c.put32(c.r(3), c.r(5) ^ c.r(6));
        c.set(3, 0);
        c.finish();
        return true;
    }
    if (s.pc == 0x10718130 || s.pc == 0x100d9440) {
        require(c.r(s.pc == 0x10718130 ? 3 : 4) == fixture + 0x1360, "Quote date argument order");
        c.set(3, s.pc == 0x10718130 ? c.r(4) : c.u8(c.r(3) + 0x240));
        c.finish();
        return true;
    }
    if (s.pc == 0x100de260) {
        c.set(3, c.u8(c.r(3) + 0x240));
        c.finish();
        return true;
    }
    if (s.pc != 0x10082f20) return false;
    require(c.r(3) == 0x31003000 && c.r(4) == player, "Original primary-contract argument order");
    c.set(3, c.u32(seed_contract));
    c.finish();
    return true;
}

struct ScopedQuoteProbe {
    decltype(dispatch_probe) saved = dispatch_probe;
    std::vector<QuoteCall>* prior_trace = active_trace;
    explicit ScopedQuoteProbe(std::vector<QuoteCall>& trace) {
        dispatch_probe = primary_dependency;
        active_trace = &trace;
    }
    ~ScopedQuoteProbe() {
        dispatch_probe = saved;
        active_trace = prior_trace;
    }
};

unsigned compare_quote(const State& seed, bool negative = false) {
    auto original = clone(seed), modern = clone(seed);
    Effects old_effects, new_effects;
    old_effects.state = original.get();
    new_effects.state = modern.get();
    original->memory.date_observer = &old_effects;
    modern->memory.date_observer = &new_effects;
    std::vector<QuoteCall> original_calls, modern_calls;
    const auto execute = [&](State& s, bool original_route, std::vector<QuoteCall>& calls) {
        ScopedQuoteProbe probe(calls);
        try {
            if (original_route) staff_contracts_run_original(s, seed.pc);
            else staff_contracts_run_rewrite(s, seed.pc);
            return std::string{};
        } catch (const std::runtime_error& error) {
            return std::string(error.what());
        }
    };
    const auto old_stop = execute(*original, true, original_calls);
    const auto new_stop = execute(*modern, false, modern_calls);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto label = "Wage quote " + hex_address(seed.pc) + " role=" + std::to_string(seed.r<4>(28))
        + " age=" + std::to_string(original->memory.read<2>(player + 0x5c))
        + " budget=" + std::to_string(seed.budget) + " original PC=" + hex_address(original->pc)
        + " C++ PC=" + hex_address(modern->pc);
    require(old_stop == new_stop, label + " original=" + old_stop + " C++=" + new_stop);
    require(old_stop.empty() || old_stop == "Native instruction budget exhausted"
                || old_stop.starts_with("Unmapped data access at "), label + " unexpected stop " + old_stop);
    require(old_effects.bytes == new_effects.bytes, label + " changed the ordered memory accesses");
    require(original_calls == modern_calls, label + " changed an external call or its arguments");
    require(original->executed == modern->executed, label + " changed instruction accounting");
    const Image reference(*original);
    reference.compare(*modern, label);
    if (negative) {
        modern->wr<8>(aot::guest::reg::fpr(1), modern->r<8>(aot::guest::reg::fpr(1)) ^ 1);
        bool caught = false;
        try {
            reference.compare(*modern, label);
        } catch (const std::runtime_error& error) {
            caught = std::string_view(error.what()).starts_with(label + " register byte=");
        }
        require(caught, label + " missed a one-bit wage mutation");
    }
    return unsigned(modern->executed);
}
}

bool wage_quote_hook(std::uint32_t entry, RoutineChecker& checker) {
    if (std::ranges::find(quote_entries, entry) == quote_entries.end()) return false;
    auto seed = wage_quote_fixture();
    seed->pc = entry;
    std::vector<QuoteCall> calls;
    ScopedQuoteProbe probe(calls);
    const auto before = checker.totals().find(entry);
    const auto prior_checks = before == checker.totals().end() ? 0 : before->second.checked;
    run_until(*seed, return_sentinel);
    const auto result = checker.totals().find(entry);
    require(result != checker.totals().end() && result->second.checked == prior_checks + 1
                && result->second.differences == 0 && seed->pc == return_sentinel,
            "Default wage quote page did not use the C++ owner at " + hex_address(entry));
    if (entry == reference_quote_entry) {
        struct QuoteWitness final : ShortlistQuotationBoundaryObserver {
            unsigned started = 0, finished = 0, blocks = 0;
            void enter(const State&, std::uint32_t, bool) override { ++started; }
            void leave(State&, std::uint32_t, bool, ShortlistQuotationOutcome outcome) override {
                require(outcome == ShortlistQuotationOutcome::returned, "Reference quote observer return");
                ++finished;
            }
            void before_step(State&, std::uint32_t) override { ++blocks; }
        } witness;
        for (bool original : {false, true}) {
            auto traced = wage_quote_fixture();
            traced->pc = entry;
            auto& diagnostics = shortlist_quotation_diagnostics(*traced);
            diagnostics.observer = &witness;
            diagnostics.original = original;
            run_until(*traced, return_sentinel);
            require(diagnostics.calls[1] == unsigned(!original) && diagnostics.returns[1] == unsigned(!original)
                        && diagnostics.original_calls[1] == unsigned(original) && diagnostics.original_body == 0,
                    "Reference quote retains the shortlist original selector");
        }
        require(witness.started == 2 && witness.finished == 2 && witness.blocks > 1,
                "Reference quote retains the shortlist boundary and block observers");
    }
    return true;
}

void wage_quote_cases() {
    auto seed = wage_quote_fixture();
    Cpu c(*seed);
    unsigned completed = 0, budgets = 0, faults = 0;
    RoutineChecker dependencies([](std::uint32_t entry) { return entry != 0x10082f20; }, 0, 0);
    struct RestoreChecker {
        RoutineChecker* previous = routine_checker;
        ~RestoreChecker() { routine_checker = previous; }
    } restore;
    routine_checker = &dependencies;
    for (const auto entry : quote_entries) {
        seed->pc = entry;
        for (unsigned region = 0; region <= 32; ++region) {
            c.put32(destination_league + 0x68, region);
            for (const auto level : {0, 8, 25, 255}) {
                c.put8(destination_league + 0x11f, level);
                for (const auto type : {0, 1, 3}) {
                    c.put8(club + 0x23d, type);
                    for (const auto reputation : {1249, 2251, 2751, 7450}) {
                        c.put16(club + 0x1ec, reputation);
                        c.put16(profile + 12, reputation);
                        for (const auto age : {17, 25, 34}) {
                            c.put16(player + 0x5c, age);
                            compare_quote(*seed);
                            ++completed;
                        }
                    }
                }
            }
        }
        for (const auto role : {0, 1, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 127, 128, 255}) {
            c.put8(player + 0x62, role);
            c.set(8, role);
            for (const auto mode : {0, 1, 5}) {
                c.set(6, mode);
                c.put32(player + 0x1c, mode == 1 ? nation : 0);
                c.put32(player + 0xc, mode == 5 ? home_league : destination_league);
                for (const auto service : {0u, club}) {
                    c.put32(fixture + 0x1350, service);
                    for (const auto current : {0u, contract}) {
                        c.put32(seed_contract, current);
                        compare_quote(*seed);
                        ++completed;
                    }
                }
            }
        }
        c.set({{6, 0}, {7, 3}, {8, 11}});
        c.put8(player + 0x62, 11);
        c.put8(profile + 0x14, 20);
        c.put8(player + 0x65, 1);
        c.put32(seed_contract, contract);
        const auto steps = compare_quote(*seed, true);
        ++completed;
        for (unsigned budget = 1; budget < steps; ++budget) {
            seed->budget = budget;
            compare_quote(*seed);
            ++budgets;
        }
        seed->budget = 10000;
        for (const auto missing : {0x30002000u, 0x30003000u}) {
            c.set(5, missing);
            compare_quote(*seed);
            ++faults;
        }
        c.set(5, club);
    }
    std::cout << "Wage quotes " << completed << " returns, " << budgets << " budget stops, "
              << faults << " partial faults, and 2 detected result mutations\n";
}
}
