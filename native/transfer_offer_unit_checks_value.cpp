// TRANSFER_OFFER.CPP checker cases for the value block (transfer_offer_unit_value.hpp): the draw below a
// limit (0x10415e50), the value of an offer (0x10416840), the comparison of two offers' terms
// (0x10416e10), taking a player out of the exchange (0x104170f0) and the reshaping of a fee (0x104172f0).
//
// Offers are built in the scratch area with real person and club indices; people, clubs and their records
// are the career's own. Exchange lists are the game's own list objects: the snapshot has no offer with one,
// so they are found in memory by their shape (a header of count, head, tail and cursor over nodes of
// data pointer, row size, next and previous, whose rows are 4-byte person indices) and are used as the
// exchange list of a scratch offer; a case may also write their row words (undone afterwards). The
// callees outside the unit run for real; some cases answer them (the contract finder, the employer's
// relation, the assessment) to reach every rung of the value's switch.
#include "transfer_offer_unit_checks.hpp"
#include <algorithm>
#include <cstdint>
#include <string>

namespace cm::aot::transfer_offer_unit_checks {
namespace {
constexpr std::uint32_t draw_below = 0x10415e50, offer_value = 0x10416840, same_terms = 0x10416e10,
                        remove_player = 0x104170f0, reshape = 0x104172f0;
constexpr std::uint32_t finder = 0x10082f20, relation = 0x100f55b0, assess = 0x10219e40;
constexpr std::uint32_t area = scratch_base + 0x8000, second_area = area + 0x100, found_area = area + 0x200;

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

struct Builder {
    Case item;
    void put(unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); }
    void answer(std::uint32_t entry, std::uint32_t value) { item.answers.push_back({entry, value}); }
};

// An offer's 0x54 bytes in the scratch area: every word written, then the terms.
struct Terms {
    std::uint32_t exchange = 0, buyer = 0, player = 0, fee = 0, instalments = 0, extra = 0;
    std::uint32_t kind = 0xb, kind_byte = 0, instalment_months = 0, extra_count = 0, sell_on = 0, months = 0;
};
void write_offer(Builder& b, std::uint32_t at, const Terms& t) {
    for (std::uint32_t word = 0; word < 0x54; word += 4) b.put(4, at + word, 0);
    b.put(4, at + 4, t.exchange);
    b.put(4, at + 8, t.buyer);
    b.put(4, at + 0xc, t.player);
    b.put(1, at + 0x11, t.kind);
    b.put(4, at + 0x20, t.fee);
    b.put(1, at + 0x38, t.kind_byte);
    b.put(4, at + 0x40, t.instalments);
    b.put(1, at + 0x44, t.instalment_months);
    b.put(4, at + 0x48, t.extra);
    b.put(1, at + 0x4c, t.extra_count);
    b.put(1, at + 0x50, t.sell_on);
    b.put(1, at + 0x51, t.months);
}

// A game list found in memory: its header, and its rows' addresses and person indices.
struct List {
    std::uint32_t header;
    std::vector<std::uint32_t> rows, people;
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
        for (std::uint64_t a = low; a + 16 <= low + size; a += 4) {
            const auto header = std::uint32_t(a);
            const std::int32_t count = std::int16_t(m.read<2>(header));
            if (count < 1 || count > 12) continue;
            const auto head = sc.r32(header + 4), tail = sc.r32(header + 8);
            if (!head || (head & 3) || !tail || (tail & 3) || !mapped(head) || !mapped(tail)) continue;
            List list{header, {}, {}};
            bool good = true;
            std::uint32_t node = head, previous = 0;
            while (node && list.rows.size() <= std::size_t(count)) {
                if (!mapped(node)) { good = false; break; }
                const auto data = sc.r32(node);
                if (!mapped(data) || sc.r32(node + 12) != previous || std::int16_t(m.read<2>(node + 4)) != 4 ||
                    sc.r32(data) >= sc.people()) { good = false; break; }
                list.rows.push_back(data);
                list.people.push_back(sc.r32(data));
                previous = node;
                node = sc.r32(node + 8);
            }
            if (good && node == 0 && previous == tail && list.rows.size() == std::size_t(count)) lists.push_back(std::move(list));
        }
    }
    return lists;
}

// People of the career grouped by what the value reads of them.
struct People {
    std::vector<unsigned> owned, free_agents, bare;  // a profile and an employer; a profile only; no profile
    explicit People(const Scenario& sc) {
        for (unsigned p = 0; p < sc.people(); p += 7) {
            const auto record = sc.person(p);
            if (!sc.r32(record + 0x14)) bare.push_back(p);
            else if (sc.r32(record + 0x24)) owned.push_back(p);
            else free_agents.push_back(p);
        }
    }
};

std::string text(std::int64_t value) { return std::to_string(value); }
}

TRANSFER_OFFER_UNIT_CASES("value") {
    if (scenario.match) return;
    Rng rng{0x7a11e5};
    const auto clubs = unsigned(scenario.global(-0x7b50));
    // ---- the draw below a limit (0x10415e50) ----
    {
        // The sample table's cursor and base cells, and its last entry: a cursor at the last entry (or one draw
        // before it) makes the first (second) draw of the call redraw the base from the generator.
        const auto cursor_cell = scenario.toc(-0x7b84), base_cell = scenario.toc(-0x7b90);
        const auto last = scenario.global(-0x7b88);
        const std::int32_t limits[] = {0, 1, -1, 2, 3, 7, -7, 100, -100, 0xfffe, 0xffff, -0xfffe, -0xffff, 0x10000, -0x10000,
                                       0x10001, -0x10001, 0x10010, 100000, -100000, 777777, 5000000, -5000000, 65535 * 10,
                                       214748364, -214748364, 0x7fffffff, std::int32_t(0x80000000u), 0x3fffffff, 0x20000000,
                                       0x1999999a, 0x19999999};
        for (const auto limit : limits)
            for (const unsigned place : {0u, 1u, 2u})  // 0: no redraw, 1: redraw at the first draw, 2: at the second
                for (const std::uint32_t base : {0u, 0x1234u, 0xffffu}) {
                    Builder b{.item = {.entry = draw_below, .registers = {{3, std::uint32_t(limit)}}}};
                    if (place) b.put(4, cursor_cell, last - (place == 1 ? 0u : 2u));
                    b.put(4, base_cell, base);
                    b.item.label = "draw " + text(limit) + " place " + text(place) + " base " + text(base);
                    cases.push_back(std::move(b.item));
                }
        // Walks of the cursor from the table's first entry for a run of calls' worth of samples.
        for (unsigned step = 0; step < 24; ++step) {
            Builder b{.item = {.entry = draw_below, .registers = {{3, std::uint32_t(0x12345 + step * 7777)}}}};
            b.put(4, cursor_cell, last - 2 * (step % 5));
            b.item.label = "draw walk " + text(step);
            cases.push_back(std::move(b.item));
        }
    }
    const auto lists = find_lists(scenario);
    const People people(scenario);
    if (!clubs || people.owned.empty() || people.free_agents.empty() || people.bare.empty() || lists.size() < 4) return;
    const auto some = [&](const std::vector<unsigned>& group) { return group[rng.below(unsigned(group.size()))]; };
    // ---- the value of an offer (0x10416840) ----
    for (unsigned n = 0; n < 420; ++n) {
        const bool answered = n % 3 != 0;  // two thirds answer the finder, relation and employer; the rest run for real
        unsigned person;
        switch (rng.below(10)) {
            case 0: person = some(people.bare); break;
            case 1: case 2: person = some(people.free_agents); break;
            default: person = some(people.owned);
        }
        const auto record = scenario.person(person);
        Terms t;
        t.player = person;
        t.buyer = rng.below(clubs);
        // The original reads the profile of a kind-0xb offer's player unchecked: only people with one.
        t.kind = rng.chance(75) && scenario.r32(record + 0x14) ? 0xb : rng.pick<std::uint32_t>({0, 3, 0x8b});
        t.kind_byte = rng.below(256);
        t.fee = rng.pick<std::uint32_t>({0, 1, 5000, 250000, 1000000, 5547205, 20000000, 0x7fffffff, 0x80000000u, 0xfffff000u, rng.below(30000000)});
        if (rng.chance(55)) t.instalments = rng.pick<std::uint32_t>({1, 4000, 500000, 3000000, 0x7fffffff, 0xffffff00u, rng.below(9000000)});
        t.instalment_months = rng.pick<std::uint32_t>({0, 1, 6, 12, 24, 0x7f, 0x80, 0xff, 0x100});
        if (rng.chance(60)) t.extra = rng.pick<std::uint32_t>({1, 100000, 750000, 4000000, 0x7fffffff, 0xfffff000u, rng.below(5000000)});
        t.extra_count = rng.pick<std::uint32_t>({0, 1, 3, 10, 20, 0x7f, 0x80, 0xff});
        if (rng.chance(55)) t.sell_on = rng.pick<std::uint32_t>({1, 5, 10, 20, 50, 0x7f, 0x80, 0xff, 0x101});
        t.months = rng.pick<std::uint32_t>({0, 1, 3, 6, 7, 9, 12, 13, 14, 24, 0x7f, 0x80, 0xff});
        if (rng.chance(35)) {
            const auto& list = lists[rng.below(unsigned(lists.size()))];
            t.exchange = list.header;
        }
        Builder b{.item = {.entry = offer_value}};
        write_offer(b, area, t);
        const auto flag = [&](unsigned zero_percent) {
            return rng.chance(zero_percent) ? rng.pick<std::uint32_t>({0, 0x100, 0xff00}) : rng.pick<std::uint32_t>({1, 0x7f, 0x80, 0xff, 0x101});
        };
        b.item.registers = {{3, area}, {4, flag(50)}, {5, flag(70)}, {6, flag(50)}, {7, flag(25)}, {8, flag(35)}};
        // The records the value reads, set to reach each rung: the person's age, whether settled, the other
        // club's rating, the person's value.
        if (rng.chance(60)) b.put(1, record + 0x5d, rng.pick<std::uint32_t>({16, 18, 20, 23, 24, 25, 27, 28, 29, 35, 0xf0}));
        if (rng.chance(60)) b.put(1, record + 0x5e, rng.chance(75) ? 0 : 1);
        if (rng.chance(40)) b.put(4, record + 8, rng.pick<std::uint32_t>({0, 1000, 250000, 5000000, 0x7fffffff}));
        if (const auto contract = scenario.r32(record + 0xc); contract && rng.chance(60))
            b.put(2, contract + 0xac, rng.pick<std::uint32_t>({0, 100, 1000, 2500, 0x7fff, 0x8000, 0xffff}));
        if (answered) {
            // A contract record of the scratch area: a real club and a class byte (5 is a free player).
            b.put(4, found_area + 4, rng.below(clubs));
            b.put(1, found_area + 0x38, rng.pick<std::uint32_t>({0, 4, 5, 0x14, 0x15, 0x16, 0x80, 0xff}));
            b.answer(finder, rng.chance(12) ? 0 : found_area);
            b.answer(relation, rng.pick<std::uint32_t>({0, 0, 0xff, 0xfe, 0xfd, 0x80, 1, 1, 2, 5, 0x7f, 0x1ff, 0x100}));
            if (rng.chance(60)) b.answer(assess, rng.pick<std::uint32_t>({0, 1, 5, 100, 1000, 20000, 0x7fff, 0xffff8000u, 0xffffff9c, 0x1ffff, 4500}));
        }
        b.item.label = std::string(answered ? "value answered " : "value real ") + text(n);
        cases.push_back(std::move(b.item));
    }
    // The extra's share and the sell-on share with chosen assessments (the first and second calls of 0x10219e40
    // answered apart): the share is 0.3 + gap / 750, times (1 - extra_count / 200), times (1 - |age - 24| / 15), and
    // is capped at 0.75: a gap past 337 at age 24 and no count reaches the cap; a gap of 290 does not.
    {
        std::vector<unsigned> with_contract;
        for (const auto p : people.owned) if (scenario.r32(scenario.person(p) + 0xc)) with_contract.push_back(p);
        if (with_contract.empty()) return;
        const std::int32_t pairs[][2] = {{100, 1000}, {100, 6}, {0, 1000}, {-5, 50}, {200, 7}, {300, 200}, {100, 200}, {32000, 32767},
                                         {-32768, -32768}, {5, 5}, {1000, 600}, {0, 0}};
        unsigned n = 0;
        for (const auto& pair : pairs)
            for (const std::uint32_t age : {24u, 20u, 27u, 28u, 40u})
                for (const std::uint32_t count : {0u, 10u, 0x7fu})
                    for (const std::uint32_t rating : {100u, 0u, 30000u}) {
                        if ((n++ % 3) != 0) continue;  // a third of the grid
                        const auto person = with_contract[rng.below(unsigned(with_contract.size()))];
                        const auto record = scenario.person(person);
                        Terms t;
                        t.player = person;
                        t.buyer = rng.below(clubs);
                        t.fee = rng.pick<std::uint32_t>({5000, 2000000});
                        t.extra = rng.pick<std::uint32_t>({1, 750000});
                        t.extra_count = count;
                        t.sell_on = rng.pick<std::uint32_t>({10, 40});
                        Builder b{.item = {.entry = offer_value}};
                        write_offer(b, area, t);
                        b.item.registers = {{3, area}, {4, 0}, {5, 1}, {6, rng.below(2)}, {7, 0}, {8, 1}};
                        b.put(1, record + 0x5d, age);
                        b.put(1, record + 0x5e, 0);
                        b.put(2, scenario.r32(record + 0xc) + 0xac, rating);
                        b.put(4, found_area + 4, rng.below(clubs));
                        b.put(1, found_area + 0x38, 4);
                        b.answer(finder, found_area);
                        b.item.nth_answers = {{assess, 1, std::uint32_t(pair[0]) & 0xffff}, {assess, 2, std::uint32_t(pair[1]) & 0xffff}};
                        b.item.label = "value share " + text(pair[0]) + "/" + text(pair[1]) + " age " + text(age) + " count " + text(count) +
                                       " rating " + text(rating);
                        cases.push_back(std::move(b.item));
                    }
    }
    // ---- two offers' terms (0x10416e10) ----
    for (unsigned n = 0; n < 200; ++n) {
        Terms t;
        t.buyer = rng.below(clubs);
        t.player = some(people.owned);
        t.fee = rng.pick<std::uint32_t>({0, 5000, 1000000});
        t.instalments = rng.pick<std::uint32_t>({0, 0, 4000, 0xffffff00u});
        t.instalment_months = rng.pick<std::uint32_t>({0, 12, 0x80});
        t.extra = rng.pick<std::uint32_t>({0, 0, 700000, 0xfffff000u});
        t.extra_count = rng.pick<std::uint32_t>({0, 10, 0xff});
        t.sell_on = rng.pick<std::uint32_t>({0, 0, 10, 0x80});
        t.months = rng.pick<std::uint32_t>({0, 6, 0xff});
        t.kind_byte = rng.pick<std::uint32_t>({0, 0x10, 0x18, 0x78, 0x80, 0xf0, 0xf8});
        auto other = t;
        // Differences in each term, alone or together.
        const auto vary = [&](std::uint32_t& value, std::initializer_list<std::uint32_t> others) {
            if (rng.chance(25)) value = rng.pick<std::uint32_t>(others);
        };
        vary(other.fee, {1, 5000, 0x80000000u});
        vary(other.instalments, {0, 1, 4000, 0xffffff00u, 0x7fffffff});
        vary(other.instalment_months, {0, 1, 12, 0x80, 0x100});
        vary(other.extra, {0, 1, 700000, 0xfffff000u});
        vary(other.extra_count, {0, 1, 10, 0xff});
        vary(other.sell_on, {0, 1, 10, 0x80, 0xff});
        vary(other.months, {0, 1, 6, 0xff});
        vary(other.kind_byte, {0, 0x10, 0x20, 0x78, 0x7f, 0x80});
        const auto& list = lists[rng.below(unsigned(lists.size()))];
        Builder b{.item = {.entry = same_terms}};
        switch (rng.below(6)) {
            case 0: break;                                                         // no exchange lists
            case 1: t.exchange = list.header; break;                               // only the first has one
            case 2: other.exchange = list.header; break;                           // only the other has one
            case 3: t.exchange = other.exchange = list.header; break;              // the same list for both
            default: {
                // Two different lists: equal counts made equal rows (shuffled), or just whatever they hold.
                const auto& second = lists[rng.below(unsigned(lists.size()))];
                if (second.header == list.header) break;
                t.exchange = list.header;
                other.exchange = second.header;
                if (second.rows.size() == list.rows.size() && rng.chance(60)) {
                    auto order = list.people;
                    for (std::size_t i = order.size(); i > 1; --i) std::swap(order[i - 1], order[rng.below(unsigned(i))]);
                    for (std::size_t i = 0; i < order.size(); ++i) b.put(4, second.rows[i], order[i]);
                    if (rng.chance(25)) b.put(4, second.rows[rng.below(unsigned(second.rows.size()))], rng.below(5000));
                }
            }
        }
        write_offer(b, area, t);
        write_offer(b, second_area, other);
        b.item.registers = {{3, area}, {4, second_area}};
        b.item.label = "terms " + text(n);
        cases.push_back(std::move(b.item));
    }
    // Identical terms, so the lists decide: two different lists of one length, one row of the second rewritten to
    // a person the first lacks (the walk of the second list ends without a match), or rewritten to match; and
    // lists of different lengths.
    for (unsigned n = 0; n < 40; ++n) {
        Terms t;
        t.buyer = rng.below(clubs);
        t.player = some(people.owned);
        t.fee = 5000;
        const auto& first = lists[rng.below(unsigned(lists.size()))];
        const auto& second = lists[rng.below(unsigned(lists.size()))];
        if (first.header == second.header) continue;
        t.exchange = first.header;
        auto other = t;
        other.exchange = second.header;
        Builder b{.item = {.entry = same_terms}};
        if (first.rows.size() == second.rows.size()) {
            for (std::size_t i = 0; i < second.rows.size(); ++i) b.put(4, second.rows[i], first.people[i]);
            if (n % 4 != 0) b.put(4, second.rows[rng.below(unsigned(second.rows.size()))], 0x7ffff + n);  // a person the first lacks
        }
        write_offer(b, area, t);
        write_offer(b, second_area, other);
        b.item.registers = {{3, area}, {4, second_area}};
        b.item.label = "terms lists " + text(n);
        cases.push_back(std::move(b.item));
    }
    // ---- a player out of the exchange (0x104170f0) ----
    for (unsigned n = 0; n < 90; ++n) {
        const auto& list = lists[rng.below(unsigned(lists.size()))];
        Terms t;
        t.buyer = rng.below(clubs);
        t.player = some(people.owned);
        t.fee = rng.pick<std::uint32_t>({0, 5000, 1000000, 0x7fffffff});
        t.exchange = rng.chance(8) ? 0 : list.header;
        Builder b{.item = {.entry = remove_player}};
        write_offer(b, area, t);
        // The leaving player: one of the list's rows (the first, last or any), a duplicated row, or a stranger.
        unsigned person;
        const auto how = rng.below(5);
        if (how == 0) person = some(people.owned);
        else {
            const auto at = how == 1 ? 0 : how == 2 ? list.rows.size() - 1 : rng.below(unsigned(list.rows.size()));
            person = list.people[at];
            if (how == 4 && list.rows.size() > 1) b.put(4, list.rows[(at + 1) % list.rows.size()], person);
        }
        if (scenario.r32(scenario.person(person)) != person) continue;  // the record's own index is the key
        b.item.registers = {{3, area}, {4, scenario.person(person)}};
        if (rng.chance(30)) b.answer(finder, rng.chance(20) ? 0 : found_area);
        if (b.item.answers.size()) {
            b.put(4, found_area + 4, rng.below(clubs));
            b.put(1, found_area + 0x38, rng.below(256));
        }
        b.item.label = "remove " + text(n);
        cases.push_back(std::move(b.item));
    }
    // The list emptied: every one-row list, its row's person leaving (the list is destroyed and the offer's pointer cleared).
    for (const auto& list : lists) {
        if (list.rows.size() != 1) continue;
        const auto person = list.people[0];
        if (scenario.r32(scenario.person(person)) != person) continue;
        Terms t;
        t.buyer = rng.below(clubs);
        t.player = some(people.owned);
        t.fee = 1000;
        t.exchange = list.header;
        Builder b{.item = {.entry = remove_player}};
        write_offer(b, area, t);
        b.item.registers = {{3, area}, {4, scenario.person(person)}};
        b.item.label = "remove destroy " + text(list.header);
        cases.push_back(std::move(b.item));
    }
    // ---- a fee reshaped by the clubs' standing (0x104172f0) ----
    for (unsigned n = 0; n < 360; ++n) {
        const auto person = rng.chance(10) ? some(people.bare) : rng.chance(15) ? some(people.free_agents) : some(people.owned);
        const auto record = scenario.person(person);
        const auto club_index = rng.below(clubs);
        const auto club = scenario.club(club_index);
        Terms t;
        t.buyer = club_index;
        t.player = person;
        t.fee = rng.pick<std::uint32_t>({0, 1, 2, 4000, 90000, 500000, 2500000, 12000000, 0x3fffffff, 0x40000000, 0x7fffffff, 0x80000000u,
                                         0xc0000000u, 0xfffffffeu, rng.below(40000000)});
        if (rng.chance(8)) t.sell_on = 5;
        if (rng.chance(8)) t.extra = 100;
        if (rng.chance(8)) t.instalments = 100;
        Builder b{.item = {.entry = reshape}};
        write_offer(b, area, t);
        b.item.registers = {{3, area}, {4, record}, {5, rng.next()}, {6, club}};
        // The standings: the profile's range (+4, +6) and value (+10), the reputation, the contract's rating.
        const auto profile = scenario.r32(record + 0x14);
        if (profile) {
            if (rng.chance(80)) { b.put(2, profile + 4, rng.below(20)); b.put(2, profile + 6, 30 + rng.below(40)); }
            else { b.put(2, profile + 4, rng.below(30)); b.put(2, profile + 6, rng.below(30)); }
            b.put(2, profile + 0xa, rng.pick<std::uint32_t>({0, 100, 499, 500, 2500, 4000, 4300, 4400, 5000, 7000, 0x7fff, 0x8000, 0xffff}));
        }
        b.put(2, club + 0x1ec, rng.pick<std::uint32_t>({0, 1, 5, 14, 15, 16, 100, 500, 1500, 5000, 0x7fff, 0x8000, 0xffff}));
        b.put(1, record + 0x5e, rng.chance(75) ? 0 : 1);
        if (const auto contract = scenario.r32(record + 0xc); contract && rng.chance(80))
            b.put(2, contract + 0xac, rng.pick<std::uint32_t>({0, 5000, 5001, 5002, 0x1388, 0x1389, 8000, 0x7fff, 0x8000}));
        b.item.label = "reshape " + text(n);
        cases.push_back(std::move(b.item));
    }
    // The profile cleared by the employer's funds call (0x100f5770), after the entry test read it: a contract above
    // 5000, a reputation of 15 or more (so the first path skips the profile), and then no profile for the second
    // path's test. The original reads the profile again unchecked at the third path (address 0xa), so these cases fault
    // alike on both sides: the outcome is reached, not matched past it.
    for (unsigned n = 0; n < 6; ++n) {
        const auto person = some(people.owned);
        const auto record = scenario.person(person);
        const auto contract = scenario.r32(record + 0xc);
        if (!contract) continue;
        Terms t;
        t.buyer = rng.below(clubs);
        t.player = person;
        t.fee = 2500000;
        Builder b{.item = {.entry = reshape}};
        write_offer(b, area, t);
        const auto club = scenario.club(rng.below(clubs));
        b.item.registers = {{3, area}, {4, record}, {5, 0}, {6, club}};
        const auto profile = scenario.r32(record + 0x14);
        b.put(2, profile + 4, 0);
        b.put(2, profile + 6, 50);
        b.put(2, club + 0x1ec, 100);
        b.put(1, record + 0x5e, 0);
        b.put(2, contract + 0xac, 6000);
        b.item.pokes = {{0x100f5770, {{4, record + 0x14, 0}}}};
        b.item.label = "reshape profile cleared " + text(n);
        cases.push_back(std::move(b.item));
    }
}
}
