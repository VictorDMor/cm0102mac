// Cases for transfer_manager_native_stages*.cpp: 0x103e7cf0, 0x103e80e0, 0x103ebf50, 0x103e95a0, 0x103e6cb0,
// 0x103e70f0, 0x103e76c0, 0x103ed170.
#include "transfer_manager_native_checks.hpp"
#include "guest_abi.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace cm::aot::transfer_manager_native_checks {
namespace {
using Writes = std::vector<Write>;
struct RealOffer { std::uint32_t index, address; };
std::vector<RealOffer> real_offers(const Scenario& scenario) {
    std::vector<RealOffer> offers;
    const auto array = scenario.r32(scenario.transfer_manager()), size = scenario.r32(scenario.transfer_manager() + 4);
    if (size == 0 || size > 100000) return offers;
    for (std::uint32_t i = 0; i < size && offers.size() < 60; ++i)
        if (const auto offer = scenario.r32(array + i * 4)) offers.push_back({i, offer});
    return offers;
}
std::string tag(const char* what, unsigned a, unsigned b = 0) { return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b); }
// A guest list (header {halfword count, head, tail, cursor}, nodes {data, halfword size, next, prev}, data records
// of two words) built out of real offer blocks, because the game frees what it unlinks.
std::uint32_t make_list(const std::vector<std::uint32_t>& blocks, std::size_t& at, const std::vector<std::array<std::uint32_t, 2>>& records,
                        Writes& writes) {
    if (at + 1 + 2 * records.size() > blocks.size()) return 0;
    const auto header = blocks[at++];
    std::vector<std::uint32_t> nodes;
    for (std::size_t k = 0; k < records.size(); ++k) nodes.push_back(blocks[at++]);
    for (std::size_t k = 0; k < records.size(); ++k) {
        const auto data = blocks[at++];
        writes.push_back({4, data, records[k][0]});
        writes.push_back({4, data + 4, records[k][1]});
        writes.push_back({4, nodes[k], data});
        writes.push_back({2, nodes[k] + 4, 8});
        writes.push_back({4, nodes[k] + 8, k + 1 < nodes.size() ? nodes[k + 1] : 0});
        writes.push_back({4, nodes[k] + 0xc, k ? nodes[k - 1] : 0});
    }
    writes.push_back({2, header, std::uint32_t(records.size())});
    writes.push_back({4, header + 4, nodes.empty() ? 0 : nodes.front()});
    writes.push_back({4, header + 8, nodes.empty() ? 0 : nodes.back()});
    writes.push_back({4, header + 0xc, 0});
    return header;
}
constexpr std::uint32_t status_rows = 0x9a0;
// The club table row of a club index.
std::uint32_t club_row(const Scenario& scenario, std::uint32_t index) { return scenario.club(index); }
}

TRANSFER_MANAGER_NATIVE_CASES("stages") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto offers = real_offers(scenario);
    const auto people = scenario.people();
    if (offers.size() < 12 || people == 0) return;
    const auto status_row = [&](std::uint32_t person_index) { return scenario.r32(manager + status_rows) + person_index * 10; };
    const auto date = scenario.toc(-0x7b80);
    const auto contract_record = scenario.global(-0x7b78) ? scenario.global(-0x7b78) : scratch_base + 0x300;
    const std::uint32_t stages[] = {10, 11, 12, 13, 14, 0x13, 15, 20};
    const std::uint32_t listed_stage_words[] = {0x02000000, 0x03000000, 0x02000300, 0x00000000};

    // One fixture per main offer: the offer's fields, a listed list and a bid list built from the other offers'
    // blocks, and the flag bytes of the people involved.
    for (std::size_t k = 0; k < 48; ++k) {
        const auto main = offers[k % offers.size()];
        const auto other1 = offers[(k + 3) % offers.size()], other2 = offers[(k + 7) % offers.size()];
        if (other1.index == main.index || other2.index == main.index || other1.index == other2.index) continue;
        std::vector<std::uint32_t> blocks;
        for (const auto& o : offers)
            if (o.index != main.index && o.index != other1.index && o.index != other2.index) blocks.push_back(o.address);
        const auto person_of = [&](const RealOffer& o) { return scenario.r32(o.address + 0xc); };
        const auto own_person = person_of(main);
        const auto listed_a = person_of(other1) % people, listed_b = person_of(other2) % people;
        Writes writes;
        std::size_t at = 0;
        const unsigned shape = unsigned(k) % 4;  // none, one listed, two listed, listed with the offer's own person
        std::vector<std::array<std::uint32_t, 2>> listed_rows;
        const auto word = listed_stage_words[(k / 4) % 4];
        if (shape == 1) listed_rows = {{scenario.r32(scenario.person(listed_a)), word}};
        if (shape == 2) listed_rows = {{scenario.r32(scenario.person(listed_a)), word}, {scenario.r32(scenario.person(listed_b)), word}};
        if (shape == 3) listed_rows = {{scenario.r32(scenario.person(listed_b)), word}, {scenario.r32(scenario.person(own_person % people)), word}};
        const auto listed_list = listed_rows.empty() ? 0 : make_list(blocks, at, listed_rows, writes);
        writes.push_back({4, main.address + 4, listed_list});
        std::uint32_t bid_list = 0;
        if ((k / 2) % 3 != 0) {
            std::vector<std::array<std::uint32_t, 2>> bids = {{other1.index, 0}, {other2.index, 0}};
            if ((k / 2) % 3 == 2) bids.insert(bids.begin(), {main.index, 0});
            bid_list = make_list(blocks, at, bids, writes);
        }
        writes.push_back({4, main.address, bid_list});
        writes.push_back({1, main.address + 0x34, stages[(k / 3) % 8]});
        writes.push_back({1, main.address + 0x10, (k % 5 == 4) ? 4u : 2u});
        writes.push_back({1, main.address + 0x36, (k % 3 == 1) ? 3u : 0u});
        writes.push_back({1, main.address + 0x37, (k % 7 == 3) ? 1u : 0u});
        writes.push_back({1, main.address + 0x38, (k % 2) ? 0x10u : 0u});
        writes.push_back({1, main.address + 0x24, (k % 6 == 5) ? 1u : 0u});
        writes.push_back({4, main.address + 0x28, (k % 4 == 2) ? other1.index : 0x7fffffffu});
        writes.push_back({1, main.address + 0x11, (k % 3 == 2) ? 0xbu : 5u});
        // Status rows: byte +1 (other offers of a person), byte +2 (listed in an offer).
        for (const auto pidx : {listed_a, listed_b, own_person % people}) {
            writes.push_back({1, status_row(pidx) + 1, std::uint32_t((k + pidx) % 2)});
            writes.push_back({1, status_row(pidx) + 2, std::uint32_t(1 + (k + pidx) % 3)});
        }
        // Other offers of the listed people: stage 0x13 for the first.
        writes.push_back({1, other1.address + 0x34, (k % 2) ? 0x13u : 12u});
        // The game's date for 0x103e80e0's comparison.
        Writes dated = writes;
        dated.push_back({2, date, (k % 3 == 0) ? 0x7fffu : (k % 3 == 1 ? 0u : 0xffffu)});
        dated.push_back({2, date + 2, (k % 4 == 0) ? 0u : 0x7d0u});
        const auto club_index = scenario.r32(main.address + 8);
        const auto own_club = scenario.club(club_index % 40), person_club = scenario.club((club_index + 5) % 40);
        const auto person_rec = [&](std::uint32_t i) { return scenario.person(i % people); };
        const auto label = [&](const char* what) { return tag(what, unsigned(k)); };
        const std::uint32_t club_choices[] = {0, own_club, person_club, scenario.club(0)};

        cases.push_back({.entry = 0x103e7cf0, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}}, .memory = writes, .label = label("advance")});
        cases.push_back({.entry = 0x103e80e0,
                         .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, club_choices[k % 4]}},
                         .memory = dated, .label = label("route")});
        cases.push_back({.entry = 0x103ebf50, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}}, .memory = writes, .label = label("settle")});
        cases.push_back({.entry = 0x103ed170,
                         .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, (k / 5) % 2 ? 0x100u : 0u}}, .memory = writes,
                         .label = label("review")});
        // 0x103e95a0: a listed person of this offer (and not), as the offer's holder and as another bid.
        const std::uint32_t movers[] = {listed_a, listed_b, own_person % people, (listed_a + 11) % people};
        for (unsigned m = 0; m < 4; ++m)
            for (const std::uint32_t flag : {1u, 0u, 0x100u}) {
                cases.push_back({.entry = 0x103e95a0,
                                 .registers = {{2, 0x108bba90}, {3, manager}, {4, person_rec(movers[m])}, {5, main.address}, {6, flag},
                                               {7, m}},
                                 .memory = writes, .label = tag("remove", unsigned(k), m * 4 + flag % 5)});
                // The person's contract is a scratch record at the offer's club: the club's offers include this one.
                Writes with_contract = writes;
                with_contract.push_back({4, contract_record + 4, scenario.r32(main.address + 8)});
                with_contract.push_back({1, contract_record + 0x38, 0x20});
                cases.push_back({.entry = 0x103e95a0,
                                 .registers = {{2, 0x108bba90}, {3, manager}, {4, person_rec(movers[m])}, {5, main.address}, {6, flag},
                                               {7, m}},
                                 .memory = with_contract, .label = tag("remove club", unsigned(k), m * 4 + flag % 5),
                                 .answers = {{0x10082f20, contract_record}}});
            }
        // 0x103e6cb0
        for (unsigned m = 0; m < 3; ++m)
            cases.push_back({.entry = 0x103e6cb0,
                             .registers = {{2, 0x108bba90}, {3, manager}, {4, club_choices[(k + m) % 4]}, {5, person_rec(movers[m])},
                                           {6, main.address}},
                             .memory = writes, .label = tag("answer", unsigned(k), m)});
        // 0x103e70f0 / 0x103e76c0: through the offer's index, for its person and for a listed person.
        for (unsigned m = 0; m < 3; ++m) {
            const auto person = person_rec(movers[m]);
            cases.push_back({.entry = 0x103e70f0,
                             .registers = {{2, 0x108bba90}, {3, manager}, {4, person}, {5, main.index}, {6, 1000 + m}, {7, 0x5u + m},
                                           {8, m}},
                             .memory = (k % 2) ? dated : writes, .label = tag("accept", unsigned(k), m)});
            cases.push_back({.entry = 0x103e76c0,
                             .registers = {{2, 0x108bba90}, {3, manager}, {4, person}, {5, main.index}, {6, (k + m) % 3 == 0 ? 1u : 0u}},
                             .memory = writes, .label = tag("reject", unsigned(k), m)});
        }
    }
    // Answers for routines outside the module: the club busy test, the news question, the contract lookup (a
    // record in the scratch area of kind 8 or 5) and a high rank.
    // The contract manager's last found contract is a real record: used as the answer, with its club and kind set.
    const auto real_contract = scenario.global(-0x7b78);
    const auto contract = real_contract ? real_contract : scratch_base + 0x300;
    for (std::size_t k = 0; k < 24; ++k) {
        const auto main = offers[(k + 1) % offers.size()];
        const auto other1 = offers[(k + 4) % offers.size()];
        if (main.index == other1.index) continue;
        std::vector<std::uint32_t> blocks;
        for (const auto& o : offers)
            if (o.index != main.index && o.index != other1.index) blocks.push_back(o.address);
        const auto person_idx = scenario.r32(other1.address + 0xc) % people;
        Writes writes;
        std::size_t at = 0;
        const auto word = k % 2 ? 0x02000000u : 0u;
        const auto listed_list = make_list(blocks, at, {{scenario.r32(scenario.person(person_idx)), word}, {scenario.r32(scenario.person((person_idx + 3) % people)), word}}, writes);
        writes.push_back({4, main.address + 4, listed_list});
        writes.push_back({4, main.address, 0});
        writes.push_back({1, main.address + 0x34, k % 3 == 0 ? 10u : 12u});
        writes.push_back({1, main.address + 0x10, 2});
        writes.push_back({1, main.address + 0x36, k % 4 == 1 ? 5u : 0u});
        writes.push_back({1, main.address + 0x37, k % 5 == 2 ? 1u : 0u});
        writes.push_back({1, main.address + 0x38, k % 2 ? 0x10u : 0u});
        writes.push_back({1, main.address + 0x24, std::uint32_t(k % 3)});
        writes.push_back({1, status_row(person_idx) + 2, 2});
        writes.push_back({1, status_row(person_idx) + 1, 1});
        writes.push_back({4, contract + 4, 5 + std::uint32_t(k % 7)});
        writes.push_back({1, contract + 0x38, k % 4 == 3 ? 0x14u : 0x20u});
        writes.push_back({2, contract + 0x42, 12});
        writes.push_back({4, contract + 0x30, 0x00050005});
        const std::uint32_t busy = (k / 2) % 2, ask = (k / 4) % 2;
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {{0x1014c2e0, busy}, {0x102cdbf0, ask}, {0x10082f20, contract}};
        const auto person = scenario.person(person_idx);
        const auto label = [&](const char* what) { return tag(what, unsigned(k), busy * 2 + ask); };
        cases.push_back({.entry = 0x103e7cf0, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}}, .memory = writes, .label = label("advance ans"), .answers = answers});
        cases.push_back({.entry = 0x103e80e0, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, contract - 0x100}}, .memory = writes, .label = label("route ans"), .answers = answers});
        cases.push_back({.entry = 0x103ed170, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, k % 3 == 0}}, .memory = writes, .label = label("review ans"),
                         .answers = {{0x10082f20, contract}, {0x1007f720, contract}}});
        // 0x103ed170's success path (k where it is reached) with each later lookup null in turn and with no list.
        {
            const std::vector<std::pair<std::uint32_t, std::uint32_t>> base = {{0x10082f20, contract}, {0x1007f720, contract}};
            const std::uint32_t quiet = k % 3 == 0;
            for (const unsigned nth : {2u, 3u, 4u})
                cases.push_back({.entry = 0x103ed170, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, quiet}}, .memory = writes,
                                 .label = tag("review ans second", unsigned(k), nth), .answers = base, .nth_answers = {{0x10082f20, nth, 0}}});
            for (const unsigned nth : {1u, 2u, 3u, 4u, 5u})
                cases.push_back({.entry = 0x103ed170, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, quiet}}, .memory = writes,
                                 .label = tag("review ans held", unsigned(k), nth), .answers = base, .nth_answers = {{0x1007f720, nth, 0}}});
            Writes no_list = writes;
            no_list.push_back({4, main.address + 4, 0});
            cases.push_back({.entry = 0x103ed170, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, quiet}}, .memory = no_list,
                             .label = tag("review ans no list", unsigned(k)), .answers = base});
            // answered: with the module's other routines answered (this only works with --only limited to the block's
            // own entries; in a run where they are under check the answers are not applied and these cases are plain).
            const std::vector<std::pair<std::uint32_t, std::uint32_t>> accepting = {
                {0x10082f20, contract}, {0x1007f720, contract}, {0x103ecef0, 1}, {0x103ed520, 0}, {0x1040a050, 0},
                {0x1040f270, 0}, {0x1040f780, 0}, {0x1036f100, 0}, {0x103eb4a0, 0}};
            for (const unsigned nth : {0u, 2u, 3u})
                cases.push_back({.entry = 0x103ed170, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, quiet}}, .memory = writes,
                                 .label = tag("answered: review accepted", unsigned(k), nth), .answers = accepting,
                                 .nth_answers = nth ? std::vector<Case::NthAnswer>{{0x10082f20, nth, 0}} : std::vector<Case::NthAnswer>{}});
            cases.push_back({.entry = 0x103ed170, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, quiet}}, .memory = no_list,
                             .label = tag("answered: review accepted no list", unsigned(k)), .answers = accepting});
            for (const unsigned nth : {2u, 3u})
                cases.push_back({.entry = 0x103ed170, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, quiet}}, .memory = no_list,
                                 .label = tag("answered: review accepted no list gone", unsigned(k), nth), .answers = accepting,
                                 .nth_answers = {{0x10082f20, nth, 0}}});
            for (const unsigned nth : {1u, 2u})
                cases.push_back({.entry = 0x103ed170, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, quiet}}, .memory = writes,
                                 .label = tag("answered: review accepted held gone", unsigned(k), nth), .answers = accepting,
                                 .nth_answers = {{0x1007f720, nth + 1, 0}}});
            // 0x103e7cf0 with 0x103ecda0 answered 1 (and the lists' people at stage 2), and with 0x103f9d60 answered 1.
            Writes regional = writes;
            regional.push_back({1, main.address + 0x34, 11});
            regional.push_back({1, main.address + 0x10, 2});
            regional.push_back({1, main.address + 0x36, 0});
            regional.push_back({1, main.address + 0x38, 0});
            regional.push_back({4, main.address, 0});
            regional.push_back({4, main.address + 4, 0});
            cases.push_back({.entry = 0x103e7cf0, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}}, .memory = regional,
                             .label = tag("answered: advance regional", unsigned(k)), .answers = {{0x103ecda0, 1}, {0x103ed090, 0}, {0x10082f20, contract}}});
            cases.push_back({.entry = 0x103e7cf0, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}}, .memory = regional,
                             .label = tag("answered: advance regional real", unsigned(k)), .answers = {{0x103ecda0, 1}, {0x10082f20, contract}}});
            for (const std::uint32_t refusals : {0u, 2u, 3u, 5u}) {
                Writes terms = writes;
                terms.push_back({1, main.address + 0x36, refusals});
                terms.push_back({1, main.address + 0x34, k % 2 ? 10u : 12u});
                cases.push_back({.entry = 0x103e7cf0, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}}, .memory = terms,
                                 .label = tag("answered: advance terms", unsigned(k), refusals),
                                 .answers = {{0x103f9d60, 1}, {0x103e8500, 0}, {0x103e44b0, 0}, {0x1014c2e0, 0}, {0x10082f20, contract}}});
            }
            // A one-row list: the accepted listed person empties it (the list is freed).
            const auto accepting_one_row = accepting;
            Writes one_row = writes;
            std::size_t one_at = 6;
            const auto one_list = make_list(blocks, one_at, {{scenario.r32(person), 0}}, one_row);
            if (one_list) {
                one_row.push_back({4, main.address + 4, one_list});
                cases.push_back({.entry = 0x103ed170, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, quiet}}, .memory = one_row,
                                 .label = tag("review ans one", unsigned(k)), .answers = base});
                cases.push_back({.entry = 0x103ed170, .registers = {{2, 0x108bba90}, {3, manager}, {4, main.address}, {5, quiet}}, .memory = one_row,
                                 .label = tag("answered: review one row accepted", unsigned(k)), .answers = accepting_one_row,
                                 .nth_answers = {{0x103ecef0, 1, 0}}});
            }
        }
        for (const std::uint32_t flag : {0u, 1u})
            cases.push_back({.entry = 0x103e95a0, .registers = {{2, 0x108bba90}, {3, manager}, {4, person}, {5, main.address}, {6, flag}, {7, 3}},
                             .memory = writes, .label = tag("remove ans", unsigned(k), flag), .answers = answers});
        cases.push_back({.entry = 0x103e76c0, .registers = {{2, 0x108bba90}, {3, manager}, {4, person}, {5, main.index}, {6, k % 3 == 0}},
                         .memory = writes, .label = label("reject ans"), .answers = answers});
        cases.push_back({.entry = 0x103e76c0, .registers = {{2, 0x108bba90}, {3, manager}, {4, scenario.person(scenario.r32(main.address + 0xc) % people)}, {5, main.index}, {6, 0}},
                         .memory = writes, .label = label("reject own ans"), .answers = answers});
        cases.push_back({.entry = 0x103e70f0, .registers = {{2, 0x108bba90}, {3, manager}, {4, person}, {5, main.index}, {6, 7}, {7, 1}, {8, 0}},
                         .memory = writes, .label = label("accept ans"), .answers = {{0x10082f20, contract}, {0x100c9070, 0x40}, {0x1014c2e0, busy}}});
        cases.push_back({.entry = 0x103e6cb0, .registers = {{2, 0x108bba90}, {3, manager}, {4, scenario.club(k % 30)}, {5, person}, {6, main.address}},
                         .memory = writes, .label = label("answer ans"), .answers = {{0x10080d30, k % 2}, {0x1036d240, k % 3 ? contract : 0}}});
    }
    // ---- real contracts (Scenario::contract_of): the paths that need a person with a club ----
    const auto array = scenario.r32(manager);
    // A person with no contract at all.
    std::uint32_t no_contract_person = 0;
    for (std::uint32_t i = 1; i < people && !no_contract_person; ++i)
        if (!scenario.contract_of(scenario.person(i))) no_contract_person = i;
    for (std::size_t k = 0; k < 40; ++k) {
        const auto main = offers[k % offers.size()];
        const auto second = offers[(k + 5) % offers.size()];
        const auto third = offers[(k + 9) % offers.size()];
        if (main.index == second.index || main.index == third.index || second.index == third.index) continue;
        const auto person_idx = scenario.r32(main.address + 0xc) % people;
        const auto person = scenario.person(person_idx);
        const auto contract = scenario.contract_of(person);
        if (!contract) continue;
        const auto contract_club = scenario.r32(contract + 4);
        const auto other_person_idx = scenario.r32(second.address + 0xc) % people;
        std::vector<std::uint32_t> blocks;
        for (const auto& o : offers)
            if (o.index != main.index && o.index != second.index && o.index != third.index) blocks.push_back(o.address);
        const auto person_word = scenario.r32(person);
        const auto other_word = scenario.r32(scenario.person(other_person_idx));
        Writes writes;
        std::size_t at = 0;
        const auto word = (k % 3 == 0) ? 0x02000000u : 0u;
        // `main` and `third` list the person at the contract's club; `second` lists nobody.
        std::vector<std::array<std::uint32_t, 2>> main_rows = {{person_word, word}};
        if (k % 2) main_rows.push_back({other_word, word});
        writes.push_back({4, main.address + 4, make_list(blocks, at, main_rows, writes)});
        writes.push_back({4, third.address + 4, k % 4 == 3 ? 0u : make_list(blocks, at, {{other_word, word}, {person_word, word}}, writes)});
        writes.push_back({4, main.address, 0});
        writes.push_back({4, main.address + 8, contract_club});
        writes.push_back({4, third.address + 8, contract_club});
        writes.push_back({1, status_row(person_idx) + 2, k % 2 ? 2u : 1u});
        writes.push_back({1, status_row(person_idx) + 1, 1});
        writes.push_back({1, main.address + 0x34, k % 2 ? 11u : 10u});
        writes.push_back({1, main.address + 0x10, 2});
        writes.push_back({1, main.address + 0x24, std::uint32_t(k % 2)});
        writes.push_back({1, main.address + 0x36, 0});
        writes.push_back({1, main.address + 0x37, 0});
        writes.push_back({1, main.address + 0x38, (k % 4 == 1) ? 0x10u : 0u});
        writes.push_back({4, main.address + 0x20, k % 5 == 0 ? 0x7fffffffu : (k % 5 == 1 ? 0u : 40000u)});
        const std::uint32_t reg2 = 0x108bba90;
        // Some fixtures make the offer's club a regional-approval club (its nation's type byte +0x118 is 14), so
        // 0x103ecda0 can say yes for a person without a kind-2 contract.
        if (k % 3 == 0)
            if (const auto nation = scenario.r32(scenario.club(scenario.r32(main.address + 8)) + 0x2c))
                writes.push_back({1, nation + 0x118, 14});
        // 0x103e95a0 on the offer `second` and on `main` (the offer under walk is then skipped).
        for (const std::uint32_t flag : {0u, 1u})
            for (const auto target : {second.address, main.address}) {
                cases.push_back({.entry = 0x103e95a0, .registers = {{2, reg2}, {3, manager}, {4, person}, {5, target}, {6, flag}, {7, 3}},
                                 .memory = writes, .label = tag("remove real", unsigned(k), flag * 2 + (target == main.address))});
            }
        // The offer's own person with a contract: 0x103e70f0 / 0x103e76c0 equal branch with and without a club (kind 5 too).
        for (const std::uint32_t kind : {0x20u, 0x14u}) {
            Writes kinded = writes;
            kinded.push_back({1, contract + 0x38, kind});
            for (const std::uint32_t rating : {0u, 1u, 2u, 3u, 4u, 5u, 6u}) {
                Writes rated = kinded;
                const auto mood_holder = scenario.r32(person + 0x14);
                const std::uint32_t strong[] = {0x2000, 0x2000, 0x1100, 0x2000, 0x1000, 0x800, 0x1090}, weak[] = {0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0};
                const std::uint32_t mood[] = {0x1000, 0x1400, 0, 0x2700, 0x2700, 0x270f, 0x270f};
                // The offer's club field holds the club index; its record is the table row of that index.
                                // The offer's club differs from the contract's, so the two ratings are different clubs'.
                const auto buying_club = scenario.r32(second.address + 8) != contract_club ? scenario.r32(second.address + 8) : contract_club + 1;
                rated.push_back({4, main.address + 8, buying_club});
                rated.push_back({2, club_row(scenario, buying_club) + 0x1ec, strong[rating]});
                rated.push_back({2, club_row(scenario, contract_club) + 0x1ec, weak[rating]});
                if (mood_holder) rated.push_back({2, mood_holder + 0xa, mood[rating]});
                cases.push_back({.entry = 0x103e76c0, .registers = {{2, reg2}, {3, manager}, {4, person}, {5, main.index}, {6, 0}},
                                 .memory = rated, .label = tag("reject real", unsigned(k), kind * 8 + rating)});
                if (rating < 2)
                    cases.push_back({.entry = 0x103e70f0, .registers = {{2, reg2}, {3, manager}, {4, person}, {5, main.index}, {6, contract}, {7, 1}, {8, 0}},
                                     .memory = rated, .label = tag("accept real", unsigned(k), kind * 8 + rating)});
            }
        }
        // 0x103e70f0 / 0x103e76c0: the person listed by `main` but not its own person.
        cases.push_back({.entry = 0x103e76c0, .registers = {{2, reg2}, {3, manager}, {4, scenario.person(other_person_idx)}, {5, main.index}, {6, std::uint32_t(k % 2)}},
                         .memory = writes, .label = tag("reject listed real", unsigned(k))});
        // 0x103e76c0 on a listed person while the offer's own person has no contract.
        if (no_contract_person) {
            Writes none = writes;
            none.push_back({4, main.address + 0xc, no_contract_person});
            for (const std::uint32_t quiet : {0u, 1u})
                for (const std::uint32_t slot : {0u, 1u}) {
                    const auto who = slot ? scenario.person(other_person_idx) : person;
                    cases.push_back({.entry = 0x103e76c0, .registers = {{2, reg2}, {3, manager}, {4, who}, {5, main.index}, {6, quiet}},
                                     .memory = none, .label = tag("reject no contract", unsigned(k), quiet * 2 + slot)});
                }
        }
        // The walks over a real contract's club: advance, review and settle.
        for (const std::uint32_t quiet : {0u, 1u}) {
            cases.push_back({.entry = 0x103ed170, .registers = {{2, reg2}, {3, manager}, {4, main.address}, {5, quiet}},
                             .memory = writes, .label = tag("review real", unsigned(k), quiet)});
            cases.push_back({.entry = 0x103ed170, .registers = {{2, reg2}, {3, manager}, {4, main.address}, {5, quiet}},
                             .memory = writes, .label = tag("review real a", unsigned(k), quiet),
                             .answers = {{0x1007f720, contract}}});
            cases.push_back({.entry = 0x103ed170, .registers = {{2, reg2}, {3, manager}, {4, main.address}, {5, quiet}},
                             .memory = writes, .label = tag("review real b", unsigned(k), quiet),
                             .nth_answers = {{0x1007f720, 1, contract}, {0x1007f720, 2, 0}, {0x1007f720, 3, contract}}});
        }
        cases.push_back({.entry = 0x103e7cf0, .registers = {{2, reg2}, {3, manager}, {4, main.address}}, .memory = writes, .label = tag("advance real", unsigned(k))});
        // The collected offers' removal on a null array slot (the original reads the slot as null).
        Writes null_slot = writes;
        null_slot.push_back({4, array + main.index * 4, 0});
        cases.push_back({.entry = 0x103e70f0, .registers = {{2, reg2}, {3, manager}, {4, person}, {5, main.index}, {6, 5}, {7, 1}, {8, 0}},
                         .memory = null_slot, .label = tag("accept null slot", unsigned(k))});
        cases.push_back({.entry = 0x103e76c0, .registers = {{2, reg2}, {3, manager}, {4, person}, {5, main.index}, {6, 0}},
                         .memory = null_slot, .label = tag("reject null slot", unsigned(k))});
        // Accepting: the person's state byte 2, the shortlist/squad tests answered, no club or kind 5 contract.
        for (unsigned v = 0; v < 6; ++v) {
            Writes accept = writes;
            std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {{0x10362d70, v % 3 ? 1u : 0u}, {0x104ae570, v % 2}, {0x1014c2e0, v % 4 == 3}, {0x10083e50, 0}, {0x100ce200, 0}, {0x10006600, 0}, {0x104ade40, 0}, {0x100df500, 0}, {0x103864b0, 0}, {0x104ad840, 0}};
            if (v >= 4) answers.push_back({0x10082f20, v == 4 ? 0u : contract});
            cases.push_back({.entry = 0x103e70f0, .registers = {{2, reg2}, {3, manager}, {4, person}, {5, main.index}, {6, contract}, {7, 1}, {8, 0}},
                             .memory = accept, .label = tag("accept answered", unsigned(k), v), .answers = answers,
                             .pokes = {{0x10083e50, {{1, person + 0x6d, v % 2 ? 2u : 0u}}}}});
            cases.push_back({.entry = 0x103e76c0, .registers = {{2, reg2}, {3, manager}, {4, person}, {5, main.index}, {6, 0}},
                             .memory = accept, .label = tag("reject answered", unsigned(k), v), .answers = answers});
        }
        // 0x103e7cf0 with bids: a null array slot, the offer's own deal index, and plain ones; and with the club busy
        // (1014c2e0 true) at both refusal counts, for 0x103f9d60's answer 1.
        {
            std::vector<std::uint32_t> bid_blocks;
            for (const auto& o : offers)
                if (o.index != main.index && o.index != second.index && o.index != third.index) bid_blocks.push_back(o.address);
            for (unsigned v = 0; v < 3; ++v) {
                Writes bids = writes;
                std::size_t bat = 8;
                const auto list = make_list(bid_blocks, bat, {{second.index, 0}, {third.index, 0}}, bids);
                if (!list) continue;
                bids.push_back({4, main.address, list});
                if (v == 0) bids.push_back({4, array + second.index * 4, 0});
                if (v == 1) bids.push_back({4, main.address + 0x28, second.index});
                cases.push_back({.entry = 0x103e7cf0, .registers = {{2, reg2}, {3, manager}, {4, main.address}}, .memory = bids,
                                 .label = tag("advance bids", unsigned(k), v)});
            }
            for (const std::uint32_t refusals : {0u, 5u}) {
                Writes busy = writes;
                busy.push_back({1, main.address + 0x36, refusals});
                busy.push_back({1, main.address + 0x34, 10});
                cases.push_back({.entry = 0x103e7cf0, .registers = {{2, reg2}, {3, manager}, {4, main.address}}, .memory = busy,
                                 .label = tag("advance busy", unsigned(k), refusals), .answers = {{0x1014c2e0, 1}}});
            }
        }
        // 0x103ed170 beyond the success path: the second contract lookup, the held contract or the third lookup null,
        // and a missing list.
        {
            Writes no_list = writes;
            no_list.push_back({4, main.address + 4, 0});
            cases.push_back({.entry = 0x103ed170, .registers = {{2, reg2}, {3, manager}, {4, main.address}, {5, 0}}, .memory = no_list,
                             .label = tag("review no list", unsigned(k)), .answers = {{0x1007f720, contract}}});
            cases.push_back({.entry = 0x103ed170, .registers = {{2, reg2}, {3, manager}, {4, main.address}, {5, 0}}, .memory = writes,
                             .label = tag("review second null", unsigned(k)), .answers = {{0x1007f720, contract}},
                             .nth_answers = {{0x10082f20, 2, 0}}});
            cases.push_back({.entry = 0x103ed170, .registers = {{2, reg2}, {3, manager}, {4, main.address}, {5, 0}}, .memory = writes,
                             .label = tag("review third null", unsigned(k)), .answers = {{0x1007f720, contract}},
                             .nth_answers = {{0x10082f20, 3, 0}}});
            for (const unsigned nth : {2u, 3u, 4u, 5u})
                cases.push_back({.entry = 0x103ed170, .registers = {{2, reg2}, {3, manager}, {4, main.address}, {5, 0}}, .memory = writes,
                                 .label = tag("review held null", unsigned(k), nth), .answers = {{0x1007f720, contract}},
                                 .nth_answers = {{0x1007f720, nth, 0}}});
            // The person's contract index goes to -1 before the second lookup (the first routine called after the club
            // is told the offer was agreed) and before the third (after the held contract).
            const auto index_cell = scenario.global(-0x7b9c) + person_word * 100;
            for (const std::uint32_t at_call : {0x1040a050u, 0x1007f720u})
                cases.push_back({.entry = 0x103ed170, .registers = {{2, reg2}, {3, manager}, {4, main.address}, {5, 0}}, .memory = writes,
                                 .label = tag("review lookup gone", unsigned(k), at_call & 0xff), .answers = {{0x1007f720, contract}},
                                 .pokes = {{at_call, {{4, index_cell, 0xffffffffu}}}}});
            // The shortlist entry for 0x103e6cb0's own-person branch, answered with a record of the scratch area.
            Writes entry = writes;
            entry.push_back({1, scratch_base + 0x700, k % 2 ? 0xbu : 3u});
            cases.push_back({.entry = 0x103e6cb0, .registers = {{2, reg2}, {3, manager}, {4, scenario.club(k % 30)}, {5, person}, {6, main.address}},
                             .memory = entry, .label = tag("answer entry", unsigned(k)),
                             .answers = {{0x1036d240, scratch_base + 0x700}, {0x10365590, 0}}});
        }
        // 0x103e80e0: the offer's own club at stages 11-13 (and 12 for another club).
        for (const std::uint32_t stage : {11u, 12u, 13u}) {
            Writes staged = writes;
            staged.push_back({1, main.address + 0x34, stage});
            cases.push_back({.entry = 0x103e80e0, .registers = {{2, reg2}, {3, manager}, {4, main.address}, {5, club_row(scenario, scenario.r32(main.address + 8))}},
                             .memory = staged, .label = tag("route own", unsigned(k), stage)});
            for (unsigned d = 0; d < 12; ++d) {
                Writes dated_stage = staged;
                dated_stage.push_back({1, main.address + 0x38, std::uint32_t(((d + stage) % 6 == 0 ? 1u : (d + stage) % 6 == 1 ? 4u : (d + stage) % 6 == 2 ? 5u : (d + stage) % 6 == 3 ? 2u : (d + stage) % 6 == 4 ? 3u : 7u) << 4)});
                for (unsigned b = 0; b < 7; ++b) dated_stage.push_back({1, main.address + 0x39 + b, std::uint32_t((d * 37 + b * 11 + k) % 256)});
                dated_stage.push_back({2, date + 2, (d / 2) % 3 == 0 ? 0u : ((d / 2) % 3 == 1 ? 0x7d1u : 0xffffu)});
                dated_stage.push_back({2, date, d % 2 ? 0x7000u : 0x8001u});
                cases.push_back({.entry = 0x103e80e0, .registers = {{2, reg2}, {3, manager}, {4, main.address}, {5, club_row(scenario, scenario.r32(main.address + 8))}},
                                 .memory = dated_stage, .label = tag("route dated", unsigned(k), stage * 8 + d)});
            }
        }
    }
    // Index out of range and offers that are not there.
    const auto main = offers[0];
    for (const std::uint32_t index : {0xffffffffu, 0x7fffffffu, scenario.r32(manager + 4), scenario.r32(manager + 4) + 5}) {
        cases.push_back({.entry = 0x103e70f0, .registers = {{2, 0x108bba90}, {3, manager}, {4, scenario.person(0)}, {5, index}, {6, 5}, {7, 1}, {8, 0}},
                         .label = tag("accept range", index)});
        cases.push_back({.entry = 0x103e76c0, .registers = {{2, 0x108bba90}, {3, manager}, {4, scenario.person(0)}, {5, index}, {6, 0}},
                         .label = tag("reject range", index)});
    }
    (void)main;
}
}
