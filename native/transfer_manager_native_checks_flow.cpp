// Cases for transfer_manager_native_flow*.cpp: the offer rounds (0x103e4740, 0x103ee1b0, 0x103e4d70, 0x103e5810)
// and the candidate offer (0x103eec00). The career holds a handful of plain offers, so the cases rebuild each
// real offer with the fields the listings test (people, clubs, kinds, lists made from other offers' heap blocks,
// a contract and a record in the scratch area) and answer the routines of other modules that decide the
// branches, from a fixed pseudo-random sequence.
#include "transfer_manager_native_checks.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t toc_base = 0x108bba90;
using Writes = std::vector<Write>;
std::string tag(const char* what, unsigned a, unsigned b = 0) { return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b); }

struct Rng {
    std::uint32_t state;
    std::uint32_t next() { return state = state * 1664525u + 1013904223u; }
    std::uint32_t below(std::uint32_t n) { return (next() >> 8) % n; }
    bool chance(std::uint32_t percent) { return below(100) < percent; }
    template <typename T> const T& pick(const std::vector<T>& v) { return v[below(std::uint32_t(v.size()))]; }
};

// A heap list (header {halfword count, head, tail, cursor}; nodes {data, halfword size, next, prev}) of items
// {word person index, byte, byte} built from real offers' blocks, because the game frees what it unlinks and a
// scratch address would not be a heap block.
struct Items {
    struct Item { std::uint32_t index; std::uint32_t flag4, flag5; };
};
std::uint32_t heap_list(const std::vector<std::uint32_t>& blocks, std::size_t& at, const std::vector<Items::Item>& items, Writes& writes) {
    if (at + 1 + 2 * items.size() > blocks.size()) return 0;
    const auto header = blocks[at++];
    std::vector<std::uint32_t> nodes;
    for (std::size_t k = 0; k < items.size(); ++k) nodes.push_back(blocks[at++]);
    for (std::size_t k = 0; k < items.size(); ++k) {
        const auto data = blocks[at++];
        writes.push_back({4, data, items[k].index});
        writes.push_back({1, data + 4, items[k].flag4});
        writes.push_back({1, data + 5, items[k].flag5});
        writes.push_back({4, nodes[k], data});
        writes.push_back({2, nodes[k] + 4, 8});
        writes.push_back({4, nodes[k] + 8, k + 1 < nodes.size() ? nodes[k + 1] : 0});
        writes.push_back({4, nodes[k] + 0xc, k ? nodes[k - 1] : 0});
    }
    writes.push_back({2, header, std::uint32_t(items.size())});
    writes.push_back({4, header + 4, nodes.empty() ? 0 : nodes.front()});
    writes.push_back({4, header + 8, nodes.empty() ? 0 : nodes.back()});
    writes.push_back({4, header + 0xc, 0});
    return header;
}
// The same in the scratch area, for lists nothing frees.
std::uint32_t scratch_list(std::uint32_t at, const std::vector<Items::Item>& items, Writes& writes) {
    const auto header = at, nodes = at + 0x10, data = nodes + 0x10 * std::uint32_t(items.size());
    for (std::size_t k = 0; k < items.size(); ++k) {
        const auto node = nodes + 0x10 * std::uint32_t(k), record = data + 8 * std::uint32_t(k);
        writes.push_back({4, record, items[k].index});
        writes.push_back({1, record + 4, items[k].flag4});
        writes.push_back({1, record + 5, items[k].flag5});
        writes.push_back({4, node, record});
        writes.push_back({2, node + 4, 8});
        writes.push_back({4, node + 8, k + 1 < items.size() ? node + 0x10 : 0});
        writes.push_back({4, node + 0xc, k ? node - 0x10 : 0});
    }
    writes.push_back({2, header, std::uint32_t(items.size())});
    writes.push_back({4, header + 4, items.empty() ? 0 : nodes});
    writes.push_back({4, header + 8, items.empty() ? 0 : nodes + 0x10 * std::uint32_t(items.size() - 1)});
    writes.push_back({4, header + 0xc, 0});
    return header;
}

constexpr std::uint32_t rating_area = scratch_base + 0x2000, contract_area = scratch_base + 0x1000, record_area = scratch_base + 0x3000, pair_area = scratch_base + 0x4000,
                        row_area = scratch_base + 0x4800, list_area = scratch_base + 0x8000, second_list_area = scratch_base + 0xa000;

struct World {
    const Scenario& scenario;
    std::uint32_t manager, people, clubs;
    std::vector<std::uint32_t> offers;        // real offers (addresses)
    std::vector<std::uint32_t> person_choices, club_choices;
};

// One randomised variant of an offer round: the writes (offer, person, scratch contract, lists) and the answers.
struct Variant {
    Writes memory;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
    std::uint32_t person_index = 0, club_index = 0;
};

Variant make_variant(const World& world, Rng& rng, std::size_t offer_index, bool heap_lists, bool scratch_lists_only_first = false) {
    (void)scratch_lists_only_first;
    Variant v;
    const auto offer = world.offers[offer_index];
    const auto& scenario = world.scenario;
    const auto person_index = rng.chance(60) ? scenario.r32(offer + 0xc) : rng.pick(world.person_choices);
    const auto club_index = rng.chance(60) ? scenario.r32(offer + 8) : rng.pick(world.club_choices);
    v.person_index = person_index;
    v.club_index = club_index;
    const auto person = scenario.person(person_index);
    auto& m = v.memory;
    m.push_back({4, offer + 0xc, person_index});
    m.push_back({4, offer + 8, club_index});
    m.push_back({1, offer + 0x24, rng.chance(35) ? 1u : rng.pick<std::uint32_t>({0, 2})});
    m.push_back({1, offer + 0x11, rng.chance(50) ? 0xbu : rng.pick<std::uint32_t>({0, 1, 5, 0xa, 0xc})});
    m.push_back({4, offer + 0x30, rng.chance(15) ? 0xffffffffu : rng.below(30)});
    m.push_back({4, offer + 0x28, rng.below(world.offers.size() + 2)});
    m.push_back({4, offer + 0x20, rng.pick<std::uint32_t>({0, 1000, 250000, 4000000})});
    m.push_back({1, offer + 0x3c, rng.pick<std::uint32_t>({0x00, 0x10, 0x20, 0x30, 0x1f})});
    m.push_back({1, offer + 0x38, rng.pick<std::uint32_t>({0x00, 0x50, 0xa0, 0xf5})});
    m.push_back({1, offer + 0x44, rng.pick<std::uint32_t>({0, 1})});
    // The person's record of ratings (read by 0x10117490: bytes +0xf..+0x19) in the scratch area, or none.
    for (std::uint32_t at = 0xf; at <= 0x19; ++at) m.push_back({1, rating_area + at, rng.below(21)});
    m.push_back({4, person + 0x14, rng.chance(85) ? rating_area : 0});
    m.push_back({4, person + 8, rng.pick<std::uint32_t>({0, 50000, 900000, 5000000, 40000000})});
    m.push_back({1, person + 0x63, rng.pick<std::uint32_t>({10, 16, 20, 25})});
    m.push_back({1, person + 0x65, rng.pick<std::uint32_t>({0, 1, 3, 5, 12, 16, 20, 30})});
    m.push_back({1, person + 0x67, rng.pick<std::uint32_t>({0, 5, 100})});
    m.push_back({1, person + 0x69, rng.pick<std::uint32_t>({0xff, 0, 1, 3, 9, 0x7f})});
    m.push_back({2, person + 0x5c, rng.pick<std::uint32_t>({10, 0x14, 0x16, 0x17, 40})});
    // A scratch contract (kind byte +0x38, club at +4, flags at +0x50) and the pairs of rows the answers may use.
    for (unsigned k = 0; k < 4; ++k) {
        const auto contract = contract_area + k * 0x100;
        m.push_back({4, contract + 4, rng.pick(world.club_choices)});
        m.push_back({1, contract + 0x38, rng.pick<std::uint32_t>({0x14, 0x15, 0x00, 0x40, 0x80})});
        m.push_back({4, contract + 0x50, rng.pick<std::uint32_t>({0, 1, 0x100, 0x101, 4, 8, 0x10, 0x14})});
        m.push_back({4, contract + 0xc, rng.pick<std::uint32_t>({0, 100, 100000, 5000000})});
    }
    for (unsigned k = 0; k < 3; ++k) {
        const auto record = record_area + k * 0x100;
        m.push_back({1, record, rng.pick<std::uint32_t>({0xb, 1, 0})});
    }
    // Lists: the people of the offer (+4) and the other offers it competes with (+0).
    std::vector<std::uint32_t> blocks;
    for (std::size_t k = 1; k <= world.offers.size(); ++k) {
        const auto other = world.offers[(offer_index + k) % world.offers.size()];
        if (other != offer) blocks.push_back(other);
    }
    std::size_t at = 0;
    const auto count_people = rng.below(4);
    std::vector<Items::Item> people_items;
    for (unsigned k = 0; k < count_people; ++k)
        people_items.push_back({rng.pick(world.person_choices), rng.below(3), rng.pick<std::uint32_t>({0, 1, 2, 5})});
    const auto count_bids = rng.below(3);
    std::vector<Items::Item> bid_items;
    for (unsigned k = 0; k < count_bids; ++k) bid_items.push_back({rng.below(std::uint32_t(world.offers.size() + 1)), 0, 0});
    if (people_items.empty() || rng.chance(15)) {
        m.push_back({4, offer + 4, 0});
    } else {
        const auto list = heap_lists ? heap_list(blocks, at, people_items, m) : scratch_list(list_area, people_items, m);
        m.push_back({4, offer + 4, list});
    }
    if (bid_items.empty() || rng.chance(15)) {
        m.push_back({4, offer, 0});
    } else {
        const auto list = heap_lists ? heap_list(blocks, at, bid_items, m) : scratch_list(second_list_area, bid_items, m);
        m.push_back({4, offer, list});
    }
    // The shared pair list the settle routine looks its row up in: one row naming the person and club.
    {
        const auto row = row_area;
        m.push_back({4, row, scenario.r32(person)});
        m.push_back({4, row + 4, scenario.r32(scenario.club(club_index))});
        m.push_back({4, row + 0xc, rng.pick<std::uint32_t>({0, 50, 1000, 100000, 3000000})});
        m.push_back({1, row + 0x1c, rng.below(5)});
        m.push_back({1, row + 0x1d, rng.below(5)});
        const auto today = scenario.global(-0x7b80), today_high = today >> 16, today_low = today & 0xffff;
        m.push_back({2, row + 0x30, today_high + rng.pick<std::uint32_t>({0, 0, 1, 0xffff})});
        m.push_back({2, row + 0x32, today_low + rng.pick<std::uint32_t>({0xffff, 0xfffd, 0, 1, 3, 10, 400})});
        m.push_back({1, row + 0x34, rng.below(20)});
        const auto pairs = scratch_list(pair_area, {}, m);
        (void)pairs;
        // A one-node list: header, node, then the row as the node's data.
        m.push_back({2, pair_area, 1});
        m.push_back({4, pair_area + 4, pair_area + 0x10});
        m.push_back({4, pair_area + 8, pair_area + 0x10});
        m.push_back({4, pair_area + 0xc, 0});
        m.push_back({4, pair_area + 0x10, row});
        m.push_back({2, pair_area + 0x14, 8});
        m.push_back({4, pair_area + 0x18, 0});
        m.push_back({4, pair_area + 0x1c, 0});
        if (rng.chance(70)) m.push_back({4, world.manager + 0xa78, pair_area});
    }
    // Answers of other modules' routines.
    const auto answer = [&](std::uint32_t entry, std::vector<std::uint32_t> values) {
        const auto value = rng.pick(values);
        if (const char* allowed = std::getenv("CM_FLOW_ANSWERS")) {
            char text[16];
            std::snprintf(text, sizeof text, "%x", entry);
            if (!std::strstr(allowed, text)) return;
        }
        v.answers.push_back({entry, value});
    };
    const auto some = [&](std::uint32_t entry, std::uint32_t percent_answered, std::vector<std::uint32_t> values) {
        if (rng.chance(percent_answered)) answer(entry, values);
    };
    some(0x1014c2e0, 60, {0, 1});
    some(0x102cdbf0, 60, {0, 1, 0x100});
    some(0x10082f20, 60, {0, contract_area, contract_area + 0x100, contract_area + 0x200, contract_area + 0x300});
    some(0x10082f90, 60, {0, contract_area + 0x100, contract_area + 0x200});
    some(0x1036d240, 60, {0, record_area, record_area + 0x100});
    some(0x1036d510, 40, {0, 1});
    some(0x10084be0, 60, {0, 100000, 300000, 1000000, 1500000, 3000000, 0xfffff000});
    some(0x10085860, 60, {0, 100000, 2000000, 0xfffff000});
    some(0x10416840, 60, {0, 100000, 400000, 3000000, 20000000, 0x80000000});
    some(0x10416710, 60, {0, 50000, 1000000, 5000000});
    some(0x10374f50, 60, {0, 100000, 6000000});
    some(0x100f2680, 60, {0, 10000, 1000000, 90000000, 0xffffff00});
    some(0x10361490, 50, {0, 3000, 6000000, 0x7fffffff});
    some(0x10773520, 12, {0});
    some(0x10364d00, 50, {0, 1});
    some(0x10147e70, 50, {0, 1});
    some(0x10149df0, 50, {0, 1});
    some(0x100c96e0, 40, {3, 9});
    some(0x100da0b0, 50, {0, 1});
    some(0x100da050, 50, {0, 1});
    some(0x100d3300, 50, {0, 1});
    some(0x1037fa70, 50, {0, 8, 0x10});
    some(0x10718130, 60, {0, 40, 500, 5000, 0xffffff00});
    some(0x10103510, 25, {0});
    some(0x103f4d10, 30, {0, 1});
    some(0x103fb390, 40, {0, 1});
    return v;
}
}

TRANSFER_MANAGER_NATIVE_CASES("flow") {
    if (scenario.match) return;
    World world{scenario, scenario.transfer_manager(), scenario.people(), 0, {}, {}, {}};
    const auto manager = world.manager;
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    if (size == 0 || size > 100000) return;
    for (std::uint32_t i = 0; i < size; ++i)
        if (const auto offer = scenario.r32(array + i * 4)) world.offers.push_back(offer);
    if (world.offers.size() < 8) return;
    for (const auto offer : world.offers) {
        world.person_choices.push_back(scenario.r32(offer + 0xc));
        world.club_choices.push_back(scenario.r32(offer + 8));
    }
    std::uint32_t highest_club = 0;
    for (const auto club : world.club_choices) highest_club = std::max(highest_club, club);
    for (unsigned k = 0; k < 24; ++k) {
        world.person_choices.push_back((k * 977 + 13) % world.people);
        world.club_choices.push_back((k * 53 + 7) % (highest_club + 1));
    }
    const auto slot_of = [&](std::size_t index) { return array + index * 4; };
    // The slots the routines take are positions in the manager's array; compute each real offer's position.
    std::vector<std::uint32_t> slots;
    for (std::uint32_t i = 0; i < size; ++i)
        if (scenario.r32(array + i * 4)) slots.push_back(array + i * 4);
    (void)slot_of;
    Rng rng{0x1234567u};
    const unsigned rounds = std::getenv("CM_FLOW_ROUNDS") ? unsigned(std::atoi(std::getenv("CM_FLOW_ROUNDS"))) : 40;
    const auto add_cases = [&](std::uint32_t entry, const char* name, std::initializer_list<std::uint32_t> flags, bool heap) {
        for (unsigned round = 0; round < rounds; ++round)
            for (std::size_t i = 0; i < slots.size(); ++i) {
                auto variant = make_variant(world, rng, i, heap);
                for (const auto flag : flags) {
                    Case c{.entry = entry, .registers = {{2, toc_base}, {3, manager}, {4, slots[i]}, {5, flag}}, .memory = variant.memory,
                           .label = tag(name, round * 100 + unsigned(i), flag), .answers = variant.answers};
                    cases.push_back(c);
                }
            }
    };
    // Plain real offers first (no rebuilding).
    for (const auto entry : {0x103e4740u, 0x103ee1b0u, 0x103e4d70u})
        for (std::size_t i = 0; i < slots.size(); ++i)
            for (const std::uint32_t flag : {0u, 1u})
                cases.push_back({.entry = entry, .registers = {{2, toc_base}, {3, manager}, {4, slots[i]}, {5, flag}}, .label = tag("real", unsigned(i), flag)});
    for (std::size_t i = 0; i < slots.size(); ++i)
        cases.push_back({.entry = 0x103e5810, .registers = {{2, toc_base}, {3, manager}, {4, slots[i]}}, .label = tag("real settle", unsigned(i))});
    add_cases(0x103e4740, "conclude", {0, 1, 0x100}, true);
    add_cases(0x103ee1b0, "review", {0, 1}, false);
    add_cases(0x103e4d70, "evaluate", {0, 1}, false);
    add_cases(0x103e5810, "settle", {0}, false);
    // 0x103e4d70 down the long chain (a record, an offered sum over the limit, a contract at the partner, enough days),
    // with the answers that vary the rating and the price drawn at random.
    for (unsigned round = 0; round < rounds * 3; ++round)
        for (std::size_t i = 0; i < slots.size(); ++i) {
            auto variant = make_variant(world, rng, i, false);
            const auto set = [&](std::uint32_t entry, std::uint32_t value) {
                for (auto& answer : variant.answers)
                    if (answer.first == entry) { answer.second = value; return; }
                variant.answers.push_back({entry, value});
            };
            const auto drop = [&](std::uint32_t entry) {
                for (auto it = variant.answers.begin(); it != variant.answers.end(); ++it)
                    if (it->first == entry) { variant.answers.erase(it); break; }
            };
            set(0x1036d240, record_area);
            set(0x10361490, rng.pick<std::uint32_t>({100000, 500000}));
            set(0x10416840, rng.pick<std::uint32_t>({1000000, 5000000, 9000000, 17000000}));
            set(0x10082f20, contract_area + 0x100 * rng.below(4));
            set(0x10082f90, contract_area + 0x100 * (1 + rng.below(3)));
            set(0x10718130, rng.pick<std::uint32_t>({5000, 90000}));
            set(0x100d3300, rng.chance(60) ? 1u : 0u);
            set(0x10084be0, rng.pick<std::uint32_t>({100000, 400000, 1500000, 3000000, 20000000}));
            drop(0x10773520);
            variant.memory.push_back({2, scenario.club(variant.club_index) + 0x1ec, rng.pick<std::uint32_t>({0, 100, 3000, 0x7ff0})});
            variant.memory.push_back({2, world.scenario.club(rng.pick(world.club_choices)) + 0x1ec, rng.pick<std::uint32_t>({0, 5000})});
            for (const std::uint32_t flag : {0u, 1u})
                cases.push_back({.entry = 0x103e4d70, .registers = {{2, toc_base}, {3, manager}, {4, slots[i]}, {5, flag}},
                                 .memory = variant.memory, .label = tag("evaluate chain", round * 100 + unsigned(i), flag),
                                 .answers = variant.answers});
        }
    // 0x103eec00: clubs and people spread, with the finance manager's account answered as missing for one set.
    for (unsigned k = 0; k < 12 && world.people; ++k) {
        const auto club = scenario.club(k * 7);
        const auto person = scenario.person((k * 131) % world.people);
        for (const bool missing : {false, true}) {
            Case entry{.entry = 0x103eec00,
                       .registers = {{2, toc_base}, {3, manager}, {4, club}, {5, person}, {6, scratch_base + 0x100}, {7, scratch_base + 0x200},
                                     {8, 100000u * (k + 1)}},
                       .label = tag("candidate", k, missing)};
            if (missing) entry.answers = {{0x10103510, 0}};
            cases.push_back(entry);
        }
    }
}
}
