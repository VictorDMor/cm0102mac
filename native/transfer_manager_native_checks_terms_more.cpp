// More cases for transfer_manager_native_terms*.cpp: fixtures that make the squad, row, rule and money outcomes happen
// (the career snapshot's offers are mostly closed and its squads are not full). Fields the routines read are written
// into real offers, people, contracts and clubs; callees outside the module that decide a branch are answered.
#include "transfer_manager_native_checks.hpp"
#include "guest_abi.hpp"
#include <algorithm>
#include <array>
#include <string>

namespace cm::aot::transfer_manager_native_checks {
namespace {
using Writes = std::vector<Write>;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
constexpr std::uint32_t toc_value = 0x108bba90;
std::string tag(const char* what, unsigned a, unsigned b = 0, unsigned c = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c);
}
void add(Writes& to, const Writes& more) { to.insert(to.end(), more.begin(), more.end()); }
// A guest list in the scratch area (header {count, head, tail, cursor}, nodes {record, size, next, prev}, records of two
// words); nothing frees it, the routines under test only read it.
std::uint32_t scratch_list(std::uint32_t at, const std::vector<std::array<std::uint32_t, 2>>& records, Writes& writes) {
    const auto nodes = at + 0x10, data = nodes + 0x10 * std::uint32_t(records.size());
    for (std::size_t k = 0; k < records.size(); ++k) {
        const auto node = nodes + 0x10 * std::uint32_t(k), record = data + 8 * std::uint32_t(k);
        writes.push_back({4, record, records[k][0]});
        writes.push_back({4, record + 4, records[k][1]});
        writes.push_back({4, node, record});
        writes.push_back({2, node + 4, 8});
        writes.push_back({4, node + 8, k + 1 < records.size() ? node + 0x10 : 0});
        writes.push_back({4, node + 0xc, k ? node - 0x10 : 0});
    }
    writes.push_back({2, at, std::uint32_t(records.size())});
    writes.push_back({4, at + 4, records.empty() ? 0 : nodes});
    writes.push_back({4, at + 8, records.empty() ? 0 : nodes + 0x10 * std::uint32_t(records.size() - 1)});
    writes.push_back({4, at + 0xc, 0});
    return at;
}
struct Held { std::uint32_t person, contract, club; };
// People whose contract is at a club (the contract's kind is not 5), with that club's index.
std::vector<Held> held_people(const Scenario& scenario, unsigned wanted) {
    std::vector<Held> held;
    for (unsigned i = 0; i < scenario.people() && held.size() < wanted; ++i) {
        const auto contract = scenario.contract_of(scenario.person(i));
        if (contract && (std::int8_t(scenario.s.memory.read<1>(contract + 0x38)) >> 2) != 5) held.push_back({i, contract, scenario.r32(contract + 4)});
    }
    return held;
}
std::uint32_t person_without_contract(const Scenario& scenario, unsigned from) {
    for (unsigned k = 0; k < scenario.people(); ++k) {
        const auto index = (from + k * 5) % scenario.people();
        if (!scenario.contract_of(scenario.person(index))) return index;
    }
    return 0;
}
}

TRANSFER_MANAGER_NATIVE_CASES("terms") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    if (size < 6 || scenario.people() < 200) return;
    std::vector<std::uint32_t> offers;
    for (std::uint32_t i = 0; i < size && offers.size() < 120; ++i)
        if (const auto offer = scenario.r32(array + i * 4)) offers.push_back(offer);
    if (offers.size() < 6) return;
    // The career holds few offers: the cases cycle through them (a case's writes to one offer are one state), and the
    // rows fixtures give the manager a longer array of the same offers.
    const auto real_offer_count = offers.size();
    for (std::size_t i = 0; offers.size() < 120; ++i) offers.push_back(offers[i % real_offer_count]);
    const auto held = held_people(scenario, 4000);
    if (held.size() < 200) return;
    const auto date = scenario.toc(-0x7b80);
    const auto limit_cell = scenario.toc(-0x7be4);
    const auto code = scratch_base + 0x1100, rows_out = scratch_base + 0x1200, scratch_record = scratch_base + 0x1400;
    const std::uint32_t any_club = scenario.club(5);
    (void)any_club;

    // ---- 0x103f89e0 / 0x103f9d60: the contract's date against the default date (equal second halfwords, every first one).
    for (std::size_t k = 0; k < 6; ++k) {
        const auto offer = offers[k];
        const auto& h = held[k * 17];
        const auto day = scenario.r32(h.contract + 0x30) >> 16, year = scenario.r32(h.contract + 0x30) & 0xffff;
        (void)day;
        const std::uint32_t firsts[] = {0u, 1u, 0x7fffu, 0x8000u, 0xffffu, std::uint32_t(day), std::uint32_t(day) + 1, std::uint32_t(day) - 1,
                                        std::uint32_t(day) + 2, std::uint32_t(day) + 400};
        for (const auto first : firsts)
            for (const std::uint32_t second_shift : {0u, 1u, 0xffffu}) {
                Writes writes = {{1, offer + 0x38, 0x30}, {4, offer + 0xc, h.person}, {2, date, first & 0xffff},
                                 {2, date + 2, (year + second_shift) & 0xffff}, {1, offer + 0x34, 0xa}};
                cases.push_back({.entry = 0x103f89e0, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, scratch_base + 0x1000}},
                                 .memory = writes, .label = tag("date compare", unsigned(k), first & 0xffff, second_shift)});
                cases.push_back({.entry = 0x103f9d60,
                                 .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, scenario.club(h.club)}, {6, code}, {7, 1}},
                                 .memory = writes, .label = tag("terms date", unsigned(k), first & 0xffff, second_shift)});
            }
        // 0x104058e0: the second lookup finds a contract without a club (the first one did not).
        Writes news = {{4, offer + 0xc, h.person}, {1, offer + 0x34, 0xa}, {1, scratch_record + 0x38, 0x14}, {4, scratch_record + 4, 0}};
        cases.push_back({.entry = 0x104058e0, .registers = {{2, toc_value}, {3, manager}, {4, offer}}, .memory = news,
                         .label = tag("term news second", unsigned(k)),
                         .nth_answers = {{0x10082f20, 1, h.contract}, {0x10082f20, 2, scratch_record}}});
        // 0x103f89e0 kind 2 and 3 for a person without a contract / with a contract of kind 5 and no club.
        const auto none = person_without_contract(scenario, unsigned(k));
        for (const std::uint32_t nibble : {2u, 3u})
            cases.push_back({.entry = 0x103f89e0, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, scratch_base + 0x1000}},
                             .memory = {{1, offer + 0x38, nibble << 4}, {4, offer + 0xc, none}}, .label = tag("date no contract", unsigned(k), nibble)});
    }

    // ---- 0x103f9d60: the rules, the money, the squads. Answers pin what the snapshot cannot produce.
    for (std::size_t k = 0; k < 40; ++k) {
        const auto offer = offers[k];
        const auto& h = held[(k * 29) % held.size()];
        const auto holder = scenario.club(h.club);
        const auto buyer_index = unsigned((h.club + 1 + k) % 40);
        const auto buyer = scenario.club(buyer_index);
        const auto person = scenario.person(h.person);
        Writes base = {{4, offer + 0xc, h.person}, {4, offer + 8, buyer_index}, {1, offer + 0x34, 0xa}, {1, offer + 0x38, k % 2 ? 0x10u : 0u},
                       {1, offer + 0x37, (k % 3 == 0) ? 1u : 0u}, {4, person + 0x14, 1}};
        for (const auto club : {holder, buyer})
            for (const std::uint32_t answers_kind : {0u, 1u, 2u, 3u}) {
                Answers answers = {{0x100c7a20, 1}};
                if (answers_kind == 1) answers.push_back({0x103f5a50, 1});
                if (answers_kind == 2) { answers.push_back({0x103f5a50, 0}); answers.push_back({0x103f4d10, 0}); }
                if (answers_kind == 3) { answers.push_back({0x103f5a50, 1}); answers.push_back({0x103f4d10, 1}); }
                cases.push_back({.entry = 0x103f9d60, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, club}, {6, code}, {7, 0}},
                                 .memory = base, .label = tag("rules answered", unsigned(k), club == holder, answers_kind), .answers = answers});
            }
        // Money: the buying club's cash answered negative (no money), the fee asked from the real offer.
        for (const std::uint32_t club_busy : {0u, 1u}) {
            Writes more = base;
            Answers answers = {{0x100f5770, 0xffffffffu}, {0x1014c2e0, club_busy}};
            // The people the buying club would sell: a fee record in the scratch area answers the sale lookup.
            more.push_back({4, scratch_record + 0x20, (k % 4 == 0) ? 0x7fffffffu : (k % 4 == 1) ? 5u : (k % 4 == 2) ? 0xfffffff0u : 0u});
            if (k % 2) answers.push_back({0x103f5d20, scratch_record});
            cases.push_back({.entry = 0x103f9d60, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, buyer}, {6, code}, {7, 1}},
                             .memory = more, .label = tag("terms money", unsigned(k), club_busy), .answers = answers});
        }
        // The squad check answered refusing, for the buying and the holding club (the terms date skipped: kind 0).
        Writes plain = base;
        plain.push_back({1, offer + 0x38, 0});
        for (const auto club : {holder, buyer})
            for (const std::uint32_t refuse : {0u, 1u})
                cases.push_back({.entry = 0x103f9d60, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, club}, {6, code}, {7, 1}},
                                 .memory = plain, .label = tag("terms squad refused", unsigned(k), club == holder, refuse),
                                 .nth_answers = {{0x103f4fd0, 1, refuse}, {0x103f4fd0, 2, refuse ^ 1u}}});
        // The buying club able to pay (fee zero, cash answered), then the squad check refusing.
        {
            Writes cheap = plain;
            cheap.push_back({4, offer + 0x20, 0});
            for (const std::uint32_t refuse : {0u, 1u})
                cases.push_back({.entry = 0x103f9d60, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, buyer}, {6, code}, {7, 1}},
                                 .memory = cheap, .label = tag("terms cheap", unsigned(k), refuse),
                                 .answers = {{0x100f5770, 0}, {0x10416710, 0}, {0x1000a060, 0}}, .nth_answers = {{0x103f4fd0, 1, refuse}}});
        }
        // The person without a contract, and with a contract that has no club (kind 5): no holding club.
        {
            Writes none = plain;
            none.push_back({4, offer + 0xc, person_without_contract(scenario, unsigned(k))});
            Writes five = plain;
            five.push_back({1, h.contract + 0x38, 0x14});
            for (const auto* writes : {&none, &five})
                for (const auto club : {holder, buyer, scenario.club(2)})
                    cases.push_back({.entry = 0x103f9d60, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, club}, {6, code}, {7, 0}},
                                     .memory = *writes, .label = tag("terms no holder", unsigned(k), writes == &none, club == buyer),
                                     .answers = {{0x100c7a20, k % 2}}});
        }
        // The date's reason code nonzero (the first-date rules refused), for each kind that asks them.
        for (const std::uint32_t nibble : {1u, 4u})
            for (const std::uint32_t reason : {1u, 0x10000u, 0xffffu}) {
                Writes dated = base;
                dated.push_back({1, offer + 0x38, nibble << 4});
                cases.push_back({.entry = 0x103f9d60, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, buyer}, {6, (k % 2) ? code : 0}, {7, 0}},
                                 .memory = dated, .label = tag("terms reason", unsigned(k), nibble, reason),
                                 .answers = {{0x103f8cc0, reason}}});
            }
        // The money case without a code word.
        cases.push_back({.entry = 0x103f9d60, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, buyer}, {6, 0}, {7, 1}},
                         .memory = plain, .label = tag("terms money bare", unsigned(k)),
                         .answers = {{0x100f5770, 0xffffffffu}, {0x1014c2e0, 0}}});
        // The nation rules (the call through the vector) answered with every code at the person's nation.
        for (const std::uint32_t verdict : {0u, 1u})
            for (const std::uint32_t reason : {0u, 9u}) {
                Writes more = base;
                more.push_back({2, code, reason});
                cases.push_back({.entry = 0x103f9d60, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, buyer}, {6, code}, {7, 1}},
                                 .memory = more, .label = tag("terms vector", unsigned(k), verdict, reason),
                                 .answers = {{0x10781320, verdict}}});
            }
    }

    // The buying club's shortlist sum against the fee: every club as the buyer, the sale record answering with a value.
    for (unsigned club_index = 0; club_index < 120; ++club_index) {
        const auto offer = offers[club_index % offers.size()];
        const std::uint32_t extras[] = {0x7fffffffu, 5u, 0xfffffff0u, 0xffffffffu, 0x40000000u, 0u};
        Writes writes = {{4, offer + 8, club_index}, {1, offer + 0x34, 0xa}, {1, offer + 0x38, 0},
                         {4, scratch_record + 0x20, extras[club_index % 6]}};
        cases.push_back({.entry = 0x103f9d60,
                         .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, scenario.club(club_index)}, {6, code}, {7, 1}},
                         .memory = writes, .label = tag("terms shortlist", club_index),
                         .answers = {{0x100f5770, 0xffffffffu}, {0x1014c2e0, 0}, {0x103f5d20, scratch_record}}});
    }

    // ---- 0x103f4fd0: the squad rows. A club's 50-person row is filled with real people, the limit raised so each counts.
    const std::uint32_t roles[] = {5, 8, 9, 10, 6, 0xd, 0xf, 0xb, 3, 0x20};
    for (std::size_t k = 0; k < 60; ++k) {
        const auto offer = offers[k];
        const auto club_index = unsigned(3 + (k * 7) % 30), other_index = unsigned(40 + (k * 3) % 20);
        const auto club = scenario.club(club_index), other = scenario.club(other_index);
        const auto& h = held[(k * 13) % held.size()];
        const auto none = person_without_contract(scenario, unsigned(k));
        Writes base = {{4, limit_cell, 0x7fffffffu}, {4, offer + 8, club_index}, {1, offer + 0x11, roles[k % 10]}};
        // Fill the club's rows and the other club's with people (rows +0x98 x50, +0x160 x5, +0x174 x7, +0x190 x3).
        const auto fill = [&](std::uint32_t target, unsigned seed, Writes& writes, unsigned density) {
            for (unsigned i = 0; i < 50; ++i) writes.push_back({4, target + 0x98 + i * 4, (i * 3 + seed) % 5 < density ? scenario.person((seed * 7 + i) % 300) : 0u});
            for (unsigned i = 0; i < 5; ++i) writes.push_back({4, target + 0x160 + i * 4, (i + seed) % 4 < density ? scenario.person((seed + i) % 300) : 0u});
            for (unsigned i = 0; i < 7; ++i) writes.push_back({4, target + 0x174 + i * 4, (i + seed) % 4 < density ? scenario.person((seed + i + 9) % 300) : 0u});
            for (unsigned i = 0; i < 3; ++i) writes.push_back({4, target + 0x190 + i * 4, (i + seed) % 4 < density ? scenario.person((seed + i + 20) % 300) : 0u});
            writes.push_back({4, target + 0x94, (seed % 2) ? scenario.person(seed) : 0u});
        };
        fill(club, unsigned(k), base, 1 + unsigned(k % 5));
        fill(other, unsigned(k) + 1, base, 1 + unsigned(k % 4));
        // Person: not held by this club (the offer's person without contract, or held elsewhere), and the offer's list empty.
        Writes a_branch = base;
        a_branch.push_back({4, offer + 0xc, (k % 3 == 0) ? none : h.person});
        a_branch.push_back({4, offer + 4, 0});
        const Answers truth = {{0x100c7a20, 1}, {0x100c7a00, 1}, {0x100ce320, other}};
        for (const std::uint32_t trim : {0u, 1u})
            for (unsigned shape = 0; shape < 4; ++shape) {
                Answers answers = truth;
                if (shape == 1) answers = {{0x100c7a20, 1}, {0x100c7a00, 0}, {0x100ce320, other}};
                if (shape == 2) answers = {{0x100c7a20, 1}, {0x100c7a00, 1}, {0x100ce320, 0}};
                if (shape == 3) answers = {{0x100c7a20, 0}, {0x100c7a00, 1}, {0x100ce320, other}};
                cases.push_back({.entry = 0x103f4fd0,
                                 .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, trim}, {7, (k % 2) ? code : 0}},
                                 .memory = a_branch, .label = tag("squad A", unsigned(k), trim, shape), .answers = answers});
            }
        // The terms check of the buying club whose squad has no room (the squad check inside it answers 0).
        for (const std::uint32_t shape : {0u, 1u}) {
            Writes full = a_branch;
            full.push_back({1, offer + 0x38, 0});
            full.push_back({1, offer + 0x34, 0xa});
            cases.push_back({.entry = 0x103f9d60, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, club}, {6, (k % 2) ? code : 0}, {7, 1}},
                             .memory = full, .label = tag("terms squad full", unsigned(k), shape),
                             .answers = {{0x100c7a20, 1}, {0x100c7a00, shape}, {0x100ce320, other}, {0x100f5770, 0}, {0x10416710, 0}, {0x1000a060, 0}}});
        }
        // Hold the offer's person at the club: the person counts for the club (loan and plain contract paths).
        Writes held_here = a_branch;
        held_here.push_back({4, offer + 0xc, h.person});
        held_here.push_back({4, h.contract + 4, club_index});
        cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, 1}, {7, code}},
                         .memory = held_here, .label = tag("squad held", unsigned(k)), .answers = truth});
        Writes kind_five = held_here;
        kind_five.push_back({1, h.contract + 0x38, 0x14});
        cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, 1}, {7, code}},
                         .memory = kind_five, .label = tag("squad kind 5", unsigned(k)),
                         .answers = truth, .nth_answers = {{0x10082f20, 2, h.contract}}});
        cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, 1}, {7, code}},
                         .memory = kind_five, .label = tag("squad kind 5 gone", unsigned(k)),
                         .answers = truth, .nth_answers = {{0x10082f20, 2, 0}}});
        // The person held as a second (loan) contract at this club, and at another one.
        for (const std::uint32_t loan_club : {club_index, club_index + 1}) {
            Writes loan = a_branch;
            loan.push_back({4, scratch_base + 0x1500 + 4, loan_club});
            cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, 1}, {7, code}},
                             .memory = loan, .label = tag("squad loan", unsigned(k), loan_club == club_index),
                             .answers = {{0x100c7a20, 1}, {0x100c7a00, 1}, {0x100ce320, other}, {0x10083110, scratch_base + 0x1500}}});
        }
        // A listed list on the offer (A branch): its people leave the club's count.
        {
            Writes listed_a = a_branch;
            std::vector<std::array<std::uint32_t, 2>> people_listed;
            for (unsigned i = 0; i < 1 + k % 3; ++i) people_listed.push_back({(unsigned(k) * 5 + i) % 300, 0});
            listed_a.push_back({4, offer + 4, scratch_list(scratch_base + 0x3000 + std::uint32_t(k % 4) * 0x400, people_listed, listed_a)});
            for (unsigned i = 0; i < 3; ++i)
                listed_a.push_back({1, scenario.person((unsigned(k) * 5 + i) % 300) + 0x62, roles[(k + i) % 10]});
            for (const std::uint32_t kind : {0u, 1u})
                cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, 1}, {7, code}},
                                 .memory = listed_a, .label = tag("squad A listed", unsigned(k), kind),
                                 .answers = {{0x100c7a20, 1}, {0x100c7a00, kind}, {0x100ce320, other}}});
        }
        // Rows with an open slot empty at both clubs, for the roles that need it (6 and 0xd).
        for (const std::uint32_t role : {6u, 0xdu}) {
            Writes open = a_branch;
            open.push_back({1, offer + 0x11, role});
            open.push_back({4, club + 0x94, 0});
            open.push_back({4, other + 0x94, 0});
            cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, 1}, {7, code}},
                             .memory = open, .label = tag("squad open", unsigned(k), role),
                             .answers = {{0x100c7a20, 0}, {0x100c7a00, 1}, {0x100ce320, (k % 2) ? other : 0}}});
        }
        cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, 1}, {7, code}},
                         .memory = held_here, .label = tag("squad second contract", unsigned(k)),
                         .answers = {{0x100c7a20, 1}, {0x100c7a00, 1}, {0x100ce320, other}, {0x10083110, scratch_record}},
                         .nth_answers = {}});
        // B branch (another club than the offer's buyer): the offer's listed people count against it.
        Writes b_branch = base;
        b_branch.push_back({4, offer + 8, (club_index + 1) % 40});
        std::vector<std::array<std::uint32_t, 2>> listed;
        const unsigned n_listed = 1 + unsigned(k % 4);
        for (unsigned i = 0; i < n_listed; ++i) listed.push_back({(unsigned(k) * 3 + i) % 300, roles[(k + i) % 10] << 8});
        b_branch.push_back({4, offer + 4, scratch_list(scratch_base + 0x2000 + std::uint32_t(k % 4) * 0x400, listed, b_branch)});
        b_branch.push_back({1, scenario.person(h.person) + 0x62, roles[(k * 3) % 10]});
        b_branch.push_back({4, offer + 0xc, h.person});
        for (unsigned shape = 0; shape < 5; ++shape) {
            Answers answers = truth;
            if (shape == 1) answers = {{0x100c7a20, 0}, {0x100c7a00, 1}, {0x100ce320, other}};
            if (shape == 2) answers = {{0x100c7a20, 1}, {0x100c7a00, 1}, {0x100ce320, 0}};
            if (shape == 3) answers = {{0x100c7a20, 1}, {0x100c7a00, 0}, {0x100ce320, other}};
            if (shape == 4) answers = {{0x100c7a20, 0}, {0x100c7a00, 0}, {0x100ce320, other}};
            cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, 1}, {7, (k % 2) ? code : 0}},
                             .memory = b_branch, .label = tag("squad B", unsigned(k), shape), .answers = answers});
        }
        // Trim candidates: the first row's people are made sellable with decreasing and increasing values.
        Writes trim_writes = a_branch;
        std::vector<std::uint32_t> trim_ids;
        Writes other_slots;
        for (unsigned i = 0; i < 50; ++i) {
            auto candidate = held[(k * 3 + i) % held.size()];
            if (i % 4 == 1) {
                // An offered person (the manager knows him): the person of one of the offers.
                const auto offered = scenario.r32(offers[(k + i) % real_offer_count] + 0xc);
                const auto found = scenario.contract_of(scenario.person(offered));
                if (found) candidate = {offered, found, scenario.r32(found + 4)};
            }
            const auto person_record = scenario.person(candidate.person);
            trim_writes.push_back({4, club + 0x98 + i * 4, person_record});
            other_slots.push_back({4, other + 0x98 + i * 4, person_record});
            trim_ids.push_back(scenario.r32(person_record));
            trim_writes.push_back({1, person_record + 0x62, (i % 7 == 6) ? 5u : (k % 3 == 0) ? 0xbu : (k % 3 == 1) ? 0xfu : 0xdu});
            if (i % 9 == 2) {
                // A second (loan) contract for him: the second index of his row points at his own contract.
                const auto contracts_base = scenario.r32(scenario.toc(-0x7b7c));
                trim_writes.push_back({4, scenario.global(-0x7b9c) + scenario.r32(person_record) * 100 + 4, (candidate.contract - contracts_base) / 100});
            }
            trim_writes.push_back({4, person_record + 8, (k % 2) ? 400u - i * 3u : 100u + i * 3u});
            trim_writes.push_back({1, candidate.contract + 0x44, (i % 5 == 4) ? 3u : (i % 2) ? 0xffu : 0u});
            if (i % 11 == ((k % 2) ? 10 : 0)) trim_writes.push_back({1, candidate.contract + 0x38, 0x14});
        }
        Writes with_second = trim_writes;
        with_second.push_back({4, scratch_base + 0x1500 + 4, 0});
        // A list of sellers the manager already holds (the person's id among them).
        Writes with_listed = trim_writes;
        std::vector<std::array<std::uint32_t, 2>> sellers;
        for (unsigned i = 0; i < 50; i += 3) sellers.push_back({scenario.r32(scenario.person(held[(k * 3 + i) % held.size()].person)), 0});
        const auto sellers_list = scratch_list(scratch_base + 0x5000, sellers, with_listed);
        with_listed.push_back({2, manager + 0xa5c, std::uint32_t(sellers.size())});
        with_listed.push_back({4, manager + 0xa60, sellers_list + 0x10});
        with_listed.push_back({4, manager + 0xa64, sellers_list + 0x10 + 0x10 * std::uint32_t(sellers.size() - 1)});
        with_listed.push_back({4, manager + 0xa68, 0});
        for (const std::uint32_t pass : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u}) {
            Answers answers = {{0x100c7a20, 1}, {0x100c7a00, 0}, {0x100ce320, 0}};
            answers.push_back({0x1014c2e0, pass == 2 ? 1u : 0u});
            answers.push_back({0x10083110, pass == 3 ? scratch_base + 0x1500 : 0u});
            if (pass != 4 && pass != 5 && pass != 6) {
                answers.push_back({0x103eaea0, 0});
                answers.push_back({0x103eb020, 0});
                answers.push_back({0x103f43b0, 0});
            } else {
                answers.push_back({0x103eaea0, pass == 4 ? 1u : 0u});
                answers.push_back({0x103eb020, pass == 5 ? 1u : 0u});
                answers.push_back({0x103f43b0, pass == 6 ? 1u : 0u});
            }
            if (pass == 1 && k % 2) answers.push_back({0x103e6360, 0});
            Writes memory = pass == 3 ? with_second : pass == 7 ? with_listed : trim_writes;
            if (pass == 8) {
                auto ids = trim_ids;
                std::sort(ids.begin(), ids.end());
                memory.push_back({4, limit_cell, ids[ids.size() * 6 / 10]});
                add(memory, other_slots);
                answers.erase(std::remove_if(answers.begin(), answers.end(), [](const auto& a) { return a.first == 0x100ce320; }), answers.end());
                answers.push_back({0x100ce320, other});
            }
            std::vector<Case::NthAnswer> nth;
            if (pass == 5) {
                answers.erase(std::remove_if(answers.begin(), answers.end(), [](const auto& a) { return a.first == 0x103eb020; }), answers.end());
                nth.push_back({0x103eb020, 1, 1});
            }
            // The club other than the offer's buyer: no selling.
            cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, pass == 1 ? 1u : 0u}, {7, code}},
                             .memory = memory, .label = tag("squad trim", unsigned(k), pass), .answers = answers, .nth_answers = nth});
        }
        Writes elsewhere = trim_writes;
        elsewhere.push_back({4, offer + 8, club_index + 1});
        cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, 1}, {7, code}},
                         .memory = elsewhere, .label = tag("squad trim elsewhere", unsigned(k)),
                         .answers = {{0x100c7a20, 1}, {0x100c7a00, 0}, {0x100ce320, 0}, {0x1014c2e0, 0}, {0x10083110, 0}}});
        if (k < 6)
            for (std::uint32_t offer_role = 0; offer_role < 0x13; ++offer_role)
                for (const std::uint32_t listed_role : {0u, 0xbu}) {
                    Writes real_roles = trim_writes;
                    real_roles.push_back({1, offer + 0x11, offer_role});
                    real_roles.push_back({4, limit_cell, 0x7fffffffu});
                    real_roles.push_back({4, offer + 4, scratch_list(scratch_base + 0x3800, {{(unsigned(k) * 7) % 300, 0}}, real_roles)});
                    real_roles.push_back({1, scenario.person((unsigned(k) * 7) % 300) + 0x62, listed_role});
                    cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, 1}, {7, code}},
                                     .memory = real_roles, .label = tag("squad real roles", unsigned(k), offer_role, listed_role),
                                     .answers = {{0x100ce320, 0}}});
                }
        Writes with_list = trim_writes;
        with_list.push_back({4, offer + 4, scratch_list(scratch_base + 0x3400, {{1, 0}}, with_list)});
        cases.push_back({.entry = 0x103f4fd0, .registers = {{2, toc_value}, {3, manager}, {4, club}, {5, offer}, {6, 1}, {7, code}},
                         .memory = with_list, .label = tag("squad trim listed offer", unsigned(k)),
                         .answers = {{0x100c7a20, 1}, {0x100c7a00, 0}, {0x100ce320, 0}, {0x1014c2e0, 0}, {0x10083110, 0}}});
    }

    // ---- 0x103f45a0: a club's rows: many offers at one club, the stages of the jump table, the failing allocation.
    const auto row_club_index = 12u;
    const auto row_club = scenario.club(row_club_index);
    for (unsigned count = 12; count < 60; count += 2) {
        Writes writes = {{4, manager, scratch_base + 0x6000}, {4, manager + 4, count}};
        for (unsigned i = 0; i < count; ++i) writes.push_back({4, scratch_base + 0x6000 + i * 4, (i % 9 == 4) ? 0u : offers[i]});
        for (unsigned i = 0; i < count; ++i) {
            const auto offer = offers[i];
            writes.push_back({4, offer + 8, row_club_index});
            writes.push_back({1, offer + 0x24, (i % 13 == 5) ? 1u : 0u});
            writes.push_back({4, offer + 0xc, held[(i * 5) % held.size()].person});
            if (i % 7 == 3) writes.push_back({1, held[(i * 5) % held.size()].contract + 0x38, 0x14});
            if (i % 7 == 5) writes.push_back({4, held[(i * 5) % held.size()].contract + 4, row_club_index});
            writes.push_back({1, offer + 0x34, (i * 3) % 0x15});
            writes.push_back({1, offer + 0x37, (i % 4 == 1) ? 1u : 0u});
            writes.push_back({1, offer + 0x38, ((i % 6) << 4) | 3u});
            writes.push_back({4, offer + 0x20, (i % 5 == 2) ? 0u : 25000u + i});
            if (i % 3 != 2) writes.push_back({4, offer + 4, scratch_list(scratch_base + 0x4000 + (i % 8) * 0x200, {{(i * 11) % 300, 0}, {(i * 13 + 1) % 300, 0}}, writes)});
            else writes.push_back({4, offer + 4, 0});
            writes.push_back({4, offer + 0x28, scenario.r32(manager + 0xa58) ? scenario.r32(scenario.r32(manager + 0xa50) + 8) : 0x7fffffffu});
        }
        const Answers open = {{0x103fb390, 0}};
        cases.push_back({.entry = 0x103f45a0, .registers = {{2, toc_value}, {3, manager}, {4, row_club}, {5, rows_out}},
                         .memory = writes, .label = tag("rows many", count), .answers = open});
    }
    // Rows of offers held by the club but bought by another: the holders' club answers for each person.
    {
        Writes writes;
        unsigned used = 0;
        for (; used < 10; ++used) {
            const auto& h = held[used * 3];
            const auto offer = offers[used];
            writes.push_back({4, h.contract + 4, row_club_index});
            writes.push_back({4, offer + 0xc, h.person});
            writes.push_back({4, offer + 8, row_club_index + 1});
            writes.push_back({1, offer + 0x24, 0});
            writes.push_back({1, offer + 0x34, 5u + used});
            if (used % 2) writes.push_back({4, offer + 4, scratch_list(scratch_base + 0x4000 + used * 0x200, {{used, 0}, {used + 1, 0}}, writes)});
        }
        cases.push_back({.entry = 0x103f45a0, .registers = {{2, toc_value}, {3, manager}, {4, row_club}, {5, rows_out}}, .memory = writes,
                         .label = tag("rows held", used), .answers = {{0x103fb390, 0}}});
    }
    // Offers at zero fee with the flag set and kind 3 or 4 (the row's flag byte 4), and the other kinds.
    for (const std::uint32_t nibble : {2u, 3u, 4u, 5u})
        for (const std::uint32_t fee : {0u, 7u}) {
            Writes writes;
            for (unsigned i = 0; i < 6; ++i) {
                writes.push_back({4, offers[i] + 8, row_club_index});
                writes.push_back({1, offers[i] + 0x24, 0});
                writes.push_back({1, offers[i] + 0x34, 5});
                writes.push_back({1, offers[i] + 0x37, 1});
                writes.push_back({1, offers[i] + 0x38, nibble << 4});
                writes.push_back({4, offers[i] + 0x20, fee});
            }
            cases.push_back({.entry = 0x103f45a0, .registers = {{2, toc_value}, {3, manager}, {4, row_club}, {5, rows_out}}, .memory = writes,
                             .label = tag("rows flag", nibble, fee), .answers = {{0x103fb390, 0}}});
        }
    for (const std::uint32_t stage : {0u, 1u, 5u, 0x13u, 0x14u, 0x7fu, 0x80u, 0xffu})
        for (unsigned variant = 0; variant < 2; ++variant) {
            Writes writes;
            for (unsigned i = 0; i < 6; ++i) {
                writes.push_back({4, offers[i] + 8, row_club_index});
                writes.push_back({1, offers[i] + 0x34, stage});
                writes.push_back({1, offers[i] + 0x24, 0});
                if (variant) writes.push_back({4, offers[i] + 0xc, person_without_contract(scenario, i)});
            }
            cases.push_back({.entry = 0x103f45a0, .registers = {{2, toc_value}, {3, manager}, {4, row_club}, {5, rows_out}}, .memory = writes,
                             .label = tag("rows stage", stage, variant), .answers = {{0x103fb390, 0}}});
        }
    (void)add;
}
}
