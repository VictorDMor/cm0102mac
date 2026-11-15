// FINANCE.CPP checker cases for the season block: the donation to a club in need (0x100fe640).
//
// The club and its account are the career's own; a case changes the balance and reputation the rules read
// and, for the owner record (club +0x80), builds one in the scratch area. The wealth band is answered where
// a case needs a band the account cannot reach (-3 and 3 never come out of 0x100f55b0). The doubles of the
// pool (the numerator at +0x308 and the divisors at +0x2a0, +0x300) are written to steer the draw's limit,
// including the limits past 0xffff the real pool never produces.
#include "finance_checks.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <string>

namespace cm::aot::finance_checks {
namespace {
constexpr std::uint32_t donation = 0x100fe640, wealth = 0x100f55b0, announce = 0x10101ff0;

struct Memory {
    std::vector<Write> writes;
    void put(std::uint32_t at, std::uint32_t value, unsigned size = 4) { writes.push_back({size, at, value}); }
    void put64(std::uint32_t at, std::uint64_t value) {
        put(at, std::uint32_t(value >> 32));
        put(at + 4, std::uint32_t(value));
    }
    void put_double(std::uint32_t at, double value) { put64(at, std::bit_cast<std::uint64_t>(value)); }
};
}

FINANCE_CASES("season-donation") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ab8);
    const auto accounts = scenario.r32(manager);
    const auto account = [&](std::uint32_t index) { return accounts + index * 0x168; };
    const auto pool = scenario.toc(-0x55a0);
    const auto owner_object = scratch_base + 0x400, person = scratch_base + 0x500;
    constexpr std::uint32_t index = 9;  // the club whose record the cases change
    const auto club = scenario.club(index), acct = account(index);

    // One case: the band the wealth routine answers (none: the real one runs), the reputation, the balance,
    // the owner (-2: the real one; -1: none; 0: a record without a person; 1: with the person's byte) and the pool.
    struct Setup {
        int band = 99;
        std::int32_t reputation = -100;
        std::int64_t balance = std::int64_t(-1) << 40;
        int owner = -1;
        std::int32_t frequency = 5;
        double numerator = 6.0, short_divisor = 2.0, sound_divisor = 0.4;
    };
    const auto push = [&](const Setup& setup, const std::string& label) {
        Memory memory;
        memory.put(club + 0x1ec, std::uint32_t(setup.reputation), 2);
        memory.put64(acct, std::uint64_t(setup.balance));
        if (setup.owner != -2) {
            memory.put(club + 0x80, setup.owner < 0 ? 0 : owner_object);
            memory.put(owner_object + 0x1c, setup.owner == 0 ? 0 : person);
            memory.put(person + 0x40, std::uint32_t(setup.frequency), 1);
        }
        memory.put_double(pool + 0x308, setup.numerator);
        memory.put_double(pool + 0x2a0, setup.short_divisor);
        memory.put_double(pool + 0x300, setup.sound_divisor);
        Case item{.entry = donation, .registers = {{3, manager}, {4, club}, {5, acct}}, .memory = memory.writes, .label = label};
        if (setup.band != 99) item.answers = {{wealth, std::uint32_t(setup.band)}};
        item.answers.push_back({announce, 0});
        cases.push_back(std::move(item));
    };

    // The real wealth routine, on a few clubs, as the game calls it.
    for (const std::uint32_t other : {1u, 20u, 300u, 1000u})
        cases.push_back({.entry = donation, .registers = {{3, manager}, {4, scenario.club(other)}, {5, account(other)}},
                         .label = "real club " + std::to_string(other), .answers = {{announce, 0}}});

    // Every band the wealth routine can return (-3 and 3 never come out of it: 0x100fe694 is dead) against the reputations around the thresholds.
    for (int band = -2; band <= 2; ++band)
        for (const std::int32_t reputation : {-32768, -1, 0, 1, 1249, 1250, 3499, 3500, 3501, 5000, 32767}) {
            Setup setup{.band = band, .reputation = reputation};
            push(setup, "band " + std::to_string(band) + " reputation " + std::to_string(reputation));
        }
    // The sound club's own limit: reputation 25 times against the balance, 64-bit signed.
    for (const std::int32_t reputation : {-100, -1, 0, 1, 100, 3500})
        for (const std::int64_t shift : {-5'000'000'000LL, -2LL, -1LL, 0LL, 1LL, 2LL, 5'000'000'000LL})
            for (const std::int64_t jitter : {0LL, 1LL << 32, -(1LL << 32)}) {
                Setup setup{.band = 0, .reputation = reputation, .balance = std::int64_t(reputation) * 25 + shift + jitter, .owner = -1};
                push(setup, "band 0 reputation " + std::to_string(reputation) + " balance " + std::to_string(setup.balance));
            }
    // The first chance: the reputation against a draw below 7000 above 3500.
    for (std::int32_t reputation = 3400; reputation <= 10600; reputation += 300) {
        Setup setup{.band = -2, .reputation = reputation, .owner = -1, .numerator = 0.0};
        push(setup, "first chance reputation " + std::to_string(reputation));
    }
    // The sums: the reputation's three sizes against the balance around each limit.
    for (const std::int32_t reputation : {-1000, -1, 0, 1, 1000, 1249, 1250, 2000, 3499})
        for (const std::int64_t base : {-450LL * reputation, -300LL * reputation})
            for (const std::int64_t shift : {-1'000'000'000'000LL, -1LL, 0LL, 1LL, 1'000'000'000'000LL}) {
                Setup setup{.band = -2, .reputation = reputation, .balance = base + shift, .owner = -1, .numerator = 0.0};
                push(setup, "sum reputation " + std::to_string(reputation) + " balance " + std::to_string(setup.balance));
                setup.owner = 1;
                setup.frequency = 127;
                push(setup, "owner sum reputation " + std::to_string(reputation) + " balance " + std::to_string(setup.balance));
            }
    // The draws: the numerator over the divisor as the limit, for the club without and with an owner's person.
    const double nan = std::numeric_limits<double>::quiet_NaN(), inf = std::numeric_limits<double>::infinity();
    const double numerators[] = {0.0, -0.0, 0.5, 1.0, 2.0, 3.0, 6.0, 4.0, -2.0, -6.0, 131070.0, 131072.0, 262140.0,
                                 -131070.0, -131072.0, 400000.0, -400000.0, 1e12, -1e12, 1e300, nan, inf, -inf};
    for (const double numerator : numerators)
        for (const int owner : {-1, 0, 1})
            for (const std::int32_t frequency : {0, 1, -1, 7, -7, 127, -128})
                for (const int band : {-2, -1}) {
                    if (owner != 1 && frequency != 5 && frequency != 0) continue;
                    Setup setup{.band = band, .reputation = -300, .balance = std::int64_t(-1) << 40, .owner = owner, .frequency = frequency,
                                .numerator = numerator};
                    push(setup, "draw numerator " + std::to_string(numerator) + " owner " + std::to_string(owner) + " frequency " +
                                std::to_string(frequency) + " band " + std::to_string(band));
                }
    // The divisors: band -1 above and below the reputation 0xdac, zero and negative divisors.
    for (const double divisor : {0.0, -0.4, 0.4, 1e-300, 6.0, 0.001, nan})
        for (const std::int32_t reputation : {-50, 0xdab, 0xdac, 0xdad})
            for (const int band : {-2, -1}) {
                Setup setup{.band = band, .reputation = reputation, .balance = std::int64_t(-1) << 40, .owner = -1, .short_divisor = divisor,
                            .sound_divisor = divisor};
                push(setup, "divisor " + std::to_string(divisor) + " reputation " + std::to_string(reputation) + " band " + std::to_string(band));
            }
}
}
