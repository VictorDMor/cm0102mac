// Cases for transfer_manager_native_daily*.cpp: 0x103ea840, 0x103eb650, 0x103ec0f0, 0x103e9ce0, 0x103e8280, 0x103f3100.
// The career snapshot's offers are almost all closed (stage 0x13), so the cases write the fields the routines read
// (stage, phase bits, rival flag, list, club, person) into real offers; the callees outside the module that decide
// a branch (the club flag, the shortlist checks, the news desk) are answered where a case needs a given outcome.
#include "transfer_manager_native_checks.hpp"
#include "guest_abi.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <set>

namespace cm::aot::transfer_manager_native_checks {
namespace {
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
using Writes = std::vector<Write>;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
constexpr std::uint32_t toc_base = 0x108bba90;
constexpr std::uint32_t club_flag = 0x1014c2e0, shortlist_check = 0x1036f170, contract_exists = 0x1007f610, afford = 0x10080d30,
                        shortlist_row = 0x1036d240, news_check = 0x102cdbf0;
std::string label(const char* what, unsigned a, unsigned b = 0, unsigned c = 0, unsigned d = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c) + "/" + std::to_string(d);
}
Registers with_toc(Registers registers) {
    registers.push_back({2, toc_base});
    return registers;
}
void add(Writes& to, const Writes& more) { to.insert(to.end(), more.begin(), more.end()); }
// A guest list of people in the scratch area (header {count, head, tail, cursor}, nodes {record, size, next, prev},
// records of two words: the person's index and 0); nothing frees it.
std::uint32_t scratch_list(std::uint32_t at, const std::vector<std::uint32_t>& people, Writes& writes) {
    const auto nodes = at + 0x10, data = nodes + 0x10 * std::uint32_t(people.size());
    for (std::size_t k = 0; k < people.size(); ++k) {
        const auto node = nodes + 0x10 * std::uint32_t(k), record = data + 8 * std::uint32_t(k);
        writes.push_back({4, record, people[k]});
        writes.push_back({4, record + 4, 0});
        writes.push_back({4, node, record});
        writes.push_back({2, node + 4, 8});
        writes.push_back({4, node + 8, k + 1 < people.size() ? node + 0x10 : 0});
        writes.push_back({4, node + 0xc, k ? node - 0x10 : 0});
    }
    writes.push_back({2, at, std::uint32_t(people.size())});
    writes.push_back({4, at + 4, people.empty() ? 0 : nodes});
    writes.push_back({4, at + 8, people.empty() ? 0 : nodes + 0x10 * std::uint32_t(people.size() - 1)});
    writes.push_back({4, at + 0xc, 0});
    return at;
}
// A guest list built out of real offer blocks (the game frees what it unlinks, so scratch memory won't do for
// a list a routine copies): header {count, head, tail, cursor}, nodes {data, size, next, prev}, data of two words.
std::uint32_t heap_list(const std::vector<std::uint32_t>& blocks, std::size_t from, const std::vector<std::uint32_t>& people, Writes& writes) {
    std::size_t at = from;
    const auto next = [&] { return blocks[at++ % blocks.size()]; };
    const auto header = next();
    std::vector<std::uint32_t> nodes, data;
    for (std::size_t k = 0; k < people.size(); ++k) nodes.push_back(next());
    for (std::size_t k = 0; k < people.size(); ++k) data.push_back(next());
    for (std::size_t k = 0; k < people.size(); ++k) {
        writes.push_back({4, data[k], people[k]});
        writes.push_back({4, data[k] + 4, 0});
        writes.push_back({4, nodes[k], data[k]});
        writes.push_back({2, nodes[k] + 4, 8});
        writes.push_back({4, nodes[k] + 8, k + 1 < nodes.size() ? nodes[k + 1] : 0});
        writes.push_back({4, nodes[k] + 0xc, k ? nodes[k - 1] : 0});
    }
    writes.push_back({2, header, std::uint32_t(people.size())});
    writes.push_back({4, header + 4, nodes.empty() ? 0 : nodes.front()});
    writes.push_back({4, header + 8, nodes.empty() ? 0 : nodes.back()});
    writes.push_back({4, header + 0xc, 0});
    return header;
}
constexpr std::uint32_t stages[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xc, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x1c};
}

TRANSFER_MANAGER_NATIVE_CASES("daily") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto offers_count = scenario.r32(manager + 4);
    const auto array = scenario.r32(manager);
    const auto date_cell = scenario.toc(-0x7b80);
    const auto people = scenario.people();
    if (offers_count == 0 || offers_count > 64 || people == 0) return;
    const auto offer_at = [&](std::uint32_t i) { return scenario.r32(array + i * 4); };
    const auto person_of = [&](std::uint32_t k) { return (k * 9973u + 11u) % people; };
    const std::uint32_t picks[] = {0, 1, 4, 7, 12, 17, 20};
    const auto scratch_pointer = scratch_base + 0x300;
    const Writes pointer_byte{{1, scratch_pointer, 0x0b}, {1, scratch_pointer + 8, 5}};
    std::vector<std::uint32_t> spare_blocks;
    for (const std::uint32_t i : {2u, 3u, 5u, 6u, 8u, 9u, 10u, 11u, 13u, 14u, 15u, 16u, 18u, 19u})
        if (i < offers_count) spare_blocks.push_back(offer_at(i));
    if (spare_blocks.size() < 8) return;
    std::uint32_t list_serial = 0;
    const auto list_slot = [&] { return scratch_base + 0x10000 + 0x200 * (list_serial++ % 100); };

    // ---- 0x103eb650: one offer through each stage, with the answers that choose its branch. ----
    for (const auto i : picks) {
        const auto offer = offer_at(i);
        if (!offer) continue;
        const auto slot = array + i * 4;
        const auto own_person = scenario.r32(offer + 0xc);
        for (const auto stage : stages) {
            for (unsigned variant = 0; variant < 6; ++variant) {
                Writes writes{{1, offer + 0x34, stage}};
                const auto kind = variant & 1;
                writes.push_back({1, offer + 0x24, kind});
                const auto person = variant >= 2 ? person_of(i * 31 + variant) : own_person;
                writes.push_back({4, offer + 0xc, person});
                if (variant >= 4) writes.push_back({4, offer + 8, scenario.r32(offer_at((i + variant) % offers_count) + 8)});
                if (stage == 0xf) writes.push_back({1, offer + 0x36, variant & 2 ? 3u : 0u});
                if (stage == 0x11 || stage == 0x12) writes.push_back({1, offer + 0x10, variant & 1});
                if (stage == 5 || stage == 6 || stage == 7) {
                    // Candidates: the offer's own person and others; with the person's word 0 set so both compares occur.
                    if (variant & 1 || stage == 7) {
                        const auto list = heap_list(spare_blocks, (i * 7 + variant * 3) % spare_blocks.size(),
                                                    variant & 2 ? std::vector<std::uint32_t>{own_person, person_of(i + 3), person_of(i + 5)}
                                                                : std::vector<std::uint32_t>{person_of(i + 3), person_of(i + 5)}, writes);
                        writes.push_back({4, offer + 4, list});
                    }
                }
                if (stage == 7 && variant < 2) add(writes, {{4, offer + 4, 0}});
                std::vector<Answers> answer_sets;
                switch (stage) {
                    case 0:
                        for (const auto flag : {0u, 1u})
                            for (const auto exists : {0u, 1u})
                                for (const auto check : {0u, 1u})
                                    answer_sets.push_back({{club_flag, flag}, {contract_exists, exists}, {shortlist_check, check}, {shortlist_row, 0}});
                        answer_sets.push_back({{club_flag, 0}, {shortlist_check, 1}, {shortlist_row, scratch_pointer}});
                        break;
                    case 1: for (const auto flag : {0u, 1u}) answer_sets.push_back({{club_flag, flag}}); break;
                    case 5: case 6: case 7:
                        for (const auto flag : {0u, 1u})
                            for (const auto verdict : {0u, 1u})
                                for (const auto row : {0u, scratch_pointer})
                                    answer_sets.push_back({{club_flag, flag}, {afford, verdict}, {shortlist_row, row}});
                        break;
                    case 0x11: case 0x12:
                        for (const auto news : {0u, 1u})
                            for (const auto check : {0u, 1u, 2u, 3u, 0xffu})
                                answer_sets.push_back({{news_check, news}, {shortlist_check, check}});
                        break;
                    default: answer_sets.push_back({}); break;
                }
                for (std::size_t a = 0; a < answer_sets.size(); ++a) {
                    if (variant > 1 && a % 2) continue;
                    auto all = writes;
                    add(all, pointer_byte);
                    cases.push_back({.entry = 0x103eb650, .registers = with_toc({{3, manager}, {4, slot}}), .memory = all,
                                     .label = label("process offer", i, stage, variant, unsigned(a)), .answers = answer_sets[a]});
                }
            }
        }
    }

    // The stages that read the block test and the wait byte, with the club flag (outside the module) answered both ways.
    for (const auto i : picks) {
        const auto offer = offer_at(i);
        if (!offer) continue;
        for (const std::uint32_t stage : {1u, 4u, 5u, 6u, 0xfu, 0x13u, 0x11u, 0xa6u})
            for (const std::uint32_t blocked : {0u, 1u})
                for (const std::uint32_t wait : {0u, 3u}) {
                    cases.push_back({.entry = 0x103eb650, .registers = with_toc({{3, manager}, {4, array + i * 4}}),
                                     .memory = {{1, offer + 0x34, stage}, {1, offer + 0x36, wait}},
                                     .label = label("process offer stage with waits", i, stage, blocked, wait),
                                     .answers = {{club_flag, blocked}}});
                }
    }

    // ---- 0x103e8280: answered offers over clubs and people, both final answers. ----
    for (const auto i : picks) {
        const auto offer = offer_at(i);
        if (!offer) continue;
        for (unsigned club_variant = 0; club_variant < 4; ++club_variant)
            for (unsigned person_variant = 0; person_variant < 3; ++person_variant)
                for (unsigned shape = 0; shape < 3; ++shape) {
                    Writes writes;
                    if (club_variant) writes.push_back({4, offer + 8, scenario.r32(offer_at((i + club_variant * 5) % offers_count) + 8)});
                    if (person_variant) writes.push_back({4, offer + 0xc, person_of(i * 17 + person_variant)});
                    writes.push_back({1, offer + 0x37, shape == 1 ? 0u : 1u});
                    writes.push_back({1, offer + 0x38, shape == 2 ? 0x3fu : 0x0fu});
                    for (const std::uint32_t final_answer : {0u, 1u, 0x100u})
                        cases.push_back({.entry = 0x103e8280, .registers = with_toc({{3, manager}, {4, offer}, {5, final_answer}}), .memory = writes,
                                         .label = label("answered offer", i, club_variant * 10 + person_variant, shape, final_answer)});
                }
    }

    // The terms check and the steps after it run for real; the offers are fuzzed over the fields they read so that each
    // code of the terms check (0, 1, 2, others) and each club/person pairing comes up.
    std::uint32_t seed = 12345;
    const auto roll = [&](std::uint32_t n) { seed = seed * 1664525u + 1013904223u; return (seed >> 8) % n; };
    const std::uint32_t kinds38[] = {0x0f, 0x1f, 0x3f, 0x5f, 0x2f, 0x4f, 0x8f};
    const std::uint32_t amounts[] = {0, 1000, 250000, 5000000, 50000000, 0x7fffffffu};
    const auto some_contract_club = [&] {
        for (unsigned tries = 0; tries < 50; ++tries) {
            const auto contract = scenario.contract_of(scenario.person(roll(people)));
            if (contract && scenario.r32(contract + 4) < 100000) return scenario.r32(contract + 4);
        }
        return scenario.r32(offer_at(0) + 8);
    };
    const auto fuzz_writes = [&](std::uint32_t offer) {
        Writes w{{1, offer + 0x34, roll(2) ? 0x13u : stages[roll(sizeof stages / sizeof stages[0])]}, {1, offer + 0x37, roll(2)},
                 {1, offer + 0x38, roll(2) ? 0x0fu : kinds38[roll(7)]}, {1, offer + 0x11, roll(4) == 0 ? 5u : 0xbu},
                 {4, offer + 0x20, amounts[roll(6)]}, {4, offer + 0x28, roll(4)}, {1, offer + 0x24, roll(3)}, {1, offer + 0x10, roll(2)},
                 {4, offer + 8, roll(3) ? some_contract_club() : scenario.r32(offer_at(roll(offers_count)) + 8)}};
        const auto holder = roll(4);
        w.push_back({4, offer + 0xc, holder == 0 ? scenario.r32(offer_at(roll(offers_count)) + 0xc) : person_of(roll(100000))});
        if (holder == 3) {
            // A person of a club that holds the contract, offered to that same club (the contract row the checks look up exists).
            for (unsigned tries = 0; tries < 200; ++tries) {
                const auto index = roll(people);
                const auto contract = scenario.contract_of(scenario.person(index));
                if (!contract || scenario.r32(contract + 4) >= 100000) continue;
                w.push_back({4, offer + 0xc, index});
                if (roll(2)) w.push_back({4, offer + 8, scenario.r32(contract + 4)});
                break;
            }
        }
        return w;
    };
    // The contract lookups (outside the module) answered with a scratch contract whose club is the offer's or another.
    std::uint32_t scratch_contract = scratch_base + 0x700;  // a real contract record, found below
    for (std::uint32_t k = 0; k < people; ++k)
        if (const auto contract = scenario.contract_of(scenario.person(k)); contract && scenario.r32(contract + 4) < 100000) { scratch_contract = contract; break; }
    const auto contract_variant = [&](Writes& w, std::uint32_t offer, Answers& answers) {
        // The contract is real and untouched; the offer is at its club half the time.
        if (roll(2)) w.push_back({4, offer + 8, scenario.r32(scratch_contract + 4)});
        answers = {{0x10082f20, scratch_contract}};
        if (roll(2)) answers.push_back({0x1007f720, scratch_contract});
    };
    for (unsigned k = 0; k < 25000; ++k) {
        const auto offer = offer_at(roll(offers_count));
        auto writes = fuzz_writes(offer);
        Answers answers;
        if (k % 3 == 0) contract_variant(writes, offer, answers);
        cases.push_back({.entry = 0x103e8280, .registers = with_toc({{3, manager}, {4, offer}, {5, roll(3) == 0 ? 0u : 1u}}),
                         .memory = writes, .label = label("answered offer fuzz", k), .answers = answers});
    }

    // Answered: the terms check (0x103f9d60) steered with nth answers, run with --only limited to the daily entries so it is a
    // dispatched original on both sides. The first call returns 0 and the second 1 (the contract club accepts: the step taken
    // when it is not blocked, and the refusal when it is); the halfword the answer cannot set is poked.
    const auto entry_sp = guest::Cpu(scenario.s).sp();
    for (const auto i : picks) {
        const auto offer = offer_at(i);
        if (!offer) continue;
        for (const std::uint32_t second : {1u, 2u})
            for (const std::uint32_t half : {3u, 0u, 9u})
                for (const std::uint32_t blocked : {0u, 1u})
                    for (const std::uint32_t final_answer : {0u, 1u})
                        cases.push_back({.entry = 0x103e8280, .registers = with_toc({{3, manager}, {4, offer}, {5, final_answer}}),
                                         .memory = {{1, offer + 0x37, 1}, {1, offer + 0x38, 0x0f}},
                                         .label = label("answered: second terms code", i, second, half, blocked * 2 + final_answer),
                                         .answers = {{0x103f9d60, 0}, {0x103e8500, 0}, {0x103e9910, 0}, {club_flag, blocked}},
                                         .pokes = {{0x103f9d60, {{2, entry_sp - 0x60 + 0x38, half}}}},
                                         .nth_answers = {{0x103f9d60, 2, second}}});
    }
    // The same for the due rows: the first terms call 0, the second nonzero with a halfword other than 9.
    if (scenario.r32(manager + 0xa50) && scenario.r32(manager + 0xa58))
        for (const std::uint32_t second : {1u, 2u})
            for (const std::uint32_t half : {3u, 0x17u, 9u})
                for (const std::uint32_t first : {0u, 2u}) {
                    Writes writes{{2, date_cell, 0x7fff}, {2, date_cell + 2, 0x7d3}};
                    cases.push_back({.entry = 0x103f3100, .registers = with_toc({{3, manager}}), .memory = writes,
                                     .label = label("answered: due offers second terms code", second, half, first),
                                     .answers = {{0x103ed170, 1}, {0x103f9d60, first}, {0x103e9910, 0}, {0x103efaa0, 0}, {0x10407460, 0}, {0x103eb4a0, 0}},
                                     .pokes = {{0x103f9d60, {{2, entry_sp - 0xd0 + 0x38, half}}}},
                                     .nth_answers = {{0x103f9d60, 2, second}}});
                }

    // ---- 0x103e9ce0: the nations of the offers' clubs, with every offer in some stage. ----
    std::set<std::uint32_t> nations;
    for (std::uint32_t i = 0; i < offers_count; ++i) nations.insert(scenario.r32(scenario.club(scenario.r32(offer_at(i) + 8)) + 0x2c));
    for (std::uint32_t i = 0; i < offers_count; ++i)
        if (const auto contract = scenario.contract_of(scenario.person(scenario.r32(offer_at(i) + 0xc) % people)); contract && scenario.r32(contract + 4) < 100000)
            nations.insert(scenario.r32(scenario.club(scenario.r32(contract + 4)) + 0x2c));
    nations.insert(0x7fff);
    unsigned nation_count = 0;
    for (const auto nation : nations) {
        if (++nation_count > 6) break;
        for (unsigned pattern = 0; pattern < 40; ++pattern) {
            Writes writes;
            for (std::uint32_t i = 0; i < offers_count; ++i) {
                const auto offer = offer_at(i);
                const auto stage = stages[(i * 3 + pattern) % (sizeof stages / sizeof stages[0])];
                writes.push_back({1, offer + 0x34, stage});
                writes.push_back({1, offer + 0x38, (i + pattern) % 3 == 0 ? 0x0fu : (i + pattern) % 3 == 1 ? 0x3fu : 0x1fu});
                writes.push_back({1, offer + 0x10, (i + pattern / 2) & 1});
                if ((i + pattern) % 4 == 0) {
                    const auto list = scratch_list(list_slot(), {scenario.r32(offer + 0xc), person_of(i)}, writes);
                    writes.push_back({4, offer + 4, list});
                }
                if (pattern & 1) writes.push_back({4, offer + 0xc, person_of(i * 7 + pattern)});
            }
            cases.push_back({.entry = 0x103e9ce0, .registers = with_toc({{3, manager}, {4, nation}}), .memory = writes,
                             .label = label("nation offers", nation, pattern)});
            if (pattern % 5 == 0) {
                // The news desk takes the clubs (forwarding the offer), the club flag blocks, and closed offers with lists.
                auto more = writes;
                for (std::uint32_t i = 0; i < offers_count; ++i)
                    if (pattern % 10 == 0) {
                        more.push_back({1, offer_at(i) + 0x34, 0x13u});
                        more.push_back({1, offer_at(i) + 0x38, 0x0fu});
                    }
                if (pattern % 20 == 0) more.push_back({4, array + 4 * (pattern % offers_count), 0});
                for (const std::uint32_t news : {0u, 1u})
                    for (const std::uint32_t flag : {0u, 1u})
                        cases.push_back({.entry = 0x103e9ce0, .registers = with_toc({{3, manager}, {4, nation}}), .memory = more,
                                         .label = label("nation offers answered", nation, pattern, news, flag),
                                         .answers = {{news_check, news}, {club_flag, flag}}});
            }
        }
    }

    // Every offer opened at once: stage 0 or 1 with phase 0, with and without lists, the news desk and the club flag answered.
    nation_count = 0;
    for (const auto nation : nations) {
        if (++nation_count > 40) break;
        for (unsigned shape = 0; shape < 16; ++shape) {
            Writes writes;
            for (std::uint32_t i = 0; i < offers_count; ++i) {
                const auto offer = offer_at(i);
                writes.push_back({1, offer + 0x34, shape & 1 ? 1u : 0u});
                writes.push_back({1, offer + 0x38, shape & 4 ? 0x3fu : 0x0fu});
                // Contracts of kind 5 (not usable for the sides) for the people of half the shapes.
                if (shape & 8)
                    if (const auto contract = scenario.contract_of(scenario.person(scenario.r32(offer + 0xc) % people))) writes.push_back({1, contract + 0x38, 0x14});
                writes.push_back({1, offer + 0x10, 0});
                if (shape & 2) writes.push_back({4, offer + 4, heap_list(spare_blocks, i % 7, {person_of(i)}, writes)});
            }
            for (const std::uint32_t news : {0u, 1u})
                for (const std::uint32_t flag : {0u, 1u}) {
                    cases.push_back({.entry = 0x103e9ce0, .registers = with_toc({{3, manager}, {4, nation}}), .memory = writes,
                                     .label = label("nation open", nation, shape, news, flag),
                                     .answers = {{news_check, news}, {club_flag, flag}, {0x1040ce40, 0}, {0x103e44b0, 0}, {0x103e4130, 0}}});
                    cases.push_back({.entry = 0x103e9ce0, .registers = with_toc({{3, manager}, {4, nation}}), .memory = writes,
                                     .label = label("nation open real", nation, shape, news, flag), .answers = {{news_check, news}, {club_flag, flag}}});
                }
        }
    }

    // The second contract lookup of an opened offer answered with a contract of kind 5, after a usable first one.
    nation_count = 0;
    for (const auto nation : nations) {
        if (++nation_count > 12) break;
        for (const auto j : picks) {
            Writes writes{{4, scratch_base + 0x704, scenario.r32(offer_at(j) + 8)}, {1, scratch_base + 0x738, 0x14}};
            for (std::uint32_t i = 0; i < offers_count; ++i) {
                const auto offer = offer_at(i);
                writes.push_back({1, offer + 0x34, i == j ? 1u : 0x13u});
                writes.push_back({1, offer + 0x38, 0x0fu});
                writes.push_back({1, offer + 0x10, 0});
                writes.push_back({4, offer + 4, 0});
            }
            for (unsigned nth = 1; nth <= 8; ++nth)
                cases.push_back({.entry = 0x103e9ce0, .registers = with_toc({{3, manager}, {4, nation}}), .memory = writes,
                                 .label = label("nation lookup unusable", nation, j, nth),
                                 .answers = {{news_check, 0}, {club_flag, 0}}, .nth_answers = {{0x10082f20, nth, scratch_base + 0x700}}});
        }
    }

    // The second look of 0x103e9ce0 forwarding the offer: one open offer (stage 1, phase 3, a list) for a person under contract
    // at a club of the nation, every other offer closed, the news desk taking the club.
    for (const auto j : picks) {
        const auto holder = scenario.person(scenario.r32(offer_at(j) + 0xc) % people);
        const auto contract = scenario.contract_of(holder);
        if (!contract || scenario.r32(contract + 4) >= 100000) continue;
        const auto nation = scenario.r32(scenario.club(scenario.r32(contract + 4)) + 0x2c);
        for (unsigned variant = 0; variant < 4; ++variant) {
            Writes writes;
            for (std::uint32_t i = 0; i < offers_count; ++i) {
                const auto offer = offer_at(i);
                writes.push_back({1, offer + 0x34, i == j ? 1u : 0x13u});
                writes.push_back({1, offer + 0x38, i == j ? 0x3fu : 0x0fu});
                writes.push_back({4, offer + 4, 0});
                writes.push_back({1, offer + 0x10, 0});
            }
            writes.push_back({4, offer_at(j) + 4, scratch_list(list_slot(), {scenario.r32(offer_at(j) + 0xc)}, writes)});
            if (variant & 1) writes.push_back({4, offer_at(j) + 8, scenario.r32(contract + 4)});
            cases.push_back({.entry = 0x103e9ce0, .registers = with_toc({{3, manager}, {4, nation}}), .memory = writes,
                             .label = label("nation second look", j, variant), .answers = {{news_check, variant >> 1}, {club_flag, 0}, {0x105bd960, 0}, {0x105be000, 0}, {0x105bfa10, 0}, {0x105c0630, 0}, {0x105c13e0, 0},
                                          {0x105c1fe0, 0}, {0x105c2d90, 0}, {0x105c3db0, 0}, {0x105c4550, 0}}});
        }
    }

    // ---- 0x103ec0f0: every day of three years as the date. ----
    const auto scratch_date = scratch_base + 0x400;
    for (const std::uint32_t year : {0x7d0u, 0x7d1u, 0x7d2u, 0x7d3u})
        for (std::uint32_t doy = 0; doy < 366; ++doy) {
            cases.push_back({.entry = 0x103ec0f0, .registers = with_toc({{3, manager}, {4, scratch_date}}),
                             .memory = {{2, scratch_date, doy}, {2, scratch_date + 2, year}, {4, scratch_date + 4, 0}},
                             .label = label("rule dates", year, doy)});
            if (doy % 9 == 0) {
                // The nation objects without a nation to open the offers of (+0xc).
                Writes more{{2, scratch_date, doy}, {2, scratch_date + 2, year}, {4, scratch_date + 4, 0}};
                for (unsigned n = 0; n < 27; ++n)
                    if (const auto object = scenario.r32(manager + 0x9e0 + n * 4)) more.push_back({4, object + 0xc, 0});
                cases.push_back({.entry = 0x103ec0f0, .registers = with_toc({{3, manager}, {4, scratch_date}}), .memory = more,
                                 .label = label("rule dates no nation", year, doy)});
            }
        }

    // ---- 0x103ea840: the daily pass over dates, phases, offers due and lists. ----
    const std::uint32_t days[] = {0, 1, 30, 31, 59, 100, 0x15e, 0x162, 0x2bc, 0xfff9, 364};
    for (const auto day : days)
        for (const std::uint32_t phase : {0u, 1u, 2u})
            for (unsigned shape = 0; shape < 6; ++shape) {
                Writes writes{{2, scratch_date, day}, {2, scratch_date + 2, 0x7d1}, {4, scratch_date + 4, 0},
                              {2, manager + 0x97c, day}, {2, manager + 0x97e, shape == 5 ? 0u : 0x7d1u}, {1, manager + 0x980, 0},
                              {1, manager + 0x984, shape == 4 ? (phase + 1) % 3 : phase}, {1, manager + 0x978, shape == 3 ? 0x31u : 0u}};
                for (std::uint32_t i = 0; i < offers_count; ++i) {
                    const auto offer = offer_at(i);
                    // Offers dated before the pass, on it, and phase-matched; stages that start, stop or wait.
                    writes.push_back({2, offer + 0x14, i % 3 == 0 ? day - 1 : day});
                    writes.push_back({2, offer + 0x16, 0x7d1});
                    writes.push_back({1, offer + 0x1c, shape == 2 && i % 4 == 1 ? phase + 1 : phase});
                    writes.push_back({1, offer + 0x34, stages[(i * 5 + shape * 7 + day) % (sizeof stages / sizeof stages[0])]});
                    writes.push_back({1, offer + 0x24, i & 1});
                }
                // The slot's list for this phase names offers by index (the pass runs each that is dated today).
                const auto list_header = manager + 0x18 + 0x30 * (shape == 3 ? 0x31u : 0u) + 16 * phase;
                std::vector<std::uint32_t> named;
                for (std::uint32_t i = 0; i < offers_count; i += 2) named.push_back(i);
                named.push_back(offers_count + 5);  // an index past the table
                named.push_back(0xfffffff0u);        // and a negative one
                if (shape == 1 || shape == 2) {
                    const auto list = scratch_list(list_slot(), named, writes);
                    writes.push_back({4, list_header + 4, list + 0x10});
                }
                cases.push_back({.entry = 0x103ea840, .registers = with_toc({{3, manager}, {4, scratch_date}, {5, phase}}), .memory = writes,
                                 .label = label("daily pass", day, phase, shape)});
                if (shape == 1 || shape == 2)
                    cases.push_back({.entry = 0x103ea840, .registers = with_toc({{3, manager}, {4, scratch_date}, {5, phase}}), .memory = writes,
                                     .label = label("daily pass club flag", day, phase, shape), .answers = {{club_flag, 0}}});
                if (shape == 0 || shape == 1) {
                    // A null slot, a null nation object, and persons whose links are missing.
                    auto more = writes;
                    more.push_back({4, array + 4 * ((day + shape) % offers_count), 0});
                    more.push_back({4, manager + 0x9e0 + 4 * (day % 27), 0});
                    for (std::uint32_t k = 0; k < 600 && k < people; ++k) {
                        const auto person = scenario.person(k);
                        if (k % 3 == 0) more.push_back({4, person + 0x24, 0});
                        if (k % 3 == 1) more.push_back({4, person + 0x24, scratch_base + 0x500});
                        if (k % 3 == 2) more.push_back({4, person + 0x24, scratch_base + 0x540});
                    }
                    more.push_back({4, scratch_base + 0x500 + 0x2c, 0});
                    more.push_back({4, scratch_base + 0x540 + 0x2c, scratch_base + 0x580});
                    more.push_back({4, scratch_base + 0x580 + 0x74, 0});
                    cases.push_back({.entry = 0x103ea840, .registers = with_toc({{3, manager}, {4, scratch_date}, {5, phase}}), .memory = more,
                                     .label = label("daily pass holes", day, phase, shape)});
                    cases.push_back({.entry = 0x103ea840, .registers = with_toc({{3, manager}, {4, scratch_date}, {5, phase}}), .memory = more,
                                     .label = label("daily pass holes club flag", day, phase, shape), .answers = {{club_flag, 0}}});
                }
            }

    // The person loop of the pass's day 100: the global kinds the club's kind is compared with set equal to a club's kind.
    {
        std::vector<std::uint32_t> kinds;
        for (std::uint32_t k = 0; k < people && kinds.size() < 5; ++k) {
            const auto person = scenario.person(k);
            const auto link = scenario.r32(person + 0x24);
            if (!link || (scenario.r32(person + 0x4c) & 0xffff) >= 0x7cf) continue;
            const auto club = scenario.r32(link + 0x2c);
            if (!club) continue;
            const auto kind = scenario.r32(club + 0x68);
            if (std::find(kinds.begin(), kinds.end(), kind) == kinds.end()) kinds.push_back(kind);
        }
        for (const auto cell : {-0x7a14, -0x7594, -0x758c, -0x7a18, -0x79fc})
            for (const auto kind : kinds) {
                Writes writes{{2, scratch_date, 100}, {2, scratch_date + 2, 0x7d1}, {4, scratch_date + 4, 0}, {2, manager + 0x97c, 100},
                              {2, manager + 0x97e, 0x7d1}, {1, manager + 0x980, 0}, {1, manager + 0x984, 2},
                              {4, scenario.toc(cell), kind}};
                // People old enough for every date test (before the year-10 mark too), so the kind comparisons are reached.
                for (std::uint32_t k = 0; k < 600 && k < people; ++k) writes.push_back({2, scenario.person(k) + 0x4e, 1980});
                cases.push_back({.entry = 0x103ea840, .registers = with_toc({{3, manager}, {4, scratch_date}, {5, 2u}}), .memory = writes,
                                 .label = label("daily pass kinds", unsigned(-cell), kind)});
            }
    }

    // ---- 0x103f3100: the rows due, by how far the date has got, over offers in each stage. ----
    const auto rows = scenario.r32(manager + 0xa50), row_count = scenario.r32(manager + 0xa58);
    for (std::uint32_t due = 0; due <= row_count + 1 && due < 24; ++due)
        for (unsigned pattern = 0; pattern < 24; ++pattern) {
            Writes writes;
            // The date has reached the due-th row (rows are in date order).
            const auto reach = rows && due < row_count ? due : row_count ? row_count - 1 : 0;
            if (rows && row_count) {
                writes.push_back({2, date_cell, scenario.r32(rows + reach * 12) >> 16});
                writes.push_back({2, date_cell + 2, scenario.r32(rows + reach * 12) & 0xffff});
                if (due > row_count) writes.push_back({2, date_cell + 2, 0xffff});
                if (pattern % 5 == 4) writes.push_back({2, date_cell + 2, (scenario.r32(rows + reach * 12) & 0xffff) + 1});
            }
            for (std::uint32_t i = 0; i < offers_count; ++i) {
                const auto offer = offer_at(i);
                writes.push_back({1, offer + 0x34, stages[(i * 3 + pattern) % (sizeof stages / sizeof stages[0])]});
                writes.push_back({1, offer + 0x38, (i + pattern) % 3 == 0 ? 0x0fu : 0x3fu});
                writes.push_back({1, offer + 0x37, (i + pattern) & 1});
                if (pattern % 6 == 3) writes.push_back({4, offer + 8, scenario.r32(offer_at((i + 1) % offers_count) + 8)});
                if (pattern % 4 == 1) writes.push_back({4, offer + 0xc, person_of(i * 3 + pattern)});
            }
            // A row naming an empty slot (the assertion), and one naming an index past the table.
            if (pattern % 12 == 11 && rows && row_count > 1) writes.push_back({4, array + scenario.r32(rows + 8) * 4, 0});
            cases.push_back({.entry = 0x103f3100, .registers = with_toc({{3, manager}}), .memory = writes, .label = label("due offers", due, pattern),
                             .answers = pattern % 2 ? Answers{{club_flag, 0}} : Answers{}});
        }
    // The due rows over fuzzed offers (all offers fuzzed, the date past every row): the settle step, the terms
    // check and the steps after it run for real.
    for (unsigned k = 0; k < 8000 && rows && row_count; ++k) {
        Writes writes{{2, date_cell, 0x7fff}, {2, date_cell + 2, 0x7d3}};
        for (std::uint32_t i = 0; i < offers_count; ++i) add(writes, fuzz_writes(offer_at(i)));
        if (k % 17 == 16) writes.push_back({4, array + roll(offers_count) * 4, 0});
        Answers answers;
        if (k % 3 == 0) {
            // The lookups answered with one contract; the offers in the stages the settle step works on, their people asked for.
            contract_variant(writes, offer_at(0), answers);
            for (std::uint32_t i = 0; i < offers_count; ++i) {
                const auto offer = offer_at(i);
                add(writes, {{1, offer + 0x34, roll(2) ? 10u : 12u}, {1, offer + 0x10, 2}, {1, offer + 0x36, 0}, {1, offer + 0x37, 0}, {1, offer + 0x38, 0}});
                const auto status = scenario.r32(manager + 0x9a0) + (scenario.r32(offer + 0xc) % people) * 10;
                add(writes, {{1, status + 2, 2}, {1, status + 1, 1}});
            }
        }
        cases.push_back({.entry = 0x103f3100, .registers = with_toc({{3, manager}}), .memory = writes, .label = label("due offers fuzz", k), .answers = answers});
    }
}
}
