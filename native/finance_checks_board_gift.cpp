// FINANCE.CPP checker cases for the board block: the benefactor's gift (0x100fefa0).
//
// The club and its account are the career's own; each case sets the fields the rules read (reputation, balance,
// the division's kind, the owner and its person, flags) and steers the sample table so that every draw's value is
// chosen. The wealth band is answered where a case needs one the account cannot reach; the finance news and the
// account refresh run as the originals (the refresh's second word decides which news kind follows).
#include "finance_checks_board_support.hpp"
#include <limits>

namespace cm::aot::finance_checks {
using namespace board;
namespace {
constexpr std::uint32_t gift = 0x100fefa0;
}

FINANCE_CASES("board-gift") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ab8);
    const auto accounts = scenario.r32(manager);
    std::mt19937 rng(0x6b1f7);

    Pusher pusher(scenario, cases, gift, 0x6b1f8);
    const auto push = [&](const Setup& setup, const std::string& label, unsigned scripts = 10) { pusher(setup, label, scripts); };

    // The real wealth routine, on a few clubs as the game calls it.
    for (const unsigned other : {1u, 20u, 300u, 1000u}) {
        auto memory = Memory{};
        script(scenario, memory, random_values(rng, 12));
        cases.push_back({.entry = gift, .registers = {{3, manager}, {4, scenario.club(other)}, {5, accounts + other * 0x168}},
                         .memory = std::move(memory.writes), .label = "real club " + std::to_string(other)});
    }

    // A club the board keeps out of: each of the seven kinds of competition, a national side, and not.
    for (int kind = -1; kind <= 99; kind = kind == 7 ? 99 : kind + 1)
        for (const int national : {0, 1, 2, 3})
            for (const int band : {-1, 0, 2}) {
                Setup s;
                s.kind = kind;
                s.national = national;
                s.band = band;
                s.reputation = 2000;
                push(s, "kind " + std::to_string(kind) + " national " + std::to_string(national) + " band " + std::to_string(band), 2);
            }

    // The divisor by band and reputation.
    for (const int band : {-2, -1, 0, 1, 2})
        for (const std::int32_t reputation : {-100, 0xcb2, 0xcb3, 0xea6, 0xea7, 0x109a, 0x109b, 0x1482, 0x1483, 0x1869, 0x186a, 0x2800})
            for (const int owner : {-1, 0, 1})
                for (const int bytes : {0, 1, 2, 3}) {
                    if (owner != 1 && bytes != 0) continue;
                    Setup s;
                    s.band = band;
                    s.reputation = reputation;
                    s.owner = owner;
                    s.owner_byte = (bytes & 1) ? 0x10 : 0x0f;
                    s.b40 = (bytes & 2) ? 0x10 : 0x0f;
                    s.b2f = 3;
                    push(s, "divisor band " + std::to_string(band) + " rep " + std::to_string(reputation) + " owner " + std::to_string(owner) +
                            " bytes " + std::to_string(bytes), 3);
                }

    // The owner's person: its byte at +0x2f (0, negative, positive), the limit's divisor (pool overrides), the debt.
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (const int b2f : {0, 1, 5, -3, 127})
        for (const int b40 : {0, 1, 4, 5, 6, 20, -7})
            for (const std::int64_t balance : {std::int64_t(-5'000'000), std::int64_t(2'000'000'000), std::int64_t(3'200'000'000),
                                               std::int64_t(1) << 40, -(std::int64_t(1) << 40), std::int64_t(0), std::int64_t(7)})
                for (const std::int32_t reputation : {200, 1500, 1999, 2000, 4800, 5000, 9000}) {
                    Setup s;
                    s.band = 1;
                    s.reputation = reputation;
                    s.balance = balance;
                    s.owner = 1;
                    s.b2f = b2f;
                    s.b40 = b40;
                    push(s, "person b2f " + std::to_string(b2f) + " b40 " + std::to_string(b40) + " balance " + std::to_string(balance) + " rep " +
                            std::to_string(reputation), 3);
                }
    // Pool overrides: the limits' divisor and the thirty, zero, NaN, tiny and negative.
    for (const double divisor : {0.0, -0.001, 1e-9, 5.0, 100.0, nan, std::numeric_limits<double>::infinity()})
        for (const int owner : {-1, 1})
            for (const int band : {0, 1, -2}) {
                Setup s;
                s.band = band;
                s.owner = owner;
                s.b2f = 20;
                s.reputation = 3000;
                s.pool = {{0x180, divisor}, {0x318, divisor}, {0x310, divisor}, {0x328, divisor}, {0xe0, divisor}};
                push(s, "divisor " + std::to_string(divisor) + " owner " + std::to_string(owner) + " band " + std::to_string(band), 6);
            }
    // The news kind: flags, reputation and the kind of competition, for the club with and without a person.
    for (const int owner : {-1, 1})
        for (const int refused : {0, 1})
            for (const int kind : {-1, 99, 5, 7})
                for (const std::int32_t reputation : {1500, 0x128e, 0x128f, 8000})
                    for (const std::int64_t balance : {std::int64_t(2'000'000'000), std::int64_t(3'200'000'000), std::int64_t(-300'000),
                                                       std::int64_t(1) << 33, std::int64_t(-50'000'000), std::int64_t(-600'000'000),
                                                       std::int64_t(-3'000'000'000)}) {
                        Setup s;
                        s.band = 1;
                        s.owner = owner;
                        s.refused = refused;
                        s.kind = kind;
                        s.reputation = reputation;
                        s.balance = balance;
                        s.b2f = 20;
                        s.b40 = 12;
                        push(s, "news owner " + std::to_string(owner) + " refused " + std::to_string(refused) + " kind " + std::to_string(kind) +
                                " rep " + std::to_string(reputation) + " balance " + std::to_string(balance), 8);
                    }
    // A club that is no club: both sides fault at the first read of its reputation (0x100fefa0 reads +0x1ec at 0x100ff0a8).
    {
        auto memory = Memory{};
        script(scenario, memory, random_values(rng, 12));
        cases.push_back({.entry = gift, .registers = {{3, manager}, {4, 0}, {5, accounts + 9 * 0x168}}, .memory = std::move(memory.writes),
                         .label = "no club"});
    }
    // The person's gift with the account refresh not above 0 and the club flag set (news 5/6/7/1 after a poor refresh).
    for (const int refused : {0, 1})
        for (const std::int32_t reputation : {500, 1500, 1999})
            for (const int b40 : {20, 60, 127})
                for (const std::int64_t balance : {std::int64_t(-8'000'000), std::int64_t(-15'000'000), std::int64_t(-30'000'000), std::int64_t(-2'000'000)}) {
                    Setup s;
                    s.band = 1;
                    s.owner = 1;
                    s.refused = refused;
                    s.reputation = reputation;
                    s.balance = balance;
                    s.b2f = 120;
                    s.b40 = b40;
                    pusher(s, "poor refresh refused " + std::to_string(refused) + " rep " + std::to_string(reputation) + " b40 " + std::to_string(b40) +
                              " balance " + std::to_string(balance), 12);
                }
}
}
