// FINANCE.CPP checker cases for the season block: the review of one club's finances (0x100fd210).
//
// The club, its account and the finance manager are the career's own. A case writes the fields the
// review reads (balance, reputation, the club's flags) and builds, in the scratch area, the records the
// club points at (owner, competition and its record rows, favourite, manager), so that every branch runs
// on a real club record. The news routines of the unit's other blocks and the three routines outside
// (the owner check 0x1014c2e0, the record lookup 0x10459b50, the inactive test 0x100c7d00) are answered.
// Cases come in two kinds: grids that pin one stage, and a seeded random walk over the values the
// thresholds of every stage sit on.
#include "finance_checks.hpp"
#include <array>
#include <random>
#include <string>

namespace cm::aot::finance_checks {
namespace {
constexpr std::uint32_t review = 0x100fd210, inactive = 0x100c7d00, owner_check = 0x1014c2e0,
                        lookup = 0x10459b50, news_budget = 0x10102c90, news_finance = 0x10102e00, news_state = 0x10102820,
                        settle_debts = 0x100f9f40;

struct Memory {
    std::vector<Write> writes;
    void put(std::uint32_t at, std::uint32_t value, unsigned size = 4) { writes.push_back({size, at, value}); }
    void put64(std::uint32_t at, std::uint64_t value) {
        put(at, std::uint32_t(value >> 32));
        put(at + 4, std::uint32_t(value));
    }
};

// One club's situation.
struct Config {
    unsigned index = 9;
    bool owner = true, competition = true, favourite = false, manager = false, record = true, row = true, found = true;
    std::uint32_t taken = 1, kind = 1, level = 10, wants = 1, flags = 0, a165 = 0, a166 = 0;
    std::int32_t reputation = 1000;
    std::uint32_t balance_high = 0, balance_low = 20'000'000;
    std::uint32_t money = 100'000, spent = 5000, reserve = 100'000, helped = 0, rewarded = 0;
    std::uint32_t limit42 = 127, size = 100, g5 = 5, g8 = 50'000, m40 = 5, m65 = 3;
    int rival = -1;
    std::uint32_t rival_type = 1, count = 0xffffffff;
    std::uint32_t check[3] = {0, 0, 0};
    std::uint32_t check_default = 0;
    bool is_inactive = false, no_account = false, no_rival_account = false, person = true, lookup_null = false, poke_owner = false;
    int decoy = -1;
};
}

FINANCE_CASES("season-review") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ab8);
    const auto accounts = scenario.r32(manager);
    const auto clubs = scenario.global(-0x7b50);
    const auto account = [&](std::uint32_t index) { return accounts + index * 0x168; };
    const auto flag_base = scenario.r32(scenario.global(-0x79f0));
    const auto record_base = scenario.global(-0x7b5c);
    constexpr std::uint32_t owner_record = scratch_base + 0x1000, competition = scratch_base + 0x2000,
                            table_cell = scratch_base + 0x3000, row = scratch_base + 0x3100, row_record = scratch_base + 0x3200,
                            entry = scratch_base + 0x3300, favourite = scratch_base + 0x4000, favourite_person = scratch_base + 0x4100,
                            manager_record = scratch_base + 0x5000, manager_person = scratch_base + 0x5100;
    const std::uint32_t table_index = std::uint32_t(table_cell - record_base) >> 2;

    const auto push = [&](const Config& cfg, const std::string& label) {
        // A missing rival account is steered by memory (0x10103510 runs in place, so it cannot be answered): an account is
        // the manager's array plus the club's index word times 0x168, so the rival gets the index word that makes it 0.
        const auto club = scenario.club(cfg.index), acct = account(cfg.index);
        Memory memory;
        if (cfg.no_account) { memory.put(manager, 0); memory.put(club, 0); }
        if (cfg.no_rival_account) {
            // r * 0x168 = -accounts (mod 2^32): solvable when 8 divides the array's address.
            const std::uint32_t inverse = [] { std::uint32_t x = 1; for (int i = 0; i < 6; ++i) x *= 2 - 45 * x; return x; }();
            const std::uint32_t word = (0u - (accounts >> 3)) * inverse & 0x1fffffff;
            memory.put(scenario.club(unsigned(cfg.rival)), word);
        }
        memory.put(club + 0x3c, cfg.owner ? owner_record : 0);
        memory.put(club + 0x23e, cfg.taken, 1);
        memory.put(club + 0x30, cfg.competition ? competition : 0);
        memory.put(club + 0x90, cfg.favourite ? favourite : 0);
        memory.put(club + 0x80, cfg.manager ? manager_record : 0);
        memory.put(club + 0x1ec, std::uint32_t(cfg.reputation), 2);
        memory.put(club + 0x23d, cfg.kind, 1);
        memory.put(club + 0x240, cfg.level, 1);
        memory.put(club + 0x241, cfg.wants, 1);
        memory.put(flag_base + cfg.index * 0x4c + 2, cfg.flags, 1);
        memory.put(acct, cfg.balance_high);
        memory.put(acct + 4, cfg.balance_low);
        memory.put(acct + 0x165, cfg.a165, 1);
        memory.put(acct + 0x166, cfg.a166, 1);
        memory.put(owner_record, 0x1234);
        memory.put(owner_record + 4, cfg.money);
        memory.put(owner_record + 8, cfg.spent);
        memory.put(owner_record + 0xc, cfg.reserve);
        memory.put(owner_record + 0x4c, cfg.helped, 1);
        memory.put(owner_record + 0x4d, cfg.rewarded, 1);
        memory.put(competition, cfg.record ? table_index : table_index + 0x100);
        memory.put(competition + 0x18, cfg.size, 2);
        memory.put(table_cell, cfg.row ? row : 0);
        memory.put(row + 8, cfg.found ? row_record : 0);
        memory.put(row_record, 0);
        memory.put(entry + 5, cfg.g5, 1);
        memory.put(entry + 8, cfg.g8);
        memory.put(favourite + 0x1c, cfg.person ? favourite_person : 0);
        memory.put(favourite_person + 0x42, cfg.limit42, 1);
        memory.put(manager_record + 0x1c, manager_person);
        memory.put(manager_record + 0x65, cfg.m65, 1);
        memory.put(manager_person + 0x40, cfg.m40, 1);
        if (cfg.rival >= 0) {
            const auto other = scenario.club(unsigned(cfg.rival));
            memory.put(other + 0x3c, owner_record);
            memory.put(other + 0x23e, 1, 1);
            memory.put(account(unsigned(cfg.rival)) + 0x165, cfg.rival_type, 1);
        }
        if (cfg.decoy >= 0) {
            memory.put(scenario.club(unsigned(cfg.decoy)) + 0x3c, owner_record);
            memory.put(scenario.club(unsigned(cfg.decoy)) + 0x23e, 0, 1);
        }
        memory.put(scenario.toc(-0x7b50), cfg.count);
        Case item{.entry = review, .registers = {{3, manager}, {4, club}}, .memory = memory.writes, .label = label};
        item.answers = {{inactive, cfg.is_inactive ? 1u : 0u}, {owner_check, cfg.check_default}, {lookup, cfg.record && !cfg.lookup_null ? entry : 0},
                        {news_budget, 0}, {news_finance, 0}, {news_state, 0}, {settle_debts, 0}};
        for (unsigned n = 0; n < 3; ++n) item.nth_answers.push_back({owner_check, n + 1, cfg.check[n]});
        if (cfg.poke_owner) item.pokes.push_back({owner_check, {{4, club + 0x3c, 0}}});
        cases.push_back(std::move(item));
    };

    // The club the game does not run, the account that is missing (the assertion), and the club left alone.
    push({.is_inactive = true}, "inactive club");
    push({.no_account = true}, "no account");
    push({.owner = false}, "no owner");
    push({.competition = false}, "no competition, owner held");
    push({.competition = false, .taken = 0}, "no competition, owner free");
    for (const unsigned index : {1u, 40u, 257u, 1000u})
        cases.push_back({.entry = review, .registers = {{3, manager}, {4, scenario.club(index)}}, .label = "real club " + std::to_string(index),
                         .answers = {{news_budget, 0}, {news_finance, 0}, {news_state, 0}, {settle_debts, 0}}});

    // The owner's record pulled away by the owner check (a poke), the favourite without a person, a lookup that finds nothing.
    for (const std::uint32_t flags : {2u, 3u}) {
        Config cfg{.owner = true, .flags = flags, .balance_low = 1'000'000, .poke_owner = true};
        cfg.check[0] = 1;
        cfg.check_default = 1;
        push(cfg, "owner removed during the check, flags " + std::to_string(flags));
    }
    push({.favourite = true, .level = 12, .reputation = 100, .person = false}, "favourite without a person");
    for (const std::uint32_t low : {0x501bd1u, 20'000'000u})
        push({.balance_low = low, .lookup_null = true}, "lookup finds nothing, balance " + std::to_string(low));
    push({.taken = 0, .balance_low = 40'000'000, .rival = 3, .count = clubs, .decoy = 1}, "a free owner's decoy before the rival");
    push({.taken = 0, .balance_low = 40'000'000, .rival = 3, .count = clubs, .decoy = 2}, "two decoys' neighbour");

    // ---- an owner without a club of his own: the rival search ----
    for (const std::uint32_t low : {0u, 35'000'000u, 35'000'001u, 0x80000000u, 0xffffffffu, 0x7fffffffu})
        for (const std::uint32_t high : {0u, 0xffffffffu, 1u}) {
            Config cfg{.taken = 0, .balance_high = high, .balance_low = low};
            push(cfg, "free owner balance " + std::to_string(high) + ":" + std::to_string(low));
        }
    for (const int rival : {-1, 0, 3, 20, 500}) {
        for (const std::uint32_t count : {0u, 0xffffffffu, 1u, 2u, clubs})
            for (const std::uint32_t type : {0u, 1u, 2u, 3u, 0xffu})
                for (const bool missing : {false, true}) {
                    if (missing && (type != 1 || count != clubs || rival != 3)) continue;
                    Config cfg{.taken = 0, .balance_low = 40'000'000, .rival = rival, .rival_type = type, .count = count, .no_rival_account = missing};
                    push(cfg, "rival " + std::to_string(rival) + " count " + std::to_string(count) + " type " + std::to_string(type) +
                              (missing ? " no account" : ""));
                }
    }

    // ---- a club held by its owner: 0x100fd52c, stage by stage ----
    // The leaving sum: the balance's low word past 750000 and the account's byte past 10.
    for (const std::uint32_t low : {750'000u, 750'001u, 0x80000000u, 20'000'000u})
        for (const std::uint32_t byte : {0u, 10u, 11u, 100u, 127u, 128u, 0xffu}) {
            Config cfg{.level = 20, .a166 = byte, .balance_low = low};
            push(cfg, "leaving sum balance " + std::to_string(low) + " byte " + std::to_string(byte));
        }
    // The level: the favourite's chance and the three ways the level rises.
    for (const std::uint32_t level : {0u, 1u, 9u, 15u, 16u, 18u, 19u, 20u, 0x7fu, 0x80u, 0xffu})
        for (const std::int32_t reputation : {-32768, -1, 0, 1, 100, 499, 500, 3499, 3500, 5499, 5500, 8250, 32767})
            for (const std::uint32_t limit : {0x80u, 0u, 127u})
                for (const std::uint32_t low : {0x225510u, 0x225511u, 15'000'000u, 15'000'001u, 20'000'000u})
                    for (const std::uint32_t check : {0u, 1u}) {
                        if (limit != 127 && (reputation != 100 || level < 9)) continue;
                        if (low != 20'000'000 && reputation != 100) continue;
                        Config cfg{.favourite = true, .level = level, .reputation = reputation, .balance_low = low, .limit42 = limit};
                        cfg.check[0] = check;
                        push(cfg, "level " + std::to_string(level) + " reputation " + std::to_string(reputation) + " limit " +
                                  std::to_string(limit) + " balance " + std::to_string(low) + " check " + std::to_string(check));
                    }
    for (const std::uint32_t high : {0u, 0xffffffffu, 1u, 0x7fffffffu})
        for (const std::uint32_t low : {14'999'999u, 15'000'000u, 15'000'001u, 0xffffffffu}) {
            Config cfg{.favourite = true, .level = 12, .reputation = 100, .balance_high = high, .balance_low = low};
            push(cfg, "level lift balance " + std::to_string(high) + ":" + std::to_string(low));
        }
    // The flag byte: bit 1 cleared when the owner check agrees, bit 0 settled at the end.
    for (const std::uint32_t flags : {0u, 1u, 2u, 3u, 0xffu})
        for (const std::uint32_t check : {0u, 1u, 0x100u, 0x80u})
            for (const std::int32_t reputation : {-100, 0, 1000, 32767})
                for (const std::uint32_t low : {0x40000u, 250'000u - 500u * 1000u, 250'000u, 0x7fffffffu, 0x80000000u}) {
                    Config cfg{.flags = flags, .reputation = reputation, .balance_low = low};
                    cfg.check[0] = cfg.check[1] = cfg.check[2] = check;
                    push(cfg, "flags " + std::to_string(flags) + " check " + std::to_string(check) + " reputation " +
                              std::to_string(reputation) + " balance " + std::to_string(low));
                }
    // The owner's rewards: balance, reputation, the owner's money and his two bytes.
    for (const std::uint32_t low : {0x10b0760u, 0x10b0761u, 0x47868c0u, 0x47868c1u, 20'000'000u})
        for (const std::int32_t reputation : {0x1d4c, 0x1d4d, 0x2422, 0x2423, 20000})
            for (const std::uint32_t money : {0x61a8u, 0x61a9u, 0xe09cu, 0xe09du, 0x7fffffffu})
                for (const std::uint32_t helped : {0u, 1u})
                    for (const std::uint32_t rewarded : {0u, 1u})
                        for (const std::uint32_t level : {0x14u, 19u}) {
                            Config cfg{.level = level, .reputation = reputation, .balance_low = low, .money = money, .helped = helped, .rewarded = rewarded};
                            push(cfg, "rewards balance " + std::to_string(low) + " reputation " + std::to_string(reputation) + " money " +
                                      std::to_string(money) + " helped " + std::to_string(helped) + " rewarded " + std::to_string(rewarded) +
                                      " level " + std::to_string(level));
                        }
    // The squad budget: limit by reputation, the balance on both sides, the manager's person.
    for (const std::int32_t reputation : {-100, 0, 1, 62, 63, 499, 500, 3999, 4000, 4001, 20000, 32767})
        for (const std::uint32_t wants : {0u, 1u})
            for (const bool with_manager : {false, true})
                for (const std::uint32_t m40 : {0u, 21u, 127u, 0x80u})
                    for (const std::int64_t shift : {-1'000'000'000'000LL, -1LL, 0LL, 1LL, 1'000'000'000'000LL}) {
                        if (!with_manager && m40 != 0) continue;
                        const std::int64_t limits[] = {std::int64_t(reputation) * 8000, 500'000, std::int64_t(reputation) * 6000,
                                                       std::int64_t(reputation) * 3000, 500LL * 6000, 500LL * 3000};
                        for (const auto limit : limits) {
                            const std::int64_t balance = limit + shift;
                            Config cfg{.owner = false, .manager = with_manager, .wants = wants, .reputation = reputation,
                                       .balance_high = std::uint32_t(std::uint64_t(balance) >> 32), .balance_low = std::uint32_t(balance), .m40 = m40};
                            if (limit != limits[0] && shift != 0 && shift != 1 && shift != -1) continue;
                            push(cfg, "budget reputation " + std::to_string(reputation) + " wants " + std::to_string(wants) + " manager " +
                                      std::to_string(with_manager) + ":" + std::to_string(m40) + " balance " + std::to_string(balance));
                        }
                    }
    for (const std::uint32_t m65 : {0u, 1u, 10u, 127u, 0x80u})
        for (const std::uint32_t m40 : {0u, 21u, 22u, 127u, 0x80u, 0xffu})
            for (const std::int32_t reputation : {100, 600, 3999, 4000}) {
                Config cfg{.owner = false, .manager = true, .wants = 0, .reputation = reputation, .balance_low = 50'000'000, .m40 = m40, .m65 = m65};
                push(cfg, "budget manager " + std::to_string(m40) + "/" + std::to_string(m65) + " reputation " + std::to_string(reputation));
            }

    // ---- the owner's loans and help (0x100fdaec onwards): a seeded walk over the thresholds ----
    std::mt19937 random(0x5ea5);
    const auto pick = [&](const auto& values) { return values[random() % std::size(values)]; };
    const std::int32_t reputations[] = {-32768, -1, 0, 1, 100, 499, 500, 1000, 1249, 1250, 3499, 3500, 3999, 4000, 4001, 5000, 5250, 5499,
                                        5500, 7250, 8250, 0x109a, 0x109b, 0x1d4c, 0x1d4d, 0x2422, 0x2423, 10000, 20000, 32767};
    const std::uint32_t lows[] = {0, 1, 0x501bd0, 0x501bd1, 0x501bd0 - 1000, 0x501bd0 - 1001, 0x501bd0 - 32767000u, 0x1e848, 0x1e849,
                                  5'000'000, 10'000'000, 19'999'999, 20'000'000, 20'000'001, 21'000'000, 35'000'000, 0x7fffffff,
                                  0x80000000u, 0xffffffffu, 250'000, 249'999, 500'000, 0x225511, 0x10b0761, 0x47868c1, 100'000};
    const std::uint32_t moneys[] = {0, 500, 999, 1000, 1500, 3000, 5000, 29'999, 30'000, 30'001, 100'000, 125'000, 0x1e848, 0x1e849,
                                    0x61a9, 0xe09d, 999'999, 0x7fffffff, 0xffffffff};
    const std::uint32_t reserves[] = {0, 4999, 5000, 15'000, 15'002, 100'000, 1'000'000, 0x7fffffff, 0xffffffff, 20'000, 3000, 12'000};
    const std::uint32_t spents[] = {0, 100, 1000, 5000, 20'000, 90'000, 0xffffffff, 0x7fffffff};
    const std::uint32_t g8s[] = {0, 1, 99, 12'345, 50'000, 1'000'000, 0xfffff000, 0x7fffffff};
    const std::uint32_t g5s[] = {0, 1, 2, 5, 255};
    const std::uint32_t sizes[] = {0xffffu, 0, 4, 5, 6, 24, 25, 26, 100, 0x7fff, 0x8000};
    const std::uint32_t checks[] = {0, 1, 0x100, 0x80, 0xff};
    const std::uint32_t flag_values[] = {0, 1, 2, 3};
    const unsigned indexes[] = {9, 41, 257, 1000};
    for (unsigned n = 0; n < 1400; ++n) {
        Config cfg;
        cfg.index = pick(indexes);
        cfg.reputation = pick(reputations);
        cfg.balance_low = pick(lows);
        cfg.balance_high = pick(std::array<std::uint32_t, 5>{0, 0, 0, 0xffffffffu, 1});
        cfg.kind = pick(std::array<std::uint32_t, 4>{1, 1, 0, 2});
        cfg.level = pick(std::array<std::uint32_t, 7>{0, 10, 14, 19, 20, 20, 0x80});
        cfg.wants = random() % 2;
        cfg.flags = pick(flag_values);
        cfg.a166 = pick(std::array<std::uint32_t, 4>{0, 5, 11, 100});
        cfg.money = pick(moneys);
        cfg.spent = pick(spents);
        cfg.reserve = pick(reserves);
        cfg.helped = random() % 2;
        cfg.rewarded = random() % 2;
        cfg.favourite = random() % 3 != 0;
        cfg.manager = random() % 2;
        cfg.record = random() % 6 != 0;
        cfg.row = random() % 8 != 0;
        cfg.found = random() % 8 != 0;
        cfg.limit42 = pick(std::array<std::uint32_t, 4>{127, 127, 0x80, 20});
        cfg.size = pick(sizes);
        cfg.g5 = pick(g5s);
        cfg.g8 = pick(g8s);
        cfg.m40 = pick(std::array<std::uint32_t, 4>{0, 5, 21, 0x80});
        cfg.m65 = pick(std::array<std::uint32_t, 3>{0, 3, 0x80});
        cfg.check[0] = pick(checks);
        cfg.check[1] = pick(checks);
        cfg.check[2] = pick(checks);
        cfg.check_default = pick(checks);
        push(cfg, "walk " + std::to_string(n));
    }
}
}
