// Cases for transfer_manager_native_offers*.cpp: the offers' stage changes, the club's answer, the player's verdict,
// the pending review and the withdrawals, from real offers of the career.
#include "transfer_manager_native_checks.hpp"
#include <string>

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
// The contract of a person, as 0x10082f20 finds it: a row of the per-person table names the record.
std::uint32_t contract_of(const Scenario& scenario, std::uint32_t person_index) {
    const auto rows = scenario.global(-0x7b9c);
    const auto row = std::int32_t(scenario.r32(rows + person_index * 100));
    if (row < 0) return 0;
    return scenario.r32(scenario.global(-0x7b7c)) + std::uint32_t(row) * 100;
}
// Offers with a list of people at +4 (the bidders), from anywhere in the array.
std::vector<std::uint32_t> listed_offers(const Scenario& scenario, unsigned wanted) {
    std::vector<std::uint32_t> offers;
    const auto manager = scenario.transfer_manager();
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    if (size == 0 || size > 100000) return offers;
    for (std::uint32_t i = 0; i < size && offers.size() < wanted; ++i)
        if (const auto offer = scenario.r32(array + i * 4); offer && scenario.r32(offer + 4)) offers.push_back(offer);
    return offers;
}
std::string tag(const char* what, unsigned a, unsigned b = 0) { return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b); }
void add(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }
// The offer's person as a record.
std::uint32_t offer_person(const Scenario& scenario, std::uint32_t offer) { return scenario.person(scenario.r32(offer + 0xc)); }
}

TRANSFER_MANAGER_NATIVE_CASES("offers") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    auto offers = real_offers(scenario, 16);
    if (offers.empty()) return;
    // The offers with bidders take the other half of the list, so that the bidder loops run.
    for (const auto offer : listed_offers(scenario, 12)) offers.push_back(offer);
    const auto people = scenario.people();
    const auto slot_area = scratch_base + 0x1000, link_area = scratch_base + 0x2000;

    // ---- 0x103e3860: draws of every kind of range, and with the table about to run out ----
    {
        const auto cursor_cell = scenario.toc(-0x7b84), last = scenario.global(-0x7b88);
        const std::uint32_t ranges[] = {0, 1, 2, 3, 10, 100, 0x1676, 0xfffe, 0xffff, 0x10000, 0x10001, 0x20000, 1000000, 0x7fffffff,
                                        0xffffffff, 0xfffffffb, 0xffff0001, 0xffff0000, 0xfffe0000, 0xfffffffe, 0xfffe1dc0, 0x80000000,
                                        0x80000001, 0x55555555, 0x19999999, 0x1999999a};
        unsigned k = 0;
        for (const auto range : ranges) {
            cases.push_back({.entry = 0x103e3860, .registers = {{3, range}}, .label = tag("draw", k)});
            // The refill: the cursor at the last position, and one place before (the draw that exactly fits).
            for (const std::uint32_t before : {0u, 2u, 4u})
                cases.push_back({.entry = 0x103e3860, .registers = {{3, range}}, .memory = {{4, cursor_cell, last - before}},
                                 .label = tag("draw refill", k, before)});
            ++k;
        }
    }

    // ---- 0x103e4390, 0x103e4130, 0x103e44b0, 0x103e9910: the stage changes ----
    const std::uint32_t stages[] = {0x00, 0x02, 0x0a, 0x0b, 0x10, 0x11, 0x13};
    const std::uint32_t flags[] = {0, 1, 0x100, 0xffffff00};
    const std::uint32_t reasons[] = {0, 2, 6, 0x10002, 0xffff0002, 8};
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto offer = offers[i], slot = slot_area + std::uint32_t(i) * 4;
        const auto base = std::vector<Write>{{4, slot, offer}};
        for (unsigned v = 0; v < 7; ++v) {
            auto memory = base;
            // Stage 0x13 is the one that clears the people the offer blocked.
            add(memory, {{1, offer + 0x34, stages[v]}});
            const auto flag = flags[(i + v) % 4], reason = reasons[(i + 2 * v) % 6];
            cases.push_back({.entry = 0x103e4390, .registers = {{3, manager}, {4, slot}, {5, flag}, {6, reason}}, .memory = memory, .label = tag("close2", unsigned(i), v)});
            cases.push_back({.entry = 0x103e4130, .registers = {{3, manager}, {4, slot}, {5, flag}, {6, reason}}, .memory = memory, .label = tag("close2 bidders", unsigned(i), v)});
            cases.push_back({.entry = 0x103e44b0, .registers = {{3, manager}, {4, slot}, {5, flag}, {6, reason}}, .memory = memory, .label = tag("close3", unsigned(i), v)});
            cases.push_back({.entry = 0x103e9910, .registers = {{3, manager}, {4, offer}, {5, 0x40 + v}, {6, reasons[v % 6]}, {7, flag}},
                             .memory = {{1, offer + 0x34, stages[v]}}, .label = tag("withdraw", unsigned(i), v)});
        }
    }

    // ---- the same stage changes for offers with bidders: no offer of the career has a list at +4, so copies in
    // the scratch area get one (a list object: head at +4, cursor at +0xc; nodes: a pointer to a person's index at +0,
    // the next node at +8) ----
    for (std::size_t i = 0; i < offers.size(); ++i)
        for (unsigned bidders : {1u, 3u, 6u})
            for (const std::uint32_t stage : {0x13u, 0x02u, 0x0cu, 0x0fu}) {
                const auto source = offers[i];
                const auto built = scratch_base + 0x8000 + std::uint32_t(i) * 0x400, list = built + 0x100, nodes = built + 0x120, indexes = built + 0x1c0;
                const auto slot = built + 0x1f0;
                std::vector<Write> memory = {{4, slot, built}};
                for (std::uint32_t o = 0; o < 0x54; o += 4) memory.push_back({4, built + o, scenario.r32(source + o)});
                memory.push_back({1, built + 0x34, stage});
                memory.push_back({4, built + 4, list});
                memory.push_back({4, list + 4, nodes});
                memory.push_back({4, list + 8, nodes + (bidders - 1) * 0x10});
                memory.push_back({4, list + 0xc, 0});
                for (unsigned b = 0; b < bidders; ++b) {
                    memory.push_back({4, nodes + b * 0x10, indexes + b * 4});
                    memory.push_back({4, nodes + b * 0x10 + 8, b + 1 < bidders ? nodes + (b + 1) * 0x10 : 0});
                    memory.push_back({4, indexes + b * 4, (scenario.r32(source + 0xc) + 1 + b * 97 + unsigned(i) * 1009) % people});
                }
                // Variations: the person's contract removed (the row names no record), or moved to another club.
                const auto person_index = scenario.r32(source + 0xc);
                const auto contract = contract_of(scenario, person_index);
                const auto rows = scenario.global(-0x7b9c);
                if ((i + bidders + stage) % 4 == 0) memory.push_back({4, rows + person_index * 100, 0xffffffff});
                else if (contract) memory.push_back({4, contract + 4, (unsigned(i) * 53 + bidders * 17 + stage) % 250});
                // The contract's club run by one of the last sixteen people (0x1014c2e0 then may exempt it).
                if (contract && (i + stage) % 2 == 0) {
                    const auto contract_club = (i + bidders + stage) % 4 == 0 ? 0u : ((unsigned(i) * 53 + bidders * 17 + stage) % 250);
                    memory.push_back({4, scenario.club((i + bidders + stage) % 4 == 0 ? 0 : contract_club) + 0x90, scenario.person(people - 1 - unsigned(i) % 16)});
                    if (contract_club == 0) memory.push_back({4, scenario.club(scenario.r32(contract + 4)) + 0x90, scenario.person(people - 1 - unsigned(i) % 16)});
                }
                // The offer's club and the first bidder as an entry of the shortlist (0x10362d70 finds it).
                if ((i + bidders + stage) % 3 == 1) {
                    const auto shortlist = scenario.r32(manager + 0x988), entries = built + 0x300;
                    const auto bidder_person = (scenario.r32(source + 0xc) + 1 + unsigned(i) * 1009) % people, club_index = scenario.r32(source + 8);
                    memory.push_back({4, shortlist + 0, entries});
                    memory.push_back({4, shortlist + 0x10, 1});
                    memory.push_back({4, entries, bidder_person});
                    memory.push_back({4, entries + 4, club_index});
                }
                for (const auto entry : {0x103e4130u, 0x103e44b0u})
                    cases.push_back({.entry = entry, .registers = {{3, manager}, {4, slot}, {5, 1}, {6, 2}}, .memory = memory,
                                     .label = tag("close bidders", unsigned(i), bidders * 16 + stage)});
                cases.push_back({.entry = 0x103e9910, .registers = {{3, manager}, {4, built}, {5, 0x40}, {6, 0xb}, {7, 1}}, .memory = memory,
                                 .label = tag("withdraw bidders", unsigned(i), bidders * 16 + stage)});
            }

    // ---- 0x103e5b70: the player's verdict; the traits and ratings are varied to reach each branch ----
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto offer = offers[i];
        const auto person = offer_person(scenario, offer), career = scenario.r32(person + 0x14);
        cases.push_back({.entry = 0x103e5b70, .registers = {{3, manager}, {4, offer}}, .label = tag("verdict", unsigned(i))});
        if (!career) continue;
        // The club's rating (0x1ec) high enough to flag the offer, then a spread of traits and values.
        const auto club = scenario.club(scenario.r32(offer + 8) % 200);
        const std::uint32_t trait_sets[][4] = {{0x7f, 0x01, 0x01, 0x7f}, {0x00, 0x7f, 0x7f, 0x00}, {0x80, 0x10, 0x05, 0x80}, {0x40, 0x0a, 0x08, 0x20}};
        unsigned v = 0;
        for (const auto& traits : trait_sets)
            for (const std::uint32_t value : {0x0e10u, 0x1500u, 0x7fffu})
                for (const std::uint32_t status : {0u, 8u}) {
                    std::vector<Write> memory = {{2, club + 0x1ec, 0x7fff}, {1, offer + 0x35, 1}, {1, person + 0x69, traits[0]},
                                                 {1, person + 0x65, traits[1]}, {1, person + 0x67, traits[2]}, {1, person + 0x63, traits[3]},
                                                 {2, career + 0xa, value}, {1, person + 0x5d, 0x14 + v % 16}};
                    if (const auto contract = contract_of(scenario, scenario.r32(offer + 0xc)); contract && status) add(memory, {{1, contract + 0x5c, 8}});
                    cases.push_back({.entry = 0x103e5b70, .registers = {{3, manager}, {4, offer}}, .memory = memory, .label = tag("verdict deep", unsigned(i), v)});
                    ++v;
                }
    }

    // ---- 0x103e5b70 again, over offers built for people and clubs spread over the tables: the person's value to
    // the club (0x10369080) decides which branch runs, and few of the real offers reach the deep ones ----
    for (unsigned k = 0; k < 1500; ++k) {
        const auto person_index = (k * (people / 1500 + 1) + k / 3) % people;
        const auto person = scenario.person(person_index);
        const auto career = scenario.r32(person + 0x14);
        if (!career) continue;
        const auto club_index = (k * 7 + k / 5) % 250;
        const auto club = scenario.club(club_index);
        const auto built = scratch_base + 0x4000 + (k % 64) * 0x80;
        std::vector<Write> memory;
        const auto source = offers[k % offers.size()];
        for (std::uint32_t o = 0; o < 0x54; o += 4) memory.push_back({4, built + o, scenario.r32(source + o)});
        memory.push_back({4, built + 0xc, person_index});
        memory.push_back({4, built + 8, club_index});
        memory.push_back({4, built + 4, 0});
        memory.push_back({1, built + 0x35, 1});
        memory.push_back({2, club + 0x1ec, 0x7fff});
        // Traits (+0x69, +0x65, +0x67, +0x63), the career rating (+0xa of the record at +0x14) and the byte at +0x5d.
        struct Variant { std::uint32_t traits[4], rating, byte5d; bool unrest; };
        const Variant variants[] = {{{0, 0, 0, 0}, 0, 0, false}, {{0x7f, 0x01, 0x01, 0x7f}, 0x1500, 0x14, true},
                                    {{0x00, 0x7f, 0x7f, 0x00}, 0x7fff, 0x1f, false}, {{0x80, 0x10, 0x05, 0x80}, 0x0e10, 0x18, true},
                                    {{0x7f, 0x00, 0x00, 0x7f}, 0x1500, 0x18, true}, {{0x40, 0x0a, 0x08, 0x20}, 0x2000, 0x13, false},
                                    {{0x20, 0x30, 0x30, 0x20}, 0x1500, 0x15, true}, {{0x10, 0x20, 0x20, 0x30}, 0x1490, 0x1a, false},
                                    {{0x30, 0x08, 0x08, 0x10}, 0x0e00, 0x17, true}, {{0x05, 0x50, 0x10, 0x05}, 0x2400, 0x1e, true}};
        for (unsigned v = 0; v < 18; ++v) {
            auto variant = memory;
            if (v >= 10) {
                // Mild traits (the mood near the thresholds), with the byte at +0x5d outside the window, the person's
                // status row byte set, or the career record missing.
                const std::uint32_t mild[] = {0x28, 0x18, 0x38, 0x08, 0x48, 0x10, 0x30, 0x20};
                const auto t = mild[v - 10];
                add(variant, {{1, person + 0x69, t}, {1, person + 0x65, 0x30 - t / 2}, {1, person + 0x67, 0x10 + t / 2}, {1, person + 0x63, t},
                              {2, career + 0xa, 0x1500u + v * 40u}, {1, person + 0x5d, v % 4 == 0 ? 0x30u : (v % 4 == 1 ? 0x10u : 0x18u)}});
                if (const auto contract = contract_of(scenario, person_index); contract) add(variant, {{1, contract + 0x5c, 8}});
                if (v % 4 == 3) add(variant, {{1, scenario.r32(scenario.global(-0x7b24) + 0xc) + person_index, 1}});
                if (v % 4 == 2) add(variant, {{1, scenario.r32(scenario.transfer_manager() + 0x9a0) + person_index * 10 + 1, 1}});
                if (v == 17) add(variant, {{4, person + 0x14, 0}});
            } else if (v) {
                const auto& t = variants[v];
                add(variant, {{1, person + 0x69, t.traits[0]}, {1, person + 0x65, t.traits[1]}, {1, person + 0x67, t.traits[2]},
                              {1, person + 0x63, t.traits[3]}, {2, career + 0xa, t.rating}, {1, person + 0x5d, t.byte5d}});
                if (const auto contract = contract_of(scenario, person_index); contract && t.unrest) add(variant, {{1, contract + 0x5c, 8}});
                if (v % 2 == 0) add(variant, {{1, scenario.r32(scenario.global(-0x7b24) + 0xc) + person_index, 1}});
            }
            cases.push_back({.entry = 0x103e5b70, .registers = {{3, manager}, {4, built}}, .memory = variant, .label = tag("verdict built", k, v)});
            // The module's constant block with the value limit at 0 and the ceiling out of reach, so that the last
            // comparison (the verdict against the caps) decides; the scale of that comparison is 0 or huge.
            if (v && k % 3 == 0) {
                const auto pool = scenario.toc(-0x4ba0);
                for (const std::uint32_t scale_high : {0u, 0x41cdcd65u}) {
                    auto fixture = variant;
                    add(fixture, {{4, pool + 0x68, 0}, {4, pool + 0x6c, 0}, {4, pool + 0x78, 0x41cdcd65}, {4, pool + 0x7c, 0},
                                  {4, pool + 0x18, scale_high}, {4, pool + 0x1c, 0}});
                    cases.push_back({.entry = 0x103e5b70, .registers = {{3, manager}, {4, built}}, .memory = fixture,
                                     .label = tag("verdict constants", k, v * 2 + (scale_high ? 1 : 0))});
                }
            }
        }
    }

    // ---- 0x103e8500: the club's answer ----
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto offer = offers[i], club_word = slot_area + 0x400 + std::uint32_t(i) * 4;
        const auto club = scenario.r32(offer + 8);
        for (unsigned v = 0; v < 12; ++v) {
            const std::uint32_t answers[] = {0, 1, 2, 5, 255, 3};
            std::vector<Write> memory = {{4, club_word, v % 2 ? club : club + 1}, {1, offer + 0x34, v % 5 == 4 ? 0x11 + v % 2 : stages[v % 7]},
                                         {1, offer + 0x36, answers[(v / 2) % 6]}};
            cases.push_back({.entry = 0x103e8500, .registers = {{3, manager}, {4, offer}, {5, club_word}, {6, (v % 3 == 0) ? 8u : (v % 3 == 1 ? 0u : 0x30006u)}},
                             .memory = memory, .label = tag("answer", unsigned(i), v)});
        }
    }

    // ---- 0x103e5b70 with an offer whose club index puts the club pointer at 0 (the table base plus index * 0x248 wraps) ----
    {
        const auto base = scenario.global(-0x7be8);
        if (base % 8 == 0) {
            std::uint32_t inverse = 1;  // of 73 modulo 2^29, by Newton's steps
            for (int step = 0; step < 6; ++step) inverse *= 2 - 73 * inverse;
            const auto index = ((0u - base) / 8 * inverse) & 0x1fffffff;
            const auto built = scratch_base + 0xe000;
            std::vector<Write> memory;
            for (std::uint32_t o = 0; o < 0x54; o += 4) memory.push_back({4, built + o, scenario.r32(offers[0] + o)});
            memory.push_back({4, built + 8, index});
            memory.push_back({1, built + 0x35, 1});
            cases.push_back({.entry = 0x103e5b70, .registers = {{3, manager}, {4, built}}, .memory = memory, .label = "verdict null club"});
        }
    }

    // ---- 0x103e8500 over offers built for clubs spread over the table (which clubs are exempt from the counter-offer) ----
    for (unsigned k = 0; k < 250; ++k) {
        const auto source = offers[k % offers.size()], built = scratch_base + 0xc000 + (k % 32) * 0x80, club_word = built + 0x70;
        std::vector<Write> memory;
        for (std::uint32_t o = 0; o < 0x54; o += 4) memory.push_back({4, built + o, scenario.r32(source + o)});
        memory.push_back({4, built + 8, k});
        memory.push_back({1, built + 0x34, 0});
        memory.push_back({1, built + 0x36, k % 4 == 0 ? 0u : (k % 9 == 2 ? 255u : 3u)});
        memory.push_back({4, club_word, k % 5 == 0 ? k : k + 1});
        if (const auto contract = contract_of(scenario, scenario.r32(source + 0xc)); contract && k % 10 == 0)
            memory.push_back({4, scenario.club(scenario.r32(contract + 4)) + 0x90, scenario.person(people - 1 - k % 16)});
        if (k % 3 == 0) memory.push_back({4, scenario.club(k) + 0x90, scenario.person(people - 1 - k % 16)});
        if (const auto contract = contract_of(scenario, scenario.r32(source + 0xc)); contract && k % 6 == 1) memory.push_back({1, contract + 0x38, 0x14});
        if (k % 7 == 3) memory.push_back({4, scenario.global(-0x7b9c) + scenario.r32(source + 0xc) * 100, 0xffffffff});
        cases.push_back({.entry = 0x103e8500, .registers = {{3, manager}, {4, built}, {5, club_word}, {6, k % 3 == 0 ? 8u : (k % 3 == 1 ? 6u : 8u)}},
                         .memory = memory, .label = tag("answer built", k)});
    }

    // ---- 0x103e8bb0: the pending review of an offer (a link: person, club, the amount at +8, the kind byte at +0x78) ----
    // Links for real offers, then for people and clubs spread over the tables (which of the permission checks
    // 0x103ecef0 and 0x103ed520 pass depends on them).
    {
        unsigned n = 0;
        const auto make_link = [&](std::uint32_t link, std::uint32_t person_index, std::uint32_t club_index, unsigned v) {
            std::vector<Write> memory = {{4, link, person_index}, {4, link + 4, club_index}, {4, link + 8, 1000 * (v + 1)}, {1, link + 0x78, v}};
            for (std::uint32_t o = 0x0c; o < 0x78; o += 4) memory.push_back({4, link + o, 0});
            return memory;
        };
        for (std::size_t i = 0; i < offers.size(); ++i)
            for (unsigned v = 0; v < 3; ++v) {
                const auto link = link_area + (n++ % 64) * 0x100;
                cases.push_back({.entry = 0x103e8bb0, .registers = {{3, manager}, {4, link}},
                                 .memory = make_link(link, scenario.r32(offers[i] + 0xc), (scenario.r32(offers[i] + 8) + v) % 200, v), .label = tag("review", unsigned(i), v)});
            }
        for (unsigned k = 0; k < 60; ++k)
            for (unsigned j = 0; j < 12; ++j) {
                const auto link = link_area + (n++ % 64) * 0x100;
                const auto person_index = (k * (people / 60 + 1) + 7) % people;
                auto memory = make_link(link, person_index, (j * 17 + k) % 250, (k + j) % 4);
                // Half of them with the contract's kind (the high six bits of the byte at +0x38) at 5.
                if (const auto contract = contract_of(scenario, person_index); contract && (k + j) % 2 == 0)
                    memory.push_back({1, contract + 0x38, 0x14 | ((k + j) & 3)});
                else if (k % 5 == 0) memory.push_back({4, scenario.global(-0x7b9c) + person_index * 100, 0xffffffff});
                cases.push_back({.entry = 0x103e8bb0, .registers = {{3, manager}, {4, link}}, .memory = memory, .label = tag("review spread", k, j)});
            }
    }

    // ---- 0x103e9ac0: every offer of a club, and of the clubs linked to it ----
    for (std::size_t i = 0; i < offers.size(); ++i)
        cases.push_back({.entry = 0x103e9ac0, .registers = {{3, manager}, {4, scenario.club(scenario.r32(offers[i] + 8))}}, .label = tag("withdraw club", unsigned(i))});
    // Clubs whose record lists people (at +0x98, fifty pointers) who have offers; some of the offers protected (+0x37).
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto club = scenario.club(scenario.r32(offers[i] + 8));
        std::vector<Write> memory;
        for (std::size_t b = 0; b < offers.size(); ++b) memory.push_back({4, club + 0x98 + std::uint32_t(b) * 4, offer_person(scenario, offers[(i + b) % offers.size()])});
        for (std::size_t b = 0; b < offers.size(); b += 3) memory.push_back({1, offers[b] + 0x37, 1});
        if (i % 2) memory.push_back({1, offers[i] + 0x34, 0x13});
        cases.push_back({.entry = 0x103e9ac0, .registers = {{3, manager}, {4, club}}, .memory = memory, .label = tag("withdraw linked", unsigned(i))});
    }
    for (unsigned k = 0; k < 60; ++k)
        cases.push_back({.entry = 0x103e9ac0, .registers = {{3, manager}, {4, scenario.club((k * 13 + 1) % 250)}}, .label = tag("withdraw other club", k)});
}
}
