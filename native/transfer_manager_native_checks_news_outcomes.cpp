// Cases for transfer_manager_native_news_outcomes.cpp: the transfer news items built about an offer.
#include "transfer_manager_native_checks.hpp"

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t offer_size = 0x54;
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
// A copy of a real offer in the scratch area whose person (and optionally club) are chosen.
std::vector<Write> offer_copy(const Scenario& scenario, std::uint32_t real, std::uint32_t at, std::uint32_t person_index,
                              std::uint32_t club_index) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < offer_size; offset += 4) writes.push_back({4, at + offset, scenario.r32(real + offset)});
    writes.push_back({4, at + 0xc, person_index});
    writes.push_back({4, at + 8, club_index});
    return writes;
}
// An exchange list (the structure 0x10738e00 walks: cursor word at +0xc, first node at +4; a node holds a
// pointer to an index word at +0 and the next node at +8) of the given person indexes, in the scratch area.
std::vector<Write> person_list(std::uint32_t at, const std::vector<std::uint32_t>& indexes) {
    std::vector<Write> writes;
    const auto nodes = at + 0x100, words = at + 0x8000;
    writes.push_back({4, at + 0xc, indexes.empty() ? 0 : nodes});
    writes.push_back({4, at + 4, indexes.empty() ? 0 : nodes});
    for (std::uint32_t i = 0; i < indexes.size(); ++i) {
        writes.push_back({4, words + i * 4, indexes[i]});
        writes.push_back({4, nodes + i * 16, words + i * 4});
        writes.push_back({4, nodes + i * 16 + 8, i + 1 < indexes.size() ? nodes + (i + 1) * 16 : 0});
    }
    return writes;
}
void add(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }
}

TRANSFER_MANAGER_NATIVE_CASES("news_outcomes") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto offers = real_offers(scenario, 24);
    if (offers.empty()) return;
    const auto people = scenario.people();
    const auto word = scratch_base, shorts = scratch_base + 0x10, record = scratch_base + 0x40;
    const auto copy_at = scratch_base + 0x200;
    const auto describe = [](const char* what, std::uint32_t index, unsigned variant) {
        return std::string(what) + " " + std::to_string(index) + "/" + std::to_string(variant);
    };
    // Real offers, with the short argument varied (the raw register is kept whole).
    const std::uint32_t short_values[] = {0, 1, 0xb, 0xffff8001, 0x12340008};
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto offer = offers[i];
        const auto other = scenario.person((unsigned(i) * 37u) % people);
        for (unsigned v = 0; v < 5; ++v) {
            if (v < 3 || i % 3 == 0) {
                cases.push_back({.entry = 0x10409200, .registers = {{3, manager}, {4, offer}, {5, short_values[v]}}, .label = describe("fab", unsigned(i), v)});
                cases.push_back({.entry = 0x1040e2d0, .registers = {{3, manager}, {4, offer}, {5, short_values[v]}}, .label = describe("fb7", unsigned(i), v)});
                cases.push_back({.entry = 0x1040a050, .registers = {{3, manager}, {4, offer}, {5, other}, {6, short_values[v]}}, .label = describe("fb0", unsigned(i), v)});
                cases.push_back({.entry = 0x1040a050, .registers = {{3, manager}, {4, offer}, {5, 0}, {6, short_values[v]}}, .label = describe("fb0 none", unsigned(i), v)});
            }
            for (const std::uint32_t human : {0u, 1u}) {
                cases.push_back({.entry = 0x104099f0, .registers = {{4, offer}, {5, other}, {6, 0x77 + v}, {7, short_values[v]}},
                                 .label = describe("fad", unsigned(i), v * 2 + human), .answers = {{0x102cdbf0, human}}});
                if (v < 2)
                    cases.push_back({.entry = 0x1040f780, .registers = {{4, offer}, {5, other}, {6, 0x55 + v}},
                                     .label = describe("fbb", unsigned(i), v * 2 + human), .answers = {{0x102cdbf0, human}}});
            }
        }
        // Records the original reads through a pointer: halfwords for fb3 and fb6, a word for fba and a person for fb4.
        std::vector<Write> data = {{2, shorts, 0xfff0}, {2, shorts + 2, 0x8003}, {4, word, 0x1234 + unsigned(i)},
                                   {2, record + 0x30, 0xff80}, {2, record + 0x32, 0xabcd}};
        for (const std::uint32_t human : {0u, 1u}) {
            cases.push_back({.entry = 0x1040c220, .registers = {{3, manager}, {4, offer}, {5, shorts}}, .memory = data,
                             .label = describe("fb3", unsigned(i), human), .answers = {{0x102cdbf0, human}}});
        }
        cases.push_back({.entry = 0x1040ce40, .registers = {{3, manager}, {4, offer}, {5, other}}, .label = describe("fb4", unsigned(i), 0)});
        cases.push_back({.entry = 0x1040ddc0, .registers = {{3, manager}, {4, offer}, {5, record}}, .memory = data, .label = describe("fb6", unsigned(i), 0)});
        cases.push_back({.entry = 0x1040ea40, .registers = {{3, manager}, {4, offer}}, .label = describe("fb9", unsigned(i), 0)});
        cases.push_back({.entry = 0x1040f270, .registers = {{3, manager}, {4, other}, {5, word}, {6, offer}}, .memory = data, .label = describe("fba", unsigned(i), 0)});
        cases.push_back({.entry = 0x104113e0, .registers = {{4, offer}}, .label = describe("fc3", unsigned(i), 0)});
        cases.push_back({.entry = 0x104108c0, .registers = {{4, offers[(i + 1) % offers.size()]}, {5, offer}}, .label = describe("fc0", unsigned(i), 0)});
    }
    // Offers for chosen people (with and without contracts) and clubs, built in the scratch area.
    const auto template_offer = offers[0];
    for (unsigned k = 0; k < 40; ++k) {
        const auto person_index = (k * (people / 40 + 1)) % people;
        const auto person = scenario.person(person_index);
        const auto club_index = (k * 7) % 100;
        std::vector<Write> memory;
        add(memory, offer_copy(scenario, template_offer, copy_at, person_index, club_index));
        add(memory, {{2, shorts, 0x10 + k}, {2, shorts + 2, 3}, {4, word, 0x500 + k}, {2, record + 0x30, k}, {2, record + 0x32, 0x7fff},
                     {4, record + 4, k % 2 ? club_index : 0x1234}});
        const auto sample = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, const char* what) {
            cases.push_back({.entry = entry, .registers = std::move(registers), .memory = memory, .label = describe(what, k, 0)});
        };
        sample(0x10410500, {{4, person}, {5, record}}, "bcc");
        sample(0x104108c0, {{4, copy_at}, {5, copy_at}}, "fc0 own");
        sample(0x104113e0, {{4, copy_at}}, "fc3 built");
        sample(0x1040f780, {{4, copy_at}, {5, person}, {6, 9}}, "fbb built");
        sample(0x104099f0, {{4, copy_at}, {5, person}, {6, 9}, {7, 0xb}}, "fad built");
        sample(0x1040ea40, {{3, manager}, {4, copy_at}}, "fb9 built");
        sample(0x1040c220, {{3, manager}, {4, copy_at}, {5, shorts}}, "fb3 built");
        sample(0x1040ce40, {{3, manager}, {4, copy_at}, {5, person}}, "fb4 built");
        sample(0x1040ddc0, {{3, manager}, {4, copy_at}, {5, record}}, "fb6 built");
        sample(0x1040f270, {{3, manager}, {4, person}, {5, word}, {6, copy_at}}, "fba built");
        sample(0x10409200, {{3, manager}, {4, copy_at}, {5, 3}}, "fab built");
        sample(0x1040a050, {{3, manager}, {4, copy_at}, {5, person}, {6, 0xb}}, "fb0 built");
        sample(0x1040e2d0, {{3, manager}, {4, copy_at}, {5, 4}}, "fb7 built");
    }
    // The people record for bcc's second branch: the record's club word equal to and unlike the person's club.
    for (unsigned k = 0; k < 12; ++k) {
        const auto person = scenario.person((k * 53u) % people);
        for (const std::uint32_t club_word : {0u, 1u, 5u, 40u})
            cases.push_back({.entry = 0x10410500, .registers = {{4, person}, {5, record}}, .memory = {{4, record + 4, club_word + k}},
                             .label = describe("bcc club", k, club_word)});
    }
    // Offers with an exchange list: more than three players, more than 127 (the signed-byte count wraps), few and none usable.
    const auto list_at = scratch_base + 0x2000, contract_at = scratch_base + 0x1f000, club_pick = 3u;
    for (unsigned k = 0; k < 24; ++k) {
        const auto person_index = (k * (people / 24 + 1)) % people;
        std::vector<std::uint32_t> indexes;
        const unsigned length = k < 6 ? 1 + k % 3 : k < 12 ? 4 + k % 3 : k == 12 ? 130 : 2 + k % 5;
        for (unsigned j = 0; j < length; ++j) indexes.push_back((person_index + j * (people / 31 + 1)) % people);
        std::vector<Write> memory;
        add(memory, offer_copy(scenario, offers[k % offers.size()], copy_at, person_index, (k * 5) % 100));
        add(memory, person_list(list_at, indexes));
        memory.push_back({4, copy_at + 4, list_at});
        add(memory, {{2, shorts, 0xfff0}, {2, shorts + 2, 0x8003}});
        for (const std::uint32_t human : {0u, 1u}) {
            cases.push_back({.entry = 0x1040c220, .registers = {{3, manager}, {4, copy_at}, {5, shorts}}, .memory = memory,
                             .label = describe("fb3 list", k, human), .answers = {{0x102cdbf0, human}}});
        }
        cases.push_back({.entry = 0x1040ea40, .registers = {{3, manager}, {4, copy_at}}, .memory = memory, .label = describe("fb9 list", k, 0)});
        cases.push_back({.entry = 0x1040ea40, .registers = {{3, manager}, {4, offers[k % offers.size()]}}, .memory = {}, .label = describe("fb9 real", k, 0)});
    }
    // A contract record the lookup answers with (club index, status byte): a status of 5 counts as none, the other values as the club's.
    for (const std::uint32_t status : {0u, 0x14u, 0x15u, 0x84u}) {
        for (unsigned k = 0; k < 6; ++k) {
            const auto person_index = (k * (people / 6 + 1)) % people;
            std::vector<Write> memory;
            add(memory, offer_copy(scenario, offers[k % offers.size()], copy_at, person_index, (k * 11) % 100));
            add(memory, {{4, contract_at + 4, club_pick + k}, {1, contract_at + 0x38, status}});
            const auto club_id = scenario.r32(scenario.club(club_pick + k));
            memory.push_back({4, record + 4, k % 2 ? club_id : club_id + 1});
            const auto person = scenario.person(person_index);
            // Every lookup answered with the record; a second lookup answered differently (the nth answer) changes the club between them.
            const std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {{0x10082f20, contract_at}};
            const auto with = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, const char* what) {
                cases.push_back({.entry = entry, .registers = std::move(registers), .memory = memory, .label = describe(what, k, status), .answers = answers});
            };
            with(0x10410500, {{4, person}, {5, record}}, "bcc answered");
            with(0x1040a050, {{3, manager}, {4, copy_at}, {5, scenario.club(club_pick + k)}, {6, 0xb}}, "fb0 answered");
            with(0x1040a050, {{3, manager}, {4, copy_at}, {5, 0}, {6, 1}}, "fb0 answered none");
            with(0x1040f780, {{4, copy_at}, {5, person}, {6, 4}}, "fbb answered");
            for (const std::uint32_t human : {0u, 1u})
                cases.push_back({.entry = 0x1040f780, .registers = {{4, copy_at}, {5, person}, {6, 4}}, .memory = memory,
                                 .label = describe("fbb nth", k, status * 2 + human), .answers = answers,
                                 .nth_answers = {{0x102cdbf0, 1, 0}, {0x102cdbf0, 2, human}}});
            // A second lookup that finds a status 5 contract after a first that did not (fb0), or none (the null read).
            for (unsigned nth = 2; nth <= 14; ++nth)
                for (const std::uint32_t second : {0u, contract_at})
                    cases.push_back({.entry = 0x1040a050, .registers = {{3, manager}, {4, offers[k % offers.size()]}, {5, 0}, {6, 1}},
                                     .memory = {{4, contract_at + 4, club_pick}, {1, contract_at + 0x38, 0x14}},
                                     .label = describe("fb0 second", k, nth * 2 + (second != 0)), .nth_answers = {{0x10082f20, nth, second}}});
        }
    }
    // fb9's listed players are counted when 0x103ecef0 accepts them for the person's club (one whose record at +0x2c has 0xe at +0x118):
    // every lookup answered with that club's contract.
    std::vector<std::uint32_t> usable_clubs;
    for (std::uint32_t i = 0; i < 400 && usable_clubs.size() < 12; ++i)
        if (const auto record_at = scenario.r32(scenario.club(i) + 0x2c); record_at != 0 && scenario.s.memory.read<1>(record_at + 0x118) == 0xe) usable_clubs.push_back(i);
    for (unsigned k = 0; k < 48 && !usable_clubs.empty(); ++k) {
        const auto person_index = (k * (people / 48 + 1)) % people;
        std::vector<std::uint32_t> indexes;
        for (unsigned j = 0; j < 1 + k % 4; ++j) indexes.push_back((person_index + j * (people / 17 + 1)) % people);
        std::vector<Write> memory;
        add(memory, offer_copy(scenario, offers[k % offers.size()], copy_at, person_index, usable_clubs[k % usable_clubs.size()]));
        add(memory, person_list(list_at, indexes));
        memory.push_back({4, copy_at + 4, list_at});
        add(memory, {{4, contract_at + 4, usable_clubs[(k / 2) % usable_clubs.size()]}, {1, contract_at + 0x38, 0}});
        cases.push_back({.entry = 0x1040ea40, .registers = {{3, manager}, {4, copy_at}}, .memory = memory,
                         .label = describe("fb9 list answered", k, 0), .answers = {{0x10082f20, contract_at}, {0x102cdbf0, 1}}});
    }
    // One listed person at a time over the whole person table: which ones 0x103ecef0 accepts depends on the person's own record.
    for (std::uint32_t index = 0; index < people; index += people / 220 + 1) {
        std::vector<Write> memory;
        add(memory, offer_copy(scenario, offers[index % offers.size()], copy_at, index, usable_clubs.empty() ? 0 : usable_clubs[index % usable_clubs.size()]));
        add(memory, person_list(list_at, {index, (index + 1) % people}));
        memory.push_back({4, copy_at + 4, list_at});
        add(memory, {{4, contract_at + 4, usable_clubs.empty() ? 0 : usable_clubs[index % usable_clubs.size()]}, {1, contract_at + 0x38, 0}});
        cases.push_back({.entry = 0x1040ea40, .registers = {{3, manager}, {4, copy_at}}, .memory = memory,
                         .label = describe("fb9 person", index, 0), .answers = {{0x10082f20, contract_at}, {0x102cdbf0, 1}}});
    }
    // fb0's second lookup finding a status 5 contract after a first that did not: the nth lookup of the run answered differently.
    for (unsigned k = 0; k < 3; ++k)
        for (unsigned nth = 2; nth <= 16; ++nth) {
            std::vector<Write> memory;
            add(memory, offer_copy(scenario, offers[k % offers.size()], copy_at, (k * (people / 3 + 1)) % people, 5 + k));
            add(memory, {{4, contract_at + 4, 7 + k}, {1, contract_at + 0x38, 0}, {4, contract_at + 0x104, 7 + k}, {1, contract_at + 0x138, 0x14}});
            cases.push_back({.entry = 0x1040a050, .registers = {{3, manager}, {4, copy_at}, {5, 0}, {6, 1}}, .memory = memory,
                             .label = describe("fb0 status 5 second", k, nth), .answers = {{0x102cdbf0, 1}},
                             .nth_answers = {{0x10082f20, 1, contract_at}, {0x10082f20, nth, contract_at + 0x100}}});
        }
    // The shared-cell club of a person that equals a listed club (fb0 skips it when the short is 11).
    for (std::size_t i = 0; i < offers.size(); ++i)
        for (unsigned j = 0; j < (i < 6 ? 100u : 4u); ++j)
            cases.push_back({.entry = 0x1040a050, .registers = {{3, manager}, {4, offers[i]}, {5, scenario.club(j)}, {6, 0xb}},
                             .label = describe("fb0 listed club", unsigned(i), j)});
    // The buyer or seller club listed (a human-managed club's record address) is the very club the short-11 test excludes.
    for (const auto offer : offers)
        for (const std::uint32_t short_value : {0xbu, 1u})
            cases.push_back({.entry = 0x1040a050, .registers = {{3, manager}, {4, offer}, {5, scenario.club(scenario.r32(offer + 8))}, {6, short_value}},
                             .label = describe("fb0 buyer club", offer & 0xffff, short_value), .answers = {{0x102cdbf0, 1}}});

}
}
