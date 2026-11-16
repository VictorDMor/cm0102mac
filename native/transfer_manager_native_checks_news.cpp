// Cases for transfer_manager_native_news.cpp: the transfer news items about an offer (0xfa1 ... 0xfaf).
#include "transfer_manager_native_checks.hpp"

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t offer_size = 0x54;
constexpr std::uint32_t find_contract = 0x10082f20, news_accepts = 0x102cdbf0, find_contract_for_club = 0x1007f720;

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
void add(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }
// A copy of a real offer in the scratch area.
std::vector<Write> offer_copy(const Scenario& scenario, std::uint32_t real, std::uint32_t at) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < offer_size; offset += 4) writes.push_back({4, at + offset, scenario.r32(real + offset)});
    return writes;
}
// A contract record: club at +4, halfwords +0x30/+0x32, state byte +0x38 (state is the byte shifted right by 2), role +0x60.
std::vector<Write> contract_record(std::uint32_t at, std::uint32_t club_index, std::uint32_t state_byte, std::uint32_t from = 0x1234,
                                   std::uint32_t until = 0x2345, std::uint32_t role = 0x13) {
    return {{4, at, 0x77}, {4, at + 4, club_index}, {2, at + 0x30, from}, {2, at + 0x32, until}, {1, at + 0x38, state_byte}, {1, at + 0x60, role}};
}
// A list of the shape 0x10738e00 walks: header (count halfword +0, head +4), nodes (data +0, next +8), one word each.
std::vector<Write> list_of(std::uint32_t at, const std::vector<std::uint32_t>& words) {
    std::vector<Write> writes;
    writes.push_back({2, at, std::uint32_t(words.size())});
    writes.push_back({4, at + 4, words.empty() ? 0 : at + 0x20});
    writes.push_back({4, at + 0xc, 0});
    for (std::size_t i = 0; i < words.size(); ++i) {
        const auto node = at + 0x20 + std::uint32_t(i) * 0x10, data = at + 0x100 + std::uint32_t(i) * 4;
        writes.push_back({4, node, data});
        writes.push_back({4, node + 8, i + 1 < words.size() ? node + 0x10 : 0});
        writes.push_back({4, data, words[i]});
    }
    return writes;
}
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
// The same answers, with the news desk accepting every club (so 0x103fb840 finds recipients) when `accepts` is set.
Answers accepting(Answers answers, bool accepts) {
    if (accepts) answers.push_back({news_accepts, 1});
    return answers;
}
std::string label(const char* what, std::uint32_t a, unsigned b = 0, unsigned c = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c);
}
// The lookup of a person's contract answered with a record, or with none.
struct World {
    const char* name;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
};
}

TRANSFER_MANAGER_NATIVE_CASES("news") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto offers = real_offers(scenario, 12);
    if (offers.empty()) return;
    const auto people = scenario.people();
    const auto clubs_known = 40u;
    const auto live_a = scratch_base, released = scratch_base + 0x100, live_b = scratch_base + 0x200;
    std::vector<Write> contracts;
    add(contracts, contract_record(live_a, 5, 0x00));
    add(contracts, contract_record(released, 7, 0x14));
    add(contracts, contract_record(live_b, 9, 0x08, 0x7ffe, 0x4000, 0x25));
    // The contract lookup seen as the game gives it (no answer), absent, live, released.
    const std::vector<World> worlds = {{"real", {}}, {"none", {{find_contract, 0}}}, {"live", {{find_contract, live_a}}},
                                       {"released", {{find_contract, released}}}, {"live-b", {{find_contract, live_b}}}};
    const auto offer_at = scratch_base + 0x400, other_offer_at = scratch_base + 0x500, list_at = scratch_base + 0x1000,
               row_at = scratch_base + 0x800, record_at = scratch_base + 0x900;

    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto offer = offers[i];
        const auto other_offer = offers[(i + 1) % offers.size()];
        const auto person = scenario.person((unsigned(i) * 37u) % people);
        for (const auto& world : worlds) {
            // 0x103ffe80: outcome as a halfword (6 with a flagged offer clears the club), kind 2 or other.
            for (const std::uint32_t outcome : {0u, 6u, 0x12340006u, 0xffff8001u}) {
                for (const std::uint32_t kind : {0u, 2u, 3u}) {
                    for (const std::uint32_t flag : {0u, 1u}) {
                        if (i >= 4 && (kind == 3 || flag == 1) && outcome != 6) continue;
                        std::vector<Write> memory = contracts;
                        add(memory, offer_copy(scenario, offer, offer_at));
                        memory.push_back({1, offer_at + 0x34, kind});
                        memory.push_back({1, offer_at + 0x37, flag});
                        for (const bool accepts : {false, true}) {
                            if (!accepts && i >= 4) continue;
                            cases.push_back({.entry = 0x103ffe80, .registers = {{3, manager}, {4, offer_at}, {5, outcome}}, .memory = memory,
                                             .label = label("fa1", unsigned(i), kind * 2 + flag, outcome ? 1 : 0) + world.name + (accepts ? "+" : ""),
                                             .answers = accepting(world.answers, accepts)});
                        }
                    }
                }
                cases.push_back({.entry = 0x103ffe80, .registers = {{3, manager}, {4, offer}, {5, outcome}}, .memory = contracts,
                                 .label = label("fa1 real", unsigned(i), outcome ? 1 : 0) + world.name, .answers = accepting(world.answers, i % 2)});
            }
            // 0x104019d0: two offers, the first with a list of people (none, one, five).
            for (const unsigned listed : {0u, 1u, 5u}) {
                std::vector<Write> memory = contracts;
                add(memory, offer_copy(scenario, offer, offer_at));
                add(memory, offer_copy(scenario, other_offer, other_offer_at));
                if (listed) {
                    std::vector<std::uint32_t> words;
                    for (unsigned k = 0; k < listed; ++k) words.push_back((unsigned(i) * 11u + k * 977u) % people);
                    add(memory, list_of(list_at, words));
                    memory.push_back({4, offer_at + 4, list_at});
                } else {
                    memory.push_back({4, offer_at + 4, 0});
                }
                for (const bool accepts : {false, true})
                    cases.push_back({.entry = 0x104019d0, .registers = {{3, manager}, {4, offer_at}, {5, other_offer_at}}, .memory = memory,
                                     .label = label("fa2", unsigned(i), listed) + world.name + (accepts ? "+" : ""),
                                     .answers = accepting(world.answers, accepts)});
                // 0x10402b90: the same offer; the news desk accepts the club or not.
                for (const std::uint32_t accepts : {0u, 1u}) {
                    auto answers = world.answers;
                    answers.push_back({news_accepts, accepts});
                    cases.push_back({.entry = 0x10402b90, .registers = {{4, offer_at}}, .memory = memory,
                                     .label = label("fac", unsigned(i), listed, accepts) + world.name, .answers = answers});
                }
            }
            // 0x10403890: the offer and another person, with a word.
            for (const bool accepts : {false, true})
                cases.push_back({.entry = 0x10403890, .registers = {{3, manager}, {4, offer}, {5, person}, {6, 0x1234 + unsigned(i)}}, .memory = contracts,
                                 .label = label("faf", unsigned(i)) + world.name + (accepts ? "+" : ""), .answers = accepting(world.answers, accepts)});
            // 0x10406910 and 0x10406e00: rows about a person and two clubs.
            for (const std::uint32_t flag : {0u, 1u, 0x180u, 0x100u}) {
                std::vector<Write> memory = contracts;
                const auto person_index = (unsigned(i) * 37u) % people;
                memory.push_back({1, row_at, 0x81});
                memory.push_back({4, row_at + 4, unsigned(i) % clubs_known});
                memory.push_back({4, row_at + 8, (unsigned(i) * 3 + 1) % clubs_known});
                memory.push_back({4, row_at + 0xc, person_index});
                memory.push_back({4, record_at + 8, 0x5555 + flag});
                cases.push_back({.entry = 0x10406910, .registers = {{3, manager}, {4, row_at}, {5, 0x3e9 + unsigned(i)}, {6, record_at}, {7, flag}},
                                 .memory = memory, .label = label("fa7", unsigned(i), flag) + world.name, .answers = world.answers});
                std::vector<Write> second = contracts;
                second.push_back({4, row_at, person_index});
                second.push_back({4, row_at + 4, unsigned(i) % clubs_known});
                second.push_back({4, row_at + 0xc, (unsigned(i) * 5 + 2) % clubs_known});
                for (const std::uint32_t done : {0u, 1u})
                    cases.push_back({.entry = 0x10406e00, .registers = {{3, manager}, {4, row_at}, {5, 0x400 + unsigned(i)}, {6, flag}, {7, done}},
                                     .memory = second, .label = label("fa6", unsigned(i), flag, done) + world.name, .answers = world.answers});
            }
        }
    }

    // 0x10407460: the offer's flags (+0x37, state +0x38 with 3 in its top nibble), the list, the news desk's answer, the date test
    // against the contract's halfwords, the person's +0x14 link, and the cursor of the draw table.
    const auto date = scenario.r32(scenario.toc(-0x7b80));
    const auto date_low = date & 0xffff, date_high = date >> 16;
    const auto cursor_cell = scenario.toc(-0x7b84), limit = scenario.r32(scenario.toc(-0x7b88));
    struct Date { std::uint32_t from, until; };
    const Date dates[] = {{0x1234, std::uint32_t(date_low + 1)},                  // until is ahead of the date
                          {0x1234, std::uint32_t(date_low > 0 ? date_low - 1 : 0xffff)},  // behind it
                          {0x0000, date_low}, {0x8000, date_low}, {std::uint32_t(date_high), date_low},
                          {std::uint32_t(date_high ^ 0x7fff), date_low}, {0x7fff, date_low}};
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto offer = offers[i];
        for (const auto& world : {worlds[0], worlds[1], worlds[2], worlds[3]}) {
            for (const std::uint32_t flag : {0u, 1u}) {
                for (const std::uint32_t state : {0x00u, 0x30u, 0x3fu}) {
                    for (const unsigned listed : {0u, 2u, 4u}) {
                        for (const std::uint32_t accepts : {0u, 1u}) {
                            if (i >= 3 && (state != 0x30 || listed == 2)) continue;
                            std::vector<Write> memory = contracts;
                            add(memory, offer_copy(scenario, offer, offer_at));
                            memory.push_back({1, offer_at + 0x37, flag});
                            memory.push_back({1, offer_at + 0x38, state});
                            if (listed) {
                                std::vector<std::uint32_t> words;
                                for (unsigned k = 0; k < listed; ++k) words.push_back((unsigned(i) * 13u + k * 331u) % people);
                                add(memory, list_of(list_at, words));
                                memory.push_back({4, offer_at + 4, list_at});
                            } else {
                                memory.push_back({4, offer_at + 4, 0});
                            }
                            // The draw: no wrap, then a wrap (the cursor passes the table's end).
                            memory.push_back({4, cursor_cell, accepts ? limit - 4 : limit - 1});
                            auto answers = world.answers;
                            answers.push_back({news_accepts, accepts});
                            cases.push_back({.entry = 0x10407460, .registers = {{3, manager}, {4, offer_at}}, .memory = memory,
                                             .label = label("fa8", unsigned(i), flag * 4 + state, listed * 2 + accepts) + world.name,
                                             .answers = answers});
                        }
                    }
                }
            }
        }
        // The date test against the contract's halfwords.
        for (const auto& d : dates) {
            std::vector<Write> memory;
            add(memory, contract_record(live_a, 5, 0x00, d.from, d.until));
            add(memory, offer_copy(scenario, offer, offer_at));
            memory.push_back({1, offer_at + 0x37, 1});
            memory.push_back({1, offer_at + 0x38, 0x30});
            memory.push_back({4, offer_at + 4, 0});
            memory.push_back({4, cursor_cell, limit - 4});
            cases.push_back({.entry = 0x10407460, .registers = {{3, manager}, {4, offer_at}}, .memory = memory,
                             .label = label("fa8 date", unsigned(i), d.from & 0xffff, d.until & 0xffff),
                             .answers = {{find_contract, live_a}, {news_accepts, 1}}});
            if (i >= 3) break;
        }
    }
    // The person's +0x14 link at zero, for the draw's last fields.
    for (std::size_t i = 0; i < offers.size() && i < 6; ++i) {
        const auto offer_person = scenario.person(scenario.r32(offers[i] + 0xc) % people);
        std::vector<Write> memory = contracts;
        add(memory, offer_copy(scenario, offers[i], offer_at));
        memory.push_back({4, offer_at + 4, 0});
        memory.push_back({4, offer_at + 0xc, scenario.r32(offers[i] + 0xc) % people});
        memory.push_back({4, offer_person + 0x14, 0});
        cases.push_back({.entry = 0x10407460, .registers = {{3, manager}, {4, offer_at}}, .memory = memory,
                         .label = label("fa8 unlinked", unsigned(i)), .answers = {{find_contract, live_a}, {news_accepts, 0}}});
    }

    // 0x10403e20: the person, the club, the manager's other club, the kind, the offer, a contract and a list of clubs.
    const std::uint32_t kinds[] = {1, 2, 3, 4, 0x182};
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto person = scenario.person((unsigned(i) * 37u) % people);
        const auto club = scenario.club((unsigned(i) * 3u) % clubs_known), other_club = scenario.club((unsigned(i) * 5u + 1) % clubs_known);
        for (const std::uint32_t kind : kinds) {
            for (const unsigned listed : {0u, 2u, 3u, 5u, 8u}) {
                for (const unsigned contract_variant : {0u, 1u, 2u}) {
                    if (i >= 4 && (kind > 3 || listed == 2 || listed == 8)) continue;
                    if (kind != 2 && listed != 0 && listed != 5) continue;
                    std::vector<Write> memory = contracts;
                    add(memory, offer_copy(scenario, offers[i], offer_at));
                    std::vector<std::uint32_t> words;
                    for (unsigned k = 0; k < listed; ++k) words.push_back((unsigned(i) * 7u + k * 3u) % clubs_known);
                    add(memory, list_of(list_at, words));
                    // contract_variant 0: no contract argument, the lookup by club finds one or none; 1: the record at hand.
                    const std::uint32_t contract_arg = contract_variant == 1 ? live_a : 0;
                    const std::uint32_t list_arg = listed ? list_at : (i % 2 ? 0 : list_at);
                    for (const unsigned who : {0u, 1u}) {
                        if (who == 1 && (contract_variant != 0 || kind != 2)) continue;
                        Answers answers;
                        if (contract_variant == 0) answers.push_back({find_contract_for_club, who ? live_b : 0});
                        else if (contract_variant == 2) answers.push_back({find_contract_for_club, 0});
                        answers.push_back({news_accepts, 1});
                        cases.push_back({.entry = 0x10403e20,
                                         .registers = {{3, manager}, {4, person}, {5, club}, {6, who ? other_club : 0}, {7, kind},
                                                       {8, offer_at}, {9, contract_arg}, {10, list_arg}},
                                         .memory = memory, .label = label("fa4", unsigned(i), kind * 16 + listed, contract_variant * 2 + who),
                                         .answers = answers});
                    }
                }
            }
        }
    }
    // 0x10407460: the n-th contract lookup of the call finds nothing (the one before the date test among them).
    for (unsigned nth = 1; nth <= 16; ++nth) {
        std::vector<Write> memory = contracts;
        add(memory, offer_copy(scenario, offers[0], offer_at));
        memory.push_back({1, offer_at + 0x37, 1});
        memory.push_back({1, offer_at + 0x38, 0x30});
        memory.push_back({4, offer_at + 4, 0});
        memory.push_back({4, cursor_cell, limit - 4});
        cases.push_back({.entry = 0x10407460, .registers = {{3, manager}, {4, offer_at}}, .memory = memory,
                         .label = label("fa8 nth", nth), .answers = {{find_contract, live_a}, {news_accepts, 1}},
                         .nth_answers = {{find_contract, nth, 0}}});
    }
    // 0x10403e20 corners: a null club, the club equal to the offer's, an offer of type 1, no recipients, and
    // lists that hold the recipients themselves (so candidates are dropped).
    for (unsigned k = 0; k < 24; ++k) {
        const auto person = scenario.person((k * 53u) % people);
        const auto club = (k % 6 == 5) ? 0u : scenario.club(k % clubs_known);
        std::vector<Write> memory = contracts;
        add(memory, offer_copy(scenario, offers[k % offers.size()], offer_at));
        if (k % 3 == 0 && club) memory.push_back({4, offer_at + 8, scenario.r32(club)});
        if (k % 4 == 1) memory.push_back({1, offer_at + 0x24, 1});
        std::vector<std::uint32_t> words;
        for (unsigned j = 0; j < 2 + k % 7; ++j) words.push_back((k + j) % clubs_known);
        add(memory, list_of(list_at, words));
        for (const std::uint32_t kind : {2u, 3u}) {
            cases.push_back({.entry = 0x10403e20,
                             .registers = {{3, manager}, {4, person}, {5, club}, {6, k % 2 ? scenario.club((k + 3) % clubs_known) : 0}, {7, kind},
                                           {8, offer_at}, {9, k % 5 == 0 ? live_a : 0}, {10, list_at}},
                             .memory = memory, .label = label("fa4 corner", k, kind),
                             .answers = {{find_contract_for_club, k % 2 ? live_b : 0}, {news_accepts, k % 8 == 7 ? 0u : 1u}}});
        }
    }
    // Candidate ratings at the bound (every rating 0x7fff): the challenger writes the word before the slots.
    for (unsigned k = 0; k < 3; ++k) {
        std::vector<Write> memory = contracts;
        add(memory, offer_copy(scenario, offers[0], offer_at));
        std::vector<std::uint32_t> words = {1, 2, 3, 4 + k, 5};
        add(memory, list_of(list_at, words));
        for (std::uint32_t club_index = 1; club_index <= 8; ++club_index)
            memory.push_back({2, scenario.club(club_index) + 0x1ec, k == 0 ? 0x7fff : (k == 1 ? 0x2711 : 0x2712 + club_index)});
        cases.push_back({.entry = 0x10403e20,
                         .registers = {{3, manager}, {4, scenario.person(3)}, {5, scenario.club(2)}, {6, scenario.club(3)}, {7, 2},
                                       {8, offer_at}, {9, live_a}, {10, list_at}},
                         .memory = memory, .label = label("fa4 bound", k), .answers = {{news_accepts, 1}}});
    }
}
}
