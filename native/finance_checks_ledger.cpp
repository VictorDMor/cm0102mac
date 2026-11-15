// FINANCE.CPP checker cases for the accounts and their ledger (finance_ledger.hpp).
//
// The finance manager is the game's own (global -0x7ab8): one 0x168-byte account per club. Clubs
// are the career's; a case changes the balance and the reputation an account's wealth reads.
#include "finance_checks.hpp"
#include <string>

namespace cm::aot::finance_checks {
namespace {
constexpr std::uint32_t construct_account = 0x100f1000, destroy_account = 0x100f1010, scale_attendance = 0x100f4f20,
                        wealth = 0x100f55b0, destroy_manager = 0x100f8720, refresh_active = 0x100fd1a0,
                        account_of_club = 0x10103510, refresh_account = 0x100f4f80;
constexpr std::uint32_t word_lines[] = {0x100f5730, 0x100f70a0, 0x100f7130, 0x100f7170, 0x100f71b0, 0x100f71f0, 0x100f7230};
constexpr std::uint32_t wide_lines[] = {0x100f7050, 0x100f70e0};
constexpr std::uint32_t free_object = 0x107735a0, destroy_array = 0x107734a0;
}

FINANCE_CASES("ledger") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ab8);
    const auto accounts = scenario.r32(manager);
    const auto clubs = scenario.global(-0x7b50);
    const auto account = [&](std::uint32_t index) { return accounts + index * 0x168; };
    const std::uint32_t sample[] = {0, 1, 2, 57, clubs / 7, clubs / 3, clubs / 2, clubs - 2, clubs - 1};

    cases.push_back({.entry = account_of_club, .registers = {{3, manager}, {4, 0}}, .label = "account of no club"});
    for (std::uint32_t index = 0; index < clubs; index += clubs / 60 + 1)
        cases.push_back({.entry = account_of_club, .registers = {{3, manager}, {4, scenario.club(index)}},
                         .label = "account of club " + std::to_string(index)});

    // Wealth: every band of the balance against the reputation, on both sides of each limit.
    const std::int32_t reputations[] = {0, 1, 250, 4999, 5000, 5001, 9000, 32767, -1, -32768};
    for (const auto index : sample) {
        cases.push_back({.entry = wealth, .registers = {{3, account(index)}}, .label = "wealth of club " + std::to_string(index)});
        for (const auto reputation : reputations) {
            const std::int64_t r = reputation;
            const std::int64_t limits[] = {r * 2500 + 5'000'000, r * 1000 + 3'500'000, -2'500'000 - r * 1500,
                                           r < 5000 ? -(r * 250) : -((r - 5000) * 1000 + 1'250'000)};
            for (const auto limit : limits)
                for (const std::int64_t balance : {limit - 1, limit, limit + 1, limit + (std::int64_t(1) << 32), limit - (std::int64_t(1) << 32)})
                    cases.push_back({.entry = wealth, .registers = {{3, account(index)}},
                                     .memory = {{4, account(index), std::uint32_t(std::uint64_t(balance) >> 32)},
                                                {4, account(index) + 4, std::uint32_t(balance)},
                                                {2, scenario.club(scenario.r32(account(index) + 8)) + 0x1ec, std::uint32_t(reputation) & 0xffff}},
                                     .label = "wealth, reputation " + std::to_string(reputation) + " balance " + std::to_string(balance)});
            if (index != sample[0]) break;  // the full grid on one club, one reputation on the others
        }
    }

    // The ledger: amounts whose sign and carry matter, on balances near a word boundary.
    const std::uint32_t amounts[] = {0, 1, 1000, 0x7fffffff, 0x80000000, 0xffffffff, 0xfffffc18};
    const std::uint32_t lows[] = {0, 5, 0xffffffff, 0x80000000};
    for (const auto entry : word_lines)
        for (const auto amount : amounts)
            for (const auto low : lows)
                cases.push_back({.entry = entry, .registers = {{3, account(sample[3])}, {4, amount}},
                                 .memory = {{4, account(sample[3]) + 4, low}},
                                 .label = "word line amount " + std::to_string(amount) + " low " + std::to_string(low)});
    for (const auto entry : wide_lines)
        for (const std::uint32_t high : {0u, 1u, 0xffffffffu, 0x7fffffffu})
            for (const auto amount : amounts)
                for (const auto low : lows)
                    cases.push_back({.entry = entry, .registers = {{3, account(sample[4])}, {4, high}, {5, amount}},
                                     .memory = {{4, account(sample[4]) + 4, low}},
                                     .label = "wide line amount " + std::to_string(high) + ":" + std::to_string(amount) + " low " + std::to_string(low)});

    for (const std::uint32_t percent : {0u, 1u, 50u, 100u, 127u, 0x80u, 0xffu, 0x9cu, 0x164u})
        for (const std::uint32_t value : {0u, 1u, 999u, 25'000u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 30'000'000u})
            cases.push_back({.entry = scale_attendance, .registers = {{3, account(sample[5])}, {4, percent}},
                             .memory = {{4, account(sample[5]) + 0x14, value}},
                             .label = "scale " + std::to_string(value) + " by " + std::to_string(percent)});

    // The array element's constructor does nothing; its destructor frees on request.
    const auto object = scratch_base + 0x200;
    cases.push_back({.entry = construct_account, .registers = {{3, object}}, .label = "construct an account"});
    for (const std::uint32_t deleting : {0u, 1u, 0xffffu, 0x10000u, 0x7fffu})
        for (const std::uint32_t target : {0u, object})
            cases.push_back({.entry = destroy_account, .registers = {{3, target}, {4, deleting}},
                             .label = "destroy account " + std::to_string(target != 0) + " deleting " + std::to_string(deleting),
                             .answers = {{free_object, 0}}});
    for (const std::uint32_t deleting : {0u, 1u, 0xffffu, 0x10000u})
        for (const std::uint32_t target : {0u, object})
            for (const std::uint32_t array : {0u, object + 0x40})
                cases.push_back({.entry = destroy_manager, .registers = {{3, target}, {4, deleting}},
                                 .memory = {{4, object, array}},
                                 .label = "destroy manager " + std::to_string(target != 0) + " array " + std::to_string(array != 0) +
                                          " deleting " + std::to_string(deleting),
                                 .answers = {{free_object, 0}, {destroy_array, 0}}});

    // The pass over the clubs: the account refresh itself belongs to another block and is answered.
    for (const std::uint32_t count : {0u, 1u, 40u, 0xffffffffu, clubs})
        cases.push_back({.entry = refresh_active, .registers = {{3, manager}},
                         .memory = {{4, scenario.toc(-0x7b50), count}},
                         .label = "refresh the active clubs, count " + std::to_string(std::int32_t(count)),
                         .answers = {{refresh_account, 0}}});
}
}
