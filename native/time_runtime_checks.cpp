#include "time_runtime_fixtures.hpp"
#include <chrono>
#include <random>

namespace {
using namespace cm::time_runtime_checks;
Expectation calendar(std::uint32_t seconds, std::uint32_t address = fields) {
    using namespace std::chrono;
    const auto elapsed = std::uint32_t(seconds - 0x7c558180);
    const auto day = sys_days{year{1900}/1/1} + days{elapsed / 86400};
    const year_month_day date{day}; const auto within = elapsed % 86400;
    const std::array<std::uint32_t, 8> expected{within % 60, within / 60 % 60, within / 3600,
        unsigned(date.day()), unsigned(date.month()) - 1, std::uint32_t(int(date.year()) - 1900),
        weekday{day}.c_encoding(), std::uint32_t((day - sys_days{date.year()/1/1}).count())};
    return [address, expected](const State& s, const Imports&) {
        for (unsigned n = 0; n < expected.size(); ++n) require(s.memory.read<4>(address + n * 4) == expected[n],
            "Independent Gregorian field " + std::to_string(n) + " expected=" + std::to_string(expected[n]) +
            " actual=" + std::to_string(s.memory.read<4>(address + n * 4)));
        require(s.pc == caller && s.r<4>(4) == stack, "Calendar return/frame");
    };
}
void roots_and_faults() {
    for (unsigned index = 0; index < 8; ++index) {
        auto s = fixture_state(index); check(index, *s, "root own negative", {}, {}, {}, true);
        fault_matrix(index, *s, "root faults");
        const auto accesses = check(index, *s, "minimal mappings inventory");
        auto minimal = accessed_only(*s, accesses); check(index, *minimal, "only accessed bytes mapped");
    }
}
void leap_offsets() {
    auto s = fixture_state(1);
    for (int year = -405; year <= 405; ++year) {
        s->wr<4>(12, std::uint32_t(year));
        const auto floor400 = (year % 400 + 400) % 400;
        const bool leap = year % 4 == 0 && (year % 100 != 0 || floor400 == 100);
        check(1, *s, "offset=" + std::to_string(year), [leap](const State& out, const Imports&) {
            require(out.r<4>(12) == unsigned(leap), "Independent leap years since 1900");
        });
    }
    for (std::uint32_t edge : {0x80000000u, 0x80000001u, 0x7fffffffu, 0xffffffffu}) {
        s->wr<4>(12, edge); check(1, *s, "extreme=" + hex_address(edge));
    }
}
void calendar_values() {
    using namespace std::chrono;
    auto s = fixture_state(2);
    std::mt19937 random(0x23c1a001);
    for (auto seconds : {0u, 1u, 59u, 60u, 3599u, 3600u, 86399u, 86400u, 0x7c558180u, 0x7c55817fu,
                        0x7fffffffu, 0x80000000u, 0xffffffffu, 946684800u}) {
        s->wr<4>(12, seconds); check(2, *s, "timestamp=" + hex_address(seconds), calendar(seconds));
    }
    for (int y : {1900, 1904, 1950, 1970, 1999, 2000, 2001, 2024, 2036})
        for (unsigned m = 1; m <= 12; ++m) for (unsigned d : {1u, 28u}) {
            const auto seconds = std::uint32_t(duration_cast<std::chrono::seconds>((sys_days{year{y}/month{m}/day{d}} - sys_days{year{1970}/1/1})).count());
            s->wr<4>(12, seconds); check(2, *s, "month/year boundary", calendar(seconds));
        }
    for (unsigned n = 0; n < 128; ++n) {
        const auto seconds = random(); s->wr<4>(12, seconds); check(2, *s, "random=" + std::to_string(n), calendar(seconds));
    }
    s = fixture_state(2); s->wr<4>(16, 0); check(2, *s, "null destination");
    s = fixture_state(2); s->memory.write<2>(cm::time_runtime_checks::months + 26 + 2, 32);
    check(2, *s, "live leap cumulative months");
    s = fixture_state(2); s->memory.write<2>(cm::time_runtime_checks::months + 26, 0xffff);
    check(2, *s, "signed cumulative month start");
    check(2, *s, "live STM/LM address cursor", {}, [](Monitor& m, Imports&) {
        m.on_write = [](State& out, std::uint32_t address, unsigned) {
            if (address >= stack - 20 && address < stack) out.wr<4>(2432, out.r<4>(2432) + 1);
        };
        m.effects.on_read = [](State& out, Effects&, std::uint32_t address, unsigned) {
            if (address >= stack - 20 && address < stack) out.wr<4>(2432, out.r<4>(2432) + 1);
        };
    });
}
void live_effects() {
    auto s = fixture_state(3);
    check(3, *s, "live field destination after first write", {}, [](Monitor& m, Imports&) {
        m.on_write = [](State& out, std::uint32_t address, unsigned) { if (address == fields) out.wr<4>(12, fields + 64); };
    });
    s = fixture_state(2); s->wr<4>(16, 0);
    check(2, *s, "live null condition after frame", {}, [](Monitor& m, Imports&) {
        m.on_write = [](State& out, std::uint32_t address, unsigned) {
            if (address == stack - 80) { out.wr<1>(2304, 4); out.wr<4>(112, fields); }
        };
    });
    s = fixture_state(5);
    check(5, *s, "live null condition after TOC read", {}, [](Monitor& m, Imports&) {
        m.effects.on_read = [](State& out, Effects&, std::uint32_t address, unsigned) {
            if (address == relocated_toc - 0x3b20) out.wr<1>(2304, 2);
        };
    });
    s = fixture_state(2); s->wr<4>(12, 0x7c558180); s->budget = s->executed + 500;
    for (unsigned n = 0; n < 26; ++n) s->memory.write<2>(cm::time_runtime_checks::months + n * 2, 0);
    auto original = clone(*s); const auto outcome = run(*original, true);
    check(2, *s, "malformed nonprogressing month table", {}, {}, outcome, false, outcome == "Native instruction budget exhausted");
}
void imported_values() {
    for (unsigned index : {4u, 6u}) {
        auto s = fixture_state(index);
        for (auto raw : {0u, 1u, 0x7c25b07fu, 0x7c25b080u, 0xffffffffu})
            check(index, *s, "clock word wrap", [raw](const State& out, const Imports& imports) {
                require(out.r<4>(12) == std::uint32_t(raw - 0x7c25b080), "Independent Mac epoch subtraction");
                require(imports.arguments.size() == 1, "One GetDateTime import");
            }, [raw](Monitor&, Imports& imports) { imports.clock = raw; });
    }
    auto s = fixture_state(4); s->wr<4>(12, 0); check(4, *s, "time null output");
    for (auto location : {std::array<std::uint32_t,3>{0,0,0}, {0,0,1}, {0,0,0x80000000},
                          {1,0,0}, {0,1,0}, {0,0,0x01000000}, {0,0,0xff000000}}) {
        s = fixture_state(7);
        const auto expected = location == std::array<std::uint32_t,3>{0,0,0} ? UINT32_MAX : unsigned((location[2] >> 24) != 0);
        check(7, *s, "daylight byte versus full location", [expected](const State& out, const Imports& imports) {
            require(out.r<4>(12) == expected && imports.arguments.size() == 1, "Independent all-zero/DST-byte result");
        }, [location](Monitor&, Imports& imports) { imports.location = location; });
    }
    s = fixture_state(5); check(5, *s, "local composition", calendar(946684800));
    s->wr<4>(12, 0); check(5, *s, "null time initializes shared fields", [](const State& out, const Imports&) {
        require(out.r<4>(12) == fields && out.memory.read<4>(fields + 12) == 1 && out.memory.read<4>(fields + 24) == 1 &&
            out.memory.read<4>(fields + 32) == 1, "Shared default fields with live daylight overwrite");
    });
    for (unsigned index : {6u, 7u}) {
        s = fixture_state(index); check(index, *s, "import fault", {}, [](Monitor&, Imports& imports) {
            imports.body = [](State&, std::string_view) { throw std::runtime_error("Controlled clock import failure"); };
        }, "Controlled clock import failure");
    }
}
void finite_budgets() {
    for (unsigned index = 0; index < 8; ++index) {
        auto s = fixture_state(index);
        if (index == 2) s->wr<4>(12, 0x7c558180); // Exercise a short complete conversion and all phase boundaries.
        if (index == 5) s->wr<4>(12, 0);
        for (unsigned available = 0; available < 200; ++available) {
            auto original = clone(*s); original->budget = original->executed + available;
            Imports imports; original->code_fragment_observer = &imports;
            const auto outcome = run(*original, true);
            original->code_fragment_observer = nullptr;
            auto bounded = clone(*s); bounded->budget = bounded->executed + available;
            check(index, *bounded, "budget=" + std::to_string(available), {}, {}, outcome, false, true);
        }
    }
}
}
int main() {
    try {
        roots_and_faults(); leap_offsets(); calendar_values(); live_effects(); imported_values(); finite_budgets();
        for (unsigned n = 0; n < 8; ++n) {
            require(comparisons[n] && negatives[n] == 1, "Individual time-runtime evidence");
            std::cout << "time_runtime.component entry=" << hex_address(time_runtime::entries[n]) << " comparisons=" << comparisons[n]
                << " fault_budget=" << faults[n] << " negatives=" << negatives[n] << '\n';
        }
        std::cout << "time_runtime.exact=" << exact_cases << " budget_stops=" << budget_stops << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
