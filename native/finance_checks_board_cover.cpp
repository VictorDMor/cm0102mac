// FINANCE.CPP checker cases for the board block: the board's cover of a shortfall (0x100ffec0).
//
// Same method as the gift's cases (finance_checks_board_gift.cpp): the career's club and account, the fields the rules
// read set by each case, the sample table scripted, the wealth band answered where the account cannot reach one.
#include "finance_checks_board_support.hpp"
#include <limits>

namespace cm::aot::finance_checks {
using namespace board;
namespace {
constexpr std::uint32_t cover = 0x100ffec0;
}

FINANCE_CASES("board-cover") {
    if (scenario.match) return;
    Pusher push(scenario, cases, cover, 0x51ce);
    const double nan = std::numeric_limits<double>::quiet_NaN();

    // The real wealth routine, on a few clubs as the game calls it.
    {
        std::mt19937 rng(77);
        const auto manager = scenario.global(-0x7ab8);
        const auto accounts = scenario.r32(manager);
        for (const unsigned other : {1u, 20u, 300u, 1000u}) {
            auto memory = Memory{};
            script(scenario, memory, random_values(rng, 12));
            cases.push_back({.entry = cover, .registers = {{3, manager}, {4, scenario.club(other)}, {5, accounts + other * 0x168}},
                             .memory = std::move(memory.writes), .label = "real club " + std::to_string(other)});
        }
    }
    // The gates: reputation, the refused flag, the three kinds of competition kept out of.
    for (const std::int32_t reputation : {0, 0x128d, 0x128e, 0x128f, 20000})
        for (const int refused : {0, 1})
            for (const int kind : {-1, 99, 0, 1, 2, 3, 4, 5})
                for (const int band : {-1, 0}) {
                    Setup s;
                    s.band = band;
                    s.reputation = reputation;
                    s.refused = refused;
                    s.kind = kind;
                    s.b2f = 20;
                    push(s, "gate rep " + std::to_string(reputation) + " refused " + std::to_string(refused) + " kind " + std::to_string(kind) +
                            " band " + std::to_string(band), 2);
                }
    // The shares by band, the two branches (with a person and without), the person's bytes.
    for (const int band : {-2, -1, 0, 1, 2})
        for (const int owner : {-1, 0, 1})
            for (const int b2f : {0, 1, 5, 127, -3})
                for (const int b40 : {0, 3, 11, 12, 50, -5}) {
                    if (owner != 1 && (b2f != 5 || b40 != 11)) continue;
                    Setup s;
                    s.band = band;
                    s.owner = owner;
                    s.b2f = b2f;
                    s.b40 = b40;
                    s.reputation = 5000;
                    s.balance = -2'000'000'000LL;
                    push(s, "share band " + std::to_string(band) + " owner " + std::to_string(owner) + " b2f " + std::to_string(b2f) + " b40 " +
                            std::to_string(b40), 6);
                }
    // The sum: the debt that replaces it, the cap by reputation, the balance on both sides of the limit.
    for (const int owner : {-1, 1})
        for (const std::int32_t reputation : {0x128e, 5000, 9000, 20000, 32767})
            for (const std::int64_t balance :
                 {std::int64_t(2'000'000'000), std::int64_t(0), std::int64_t(-1), std::int64_t(-20'000'000), std::int64_t(-30'000'000),
                  std::int64_t(-1'000'000'000), std::int64_t(-1'000'000'000'000LL), std::int64_t(-3'000'000'000LL), std::int64_t(1) << 40})
                for (const int band : {-2, -1, 1}) {
                    Setup s;
                    s.band = band;
                    s.owner = owner;
                    s.reputation = reputation;
                    s.balance = balance;
                    s.b2f = 100;
                    s.b40 = 40;
                    push(s, "sum owner " + std::to_string(owner) + " rep " + std::to_string(reputation) + " balance " + std::to_string(balance) +
                            " band " + std::to_string(band), 6);
                }
    // The draws' limits: the divisors and numerators of the pool, zero, NaN, tiny and negative.
    for (const double divisor : {0.0, -0.1, 1e-9, 7.0, 1000.0, nan})
        for (const int owner : {-1, 1})
            for (const int band : {-2, -1, 0}) {
                Setup s;
                s.band = band;
                s.owner = owner;
                s.b2f = 120;
                s.reputation = 5000;
                s.pool = {{0x18, divisor}, {0x60, divisor}, {0xe0, divisor}, {0x180, divisor}};
                push(s, "divisor " + std::to_string(divisor) + " owner " + std::to_string(owner) + " band " + std::to_string(band), 8);
            }
    for (const double numerator : {0.0, 1.0, -4.0, 1e12, nan, 1e-300})
        for (const int owner : {-1, 1}) {
            Setup s;
            s.band = -1;
            s.owner = owner;
            s.b2f = 120;
            s.reputation = 5000;
            s.pool = {{0x230, numerator}, {0x330, numerator}};
            push(s, "numerator " + std::to_string(numerator) + " owner " + std::to_string(owner), 8);
        }
    // A club that is no club: both sides fault at the first read of its reputation (0x100ffef8 reads +0x1ec).
    {
        std::mt19937 rng(3);
        auto memory = Memory{};
        script(scenario, memory, random_values(rng, 12));
        const auto manager = scenario.global(-0x7ab8);
        cases.push_back({.entry = cover, .registers = {{3, manager}, {4, 0}, {5, scenario.r32(manager) + 9 * 0x168}},
                         .memory = std::move(memory.writes), .label = "no club"});
    }
}
}
