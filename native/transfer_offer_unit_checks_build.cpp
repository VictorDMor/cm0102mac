// TRANSFER_OFFER.CPP checker cases for building an offer (0x104126d0, transfer_offer_unit_build.hpp).
//
// The offer is built in the scratch area, every byte written first (the routine keeps the low nibbles of
// +0x38 and +0x3c), and the deal word beside it. Players, buyers and their shortlist rows are the career's
// own: the rows are the real kind-11 rows of clubs that keep a shortlist, with their category, level and
// weight sometimes rewritten (undone afterwards). The real cases run every callee as it is; the answered
// cases set the outside routines whose result picks a branch: the human-club test, the refusals, the
// earlier and pending offers (scratch offers; an exchange list is one of the game's own lists, found by its
// shape), the contract finder (a scratch contract with a real club), the valuations, the ceiling, the
// finance records and states, the balance, the role rule, the clause, the approach.
#include "transfer_offer_unit_checks.hpp"
#include "club_control.hpp"
#include <algorithm>
#include <cstdint>
#include <string>

namespace cm::aot::transfer_offer_unit_checks {
namespace {
constexpr std::uint32_t build = 0x104126d0;
constexpr std::uint32_t club_human = 0x1014c2e0, earlier_offer = 0x103f5d20, refuses = 0x1036fbf0, unwanted = 0x103faff0,
                        sign_free = 0x10080d30, contract_of = 0x10082f20, contract_pending = 0x10083110,
                        value_to = 0x10085860, human_asking = 0x10089a40, keeps_player = 0x100d3040,
                        valuation = 0x10084be0, release_clause = 0x10380520, ceiling = 0x10361490,
                        finances_of = 0x10103510, finance_state = 0x100f55b0, balance = 0x100f5770,
                        pending_offer = 0x103f4240, role_rule = 0x100c7a20,
                        clause_payable = 0x10380f20, seller_price = 0x103fac70, approach = 0x103f8820,
                        transfer_a = 0x103eaea0, transfer_b = 0x103eb020;
constexpr std::uint32_t area = scratch_base + 0x20000, offer_at = area, deal_at = area + 0x100, earlier_at = area + 0x200,
                        pending_at = area + 0x300, contract_at = area + 0x400, books_at = area + 0x500;

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
std::string text(std::int64_t value) { return std::to_string(value); }

struct Builder {
    Case item;
    void put(unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); }
    void answer(std::uint32_t entry, std::uint32_t value) { item.answers.push_back({entry, value}); }
};

// A game list found in memory by its shape (count, head, tail, cursor; nodes of data, size, next, previous)
// whose rows begin with a person index.
struct List {
    std::uint32_t header;
    std::vector<std::uint32_t> people;
};
std::vector<List> find_lists(const Scenario& sc) {
    auto& m = sc.s.memory;
    const auto mapped = [&](std::uint32_t a) {
        for (const auto& [low, size] : m.mapped_ranges())
            if (a >= low && std::uint64_t(a) + 16 <= std::uint64_t(low) + size) return true;
        return false;
    };
    std::vector<List> lists;
    for (const auto& [low, size] : m.mapped_ranges()) {
        if (low < 0x10000000 || low >= scratch_base) continue;
        for (std::uint64_t a = low; a + 16 <= low + size && lists.size() < 64; a += 4) {
            const auto header = std::uint32_t(a);
            const std::int32_t count = std::int16_t(m.read<2>(header));
            if (count < 1 || count > 6) continue;
            const auto head = sc.r32(header + 4), tail = sc.r32(header + 8);
            if (!head || (head & 3) || !tail || (tail & 3) || !mapped(head) || !mapped(tail)) continue;
            List list{header, {}};
            bool good = true;
            std::uint32_t node = head, previous = 0;
            while (node && list.people.size() <= std::size_t(count)) {
                if (!mapped(node)) { good = false; break; }
                const auto data = sc.r32(node);
                const auto row_size = std::int16_t(m.read<2>(node + 4));
                if (!mapped(data) || sc.r32(node + 12) != previous || (row_size != 4 && row_size != 8) ||
                    sc.r32(data) >= sc.people()) { good = false; break; }
                list.people.push_back(sc.r32(data));
                previous = node;
                node = sc.r32(node + 8);
            }
            if (good && node == 0 && previous == tail && list.people.size() == std::size_t(count)) lists.push_back(std::move(list));
        }
    }
    return lists;
}

// The career's people, clubs and shortlist rows grouped by what the routine reads of them.
struct World {
    const Scenario& sc;
    unsigned clubs = 0;
    std::vector<unsigned> employed, free_agents, at_human, bare;  // bare: no player record (+0x14)
    std::vector<unsigned> human_clubs;
    struct Wanted { unsigned club; std::uint32_t row; };
    std::vector<Wanted> rows, other_rows;  // kind-11 rows, and rows of other kinds
    explicit World(const Scenario& sc_) : sc(sc_), clubs(sc_.global(-0x7b50)) {
        for (unsigned id = 0; id < clubs; ++id) {
            const auto human = club_control::human_controlled(sc.s, sc.club(id), 0, 1, 0, 0);
            if (human && *human) human_clubs.push_back(id);
        }
        for (unsigned p = 0; p < sc.people(); p += 3) {
            const auto record = sc.person(p);
            if (!sc.r32(record + 0x14)) { if (bare.size() < 64) bare.push_back(p); continue; }
            const auto employer = sc.r32(record + 0x24);
            if (!employer) { free_agents.push_back(p); continue; }
            if (employer < sc.club(0) || employer >= sc.club(clubs) || (employer - sc.club(0)) % 0x248) continue;
            const auto id = (employer - sc.club(0)) / 0x248;
            (std::ranges::find(human_clubs, id) != human_clubs.end() ? at_human : employed).push_back(p);
        }
        const auto transfers = sc.global(-0x7b40);
        const auto manager = transfers ? sc.r32(transfers + 0x988) : 0;
        const auto entries = manager ? sc.r32(manager + 0xdc) : 0;
        if (!entries) return;
        for (unsigned id = 0; id < clubs; ++id) {
            const auto entry = entries + id * 0xc;
            const auto table = sc.r32(entry);
            if (!table || std::uint8_t(sc.s.memory.read<1>(entry + 4)) != 1) continue;
            for (unsigned r = 0; r < 40; ++r) {
                const auto row = table + r * 0x2c;
                const auto kind = std::uint8_t(sc.s.memory.read<1>(row));
                if (kind == 11) rows.push_back({id, row});
                else if (kind != 0 && other_rows.size() < 64) other_rows.push_back({id, row});
            }
        }
    }
    bool usable() const { return clubs && !employed.empty() && !free_agents.empty() && !rows.empty(); }
    std::uint32_t club_of_person(unsigned p) const {
        const auto employer = sc.r32(sc.person(p) + 0x24);
        return employer ? (employer - sc.club(0)) / 0x248 : 0;
    }
};

// The call's arguments and what a case writes besides.
struct Call {
    unsigned player = 0;
    std::uint32_t player_record = 0, club_record = 0, row = 0;
    std::int32_t budget = -1;
    std::uint32_t fixed = 0;
    bool with_deal = false;
};
void write_offer_area(Builder& b, Rng& rng) {
    // Every byte of the offer, the low nibbles of +0x38 and +0x3c among them, and the deal word.
    const auto fill = rng.pick<std::uint32_t>({0, 0xa5a5a5a5u, 0x0f0f0f0fu, 0xffffffffu, rng.next()});
    for (std::uint32_t at = 0; at < 0x54; at += 4) b.put(4, offer_at + at, fill);
    b.put(4, deal_at, rng.pick<std::uint32_t>({0, 0x12345678u}));
}
Case make(Builder&& b, const Call& call, const std::string& label) {
    b.item.entry = build;
    b.item.registers = {{3, offer_at}, {4, call.player_record}, {5, call.club_record}, {6, call.row},
                        {7, std::uint32_t(call.budget)}, {8, call.fixed}, {9, call.with_deal ? deal_at : 0}};
    b.item.label = label;
    return std::move(b.item);
}
// A row's bytes the routine reads, rewritten: the category (bits 3-6 of +0x28), the level and the weight.
void vary_row(Builder& b, Rng& rng, std::uint32_t row, unsigned percent) {
    if (!row) return;
    if (rng.chance(percent)) {
        const auto flags = rng.below(256);
        b.put(1, row + 0x28, (flags & 0x87) | (rng.pick<std::uint32_t>({0, 1, 2, 3, 4, 5, 5, 5, 4, 6, 15}) << 3));
    }
    if (rng.chance(percent)) b.put(4, row + 4, rng.pick<std::uint32_t>({0, 1, 1000, 250000, 1000000, 8000000, 0xffffff00u}));
    if (rng.chance(percent)) b.put(4, row + 0xc, rng.pick<std::uint32_t>({0, 1, 3, 500, 70000, 0x12345, 2000000, 0xfffff000u}));
}
// A person's bytes the routine reads, rewritten: the age byte, the role flag and the value.
void vary_player(Builder& b, Rng& rng, std::uint32_t record, unsigned percent) {
    if (rng.chance(percent)) b.put(1, record + 0x5d, rng.pick<std::uint32_t>({16, 19, 22, 24, 25, 26, 27, 28, 29, 31, 0xf0}));
    if (rng.chance(percent)) b.put(1, record + 0x5e, rng.pick<std::uint32_t>({0, 0, 0, 1, 4, 5, 9}));
    if (rng.chance(percent)) b.put(4, record + 8, rng.pick<std::uint32_t>({0, 1000, 90000, 250000, 1500000, 7000000, 0xffffff00u}));
}
}

TRANSFER_OFFER_UNIT_CASES("build") {
    if (scenario.match) return;
    const World world(scenario);
    if (!world.usable()) return;
    Rng rng{0xb01d0ffe};
    const auto some = [&](const std::vector<unsigned>& group) { return group[rng.below(unsigned(group.size()))]; };
    const auto some_row = [&]() { return world.rows[rng.below(unsigned(world.rows.size()))]; };
    // The sample table's cursor moved to another entry (and sometimes its base changed), so the cases' draws
    // differ: every case starts from the same snapshot.
    const auto first_sample = scenario.toc(-0x7b8c), last_sample = scenario.global(-0x7b88);
    const auto place_samples = [&](Builder& b) {
        if (last_sample <= first_sample || !rng.chance(60)) return;
        // Near the end, one of the call's first draws wraps and redraws the base (0x1077b770 at its site).
        const auto at = rng.chance(40) ? last_sample - 2 * rng.below(16)
                                       : first_sample + 2 * rng.below((last_sample - first_sample) / 2 + 1);
        b.put(4, scenario.toc(-0x7b84), at);
        if (rng.chance(30)) b.put(4, scenario.toc(-0x7b90), rng.below(0x10000));
    };
    const auto budget_for = [&](std::uint32_t record) {
        const auto value = std::int32_t(scenario.r32(record + 8));
        return rng.pick<std::int32_t>({-1, -1, 0, 4999, 5000, value / 3, value / 2, value, value * 2, 1000000, 5000000, 40000000});
    };
    // A player of one of the groups, the buyer of a real row (or another club), the row or none.
    const auto pick_call = [&](Builder& b, unsigned group_percent_free, unsigned group_percent_human) {
        Call call;
        const auto roll = rng.below(100);
        call.player = roll < group_percent_free ? some(world.free_agents)
                      : roll < group_percent_free + group_percent_human && !world.at_human.empty() ? some(world.at_human)
                                                                                                   : some(world.employed);
        call.player_record = scenario.person(call.player);
        const auto wanted = some_row();
        call.club_record = scenario.club(wanted.club);
        const auto shape = rng.below(10);
        if (shape < 7) call.row = wanted.row;
        else if (shape < 8 && !world.other_rows.empty()) call.row = world.other_rows[rng.below(unsigned(world.other_rows.size()))].row;
        else call.row = 0;
        call.budget = budget_for(call.player_record);
        call.fixed = rng.chance(15) ? rng.pick<std::uint32_t>({1, 0x100, 0xff}) : 0;
        call.with_deal = rng.chance(60);
        write_offer_area(b, rng);
        place_samples(b);
        return call;
    };

    // ---- null arguments: the assertion ----
    for (const unsigned which : {0u, 1u, 2u}) {
        Builder b;
        write_offer_area(b, rng);
        Call call;
        call.player_record = which == 1 ? 0 : scenario.person(some(world.employed));
        call.club_record = which == 0 ? 0 : which == 1 ? scenario.club(some_row().club) : 0;
        if (which == 2) call.player_record = 0;
        cases.push_back(make(std::move(b), call, "build null " + text(which)));
    }

    // ---- real calls: every callee runs ----
    for (unsigned n = 0; n < 90; ++n) {
        Builder b;
        auto call = pick_call(b, 12, 10);
        vary_row(b, rng, call.row, n % 3 ? 30 : 0);
        vary_player(b, rng, call.player_record, n % 4 ? 25 : 0);
        cases.push_back(make(std::move(b), call, "build real " + text(n)));
    }

    // ---- a human buyer: an earlier offer or the player's value ----
    // The career's human clubs buy as they are. An earlier offer with players in exchange needs a buyer that
    // holds them: there the club of a listed player is answered human (0x1014c2e0), and the paths that would
    // ask the finances of a human club (fixed offers, no earlier offer) are left out.
    const auto lists = find_lists(scenario);
    for (unsigned n = 0; n < 40; ++n) {
        Builder b;
        auto call = pick_call(b, 10, 0);
        const auto shape = rng.below(6);
        const bool exchange_shape = shape >= 3 && !lists.empty();
        if (!exchange_shape) {
            if (world.human_clubs.empty()) continue;
            call.club_record = scenario.club(world.human_clubs[rng.below(unsigned(world.human_clubs.size()))]);
            if (rng.chance(30)) call.fixed = 1;
            if (rng.chance(30)) b.answer(balance, rng.pick<std::uint32_t>({0, 0xffffffffu, 1}));
        } else {
            call.fixed = 0;
            b.answer(club_human, 1);
        }
        if (shape >= 1) {
            // An earlier offer in the scratch area: the record copied, its exchange list one of the game's.
            for (std::uint32_t at = 0; at < 0x54; at += 4) b.put(4, earlier_at + at, rng.next());
            b.put(1, earlier_at + 0x24, shape == 1 ? rng.pick<std::uint32_t>({1, 0x80}) : 0);
            std::uint32_t exchange = 0;
            if (exchange_shape) {
                const auto& list = lists[rng.below(unsigned(lists.size()))];
                exchange = list.header;
                // The buyer: the club of a listed player, so his row passes the club test.
                const auto listed = list.people[rng.below(unsigned(list.people.size()))];
                if (scenario.r32(scenario.person(listed) + 0x24)) call.club_record = scenario.club(world.club_of_person(listed));
                if (rng.chance(50)) b.answer(contract_pending, rng.pick<std::uint32_t>({0, 1}));
                if (rng.chance(60)) b.answer(transfer_a, rng.pick<std::uint32_t>({0, 0, 1}));
                if (rng.chance(60)) b.answer(transfer_b, rng.pick<std::uint32_t>({0, 1}));
                if (rng.chance(50)) for (const auto person : list.people) b.put(1, scenario.person(person) + 0x62, 11);
            }
            b.put(4, earlier_at + 4, exchange);
            b.answer(earlier_offer, earlier_at);
        }
        cases.push_back(make(std::move(b), call, "build human buyer " + text(n) + " shape " + text(shape)));
    }

    // ---- a human seller: his price, his floor when he keeps the player, his barred finances ----
    for (unsigned n = 0; n < (world.at_human.empty() ? 0u : 40u); ++n) {
        Builder b;
        auto call = pick_call(b, 0, 100);
        if (call.row == 0 || rng.chance(50)) call.row = some_row().row;
        vary_row(b, rng, call.row, 50);
        vary_player(b, rng, call.player_record, 40);
        if (rng.chance(60)) b.answer(keeps_player, rng.pick<std::uint32_t>({0, 1, 1, 0x100}));
        if (rng.chance(60)) b.answer(human_asking, rng.pick<std::uint32_t>({0, 90000, 400000, 1600000, 2000000, 7000000, 0x7fffffff}));
        if (rng.chance(50)) b.answer(ceiling, rng.pick<std::uint32_t>({0, 300000, 1000000, 1800000, 5000000}));
        if (rng.chance(40)) {
            for (std::uint32_t at = 0x160; at < 0x168; at += 4) b.put(4, books_at + at, 0);
            b.put(1, books_at + 0x165, rng.pick<std::uint32_t>({1, 1, 1, 0}));
            b.answer(finances_of, books_at);
            b.answer(finance_state, rng.pick<std::uint32_t>({0, 1, 0xff, 0xfe, 2, 3}));
            b.answer(balance, rng.pick<std::uint32_t>({0, 0, 0xffffffffu}));
            if (rng.chance(70)) b.answer(valuation, rng.pick<std::uint32_t>({0, 200000, 900000, 1500000, 5000000}));
        }
        if (rng.chance(25)) {
            for (std::uint32_t at = 0; at < 0x54; at += 4) b.put(4, pending_at + at, 0);
            b.put(4, pending_at + 0x20, rng.pick<std::uint32_t>({50000, 500000, 1500000}));
            b.put(1, pending_at + 0x34, 2);
            b.answer(pending_offer, pending_at);
        }
        cases.push_back(make(std::move(b), call, "build human seller " + text(n)));
    }

    // ---- refusals: a deal signed without a fee, or no offer ----
    for (unsigned n = 0; n < 24; ++n) {
        Builder b;
        auto call = pick_call(b, 10, 5);
        if (call.row == 0) call.row = some_row().row;
        b.answer(refuses, rng.pick<std::uint32_t>({0, 0, 1, 0x100}));
        b.answer(unwanted, rng.pick<std::uint32_t>({0, 1, 0x100}));
        if (rng.chance(60)) b.answer(sign_free, rng.pick<std::uint32_t>({0, 1, 0x100}));
        if (rng.chance(50)) {
            b.put(4, contract_at + 4, some_row().club);
            b.put(4, contract_at + 0x24, rng.pick<std::uint32_t>({0, 100000, 3000000}));
            b.put(1, contract_at + 0x38, rng.pick<std::uint32_t>({0, 4, 8, 0xc, 0x10, 0x14, 0x15, 0x17, 0x80, 0xff}));
            for (std::uint32_t at = 0x30; at < 0x38; ++at) b.put(1, contract_at + at, rng.below(256));
            b.answer(contract_of, contract_at);
        }
        call.with_deal = rng.chance(75);
        vary_row(b, rng, call.row, 30);
        cases.push_back(make(std::move(b), call, "build refusal " + text(n)));
    }

    // ---- a computer buyer with answered prices: the finance tiers, the budget, the terms, the revision ----
    for (unsigned n = 0; n < 160; ++n) {
        Builder b;
        auto call = pick_call(b, 6, 8);
        if (call.row == 0 && rng.chance(70)) call.row = some_row().row;
        vary_row(b, rng, call.row, 60);
        vary_player(b, rng, call.player_record, 50);
        // The two valuations agree when answered (no gap); real ones usually differ.
        if (rng.chance(55))
            b.answer(value_to, rng.pick<std::uint32_t>({0, 50000, 99999, 100000, 400000, 1000000, 2500000, 6000000, 25000000}));
        if (rng.chance(55))
            b.answer(ceiling, rng.pick<std::uint32_t>({0, 1, 60000, 300000, 750000, 1000000, 1800000, 3000000, 9000000, 0xffffff00u}));
        if (rng.chance(45)) b.answer(finance_state, rng.pick<std::uint32_t>({0, 1, 2, 3, 0xff, 0xfe, 0xfd, 0x7f, 0x102}));
        if (rng.chance(25)) b.answer(release_clause, rng.pick<std::uint32_t>({0, 1}));
        // 0x100ce250 is left to run: 0x1014c2e0 calls it too. A player without a birth club skips it.
        if (rng.chance(20)) b.put(4, call.player_record + 0x20, 0);
        if (rng.chance(20)) b.answer(keeps_player, rng.pick<std::uint32_t>({0, 1}));
        if (rng.chance(15)) b.answer(human_asking, rng.pick<std::uint32_t>({0, 400000, 2000000, 7000000}));
        if (rng.chance(25)) {
            // A pending offer at stage 2: its total (0x10416710) is the earlier total.
            for (std::uint32_t at = 0; at < 0x54; at += 4) b.put(4, pending_at + at, 0);
            b.put(4, pending_at + 8, some_row().club);
            b.put(4, pending_at + 0xc, call.player);
            b.put(1, pending_at + 0x11, 0xb);
            b.put(4, pending_at + 0x20, rng.pick<std::uint32_t>({0, 50000, 500000, 1500000, 6000000}));
            b.put(1, pending_at + 0x34, rng.pick<std::uint32_t>({2, 2, 2, 1}));
            b.answer(pending_offer, pending_at);
        }
        if (rng.chance(20)) {
            // The finances of both clubs: a scratch record, barred or not, with its state and balance answered.
            for (std::uint32_t at = 0x160; at < 0x168; at += 4) b.put(4, books_at + at, 0);
            b.put(1, books_at + 0x165, rng.pick<std::uint32_t>({1, 1, 0, 2}));
            b.answer(finances_of, books_at);
            b.answer(finance_state, rng.pick<std::uint32_t>({0, 1, 0xff, 0xfe, 2}));
            b.answer(balance, rng.pick<std::uint32_t>({0, 0, 0xffffffffu}));
            if (rng.chance(50)) b.answer(valuation, rng.pick<std::uint32_t>({0, 200000, 900000, 5000000}));
        } else if (rng.chance(15)) {
            b.answer(balance, rng.pick<std::uint32_t>({0xffffffffu, 0, 0x80000000u}));
        }
        if (rng.chance(20)) b.answer(role_rule, rng.pick<std::uint32_t>({0, 1}));
        if (rng.chance(15)) b.answer(approach, rng.pick<std::uint32_t>({0, 1}));
        if (rng.chance(15)) b.answer(clause_payable, rng.pick<std::uint32_t>({0, 1}));
        if (rng.chance(10)) b.answer(seller_price, rng.pick<std::uint32_t>({0, 123456}));
        if (rng.chance(25)) {
            b.put(4, contract_at + 4, rng.chance(80) ? some_row().club : world.club_of_person(call.player));
            b.put(4, contract_at + 0x24, rng.pick<std::uint32_t>({0, 100000, 900000, 3000000, 0x7fffffff}));
            b.put(1, contract_at + 0x38, rng.pick<std::uint32_t>({0, 4, 8, 0xc, 0x10, 0x14, 0x15, 0x80}));
            for (std::uint32_t at = 0x30; at < 0x38; ++at) b.put(1, contract_at + at, rng.below(256));
            b.answer(contract_of, contract_at);
        }
        cases.push_back(make(std::move(b), call, "build priced " + text(n)));
    }

    // ---- targeted terms: a nation of reputation over 5000, a player reputed over it (1.3 times), role flag
    // clear, a young age; the price, ceiling and budget answered so the path is the one named ----
    const auto reputed = [&](Builder& b, const Call& call, bool reputed_player, std::uint32_t age) {
        const auto nation = scenario.r32(call.player_record + 0xc);
        if (nation) b.put(2, nation + 0xac, 6000);
        b.put(2, scenario.r32(call.player_record + 0x14) + 8, reputed_player ? 9000 : 3000);
        b.put(4, call.player_record + 0x20, 0);
        b.put(1, call.player_record + 0x5e, 0);
        b.put(1, call.player_record + 0x5d, age);
    };
    const auto keen_row = [&](Builder& b, std::uint32_t row, std::uint32_t category, std::uint32_t level, std::uint32_t weight) {
        b.put(1, row + 0x28, (category << 3) | rng.pick<std::uint32_t>({0, 1, 4, 0x80}));
        b.put(4, row + 4, level);
        b.put(4, row + 0xc, weight);
    };
    // No finance tier (state 2 for both clubs: 2 x 0.1 x 2.5 = 0.5), real balance.
    const auto plain_finances = [&](Builder& b) { b.answer(0x100f55b0, 2); };
    for (unsigned n = 0; n < 96; ++n) {
        Builder b;
        auto call = pick_call(b, 0, 0);
        const auto wanted = some_row();
        call.row = wanted.row;
        call.club_record = scenario.club(wanted.club);
        call.fixed = 0;
        plain_finances(b);
        const auto shape = n % 6;
        std::string what;
        switch (shape) {
            case 0:  // the seller asks far more: negotiation, a reputed player (the appearance extra of the gap)
                keen_row(b, call.row, 5, 1000000, 50000);
                reputed(b, call, true, rng.pick<std::uint32_t>({19, 23, 30}));
                b.answer(value_to, 6000000);
                call.budget = 1000000;
                what = "negotiate reputed";
                break;
            case 1:  // a little more: the appearance extra of a young reputed player, then the random terms
                keen_row(b, call.row, rng.pick<std::uint32_t>({5, 5, 5, 4, 2}), rng.pick<std::uint32_t>({1000000, 3000000}),
                         rng.pick<std::uint32_t>({0, 400000, 2500000}));
                if (last_sample > first_sample)
                    b.put(4, scenario.toc(-0x7b84), first_sample + 2 * rng.below((last_sample - first_sample) / 2 + 1));
                reputed(b, call, rng.chance(80), rng.pick<std::uint32_t>({17, 19, 21, 24, 30, 33}));
                b.answer(value_to, 1300000);
                call.budget = 1000000;
                what = "small gap young";
                break;
            case 2:  // the ceiling at half the price: a counter offer for a reputed player
                keen_row(b, call.row, rng.pick<std::uint32_t>({5, 4}), 1000000, 50000);
                reputed(b, call, rng.chance(75), rng.pick<std::uint32_t>({20, 26}));
                if (rng.chance(30)) b.put(1, call.player_record + 0x5e, 1);  // the role flag set: the modest split
                b.answer(value_to, 1000000);
                b.answer(ceiling, rng.pick<std::uint32_t>({500000, 450000, 760000, 300000}));
                call.budget = -1;
                what = "counter";
                break;
            case 3:  // a ceiling between three quarters and four fifths of the price, a budget over it: stretched
                keen_row(b, call.row, rng.pick<std::uint32_t>({1, 5}), rng.pick<std::uint32_t>({1000000, 4000000}),
                         rng.pick<std::uint32_t>({1, 10000, 300000, 900000, 5000000}));
                b.answer(value_to, 1000000);
                b.answer(ceiling, rng.pick<std::uint32_t>({780000, 790000, 760000}));
                call.budget = rng.pick<std::int32_t>({2000000, 1500000});
                what = "stretch";
                break;
            case 4: {  // an earlier total over the fee: the club gives up (or takes 1.2 times it)
                keen_row(b, call.row, 1, 1000000, 50000);
                b.answer(value_to, 1000000);
                b.answer(ceiling, rng.pick<std::uint32_t>({1000000, 900000, 1500000}));
                for (std::uint32_t at = 0; at < 0x54; at += 4) b.put(4, pending_at + at, 0);
                b.put(4, pending_at + 8, wanted.club);
                b.put(4, pending_at + 0xc, call.player);
                b.put(1, pending_at + 0x11, 0xb);
                b.put(4, pending_at + 0x20, rng.pick<std::uint32_t>({1500000, 900000, 1100000}));
                b.put(1, pending_at + 0x34, 2);
                b.answer(pending_offer, pending_at);
                call.budget = -1;
                what = "earlier total";
                break;
            }
            default:  // a deal signed without a fee whose clause cannot be paid: the seller's price
                // A row of another kind refuses outright (no contract-kind exception).
                if (!world.other_rows.empty()) call.row = world.other_rows[rng.below(unsigned(world.other_rows.size()))].row;
                b.answer(refuses, 0);
                b.answer(sign_free, 1);
                b.answer(role_rule, 1);
                b.answer(clause_payable, rng.pick<std::uint32_t>({0, 0, 1}));
                if (rng.chance(50)) b.answer(approach, rng.pick<std::uint32_t>({0, 1}));
                call.with_deal = true;
                what = "deal clause";
        }
        if (shape != 5) call.with_deal = rng.chance(50);
        cases.push_back(make(std::move(b), call, "build targeted " + what + " " + text(n)));
    }

    // ---- the seller's two valuations apart (0x10085860 answered per call: its first dispatch the price, its
    // second the lower valuation): the negotiation by the gap, the larger sell-on share and the instalments
    // capped by the gap when it passes three quarters of the price ----
    for (unsigned n = 0; n < 36; ++n) {
        Builder b;
        auto call = pick_call(b, 0, 0);
        const auto wanted = some_row();
        call.row = wanted.row;
        call.club_record = scenario.club(wanted.club);
        call.fixed = 0;
        b.answer(0x100f55b0, 2);
        b.answer(refuses, 1);
        b.answer(unwanted, 0);
        keen_row(b, call.row, rng.pick<std::uint32_t>({1, 2, 5, 4}), 1000000, 50000);
        const auto shape = n % 3;
        if (shape == 0) reputed(b, call, true, 22);
        else if (shape == 1) reputed(b, call, false, 22);
        else b.put(1, call.player_record + 0x5e, 1);
        const std::uint32_t high = rng.pick<std::uint32_t>({6000000, 3000000, 9000000});
        const std::uint32_t low = rng.pick<std::uint32_t>({500000, 1000000, 2000000, 5500000});
        b.item.nth_answers.push_back({value_to, 1, high});
        b.item.nth_answers.push_back({value_to, 2, low});
        call.budget = rng.pick<std::int32_t>({1000000, 1500000, 2500000});
        if (last_sample > first_sample)
            b.put(4, scenario.toc(-0x7b84), first_sample + 2 * rng.below((last_sample - first_sample) / 2 + 1));
        cases.push_back(make(std::move(b), call, "build gap " + text(n)));
    }

    // ---- the chance mark (+0x35): a player of world reputation 9000 at a club of the buyer's league, a row of
    // category 0-3, the sample table placed anywhere (one draw in 20 marks) ----
    for (unsigned n = 0; n < 40; ++n) {
        Builder b;
        auto call = pick_call(b, 0, 0);
        const auto seller_id = world.club_of_person(call.player);
        const auto league = scenario.r32(scenario.club(seller_id) + 0x30);
        std::vector<World::Wanted> same;
        for (const auto& wanted : world.rows)
            if (scenario.r32(scenario.club(wanted.club) + 0x30) == league && wanted.club != seller_id) same.push_back(wanted);
        if (same.empty()) continue;
        const auto wanted = same[rng.below(unsigned(same.size()))];
        call.row = wanted.row;
        call.club_record = scenario.club(wanted.club);
        b.put(1, call.row + 0x28, rng.pick<std::uint32_t>({0, 1, 2, 3}) << 3);
        b.put(2, scenario.r32(call.player_record + 0x14) + 0xa, 9000);
        if (last_sample > first_sample)
            b.put(4, scenario.toc(-0x7b84), first_sample + 2 * rng.below((last_sample - first_sample) / 2 + 1));
        cases.push_back(make(std::move(b), call, "build chance " + text(n)));
    }

    // ---- an earlier offer whose exchange player has exactly one piece of transfer business, this offer ----
    // 0x103eb020 is answered yes, and when it is dispatched the scratch list of the frame (sp-0x130+0x5c) is
    // given one node whose row 0x103eb440's answer points at; when 0x103eb440 is dispatched (the node read)
    // the list is emptied again, so its destruction never frees the scratch node. (Answering the list's
    // destruction, 0x10738c20, is no use: the original's page runs it in place, past the answers.)
    if (!lists.empty()) {
        const auto sp = std::uint32_t(scenario.s.r<4>(4));  // r1 (the register file holds r_n at 4n)
        const auto business = sp - 0x130 + 0x5c;
        constexpr std::uint32_t node_at = area + 0x700, data_at = area + 0x740, entry_at = area + 0x760, record_at = area + 0x780;
        for (unsigned n = 0; n < 12; ++n) {
            Builder b;
            auto call = pick_call(b, 0, 0);
            call.fixed = 0;
            const auto& list = lists[rng.below(unsigned(lists.size()))];
            const auto listed = list.people[rng.below(unsigned(list.people.size()))];
            if (!scenario.r32(scenario.person(listed) + 0x24)) continue;
            call.club_record = scenario.club(world.club_of_person(listed));
            for (const auto person : list.people) b.put(1, scenario.person(person) + 0x62, 11);
            for (std::uint32_t at = 0; at < 0x54; at += 4) b.put(4, earlier_at + at, rng.next());
            b.put(1, earlier_at + 0x24, 0);
            b.put(4, earlier_at + 4, list.header);
            b.answer(club_human, 1);
            b.answer(earlier_offer, earlier_at);
            b.answer(contract_pending, 0);
            b.answer(transfer_a, 0);
            b.answer(transfer_b, 1);
            b.answer(0x103eb440, entry_at);
            b.put(4, node_at, data_at);
            b.put(2, node_at + 4, 4);
            b.put(4, node_at + 8, 0);
            b.put(4, node_at + 12, 0);
            b.put(4, data_at, rng.below(100));
            b.put(4, entry_at, record_at);
            b.put(4, record_at + 0xc, n % 3 ? call.player : call.player + 1);
            b.item.pokes.push_back({transfer_b, {{2, business, 1}, {4, business + 4, node_at}, {4, business + 8, node_at},
                                                 {4, business + 0xc, 0}}});
            b.item.pokes.push_back({0x103eb440, {{2, business, 0}, {4, business + 4, 0}, {4, business + 8, 0},
                                                 {4, business + 0xc, 0}}});
            cases.push_back(make(std::move(b), call, "build exchange business " + text(n)));
        }
    }

    // ---- branch outcomes closed one by one (the coverage of 2026-11-13). A scripted sample table: the cursor
    // at the table's first entry, the base 0, the next entries the values given (then `fill`), so the
    // routine's first draws are known; the paths below make no draw before the ones scripted. ----
    const auto script = [&](Builder& b, std::initializer_list<std::uint32_t> values, std::uint32_t fill) {
        if (last_sample <= first_sample) return;
        b.put(4, scenario.toc(-0x7b84), first_sample);
        b.put(4, scenario.toc(-0x7b90), 0);
        unsigned k = 1;
        for (const auto value : values) if (first_sample + 2 * k <= last_sample) b.put(2, first_sample + 2 * k++, value);
        for (; k <= 64 && first_sample + 2 * k <= last_sample; ++k) b.put(2, first_sample + 2 * k, fill);
    };
    // A free agent (no contract: no seller, so no finance draw) for a real player row of `category` (no chance
    // draw from 4 up); the refusal answered no.
    const auto free_agent = [&](Builder& b, std::uint32_t category) {
        Call call;
        call.player = some(world.free_agents);
        call.player_record = scenario.person(call.player);
        const auto wanted = some_row();
        call.row = wanted.row;
        call.club_record = scenario.club(wanted.club);
        call.with_deal = rng.chance(50);
        write_offer_area(b, rng);
        b.put(1, call.row + 0x28, category << 3);
        b.answer(refuses, 1);
        b.answer(unwanted, 0);
        return call;
    };
    const auto nation_of = [&](const Call& call) { return scenario.r32(call.player_record + 0xc); };
    const auto push = [&](Builder&& b, const Call& call, const std::string& what) {
        cases.push_back(make(std::move(b), call, "build close " + what));
    };
    for (unsigned n = 0; n < 4; ++n) {
        // A player row for a person without a player record: no chance mark, no offer (0x1041306c).
        Builder b;
        auto call = free_agent(b, n % 2 ? 1 : 5);
        if (world.bare.empty()) break;
        call.player = world.bare[rng.below(unsigned(world.bare.size()))];
        call.player_record = scenario.person(call.player);
        push(std::move(b), call, "bare player " + text(n));
    }
    for (unsigned n = 0; n < 6; ++n) {
        // A free agent of world reputation 9000 for a row of category 0-3: the chance draw passes and finds no
        // club (0x1041286c); the price is his own, no seller, so no seller ceiling (0x10413170, 0x1041336c).
        Builder b;
        auto call = free_agent(b, n % 4);
        b.put(2, scenario.r32(call.player_record + 0x14) + 0xa, 9000);
        b.answer(value_to, rng.pick<std::uint32_t>({400000, 2000000}));
        b.answer(ceiling, rng.pick<std::uint32_t>({300000, 3000000}));
        call.budget = rng.pick<std::int32_t>({-1, 1000000});
        push(std::move(b), call, "free agent " + text(n));
    }
    for (unsigned n = 0; n < 4; ++n) {
        // The seller's finances not found (0x10103510 answered 0): the ceiling stays (0x10413248).
        Builder b;
        auto call = pick_call(b, 0, 0);
        call.row = some_row().row;
        b.put(1, call.row + 0x28, 5 << 3);
        b.answer(refuses, 1);
        b.answer(unwanted, 0);
        b.answer(finances_of, 0);
        b.answer(finance_state, 2);
        b.answer(balance, 0);
        push(std::move(b), call, "no finances " + text(n));
    }
    for (unsigned n = 0; n < 4; ++n) {
        // A barred computer seller asking less than both the ceiling and the player's value (0x1041334c).
        Builder b;
        auto call = pick_call(b, 0, 0);
        call.player = some(world.employed);
        call.player_record = scenario.person(call.player);
        call.row = some_row().row;
        b.put(1, call.row + 0x28, 5 << 3);
        b.answer(refuses, 1);
        b.answer(unwanted, 0);
        for (std::uint32_t at = 0x160; at < 0x168; at += 4) b.put(4, books_at + at, 0);
        b.put(1, books_at + 0x165, 1);
        b.answer(finances_of, books_at);
        b.answer(finance_state, 2);
        b.answer(balance, 0);
        b.answer(value_to, 100000);
        b.answer(ceiling, 5000000);
        b.put(4, call.player_record + 8, 3000000);
        push(std::move(b), call, "barred low price " + text(n));
    }
    if (!world.human_clubs.empty())
        for (unsigned n = 0; n < 6; ++n) {
            // A human seller who keeps the player: his floor (the larger of a third and 1,500,000 less than his
            // 3,000,000) against a fee cut to the release clause (1,000,000: the floor stays over the fee;
            // 2,000,000: the floor under it), the budget under the fee.
            Builder b;
            auto call = pick_call(b, 0, 0);
            call.row = some_row().row;
            b.put(1, call.row + 0x28, 5 << 3);
            b.answer(refuses, 1);
            b.answer(unwanted, 0);
            b.put(4, contract_at + 4, world.human_clubs[n % world.human_clubs.size()]);
            b.put(4, contract_at + 0x24, n % 2 ? 2000000 : 1000000);
            b.put(1, contract_at + 0x38, 0x04);
            b.answer(contract_of, contract_at);
            b.answer(release_clause, 1);
            b.answer(human_asking, 3000000);
            b.answer(keeps_player, 1);
            b.answer(ceiling, 1200000);
            b.answer(finance_state, 2);
            call.budget = n % 2 ? 1800000 : 500000;
            push(std::move(b), call, "human floor " + text(n));
        }
    for (unsigned n = 0; n < 4; ++n) {
        // The seller asks between 1.2 and 1.4 times the budget, with a valuation gap: negotiated (0x1041389c
        // from 0x10413898).
        Builder b;
        auto call = free_agent(b, 5);
        b.item.nth_answers.push_back({value_to, 1, 1300000});
        b.item.nth_answers.push_back({value_to, 2, 1000000});
        b.answer(ceiling, 2000000);
        call.budget = 1000000;
        push(std::move(b), call, "gap a little " + text(n));
    }
    if (!world.other_rows.empty())
        for (unsigned n = 0; n < 4; ++n) {
            // A row of another kind that bids: price 0, the funds checked, no reshaping (0x10415b14).
            Builder b;
            auto call = pick_call(b, 0, 0);
            call.row = world.other_rows[rng.below(unsigned(world.other_rows.size()))].row;
            b.answer(refuses, 1);
            b.answer(unwanted, 0);
            b.answer(release_clause, 0);
            call.budget = -1;
            push(std::move(b), call, "other row funds " + text(n));
        }
    for (unsigned n = 0; n < 6; ++n) {
        // A deal signed without a fee: the role rule says no (0x10415bbc), or the player has no contract
        // (a free agent, 0x10415bc4).
        Builder b;
        auto call = n % 2 ? free_agent(b, 5) : pick_call(b, 0, 0);
        if (n % 2 == 0 && !world.other_rows.empty()) call.row = world.other_rows[rng.below(unsigned(world.other_rows.size()))].row;
        std::erase_if(b.item.answers, [](const auto& answer) { return answer.first == refuses; });
        b.answer(refuses, 0);
        b.answer(sign_free, 1);
        b.answer(role_rule, n % 2 ? 1 : 0);
        // A free agent may still have a contract record: one of his first lookups answered none.
        if (n % 2) b.item.nth_answers.push_back({contract_of, n / 2 + 1, 0});
        call.with_deal = true;
        push(std::move(b), call, "deal role " + text(n));
    }
    for (unsigned n = 0; n < 4; ++n) {
        // The deal record's allocation answered null (its first allocation): the null record is signed for.
        Builder b;
        auto call = free_agent(b, 5);
        std::erase_if(b.item.answers, [](const auto& answer) { return answer.first == refuses; });
        b.answer(refuses, 0);
        b.answer(sign_free, n % 2);
        b.item.nth_answers.push_back({0x10773520, 1, 0});
        call.with_deal = true;
        push(std::move(b), call, "deal null " + text(n));
    }
    // A free agent asking a little more than the budget (small gap, no valuation gap): the appearance extra's
    // nation tests, then the random terms with scripted draws.
    const auto small_gap = [&](Builder& b, std::uint32_t category, std::int32_t asking, std::int32_t budget) {
        auto call = free_agent(b, category);
        b.answer(value_to, std::uint32_t(asking));
        b.answer(ceiling, std::uint32_t(asking) * 2);
        call.budget = budget;
        return call;
    };
    for (unsigned n = 0; n < 8; ++n) {
        // No nation (0x1041422c), a nation of 5000 or less (0x104141dc), a role flag of 5 or more (0x104141e8),
        // a reputed young player with a role flag 1-4 (0x104142b4: the 5-15% extra).
        Builder b;
        auto call = small_gap(b, 5, 1300000, 1000000);
        const auto nation = nation_of(call);
        switch (n % 4) {
            case 0:
                // Without a nation the approach (0x103f8820, through SPAIN_RULES) would fault: answered.
                b.put(4, call.player_record + 0xc, 0);
                b.answer(approach, 1);
                b.answer(pending_offer, 0);
                break;
            case 1: if (nation) b.put(2, nation + 0xac, 4000); break;
            case 2:
                if (nation) b.put(2, nation + 0xac, 6000);
                b.put(1, call.player_record + 0x5e, 5);
                break;
            default:
                reputed(b, call, true, 17);
                b.put(1, call.player_record + 0x5e, 2);
        }
        push(std::move(b), call, "small gap nation " + text(n));
    }
    for (unsigned n = 0; n < 6; ++n) {
        // Random terms with nothing chosen before the last stretch: category 4 (no first block), an old player
        // (no sell-on), then draws below the row's weight and 0.67 of its level, small or wide and negative
        // (0x104144c4..0x104145a8 with a negative bound).
        Builder b;
        auto call = small_gap(b, 4, 1300000, 1000000);
        if (const auto nation = nation_of(call)) b.put(2, nation + 0xac, 1000);
        b.put(1, call.player_record + 0x5d, 40);
        b.put(4, call.row + 0xc, rng.pick<std::uint32_t>({std::uint32_t(-500), std::uint32_t(-2000000), std::uint32_t(-0xffff)}));
        b.put(4, call.row + 4, rng.pick<std::uint32_t>({std::uint32_t(-900), std::uint32_t(-6000000), 3000000}));
        push(std::move(b), call, "negative bounds " + text(n));
    }
    {
        // The wide draw's sum clamped below its bound (0x104145a0): the fee 8 x 1,291,039 (the bound's share of
        // 0xffff ends in .7), the first draw 65534 (the largest place), the inner draw (0x10415e50 below 197)
        // 196 (the largest tenth): 1,291,019 + 20 > 1,291,038.
        Builder b;
        auto call = small_gap(b, 4, 13000000, 10328312);
        if (const auto nation = nation_of(call)) b.put(2, nation + 0xac, 1000);
        script(b, {65534, 196}, 1);
        push(std::move(b), call, "wide clamp");
    }
    for (unsigned n = 0; n < 4; ++n) {
        // The sell-on block with the first share the lower (5 against 25): category 5, the first draw below an
        // eighth of the fee 7, the odd test even, the second 7, the shares' draws 0 and 4.
        Builder b;
        auto call = small_gap(b, 5, 500000, 400000);
        if (const auto nation = nation_of(call)) b.put(2, nation + 0xac, 1000);
        script(b, {7, 2, 7, n % 2 ? 0u : 4u, n % 2 ? 4u : 0u}, 1 + n);
        push(std::move(b), call, "sell-on order " + text(n));
    }
    for (unsigned n = 0; n < 8; ++n) {
        // A ceiling at half the price: the counter offer of the modest split (role flag set, 0x10415098), and
        // of a reputed player's (nation 6000, 0x104150a0).
        Builder b;
        auto call = free_agent(b, 5);
        if (n % 2) reputed(b, call, n % 4 == 1, 22);
        else if (n % 4 == 0) b.put(1, call.player_record + 0x5e, 1);
        else if (const auto nation = nation_of(call)) b.put(2, nation + 0xac, 3000);
        b.answer(value_to, 1000000);
        b.answer(ceiling, 500000);
        call.budget = -1;
        push(std::move(b), call, "counter split " + text(n));
    }
    for (unsigned n = 0; n < 6; ++n) {
        // The stretch toward the budget on an odd draw with the ratio at its floor (weight 1 over level
        // 1,000,000): 20 tens, clamped to 100, rounded over 50 (0x104158dc); a level of 0 skips it (0x10415788).
        Builder b;
        auto call = free_agent(b, 5);
        b.put(4, call.row + 4, n == 5 ? 0 : 1000000);
        b.put(4, call.row + 0xc, n % 2 ? 1 : 30000);
        b.answer(value_to, 1000000);
        b.answer(ceiling, 780000);
        call.budget = 2000000;
        script(b, {}, n % 3 ? 1 : 3);
        push(std::move(b), call, "stretch odd " + text(n));
    }
    for (unsigned n = 0; n < 12; ++n) {
        // A price of 8000 with a valuation gap far over it: the instalments capped with the price under 10,000
        // (0x10413c08), by the months' bound or by the share of the gap (a wrapped product).
        Builder b;
        auto call = free_agent(b, 5);
        if (n % 2) reputed(b, call, true, 22);
        b.item.nth_answers.push_back({value_to, 1, 8000});
        b.item.nth_answers.push_back({value_to, 2, rng.pick<std::uint32_t>({std::uint32_t(-5000000), std::uint32_t(-40000000),
                                                                             std::uint32_t(-900000000), std::uint32_t(-2000000000)})});
        b.answer(ceiling, 20000);
        call.budget = 5000;
        place_samples(b);
        push(std::move(b), call, "cap small price " + text(n));
    }
    for (unsigned n = 0; n < 6; ++n) {
        // A free agent reputed below his nation but with a birth club: the birth-club rule (0x100ce250, its
        // second dispatch after the buyer's 0x1014c2e0) answered yes or no.
        Builder b;
        auto call = free_agent(b, 5);
        reputed(b, call, false, 22);
        b.put(4, call.player_record + 0x20, 0x12345678);
        b.item.nth_answers.push_back({0x100ce250, 2, n % 2});
        b.answer(value_to, 6000000);
        b.answer(ceiling, 8000000);
        call.budget = 1000000;
        push(std::move(b), call, "birth rule " + text(n));
    }
    if (!lists.empty())
        for (unsigned n = 0; n < 9; ++n) {
            // An earlier offer whose first player in exchange is at the buyer: his contract looked up again
            // (the third 0x10082f20 of the run) is none, a kind-5 one, or he is of another role.
            Builder b;
            auto call = pick_call(b, 0, 0);
            call.fixed = 0;
            const auto& list = lists[n % lists.size()];
            const auto first = list.people.front();
            if (!scenario.r32(scenario.person(first) + 0x24)) continue;
            call.club_record = scenario.club(world.club_of_person(first));
            call.row = some_row().row;
            b.put(1, call.row + 0x28, 5 << 3);
            for (std::uint32_t at = 0; at < 0x54; at += 4) b.put(4, earlier_at + at, rng.next());
            b.put(1, earlier_at + 0x24, 0);
            b.put(4, earlier_at + 4, list.header);
            b.answer(club_human, 1);
            b.answer(earlier_offer, earlier_at);
            b.answer(contract_pending, 0);
            b.answer(transfer_a, 0);
            b.answer(transfer_b, 0);
            switch (n % 3) {
                case 0: b.item.nth_answers.push_back({contract_of, 3, 0}); break;
                case 1:
                    b.put(4, contract_at + 4, world.club_of_person(first));
                    b.put(1, contract_at + 0x38, 0x14);
                    b.item.nth_answers.push_back({contract_of, 3, contract_at});
                    break;
                default: b.put(1, scenario.person(first) + 0x62, 7);
            }
            push(std::move(b), call, "exchange filter " + text(n));
        }
}
}
