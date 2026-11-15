// FINANCE.CPP checker cases for the board block: the owner's bail-out (0x101006a0).
//
// Same method as the gift's cases (finance_checks_board_gift.cpp). The league's standing (a record built in the
// scratch area), the owner record and its person, the table of owners and the three owner routines (answered where
// the case builds the owner from a scratch record) are what a case sets besides reputation and balance.
#include "finance_checks_board_support.hpp"
#include <limits>

namespace cm::aot::finance_checks {
using namespace board;
namespace {
constexpr std::uint32_t bailout = 0x101006a0, owner_a = 0x100c4400, owner_b = 0x100c53e0, owner_c = 0x100c4cf0;
}

FINANCE_CASES("board-bailout") {
    if (scenario.match) return;
    Pusher push(scenario, cases, bailout, 0xba11);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> owners_answered = {{owner_a, 11}, {owner_b, 12}, {owner_c, 13}};

    // The real wealth routine on a few clubs, with the club's real owner and division.
    {
        std::mt19937 rng(91);
        const auto manager = scenario.global(-0x7ab8);
        const auto accounts = scenario.r32(manager);
        for (const unsigned other : {1u, 20u, 300u, 1000u}) {
            auto memory = Memory{};
            script(scenario, memory, random_values(rng, 12));
            cases.push_back({.entry = bailout, .registers = {{3, manager}, {4, scenario.club(other)}, {5, accounts + other * 0x168}},
                             .memory = std::move(memory.writes), .label = "real club " + std::to_string(other)});
        }
    }
    // The limit's divisor: band and reputation either side of 0x109a, the league's standing (none, small, zero, negative).
    for (const int band : {-2, -1, 0, 1, 2})
        for (const std::int32_t reputation : {-100, 0, 0x109a, 0x109b, 5000, 20000})
            for (const int has_league : {0, 1})
                for (const std::int32_t power : {1, 100, 3000, 0, -50}) {
                    if (!has_league && power != 100) continue;
                    Setup s;
                    s.band = band;
                    s.reputation = reputation;
                    s.has_league = has_league;
                    s.power = power;
                    s.owner = -1;
                    push(s, "divisor band " + std::to_string(band) + " rep " + std::to_string(reputation) + " league " + std::to_string(has_league) +
                            " power " + std::to_string(power), 5);
                }
    // The owner's person: its byte at +0x36 (0, negative, positive), the owner's presence, the balance.
    for (const int owner : {-1, 0, 1})
        for (const int b36 : {0, 1, 5, 127, -3})
            for (const int band : {-2, -1, 0, 2})
                for (const std::int64_t balance : {std::int64_t(2'000'000'000), std::int64_t(3'200'000'000LL), std::int64_t(-5'000'000),
                                                   std::int64_t(-20'000'000'000LL), std::int64_t(0)})
                    for (const std::int32_t reputation : {300, 1500, 2500, 6000}) {
                        if (owner != 1 && (b36 != 5)) continue;
                        Setup s;
                        s.band = band;
                        s.owner = owner;
                        s.b36 = b36;
                        s.balance = balance;
                        s.reputation = reputation;
                        s.power = 40;
                        push(s, "owner " + std::to_string(owner) + " b36 " + std::to_string(b36) + " band " + std::to_string(band) + " balance " +
                                std::to_string(balance) + " rep " + std::to_string(reputation), 8, owners_answered);
                    }
    // The sum and the large-sum forgiveness; the owner's takeover (count of owners, big and negative) and the traits.
    for (const int owner : {-1, 1})
        for (const std::int32_t reputation : {-50, 100, 1000, 3000, 30000})
            for (const std::int64_t balance :
                 {std::int64_t(-0x60000000LL), std::int64_t(0x40000000), std::int64_t(0xE0000000LL), std::int64_t(-1'000'000'000'000LL),
                  std::int64_t(2'500'000'000LL), std::int64_t(-3), std::int64_t(1) << 40})
                for (const std::int32_t owners : {0, 1, 5, 70000, -5, 1 << 20}) {
                    if (owner == -1 && owners != 5) continue;
                    Setup s;
                    s.band = -2;
                    s.owner = owner;
                    s.b36 = 1;
                    s.reputation = reputation;
                    s.balance = balance;
                    s.power = 1;
                    s.has_league = 1;
                    s.owners = owners;
                    s.kind = owner == 1 && owners > 1 ? 99 : -1;
                    push(s, "sum owner " + std::to_string(owner) + " rep " + std::to_string(reputation) + " balance " + std::to_string(balance) +
                            " owners " + std::to_string(owners), 10, owners_answered);
                }
    // The divisor and the 15000 numerator through the pool (zero, NaN, tiny, negative), with and without a person.
    for (const double value : {0.0, -0.001, 1e-9, 9.0, nan, std::numeric_limits<double>::infinity()})
        for (const int owner : {-1, 1})
            for (const int band : {-2, 0}) {
                Setup s;
                s.band = band;
                s.owner = owner;
                s.b36 = 1;
                s.reputation = 800;
                s.balance = 2'000'000'000;
                s.power = 1;
                s.pool = {{0x338, value}, {0x180, value}, {0x60, value}, {0x200, value}, {0x238, value}, {0x70, value}, {0x38, value}};
                push(s, "pool " + std::to_string(value) + " owner " + std::to_string(owner) + " band " + std::to_string(band), 8, owners_answered);
            }
    // The divisor infinite: 15000 over it is 0, so the draw is skipped.
    for (const int owner : {-1, 1})
        for (const int band : {-2, -1, 0}) {
            Setup s;
            s.band = band;
            s.owner = owner;
            s.b36 = 1;
            s.reputation = 800;
            s.balance = 2'000'000'000;
            s.power = 1;
            const double inf = std::numeric_limits<double>::infinity();
            s.pool = {{0x118, inf}, {0x180, inf}, {0x60, inf}, {0x200, inf}, {0x300, inf}};
            push(s, "infinite divisor owner " + std::to_string(owner) + " band " + std::to_string(band), 8, owners_answered);
        }
    // The new owner record loses its person while the owner's words are computed (a callee that writes the record).
    for (const std::int32_t owners : {1, 5})
        for (const int kind : {-1, 99}) {
            Setup s;
            s.band = -2;
            s.owner = 1;
            s.b36 = 1;
            s.reputation = 1000;
            s.balance = 0x40000000;
            s.power = 1;
            s.owners = owners;
            s.kind = kind;
            for (const auto routine : {owner_a, owner_b, owner_c})
                push(s, "owner loses its person during " + std::to_string(routine) + " kind " + std::to_string(kind), 12, owners_answered, -1,
                     {{routine, {{4, owner_record + 0x1c, 0}}}});
        }
}
}
