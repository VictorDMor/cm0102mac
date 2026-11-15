// FINANCE.CPP checker cases for the value block (finance_value.hpp): the club's budget (0x100f2680), the
// budget for one spending kind (0x100f4790) and the account refresh (0x100f4f80).
//
// The accounts and clubs are the career's own. Every record a routine follows from a club (its league,
// the two staff records of its league, the owner, the manager and their profiles) and the deal being
// weighed are built in the scratch area and pointed to by memory writes the checker undoes after the
// case. The routines of other modules and of other blocks (the wealth band, the club's rule check, the
// deal price, the contract finders) are answered, their results chosen per case, so that every rung of
// the ladders is reached; the balance conversion (0x10773c80) runs for real.
#include "finance_checks.hpp"
#include <cstdint>
#include <initializer_list>
#include <string>

namespace cm::aot::finance_checks {
namespace {
constexpr std::uint32_t club_budget = 0x100f2680, budget_for_kind = 0x100f4790, refresh_account = 0x100f4f80;
constexpr std::uint32_t inactive = 0x100c7d00, band_of = 0x100f55b0, club_rule = 0x1014c2e0, owner_trusts = 0x100ca800,
                        deal_price = 0x10084b60, days_between = 0x10718130, contract_of = 0x10082f90,
                        contract_by_index = 0x10082f20, loan_record = 0x10083110, deal_for_owner = 0x100d9520,
                        deal_for_club = 0x100d9480, signing_value = 0x1037b6f0;

struct Rng {
    std::uint64_t state;
    std::uint32_t next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return std::uint32_t(state >> 33);
    }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    bool chance(unsigned percent) { return below(100) < percent; }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + below(unsigned(values.size()))); }
};

// The competition cells the routines compare a league's identifier with (every one of the three).
constexpr std::int32_t id_slots[] = {-0x757c, -0x7574, -0x7578, -0x7650, -0x7a24, -0x7514, -0x7590, -0x75a8, -0x7504,
                                     -0x7a20, -0x79fc, -0x750c, -0x7594, -0x7a1c, -0x7af4, -0x7a00, -0x7a18, -0x7a14,
                                     -0x7a10, -0x7a28, -0x7b68, -0x7508, -0x7510};
constexpr unsigned id_count = sizeof id_slots / sizeof id_slots[0];
constexpr std::uint32_t area = scratch_base + 0x10000;
constexpr std::uint32_t league_at = area + 0x100, first_at = area + 0x200, second_at = area + 0x300, owner_at = area + 0x400,
                        profile_at = area + 0x500, manager_at = area + 0x600, manager_profile_at = area + 0x700,
                        deal_at = area + 0x800, person_at = area + 0x900, contract_at = area + 0xa00,
                        loan_at = area + 0xb00, post_at = area + 0xc00, rows_at = area + 0x1000;

struct Plan {
    std::uint32_t entry = club_budget;
    std::uint32_t club_index = 0;
    std::uint32_t reputation = 5000, type = 0, flag = 0;
    bool league = true, first = true, second = true, owner = true, owner_profile = true, manager = true,
         manager_profile = true, deal = false, person = true, director = true;
    std::uint32_t id = 0;
    std::uint8_t level_byte = 1, strength = 30;
    std::int32_t first_power = 40, second_power = 60;
    std::uint32_t balance_high = 0, balance_low = 1'000'000, total = 5000, largest = 3000, budget = 4000;
    // The deal.
    std::uint32_t buyer = 1, seller_is_club = 1, deal_kind = 5, deal_flag = 0;
    std::int32_t deal_level = 10;
    std::int32_t person_rating_a = 4000, person_rating_b = 60, person_rating_c = 3000;
    // Callee answers.
    std::uint32_t inactive_answer = 0, band = 0, rule = 0, trusts = 0, deal_for_owner_answer = 0, deal_for_club_answer = 0,
                  days = 100, price_answer = 2000, contract = 1;
    std::uint32_t rule_second = 0, rule_third = 0, rule_fourth = 0;
    std::uint32_t kind = 5;
    std::uint32_t owner_strength = 12, manager_strength = 12, owner_level = 3, profile_41 = 12, manager_value = 7000;
    std::uint32_t contract_byte = 0x2a;
    std::vector<Case::Poke> pokes;
    std::string tag;
};

class Maker {
public:
    Maker(const Scenario& scenario, std::vector<Case>& cases) : scenario(scenario), cases(cases) {}
    void add(const Plan& p) {
        Case item;
        item.entry = p.entry;
        const auto put4 = [&](std::uint32_t at, std::uint32_t v) { item.memory.push_back({4, at, v}); };
        const auto put2 = [&](std::uint32_t at, std::uint32_t v) { item.memory.push_back({2, at, v}); };
        const auto put1 = [&](std::uint32_t at, std::uint32_t v) { item.memory.push_back({1, at, v}); };
        const auto club = scenario.club(p.club_index);
        const auto account = scenario.r32(scenario.global(-0x7ab8)) + p.club_index * 0x168;
        put4(account, p.balance_high);
        put4(account + 4, p.balance_low);
        put4(account + 0xc, p.total);
        put4(account + 0x10, p.largest);
        put4(account + 0x14, p.budget);
        put2(club + 0x1ec, p.reputation);
        put1(club + 0x23d, p.type);
        put1(club + 0x241, p.flag);
        put4(club + 0x2c, p.league ? league_at : 0);
        put4(league_at + 0x68, p.id);
        put1(league_at + 0x11c, p.level_byte);
        put1(league_at + 0x11f, p.strength);
        put4(club + 0x30, p.first ? first_at : 0);
        put2(first_at + 0x18, std::uint32_t(p.first_power));
        put4(club + 0x34, p.second ? second_at : 0);
        put2(second_at + 0x18, std::uint32_t(p.second_power));
        put4(club + 0x80, p.owner ? owner_at : 0);
        put4(owner_at + 0x1c, p.owner_profile ? profile_at : 0);
        put1(owner_at + 0x63, p.owner_strength);
        put1(profile_at + 0x2f, p.owner_level);
        put1(profile_at + 0x40, p.profile_41);
        put4(club + 0x90, p.manager ? manager_at : 0);
        put1(manager_at + 0x63, p.manager_strength);
        put4(manager_at + 0x1c, p.manager_profile ? manager_profile_at : 0);
        put2(manager_profile_at + 0x2a, p.manager_value);
        if (!p.director) put4(scenario.toc(-0x7b40), 0);
        const auto outside = p.deal ? deal_at : 0;
        if (p.deal) {
            put4(deal_at + 0x14, p.person ? person_at : 0);
            put4(deal_at + 0x1c, p.buyer);
            put4(deal_at + 0x24, p.seller_is_club ? club : club + 0x248);
            put2(deal_at + 0x5c, std::uint32_t(p.deal_level));
            put1(deal_at + 0x5e, p.deal_flag);
            put1(deal_at + 0x62, p.deal_kind);
            put2(person_at + 4, std::uint32_t(p.person_rating_b));
            put2(person_at + 0xa, std::uint32_t(p.person_rating_a));
            put2(person_at + 0xc, std::uint32_t(p.person_rating_c));
        }
        put1(contract_at + 0x60, p.contract_byte);
        item.registers = {{3, account}, {4, p.entry == club_budget ? outside : p.kind}, {5, p.kind}};
        item.answers = {{inactive, p.inactive_answer}, {band_of, p.band}, {club_rule, p.rule}, {owner_trusts, p.trusts},
                        {deal_for_owner, p.deal_for_owner_answer}, {deal_for_club, p.deal_for_club_answer},
                        {days_between, p.days}, {deal_price, p.price_answer}, {contract_of, p.contract ? contract_at : 0}};
        item.nth_answers = {{club_rule, 2, p.rule_second}, {club_rule, 3, p.rule_third}, {club_rule, 4, p.rule_fourth}};
        item.pokes = p.pokes;
        item.label = p.tag;
        cases.push_back(std::move(item));
    }
    std::uint32_t id_value(unsigned n) const { return n < id_count ? scenario.global(id_slots[n]) : 0x7777u; }

private:
    const Scenario& scenario;
    std::vector<Case>& cases;
};
}

FINANCE_CASES("value") {
    if (scenario.match) return;
    const auto clubs = scenario.global(-0x7b50);
    Maker maker(scenario, cases);
    Rng rng{0x7a1e};
    const std::uint32_t club_indices[] = {2, 11, 37, 100, clubs / 3, clubs / 2, clubs - 2};
    const std::uint32_t reputations[] = {0, 1, 100, 1500, 2250, 2300, 3000, 4000, 4200, 4780, 4800, 5000, 5300, 5430, 5750,
                                         6000, 6250, 6251, 6500, 6800, 7000, 7750, 7751, 7800, 8000, 9000, 9500, 10000,
                                         32767, 0xffff, 0xfffe};
    const auto random_plan = [&](std::uint32_t entry) {
        Plan p;
        p.entry = entry;
        p.club_index = rng.pick({2u, 11u, 37u, 100u, clubs / 3, clubs / 2, clubs - 2});
        p.reputation = rng.chance(85) ? rng.pick({0u, 1u, 100u, 1500u, 2250u, 2300u, 3000u, 4000u, 4200u, 4780u, 4800u, 5000u, 5300u,
                                                  5430u, 5750u, 6000u, 6250u, 6251u, 6500u, 6800u, 7000u, 7750u, 7751u, 7800u, 8000u,
                                                  9000u, 9500u, 10000u})
                                      : rng.below(0x10000);
        p.type = rng.chance(10) ? rng.pick({0u, 3u}) : rng.pick({0u, 1u, 2u});
        p.flag = rng.chance(50);
        p.league = rng.chance(92);
        p.id = maker.id_value(rng.below(id_count + 1));
        p.level_byte = std::uint8_t(rng.chance(70) ? rng.pick({1, 1, 2, 3, 0}) : rng.below(256));
        p.strength = std::uint8_t(rng.chance(80) ? rng.below(60) : rng.below(256));
        p.first = rng.chance(85);
        p.second = rng.chance(70);
        p.first_power = std::int32_t(rng.below(120)) - 10;
        p.second_power = std::int32_t(rng.below(120)) - 10;
        p.owner = rng.chance(88);
        p.owner_profile = rng.chance(88);
        p.manager = rng.chance(88);
        p.manager_profile = rng.chance(88);
        p.director = rng.chance(60);
        p.owner_strength = rng.chance(50) ? rng.pick({0u, 10u, 15u, 16u, 30u}) : rng.below(256);
        p.manager_strength = rng.chance(50) ? rng.pick({0u, 10u, 15u, 16u, 30u}) : rng.below(256);
        p.owner_level = rng.chance(70) ? rng.pick({0u, 3u, 5u, 6u, 10u, 15u, 16u, 20u}) : rng.below(256);
        p.profile_41 = rng.chance(60) ? rng.pick({10u, 15u, 16u, 20u}) : rng.below(256);
        p.manager_value = rng.chance(50) ? rng.pick({7000u, 7250u, 7251u, 5000u}) : rng.below(0x10000);
        p.balance_high = rng.chance(60) ? 0 : rng.pick({0xffffffffu, 1u, 0x80000000u, 0x7fffffffu});
        p.balance_low = rng.chance(60) ? rng.below(60'000'000) : rng.next();
        p.total = rng.next() % 3'000'000;
        p.largest = rng.next() % 500'000;
        p.budget = rng.chance(80) ? rng.below(300'000) : rng.next();
        p.inactive_answer = rng.chance(10) ? 1 : 0;
        p.band = std::uint32_t(rng.pick({-2, -1, 0, 1, 2, 2, 1, 0, 3, -3}));
        p.rule = rng.chance(50);
        p.rule_second = rng.chance(50);
        p.rule_third = rng.chance(50);
        p.rule_fourth = rng.chance(50);
        p.trusts = rng.chance(50);
        p.deal_for_owner_answer = rng.chance(50);
        p.deal_for_club_answer = rng.chance(50);
        p.days = rng.chance(60) ? rng.below(200) : 200 + rng.below(50);
        p.price_answer = rng.chance(70) ? rng.below(60000) : rng.next();
        p.contract = rng.chance(80);
        p.kind = rng.chance(50) ? rng.pick({0xffu, 0xffffffffu}) : rng.below(14);
        p.deal = false;
        return p;
    };
    const auto with_deal = [&](Plan p) {
        p.deal = true;
        p.person = rng.chance(95);
        p.buyer = rng.chance(85);
        p.seller_is_club = rng.chance(60);
        p.deal_kind = rng.pick({0u, 1u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 0xffu});
        p.deal_flag = rng.chance(50);
        p.deal_level = std::int32_t(rng.chance(80) ? rng.below(0x40) : rng.below(0x10000)) - 4;
        p.person_rating_a = std::int32_t(rng.pick({0u, 2000u, 3750u, 3751u, 5250u, 5251u, 6750u, 6751u, 7250u, 7251u, 7750u, 7751u,
                                                   0x1ec0u, 0x1c52u, 0x1e46u, 0x1e47u, 0x1a5eu, 0x1a5fu, 0x109au, 0x109bu, 0x186au, 0x186bu}));
        p.person_rating_b = std::int32_t(rng.pick({0u, 50u, 59u, 60u, 99u, 100u, 119u, 120u, 139u, 140u, 200u}));
        p.person_rating_c = std::int32_t(rng.pick({0u, 3750u, 3751u, 5250u, 5251u, 6750u, 6751u, 0x1a5eu, 0x1a5fu, 0x1482u, 0x1483u}));
        return p;
    };

    // Every competition identifier of the unit's lists, for the club asking for itself and for a deal,
    // against every wealth band and several reputations.
    for (unsigned n = 0; n <= id_count; ++n)
        for (const std::int32_t band : {-2, -1, 0, 1, 2})
            for (const std::uint32_t reputation : {1000u, 4500u, 6300u, 7800u, 9000u}) {
                Plan p = random_plan(club_budget);
                p.id = maker.id_value(n);
                p.band = std::uint32_t(band);
                p.reputation = reputation;
                p.type = 0;
                p.level_byte = 1;
                p.tag = "budget id " + std::to_string(n) + " band " + std::to_string(band) + " reputation " + std::to_string(reputation);
                maker.add(p);
                p.entry = budget_for_kind;
                p.tag = "kind budget id " + std::to_string(n) + " band " + std::to_string(band) + " reputation " + std::to_string(reputation);
                p.kind = unsigned(n) % 14;
                maker.add(p);
                p.entry = club_budget;
                p = with_deal(p);
                p.tag = "deal budget id " + std::to_string(n) + " band " + std::to_string(band) + " reputation " + std::to_string(reputation);
                maker.add(p);
            }

    // The club's own ratio and the staff figures: levels, strengths, the two staff records.
    for (const std::uint8_t level : {0, 1, 2, 3, 4, 200})
        for (const std::uint8_t strength : {0, 1, 5, 9, 10, 11, 20, 30, 60, 127, 128, 255})
            for (const unsigned n : {14u, 5u, 12u, 9u, 20u, 0u, 17u}) {
                Plan p = random_plan(club_budget);
                p.id = maker.id_value(n);
                p.level_byte = level;
                p.strength = strength;
                p.type = 0;
                p.tag = "ratio level " + std::to_string(level) + " strength " + std::to_string(strength) + " id " + std::to_string(n);
                maker.add(p);
                p.entry = budget_for_kind;
                maker.add(p);
            }
    for (const std::int32_t low : {-10, 0, 1, 10, 50, 100})
        for (const std::int32_t high : {-5, 0, 1, 10, 51, 100})
            for (const bool second : {false, true})
                for (const bool first : {false, true}) {
                    Plan p = random_plan(club_budget);
                    p.first_power = low;
                    p.second_power = high;
                    p.first = first;
                    p.second = second;
                    p.type = 0;
                    p.band = 0;
                    p.tag = "staff " + std::to_string(low) + " " + std::to_string(high);
                    maker.add(p);
                    p.reputation = 7000;
                    p.id = maker.id_value(18);
                    maker.add(p);
                }

    // The ladders of the deal, each kind and each person rating.
    for (const std::uint32_t deal_kind : {0u, 1u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 0xffu})
        for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 0xffu, 0xffffffffu})
            for (const std::int32_t rating : {0, 3000, 3750, 3751, 5250, 5251, 6750, 6751, 7250, 7251, 7750, 7751, 8000})
                for (const bool close : {false, true}) {
                    Plan p = with_deal(random_plan(club_budget));
                    p.deal_kind = deal_kind;
                    p.kind = kind;
                    p.person_rating_a = rating;
                    p.person_rating_c = rating;
                    p.days = close ? 150 : 250;
                    p.type = 0;
                    p.id = maker.id_value(close ? 18u : 16u);
                    p.tag = "deal kind " + std::to_string(deal_kind) + " kind " + std::to_string(kind) + " rating " + std::to_string(rating);
                    maker.add(p);
                }

    // Random plans of every shape, for the three routines.
    for (unsigned n = 0; n < 2500; ++n) maker.add(random_plan(club_budget));
    for (unsigned n = 0; n < 2500; ++n) maker.add(with_deal(random_plan(club_budget)));
    for (unsigned n = 0; n < 1200; ++n) maker.add(random_plan(budget_for_kind));
    // The career's own accounts, unchanged, for the three routines.
    for (const auto index : club_indices)
        for (const auto entry : {club_budget, budget_for_kind, refresh_account}) {
            const auto account = scenario.r32(scenario.global(-0x7ab8)) + index * 0x168;
            cases.push_back({.entry = entry, .registers = {{3, account}, {4, 0}, {5, 0xffffffffu}},
                             .label = "career account " + std::to_string(index)});
        }
    (void)reputations;
}

// Coverage pass: the other club's standing rules, the negotiator's kind, the kind-2 second rule check and
// the wealth bands below -2 of the kind budget.
FINANCE_CASES("value coverage") {
    if (scenario.match) return;
    Maker maker(scenario, cases);
    const auto base = [&](std::uint32_t entry) {
        Plan p;
        p.entry = entry;
        p.club_index = 37;
        p.type = 0;
        p.flag = 0;
        p.deal = true;
        p.seller_is_club = 1;
        p.buyer = 1;
        p.inactive_answer = 0;
        p.person = true;
        p.rule = p.rule_second = p.rule_third = p.rule_fourth = 1;
        return p;
    };
    const unsigned top_id = 18;  // -0x7a10, a top division cell
    // The other club's ceiling (0x100f36d8..0x100f38ac): a high rating of the person, a deal level above 0x22,
    // staff ratings above the limits, every kind.
    for (const std::int32_t a : {0x1500, 0x1c00})
        for (const std::int32_t b : {0x70, 0x78, 0x8b, 0x8c, 0xa0})
            for (const std::int32_t d : {0xe00, 0xea6, 0x1000})
                for (const std::int32_t level : {0x10, 0x23, 0x30})
                    for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 7u, 0xffu})
                        for (const std::uint32_t flag : {0u, 1u}) {
                            Plan p = base(club_budget);
                            p.person_rating_a = a;
                            p.person_rating_b = b;
                            p.person_rating_c = d;
                            p.deal_level = level;
                            p.kind = kind;
                            p.deal_flag = flag;
                            p.id = maker.id_value(top_id);
                            p.tag = "ceiling a " + std::to_string(a) + " b " + std::to_string(b) + " d " + std::to_string(d) +
                                    " level " + std::to_string(level) + " kind " + std::to_string(kind);
                            maker.add(p);
                        }
    // Kinds 3 and 2 with a small ask under a large floor, the rule check false or true, the person's rating
    // above and below the limits.
    for (const std::uint32_t kind : {2u, 3u})
        for (const std::uint32_t reputation : {7000u, 9000u, 10000u})
            for (const std::int32_t band : {0, 1, 2})
                for (const std::uint32_t price : {0u, 50u, 500u})
                    for (const std::uint32_t second : {0u, 1u})
                        for (const std::int32_t a : {0x1500, 0x1d00, 0x1f00})
                            for (const std::uint32_t budget : {0u, 1u}) {
                                Plan p = base(club_budget);
                                p.kind = kind;
                                p.reputation = reputation;
                                p.band = std::uint32_t(band);
                                p.price_answer = price;
                                p.budget = budget;
                                p.balance_low = 0;
                                p.rule_second = second;
                                p.person_rating_a = a;
                                p.deal_level = 4;
                                p.id = maker.id_value(top_id);
                                p.tag = "kind " + std::to_string(kind) + " floor reputation " + std::to_string(reputation) + " a " + std::to_string(a);
                                maker.add(p);
                            }
    // The negotiator's kind (0xff): the contract's low nibble becomes the kind; a close deal (the club sells,
    // fewer than 200 days) and every rating of the other club.
    for (const std::uint32_t reputation : {7000u, 6000u})
        for (const std::uint32_t nibble : {0u, 1u, 2u, 3u, 4u})
            for (const std::int32_t a : {0x1400, 0x1483, 0x1500, 0x1a5e, 0x1a5f, 0x1c52, 0x1c53, 0x1e47})
                for (const std::int32_t d : {0x1000, 0x1483})
                    for (const std::uint32_t days : {100u, 250u}) {
                        Plan p = base(club_budget);
                        p.kind = 0xff;
                        p.reputation = reputation;
                        p.contract_byte = 0x20 | nibble;
                        p.person_rating_a = a;
                        p.person_rating_c = d;
                        p.days = days;
                        p.id = maker.id_value(top_id);
                        p.tag = "negotiator " + std::to_string(nibble) + " a " + std::to_string(a) + " d " + std::to_string(d);
                        maker.add(p);
                    }
    // Kind 2 whose first rule check is true and second false, the other club rated high (the deal level
    // above 0x1f keeps the kind block before it from calling the check, so the answers fall in order).
    for (const std::int32_t a : {0x1d00, 0x1e45, 0x1e46, 0x1e47, 0x1f00})
        for (const std::uint32_t fourth : {0u, 1u})
            for (const std::uint32_t third : {0u, 1u}) {
                Plan p = base(club_budget);
                p.kind = 2;
                p.deal_level = 0x30;
                p.person_rating_a = a;
                p.rule_third = third;
                p.rule_fourth = fourth;
                p.id = maker.id_value(top_id);
                p.tag = "kind 2 second rule a " + std::to_string(a);
                maker.add(p);
            }
    // The deal's other-club pointer cleared by a callee, after the original has tested it: the guards the
    // original repeats after a call (0x100f35a8, 0x100f3670, 0x100f4378). The rule check clears it and the
    // deal price (dispatched before the test that enters the block) puts it back; for the last guard the
    // owner's deal check clears it, with the club flag set so that nothing reads it afterwards.
    {
        const auto club = scenario.club(37);
        (void)club;
        const std::uint32_t outside = deal_at;
        const Case::Poke clear_after_rule{club_rule, {{4, outside + 0x14, 0}}};
        const Case::Poke restore_before_price{deal_price, {{4, outside + 0x14, person_at}}};
        for (const std::uint32_t kind : {2u, 3u})
            for (const std::uint32_t second : {0u, 1u}) {
                Plan p = base(club_budget);
                p.kind = kind;
                p.reputation = 9000;
                p.band = 1;
                p.price_answer = 0;
                p.budget = 0;
                p.balance_low = 0;
                p.rule_second = second;
                p.person_rating_a = 0x1500;
                p.deal_level = 4;
                p.id = maker.id_value(top_id);
                p.pokes = {clear_after_rule, restore_before_price};
                p.tag = "kind " + std::to_string(kind) + " other club cleared after the rule check";
                maker.add(p);
            }
        for (const std::uint32_t third : {0u, 1u})
            for (const std::uint32_t fourth : {0u, 1u}) {
                Plan p = base(club_budget);
                p.kind = 2;
                p.flag = 1;
                p.deal_level = 0x30;
                p.person_rating_a = 0x1f00;
                p.rule_third = 1;
                p.rule_fourth = fourth;
                (void)third;
                p.id = maker.id_value(top_id);
                p.pokes = {{deal_for_owner, {{4, outside + 0x14, 0}}}};
                p.tag = "kind 2 other club cleared by the owner's deal check";
                maker.add(p);
            }
    }
    // The kind budget with the wealth band outside -2..2, and -1.
    for (const std::int32_t band : {-3, -1, -128, 3, 127})
        for (const std::uint32_t kind : {1u, 5u, 8u, 10u})
            for (const unsigned id : {18u, 14u, 5u}) {
                Plan p = base(budget_for_kind);
                p.deal = false;
                p.kind = kind;
                p.band = std::uint32_t(band);
                p.id = maker.id_value(id);
                p.tag = "kind budget band " + std::to_string(band);
                maker.add(p);
            }
}

FINANCE_CASES("value refresh") {
    if (scenario.match) return;
    const auto clubs = scenario.global(-0x7b50);
    Rng rng{0x4f80};
    constexpr std::int32_t top_slots[] = {-0x7a10, -0x7a14, -0x7a18, -0x7af4, -0x7a24, -0x7a00};
    for (unsigned n = 0; n < 1500; ++n) {
        Case item;
        item.entry = refresh_account;
        const auto index = rng.pick({2u, 11u, 37u, 100u, clubs / 3, clubs / 2, clubs - 2});
        const auto club = scenario.club(index);
        const auto account = scenario.r32(scenario.global(-0x7ab8)) + index * 0x168;
        const auto put4 = [&](std::uint32_t at, std::uint32_t v) { item.memory.push_back({4, at, v}); };
        const auto put2 = [&](std::uint32_t at, std::uint32_t v) { item.memory.push_back({2, at, v}); };
        const auto put1 = [&](std::uint32_t at, std::uint32_t v) { item.memory.push_back({1, at, v}); };
        // How many contract slots are filled: few (the shortfall figure applies) or many.
        const unsigned filled = rng.chance(60) ? rng.below(16) : 14 + rng.below(37);
        for (unsigned slot = 0; slot < 50; ++slot) put4(club + 0x98 + slot * 4, slot < filled ? 100 + slot : 0);
        for (unsigned slot = 0; slot < 5; ++slot) put4(club + 0x160 + slot * 4, rng.chance(50) ? 200 + slot : 0);
        for (unsigned slot = 0; slot < 7; ++slot) put4(club + 0x174 + slot * 4, rng.chance(50) ? 300 + slot : 0);
        for (unsigned slot = 0; slot < 3; ++slot) put4(club + 0x190 + slot * 4, rng.chance(50) ? 400 + slot : 0);
        put2(club + 0x1ec, rng.chance(80) ? rng.pick({0u, 1000u, 2250u, 2251u, 3000u, 4750u, 4751u, 5000u, 5300u, 5301u, 6000u, 6600u, 6700u, 7000u, 9000u})
                                          : rng.below(0x10000));
        put1(club + 0x23d, rng.pick({0u, 1u, 1u, 2u}));
        put4(club + 0x2c, rng.chance(85) ? league_at : 0);
        put1(league_at + 0x11c, rng.pick({0u, 1u, 2u, 3u, 4u}));
        put4(league_at + 0x68, rng.chance(60) ? scenario.global(rng.pick({-0x7a10, -0x7a14, -0x7a18})) : scenario.global(top_slots[rng.below(6)]));
        put4(account + 0xc, rng.next() % 1000);
        put4(account + 0x10, rng.next() % 1000);
        put4(account + 0x14, rng.next() % 100000);
        put4(account, 0);
        put4(account + 4, rng.below(30'000'000));
        const auto record = [&](std::uint32_t at, std::uint32_t amount, std::uint32_t byte38) {
            put4(at + 0xc, amount);
            put1(at + 0x38, byte38);
        };
        for (unsigned slot = 0; slot < 4; ++slot)
            record(rows_at + slot * 0x40, rng.chance(15) ? 0xffffffffu : rng.chance(50) ? rng.below(500000) : rng.next(),
                   rng.pick({0x18u, 0x19u, 0x00u, 0xffu, 0x80u}));
        item.registers = {{3, account}};
        item.answers = {{inactive, rng.chance(10)}, {band_of, std::uint32_t(rng.pick({-2, -1, 0, 1, 2}))},
                        {contract_of, rng.chance(85) ? rows_at + rng.below(4) * 0x40 : 0},
                        {loan_record, rng.chance(60) ? rows_at + rng.below(4) * 0x40 : 0},
                        {contract_by_index, rng.chance(80) ? rows_at + rng.below(4) * 0x40 : 0}, {club_rule, rng.chance(50)},
                        {days_between, rng.below(300)}, {deal_price, rng.below(40000)}, {owner_trusts, rng.chance(50)},
                        {deal_for_owner, rng.chance(50)}, {deal_for_club, rng.chance(50)}, {signing_value, 0}};
        item.label = "refresh " + std::to_string(n);
        cases.push_back(std::move(item));
    }
    (void)loan_at;
    (void)post_at;
}
}
