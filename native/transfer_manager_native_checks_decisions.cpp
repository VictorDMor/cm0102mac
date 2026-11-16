// Cases for transfer_manager_native_decisions*.cpp: 0x103eed00, 0x103ef580, 0x103fb630, 0x103fdc20, 0x10410e50.
#include "guest_abi.hpp"
#include "transfer_manager_native_checks.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <limits>

namespace cm::aot::transfer_manager_native_checks {
namespace {
// Real offers of the manager (an array of pointers at +0, its size at +4), spread over the array.
std::vector<std::uint32_t> real_offers(const Scenario& scenario, unsigned wanted) {
    std::vector<std::uint32_t> offers;
    const auto manager = scenario.transfer_manager();
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    if (size == 0 || size > 100000) return offers;
    const auto step = size / wanted + 1;
    for (std::uint32_t i = 0; i < size; i += step)
        if (const auto offer = scenario.r32(array + i * 4)) offers.push_back(offer);
    return offers;
}
std::string tag(const char* what, unsigned a, unsigned b = 0) { return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b); }
std::uint32_t caller_sp(const Scenario& scenario) { return guest::Cpu(scenario.s).sp(); }
}


// ---- paths the snapshots' own offers and lists never take (coverage closure) ----
using Writes = std::vector<Write>;
// A guest list (header {halfword count, head, tail, cursor}, nodes {data, halfword size, next, prev}, data
// records of two words) built out of real offer blocks, because the game's own code frees what it unlinks
// and a scratch address would not be a heap block. `blocks` are real offers not otherwise used by the case.
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
// The same in the scratch area, for lists nothing frees (the planning routine only reads the offers' lists).
std::uint32_t make_scratch_list(std::uint32_t at, const std::vector<std::array<std::uint32_t, 2>>& records, Writes& writes) {
    const auto header = at;
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
    writes.push_back({2, header, std::uint32_t(records.size())});
    writes.push_back({4, header + 4, records.empty() ? 0 : nodes});
    writes.push_back({4, header + 8, records.empty() ? 0 : nodes + 0x10 * std::uint32_t(records.size() - 1)});
    writes.push_back({4, header + 0xc, 0});
    return header;
}
void add_to(Writes& to, const Writes& more) { to.insert(to.end(), more.begin(), more.end()); }

TRANSFER_MANAGER_NATIVE_CASES("decisions") {
    if (scenario.match) return;
    const auto first_case = cases.size();
    const auto manager = scenario.transfer_manager();
    const auto offers = real_offers(scenario, 30);
    const auto people = scenario.people();
    const auto out_offer = scratch_base + 0x20, out_reaction = scratch_base + 0x30;

    // ---- 0x103fb630: removing an offer; and 0x103fdc20: its news ----
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto offer = offers[i];
        cases.push_back({.entry = 0x103fb630, .registers = {{3, manager}, {4, offer}}, .label = tag("remove", unsigned(i))});
        // The bid list path (kind byte 1) and the held-slot path (+0x30 naming a slot of the held array).
        cases.push_back({.entry = 0x103fb630, .registers = {{3, manager}, {4, offer}}, .memory = {{1, offer + 0x24, 1}},
                         .label = tag("remove kind1", unsigned(i))});
        cases.push_back({.entry = 0x103fb630, .registers = {{3, manager}, {4, offer}}, .memory = {{1, offer + 0x24, 0}},
                         .label = tag("remove kind0", unsigned(i))});
        // The held-slot path: +0x30 names a slot of the manager's held array (null in the snapshots), which
        // is given here a real offer to destroy.
        if (offers.size() > 1) {
            const auto held = scratch_base + 0x400;
            cases.push_back({.entry = 0x103fb630, .registers = {{3, manager}, {4, offer}},
                             .memory = {{4, manager + 0xc, held}, {4, held + 8, offers[(i + 1) % offers.size()]}, {4, offer + 0x30, 2}},
                             .label = tag("remove held", unsigned(i))});
        }
        cases.push_back({.entry = 0x103fdc20, .registers = {{3, manager}, {4, offer}}, .label = tag("news", unsigned(i))});
        // Signing flag (+0x37) and the registration kind (+0x38 high nibble 5).
        for (const std::uint32_t signing : {0u, 1u})
            for (const std::uint32_t kind : {0x00u, 0x50u, 0x5fu, 0x40u})
                for (const std::uint32_t status : {0x00u, 0x12u, 0xf0u})
                    cases.push_back({.entry = 0x103fdc20, .registers = {{3, manager}, {4, offer}},
                                     .memory = {{1, offer + 0x37, signing}, {1, offer + 0x38, kind | (status & 0xf)}, {1, offer + 0x3c, status}},
                                     .label = tag("news signing", unsigned(i), signing * 16 + kind / 16 + status)});
    }

    // ---- 0x103ef580 and 0x103eed00: people with a career record, clubs spread over the table ----
    const unsigned club_indices[] = {0, 1, 3, 8, 17, 29, 40, 77, 120, 200};
    for (unsigned k = 0; k < 28; ++k) {
        const auto person_index = (k * (people / 28 + 1) + 5) % people;
        const auto person = scenario.person(person_index);
        const auto career = scenario.r32(person + 0x14);
        const auto contract_pointer = scenario.r32(person + 0xc);
        for (unsigned j = 0; j < 10; ++j) {
            const auto club = scenario.club(club_indices[(j + k) % 10]);
            const auto label = tag("reaction", person_index, club_indices[(j + k) % 10]);
            cases.push_back({.entry = 0x103ef580, .registers = {{3, manager}, {4, club}, {5, person}, {6, out_offer}, {7, out_reaction}, {8, 0}}, .label = label});
            if (!career) continue;
            // The club far ahead of the seller, and the value/rank and contract tests varied.
            for (const std::uint32_t rank : {0x200u, 12u, 3u})
                for (const std::uint32_t value : {0x7000u, 0x2000u, 0xe000u, 0u})
                    for (const std::uint32_t protect : {0u, 1u}) {
                        std::vector<Write> writes = {{2, club + 0x1ec, rank}, {2, career + 0xa, value}, {1, person + 0x5e, protect},
                                                     {2, career + 4, 0}, {2, career + 6, 0x40}, {1, person + 0x5d, 10}};
                        if (contract_pointer) writes.push_back({2, contract_pointer + 0xac, protect ? 0x2000u : 0x100u});
                        cases.push_back({.entry = 0x103ef580, .registers = {{3, manager}, {4, club}, {5, person}, {6, out_offer}, {7, out_reaction}, {8, 0x7}},
                                         .memory = writes, .label = tag("reaction forced", person_index, rank * 8 + value % 7 + protect)});
                    }
        }
    }
    const std::uint32_t budgets[] = {5'000'000u, 300'000u, 0u, 0x7fffffffu, 40'000'000u};
    for (unsigned k = 0; k < 60; ++k) {
        const auto person_index = (k * (people / 60 + 1) + 11) % people;
        const auto person = scenario.person(person_index);
        const auto seller = scenario.r32(person + 0x24);
        const auto terms = offers.empty() ? scratch_base + 0x100 : offers[k % offers.size()];
        std::vector<Write> writes;
        std::vector<std::uint32_t> buyers;
        // The seller club's row of the manager's table must be switched on, or nothing is offered; its 25
        // entries (0x2c each, a person index at +0x10) name the people whose clubs the plan looks at.
        if (seller) {
            const auto row = scenario.r32(scenario.r32(manager + 0x988) + 0xdc) + scenario.r32(seller) * 0xc;
            writes.push_back({1, row + 4, 1});
            const auto list = scenario.r32(row);
            for (unsigned entry = 0; list && entry < 25 && buyers.size() < 4; ++entry) {
                const auto index = scenario.r32(list + entry * 0x2c + 0x10);
                if (index >= people) continue;
                if (const auto club = scenario.r32(scenario.person(index) + 0x24); club && club != seller) buyers.push_back(club);
            }
        }
        for (unsigned j = 0; j < 3; ++j) buyers.push_back(scenario.club(club_indices[(j * 3 + k) % 10] + j));
        for (std::size_t j = 0; j < buyers.size(); ++j) {
            const auto budget = budgets[(k + j) % 5];
            const std::vector<std::pair<unsigned, std::uint32_t>> registers = {{3, manager}, {4, buyers[j]}, {5, person}, {6, terms}, {7, out_offer}, {8, out_reaction}, {9, budget}};
            cases.push_back({.entry = 0x103eed00, .registers = registers, .memory = writes, .label = tag("plan", person_index, unsigned(j))});
            if (j == 0) cases.push_back({.entry = 0x103eed00, .registers = registers, .label = tag("plan row as is", person_index, 0)});
        }
    }

    // Candidates that already have open offers (the early checks count them): the club is the one of an
    // offered player, the person a player of that club's competitor from the same list.
    for (std::size_t i = 0; i + 1 < offers.size(); ++i) {
        const auto offered = scenario.r32(offers[i] + 0xc);
        if (offered >= people) continue;
        const auto club = scenario.r32(scenario.person(offered) + 0x24);
        const auto target = scenario.person((offered + 977 * (unsigned(i) + 1)) % people);
        const auto seller = scenario.r32(target + 0x24);
        if (!club || !seller) continue;
        const auto row = scenario.r32(scenario.r32(manager + 0x988) + 0xdc) + scenario.r32(seller) * 0xc;
        std::vector<Write> writes = {{1, row + 4, 1}};
        // The seller's row names the offered player so the club is one the plan visits.
        const auto list = scenario.r32(row);
        if (list) writes.push_back({4, list + 0x10, offered});
        for (const std::uint32_t budget : {40'000'000u, 0x7fffffffu})
            cases.push_back({.entry = 0x103eed00,
                             .registers = {{3, manager}, {4, club}, {5, target}, {6, offers[i]}, {7, out_offer}, {8, out_reaction}, {9, budget}},
                             .memory = writes, .label = tag("plan open offers", unsigned(i), budget & 1)});
    }

    // ---- 0x10410e50: the refusal texts ----
    const auto sp = caller_sp(scenario);
    const auto out = scratch_base + 0x800;
    const auto stadium = scratch_base + 0x1000, coach = scratch_base + 0x1200, standing_a = scratch_base + 0x1400, standing_b = scratch_base + 0x1440;
    std::vector<Write> fixtures = {{1, stadium + 0x115, 7}, {1, stadium + 0xae, 'S'}, {1, stadium + 0xaf, 't'}, {1, stadium + 0xb0, 0},
                                   {4, stadium + 0x74, coach}, {1, coach + 0xc3, 5}, {1, coach + 0x10, 'C'}, {1, coach + 0x11, 'o'}, {1, coach + 0x12, 0},
                                   {4, standing_a + 8, 0x1234}, {4, standing_b + 8, 0x1234}, {2, standing_a + 0x18, 9}, {2, standing_b + 0x18, 4}};
    const std::uint32_t bits[] = {0x8, 0x20, 0x400, 0x1000, 0x100000, 0x1, 0x400000, 0x80000, 0x8000, 0x1000000, 0x2000000, 0x10000, 0x20000000};
    std::vector<std::uint32_t> flag_sets = {0x80000000u};
    for (std::size_t a = 0; a < std::size(bits); ++a) {
        flag_sets.push_back(bits[a]);
        for (std::size_t b = a + 1; b < std::size(bits); ++b) flag_sets.push_back(bits[a] | bits[b]);
    }
    flag_sets.push_back(0x7fffffffu);
    flag_sets.push_back(0xffffffffu);
    flag_sets.push_back(0x8 | 0x20 | 0x400);
    flag_sets.push_back(0x1 | 0x400000 | 0x80000 | 0x20000000);
    flag_sets.push_back(0x400000 | 0x8000 | 0x1000000 | 0x20000000);
    const double fees[] = {0.0, 1.0e6, 1.0e9, 3.0e13, -5.0, std::numeric_limits<double>::quiet_NaN()};
    struct Pair { int club, other; unsigned mode; };
    const Pair pairs[] = {{-1, -1, 0}, {3, -1, 0}, {-1, 5, 0}, {3, 5, 0}, {17, 29, 0}, {3, 5, 1}, {17, 29, 2}, {40, 41, 3}, {40, 41, 4}, {77, 120, 5}, {200, 8, 6}, {3, 5, 7}, {17, 29, 7}};
    for (std::size_t p = 0; p < std::size(pairs); ++p) {
        const auto [club, other, mode] = pairs[p];
        std::vector<Write> writes = {{4, sp + 0x38, out}};
        const auto club_record = club >= 0 ? scenario.club(unsigned(club)) : 0, other_record = other >= 0 ? scenario.club(unsigned(other)) : 0;
        if (mode == 1 && club_record && other_record) writes.push_back({4, other_record + 0x2c, scenario.r32(club_record + 0x2c)});
        if (mode == 2 && other_record) { for (const auto& w : fixtures) writes.push_back(w); writes.push_back({4, other_record + 0x2c, stadium}); }
        if (mode == 3 && club_record && other_record) {
            for (const auto& w : fixtures) writes.push_back(w);
            writes.push_back({4, club_record + 0x30, standing_a});
            writes.push_back({4, other_record + 0x30, standing_b});
        }
        if (mode == 4 && club_record && other_record) {
            for (const auto& w : fixtures) writes.push_back(w);
            writes.push_back({4, club_record + 0x30, standing_b});
            writes.push_back({4, other_record + 0x30, standing_a});
        }
        if (mode == 5 && club_record) writes.push_back({4, club_record + 0x14, 0xffffffffu});
        if (mode == 7 && club_record) writes.push_back({4, club_record + 0x14, 1});
        if (mode == 6 && other_record) {
            for (const auto& w : fixtures) writes.push_back(w);
            writes.push_back({4, other_record + 0x2c, stadium});
            writes.push_back({4, stadium + 0x74, 0});
        }
        for (std::size_t f = 0; f < flag_sets.size(); ++f)
            for (std::size_t m = 0; m < std::size(fees); ++m) {
                if (m > 1 && f % 5 != 0) continue;
                cases.push_back({.entry = 0x10410e50,
                                 .registers = {{4, std::uint32_t(club)}, {5, std::uint32_t(other)}, {8, flag_sets[f]}},
                                 .memory = writes, .f1 = fees[m], .label = tag("refusal", unsigned(p * 1000 + f), unsigned(m))});
            }
    }
    // ---- coverage closure: 0x103fb630 with the bid list and the offer's own player list ----
    const std::uint32_t high_water = 0x7fffffff;
    for (std::size_t i = 0; i + 8 < offers.size() && i < 6; ++i) {
        const auto offer = offers[i];
        std::vector<std::uint32_t> blocks;
        for (std::size_t k = 0; k < offers.size(); ++k) if (k != i) blocks.push_back(offers[k]);
        const auto person_word = scenario.r32(offer + 0xc), club_word = scenario.r32(offer + 8);
        // Bid list (manager+0xa78): a wrong-club entry, a wrong-person entry, then the matching one.
        {
            Writes writes = {{1, offer + 0x24, 1}, {4, manager + 0x990, high_water}};
            std::size_t at = 0;
            const auto list = make_list(blocks, at, {{person_word, club_word + 1}, {person_word + 1, club_word}, {person_word, club_word}}, writes);
            writes.push_back({4, manager + 0xa78, list});
            if (list) cases.push_back({.entry = 0x103fb630, .registers = {{3, manager}, {4, offer}}, .memory = writes, .label = tag("remove bids", unsigned(i))});
            Writes none = {{1, offer + 0x24, 1}};
            std::size_t at2 = 0;
            const auto list2 = make_list(blocks, at2, {{person_word + 5, club_word}}, none);
            none.push_back({4, manager + 0xa78, list2});
            if (list2) cases.push_back({.entry = 0x103fb630, .registers = {{3, manager}, {4, offer}}, .memory = none, .label = tag("remove bids none", unsigned(i))});
        }
        // The offer's own list of people, with the offer's person varied (one without a contract too).
        for (const std::uint32_t person_index : {person_word, 14437u % scenario.people(), 77u, 5000u % scenario.people()}) {
            Writes writes = {{4, offer + 0xc, person_index}};
            std::size_t at = 0;
            const auto list = make_list(blocks, at, {{3, 0}, {900, 0}, {person_index, 0}}, writes);
            writes.push_back({4, offer + 4, list});
            if (list) cases.push_back({.entry = 0x103fb630, .registers = {{3, manager}, {4, offer}}, .memory = writes, .label = tag("remove people", unsigned(i), person_index)});
        }
        cases.push_back({.entry = 0x103fb630, .registers = {{3, manager}, {4, offer}}, .memory = {{4, manager + 0x990, high_water}}, .label = tag("remove lowest", unsigned(i))});
    }

    // ---- coverage closure: the refusal texts ----
    {
        const auto pool = scenario.toc(-0x4ba0);
        std::uint64_t bits = (std::uint64_t(scenario.r32(pool + 0x1b0)) << 32) | scenario.r32(pool + 0x1b4);
        double limit;
        std::memcpy(&limit, &bits, 8);
        const auto table = scenario.r32(scenario.toc(-0x7984));
        for (const std::uint32_t flags : {0x1u, 0x400000u | 0x1u, 0x8000u, 0x8000u | 0x80000u, 0x1000000u, 0x2000000u, 0x2000000u | 0x1000000u, 0x10000u}) {
            const std::vector<Write> base = {{4, sp + 0x38, out}};
            auto add_case = [&](const char* what, std::int32_t club, std::int32_t other, const Writes& extra, double fee) {
                Writes writes = base;
                add_to(writes, extra);
                cases.push_back({.entry = 0x10410e50, .registers = {{4, std::uint32_t(club)}, {5, std::uint32_t(other)}, {8, flags}}, .memory = writes, .f1 = fee,
                                 .label = tag(what, flags % 1000, unsigned(club))});
            };
            add_case("refusal equal fee", 3, 5, {}, limit);
            add_case("refusal league entry", 3, 5, {{4, scenario.club(3) + 0x14, 0xffffffffu}, {4, table + 3 * 8, 1}}, 0.0);
            add_case("refusal other standing missing", 3, 5, {{4, scenario.club(3) + 0x30, standing_a}, {4, standing_a + 8, 1}, {4, scenario.club(5) + 0x30, 0}}, 0.0);
            add_case("refusal no stadium", 3, 5, {{4, scenario.club(5) + 0x2c, 0}}, 0.0);
            add_case("refusal stadium no coach", 3, 5, {{4, scenario.club(5) + 0x2c, stadium}, {4, stadium + 0x74, 0}}, 0.0);
        }
    }

    // ---- coverage closure: 0x103fdc20 with human recipients (the club test 0x102cdbf0 answered 1) ----
    {
        const auto date_at = scenario.toc(-0x7b80);
        const auto today = scenario.r32(date_at);
        const std::uint32_t today_day = today & 0xffff, today_year = today >> 16;
        const auto contract_scratch = scratch_base + 0x3000;  // two scratch contract records, 0x80 apart
        const auto bid_record = scratch_base + 0x3200;
        for (std::size_t i = 0; i < offers.size() && i < 3; ++i) {
            const auto offer = offers[i];
            const auto person_index = scenario.r32(offer + 0xc);
            const auto person = scenario.person(person_index % people);
            std::vector<std::uint32_t> blocks;
            for (std::size_t k = 0; k < offers.size(); ++k) if (k != i) blocks.push_back(offers[k]);
            struct Date { std::uint32_t year, day; const char* name; };
            const Date dates[] = {{today_year, today_day + 1, "later"}, {today_year, today_day - 1, "earlier"}, {today_year, today_day, "same"},
                                  {today_year + 1, today_day, "year later"}, {today_year - 1, today_day, "year earlier"}};
            for (const auto signing : {0u, 1u})
                for (const auto kind : {0x50u, 0x40u})
                    for (const auto flag_test : {0u, 1u}) {
                        if (!signing && flag_test) continue;
                        std::vector<Case::NthAnswer> none;
                        auto make = [&](const char* what, Writes writes, std::vector<Case::NthAnswer> nth, std::uint32_t bid, std::uint32_t club_check) {
                            add_to(writes, {{1, offer + 0x37, signing}, {1, offer + 0x38, kind}, {4, person + 0x20, bid_record}, {4, bid_record, 77}});
                            cases.push_back({.entry = 0x103fdc20, .registers = {{3, manager}, {4, offer}}, .memory = writes,
                                             .label = tag(what, unsigned(i), signing * 100 + kind / 16 * 10 + flag_test),
                                             .answers = {{0x102cdbf0, 1}, {0x100c7a20, flag_test}, {0x100ce250, bid}, {0x10012e90, club_check}},
                                             .nth_answers = nth});
                        };
                        for (const auto bid : {0u, 1u}) for (const auto club_check : {0u, 1u}) make("news human", {}, {}, bid, club_check);
                        // The first contract lookup answered with a kind-5 record or a plain one.
                        make("news kind5 contract", {{1, contract_scratch + 0x38, 0x14}, {4, contract_scratch + 4, 3}}, {{0x10082f20, 1, contract_scratch}}, 1, 1);
                        // The second lookup (the registration window), at the dispatch counts it can have.
                        for (unsigned nth = 2; nth <= 9; ++nth) {
                            if (nth == 2 && signing) continue;
                            make("news no contract", {}, {{0x10082f20, nth, 0}}, 1, 0);
                            for (const auto& date : dates) {
                                const auto record = contract_scratch + 0x80;
                                Writes writes = {{2, record + 0x30, date.year}, {2, record + 0x32, date.day}, {1, record + 0x38, 0x14 + nth}, {4, record + 4, 3}};
                                make(date.name, writes, {{0x10082f20, nth, record}}, 0, 1);
                            }
                        }
                        // The player list (field 0x18 on): four members from real persons.
                        {
                            Writes writes;
                            std::size_t at = 0;
                            const auto list = make_list(blocks, at, {{person_index % people, 0}, {(person_index + 7) % people, 0}, {(person_index + 19) % people, 0}, {(person_index + 31) % people, 0}}, writes);
                            writes.push_back({4, offer + 4, list});
                            if (list) make("news list", writes, {}, 1, 1);
                            if (list) make("news list empty bid", writes, {}, 0, 0);
                        }
                        // The kind-5 date block against a changed game year.
                        if (kind == 0x50) {
                            make("news year", {{2, date_at, today_year + 1}}, {}, 1, 1);
                            // The date global's first halfword (the window position 0x103fb130 compares) moved so that
                            // the date it returns keeps today's second halfword but not the first.
                            for (const std::uint32_t first_half : {0u, 3u, 30u, 100u, 200u, 400u, 0x7fffu})
                                make("news date half", {{2, date_at, first_half}}, {}, 1, 1);
                        }
                    }
        }
    }

    // ---- coverage closure: 0x103eed00 with chosen candidates (0x100d06f0 answered, its array written) ----
    {
        std::map<std::uint32_t, std::vector<std::uint32_t>> squads;  // club record -> its first people
        std::vector<std::uint32_t> without_career;
        for (unsigned index = 0; index < people; ++index) {
            const auto person = scenario.person(index);
            auto& squad = squads[scenario.r32(person + 0x24)];
            if (squad.size() < 30) squad.push_back(person);
            if (without_career.size() < 3 && scenario.r32(person + 0x14) == 0) without_career.push_back(person);
        }
        const auto frame_base = sp - 0x1b0, array_at = frame_base + 0x74;
        const auto status_table = scenario.r32(scenario.toc(-0x7b9c));
        const auto contract_scratch = scratch_base + 0x4000;
        const auto offered_people = [&] {
            std::vector<std::uint32_t> result;
            for (const auto offer : offers) if (const auto index = scenario.r32(offer + 0xc); index < people) result.push_back(scenario.person(index));
            return result;
        }();
        // Targets whose seller row names several people of one club: that club can succeed in more than one group.
        std::vector<std::pair<unsigned, unsigned>> ranked_targets;  // (best club frequency, person index)
        for (unsigned index = 3; index < people && index < 60000; index += 29) {
            const auto seller = scenario.r32(scenario.person(index) + 0x24);
            if (!seller) continue;
            const auto row_list = scenario.r32(scenario.r32(scenario.r32(manager + 0x988) + 0xdc) + scenario.r32(seller) * 0xc);
            if (!row_list) continue;
            std::map<std::uint32_t, unsigned> frequency;
            unsigned best = 0;
            for (unsigned entry = 0; entry < 25; ++entry) {
                const auto member = scenario.r32(row_list + entry * 0x2c + 0x10);
                if (member >= people) continue;
                if (const auto club = scenario.r32(scenario.person(member) + 0x24); club && club != seller) best = std::max(best, ++frequency[club]);
            }
            ranked_targets.push_back({best, index});
        }
        std::sort(ranked_targets.rbegin(), ranked_targets.rend());
        for (unsigned k = 0; k < 30 && k < ranked_targets.size(); ++k) {
            const auto person_index = ranked_targets[k].second;
            const auto person = scenario.person(person_index);
            const auto seller = scenario.r32(person + 0x24);
            if (!seller) continue;
            const auto row = scenario.r32(scenario.r32(manager + 0x988) + 0xdc) + scenario.r32(seller) * 0xc;
            const auto list = scenario.r32(row);
            std::vector<std::uint32_t> buyers;
            {
                // The clubs of the row's people, the most frequent first: a club with several scouts in the row
                // can succeed in more than one group.
                std::map<std::uint32_t, unsigned> frequency;
                for (unsigned entry = 0; list && entry < 25; ++entry) {
                    const auto index = scenario.r32(list + entry * 0x2c + 0x10);
                    if (index >= people) continue;
                    if (const auto club = scenario.r32(scenario.person(index) + 0x24); club && club != seller) ++frequency[club];
                }
                std::vector<std::pair<unsigned, std::uint32_t>> ranked;
                for (const auto& [club, count] : frequency) ranked.push_back({count, club});
                std::sort(ranked.rbegin(), ranked.rend());
                for (std::size_t r = 0; r < ranked.size() && r < 2; ++r) buyers.push_back(ranked[r].second);
            }
            for (std::size_t j = 0; j < buyers.size(); ++j) {
                const auto buyer = buyers[j];
                const auto& squad = squads[buyer];
                if (squad.empty()) continue;
                const auto terms = offers.empty() ? scratch_base + 0x100 : offers[k % offers.size()];
                auto candidates = [&](std::vector<std::uint32_t> chosen) {
                    Writes writes = {{1, row + 4, 1}};
                    for (unsigned slot = 0; slot < 50; ++slot) writes.push_back({4, array_at + slot * 4, slot < chosen.size() ? chosen[slot] : 0});
                    return writes;
                };
                const std::vector<std::pair<unsigned, std::uint32_t>> registers = {{3, manager}, {4, buyer}, {5, person}, {6, terms}, {7, out_offer}, {8, out_reaction}, {9, 40'000'000u}};
                const auto add = [&](const char* what, Writes writes, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers, std::vector<Case::NthAnswer> nth, unsigned detail) {
                    answers.insert(answers.begin(), {0x100d06f0, 0});
                    cases.push_back({.entry = 0x103eed00, .registers = registers, .memory = writes, .label = tag(what, k * 10 + unsigned(j), detail), .answers = answers, .nth_answers = nth});
                };
                std::vector<std::uint32_t> some(squad.begin(), squad.begin() + std::min<std::size_t>(squad.size(), 30));
                add("plan chosen", candidates(some), {}, {}, 0);
                // The reaction byte already high or negative before an increment (written when the rating is asked),
                // the first scout's value answered 0, and the allocation of the offer's list failing.
                for (const std::uint32_t reaction : {0x60u, 0x7au}) {
                    cases.push_back({.entry = 0x103eed00, .registers = registers, .memory = candidates(some),
                                     .label = tag("plan reaction", k * 10 + unsigned(j), reaction),
                                     .answers = {{0x100d06f0, 0}}, .pokes = {{0x10369080, {{1, out_reaction, reaction}}}}});
                }
                add("plan zero first value", candidates(some), {}, {{0x10211e70, 1, 0}}, 0);
                for (unsigned nth = 2; nth <= 5; ++nth) add("plan alloc fails", candidates(some), {}, {{0x10773520, nth, 0}}, nth);
                add("plan duplicate", candidates({squad[0], squad[0], squad[0], squad[0]}), {}, {}, 0);
                for (const std::uint32_t side : {0u, 1u, 2u, 3u, 4u, 5u, 0xffffffffu})
                    add("plan side", candidates(some), {{0x100ce0a0, side}}, {}, side);
                add("plan no career", candidates({without_career.empty() ? squad[0] : without_career[0], squad[0]}), {}, {}, 0);
                {
                    Writes writes = candidates(some);
                    for (const auto candidate : some) writes.push_back({1, status_table + scenario.r32(candidate) * 0x64 + 0xc, 4});
                    add("plan status", writes, {}, {}, 0);
                }
                for (unsigned nth = 3; nth <= 9; ++nth) {
                    // The 5th and 6th dispatches of 0x10082f90 are not at the same place in the original and in the
                    // native 0x103f83c0, so an answer there reaches different calls: not a valid comparison.
                    if (nth == 5 || nth == 6) continue;
                    Writes writes = candidates(some);
                    writes.push_back({1, contract_scratch + 0x38, 0x14});
                    writes.push_back({4, contract_scratch + 4, 3});
                    add("plan kind5", writes, {}, {{0x10082f90, nth, contract_scratch}}, nth);
                    Writes plain = candidates(some);
                    plain.push_back({1, contract_scratch + 0x38, 0});
                    plain.push_back({1, contract_scratch + 0x60, 3});
                    plain.push_back({4, contract_scratch + 4, 3});
                    add("plan scratch contract", plain, {}, {{0x10082f90, nth, contract_scratch}}, nth);
                }
                // The early checks refusing through the primary (club) and secondary collectors, with one listed offer.
                if (offers.size() >= 4 && some.size() >= 2) {
                    const auto status_rows = scenario.r32(manager + 0x9a0);
                    for (const auto matching : {true, false}) {
                        // primary: offers[1] belongs to the buyer club and lists every candidate
                        {
                            Writes writes = candidates(some);
                            std::vector<std::array<std::uint32_t, 2>> members;
                            for (const auto candidate : some) {
                                const auto idx = scenario.r32(candidate);
                                members.push_back({idx, 0});
                                writes.push_back({1, status_rows + idx * 10 + 2, 1});
                            }
                            const auto list = make_scratch_list(scratch_base + 0x6000, members, writes);
                            add_to(writes, {{4, offers[1] + 8, scenario.r32(buyer)}, {1, offers[1] + 0x34, 1}, {4, offers[1] + 4, list}});
                            cases.push_back({.entry = 0x103eed00,
                                             .registers = {{3, manager}, {4, buyer}, {5, person}, {6, matching ? offers[1] : offers[3]}, {7, out_offer}, {8, out_reaction}, {9, 40'000'000u}},
                                             .memory = writes, .label = tag("plan refused primary", k * 10 + unsigned(j), matching), .answers = {{0x100d06f0, 0}}});
                        }
                        // secondary: manager+0xc holds one offer (offers[1]) of the seller club, slot 2, listing the target
                        {
                            Writes writes = candidates(some);
                            const auto list = make_scratch_list(scratch_base + 0x6000, {{person_index, 0}}, writes);
                            const auto array = scratch_base + 0x5000;
                            add_to(writes, {{4, offers[1] + 8, scenario.r32(seller)}, {4, offers[1] + 0x28, 2}, {4, offers[1] + 4, list}, {1, offers[2] + 0x34, 1},
                                            {4, manager + 0xc, array}, {4, manager + 0x10, 1}, {4, array, offers[1]}});
                            cases.push_back({.entry = 0x103eed00,
                                             .registers = {{3, manager}, {4, buyer}, {5, person}, {6, matching ? offers[2] : offers[3]}, {7, out_offer}, {8, out_reaction}, {9, 40'000'000u}},
                                             .memory = writes, .label = tag("plan refused secondary", k * 10 + unsigned(j), matching), .answers = {{0x100d06f0, 0}}});
                        }
                    }
                }
            }
        }
        // Candidates that already have open offers: the buyer is the offered person's club, the seller row's first
        // entry names the offered person as the scout, and the terms are that person's own offer.
        for (std::size_t o = 0; o < offers.size() && o < 15; ++o) {
            const auto index = scenario.r32(offers[o] + 0xc);
            if (index >= people) continue;
            const auto offered = scenario.person(index);
            const auto buyer = scenario.r32(offered + 0x24);
            if (!buyer) continue;
            const auto& squad = squads[buyer];
            for (unsigned t = 0; t < 4; ++t) {
                const auto person = scenario.person((index * 7 + 1000 * (t + 1) + 13) % people);
                const auto seller = scenario.r32(person + 0x24);
                if (!seller || seller == buyer) continue;
                const auto row = scenario.r32(scenario.r32(manager + 0x988) + 0xdc) + scenario.r32(seller) * 0xc;
                const auto list = scenario.r32(row);
                if (!list) continue;
                Writes writes = {{1, row + 4, 1}, {4, list + 0x10, index}};
                std::vector<std::uint32_t> chosen = {offered};
                for (const auto member : squad) if (member != offered && chosen.size() < 6) chosen.push_back(member);
                for (unsigned slot = 0; slot < 50; ++slot) writes.push_back({4, array_at + slot * 4, slot < chosen.size() ? chosen[slot] : 0});
                for (const std::uint32_t terms : {offers[o], offers[(o + 1) % offers.size()]})
                    cases.push_back({.entry = 0x103eed00,
                                     .registers = {{3, manager}, {4, buyer}, {5, person}, {6, terms}, {7, out_offer}, {8, out_reaction}, {9, 40'000'000u}},
                                     .memory = writes, .label = tag("plan offered", unsigned(o), t), .answers = {{0x100d06f0, 0}}});
            }
        }
        // The allocation of the offer's list failing.
        for (unsigned nth = 2; nth <= 3; ++nth) {
            const auto person = scenario.person(11 % people);
            const auto seller = scenario.r32(person + 0x24);
            if (!seller) break;
            const auto row = scenario.r32(scenario.r32(manager + 0x988) + 0xdc) + scenario.r32(seller) * 0xc;
            const auto list = scenario.r32(row);
            if (!list) break;
            const auto buyer_person = scenario.r32(list + 0x10);
            if (buyer_person >= people) break;
            const auto buyer = scenario.r32(scenario.person(buyer_person) + 0x24);
            const auto& squad = squads[buyer];
            if (squad.empty()) break;
            Writes writes = {{1, row + 4, 1}};
            for (unsigned slot = 0; slot < 50; ++slot) writes.push_back({4, array_at + slot * 4, slot < squad.size() ? squad[slot] : 0});
            cases.push_back({.entry = 0x103eed00, .registers = {{3, manager}, {4, buyer}, {5, person}, {6, offers.empty() ? scratch_base + 0x100 : offers[0]}, {7, out_offer}, {8, out_reaction}, {9, 40'000'000u}},
                             .memory = writes, .label = tag("plan list alloc fails", nth), .answers = {{0x100d06f0, 0}}, .nth_answers = {{0x10773520, nth, 0}}});
        }
    }

    // Some snapshots (same-save-continue) carry no TOC pointer in r2; the routines load their globals through it.
    for (auto i = first_case; i < cases.size(); ++i) cases[i].registers.insert(cases[i].registers.begin(), {2, 0x108bba90});
}
}
