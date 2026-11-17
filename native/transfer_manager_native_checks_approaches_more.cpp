// More cases for transfer_manager_native_approaches*.cpp: tables with holes and several offers of one person,
// offer lists, and the branches that need a contract, a club or a person answered or edited.
#include "guest_abi.hpp"
#include "transfer_manager_native_checks.hpp"
#include <array>
#include <string>

namespace cm::aot::transfer_manager_native_checks {
namespace {
using Writes = std::vector<Write>;
constexpr std::uint32_t table_of_contents = 0x108bba90;
std::vector<std::pair<unsigned, std::uint32_t>> with_toc(std::vector<std::pair<unsigned, std::uint32_t>> registers) {
    registers.insert(registers.begin(), {2, table_of_contents});
    return registers;
}
std::string label(const char* what, unsigned a, unsigned b = 0, unsigned c = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c);
}
std::uint32_t caller_sp(const Scenario& scenario) { return guest::Cpu(scenario.s).sp(); }
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
struct Field { unsigned size; std::uint32_t offset, value; };
std::uint32_t scratch_offer(const Scenario& scenario, Writes& writes, std::uint32_t at, std::uint32_t from,
                            std::initializer_list<Field> fields = {}) {
    for (std::uint32_t offset = 0; offset < 0x54; offset += 4) writes.push_back({4, at + offset, scenario.r32(from + offset)});
    for (const auto& field : fields) writes.push_back({field.size, at + field.offset, field.value});
    return at;
}
// A guest list (header {halfword count, head, tail, cursor}, nodes {data, halfword size, next, prev}, data
// records of two words) built out of real heap blocks: the game frees what it unlinks, so a scratch address
// would not do.
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
void write_double(Writes& writes, std::uint32_t at, std::uint64_t bits) {
    writes.push_back({4, at, std::uint32_t(bits >> 32)});
    writes.push_back({4, at + 4, std::uint32_t(bits)});
}
}

TRANSFER_MANAGER_NATIVE_CASES("approaches") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto people = scenario.people();
    const auto offers = real_offers(scenario, 30);
    if (offers.size() < 21) return;
    const auto scratch = scratch_base + 0x20000;
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    const auto today = scenario.toc(-0x7b80);
    const auto year = scenario.r32(today) >> 16, day = scenario.r32(today) & 0xffff;

    // ---- 0x103e25d0: holes in the table, two other clubs' offers for the person (the shared list present or
    // not), and a list on the source (kinds 0 and 1, the replaced offer's list).
    for (std::size_t i = 0; i < 4; ++i) {
        const auto base = offers[i];
        const auto person = scenario.r32(base + 0xc), club = scenario.r32(base + 8);
        const auto at = scratch + std::uint32_t(i) * 0x600;
        for (std::uint32_t shape = 0; shape < 6; ++shape) {
            Writes memory;
            std::size_t block = 6;
            scratch_offer(scenario, memory, at, base, {{4, 0xc, person}, {4, 8, club + 40}, {1, 0x24, shape % 2}, {4, 0x20, 1}});
            const auto first = scratch_offer(scenario, memory, at + 0x100, base, {{4, 0xc, person}, {4, 8, club + 41}, {4, 0, 0}});
            const auto second = scratch_offer(scenario, memory, at + 0x200, base, {{4, 0xc, person}, {4, 8, club + 42}, {4, 0, 0}});
            memory.push_back({4, array, first});
            memory.push_back({4, array + 4, second});
            memory.push_back({4, array + 12, 0});
            memory.push_back({4, array + 20, 0});
            if (shape >= 2) {
                // A shared list of slots on the first offer: the person's other offers join it.
                const auto shared = make_list(offers, block, {{{0, 0}}}, memory);
                memory.push_back({4, first, shared});
            }
            if (shape >= 4) {
                const auto list = make_list(offers, block, {{{person, 0}}, {{(person + 3) % people, 1}}}, memory);
                memory.push_back({4, at + 4, list});
            }
            if (shape == 1 || shape == 3) {
                // A person with no offers of the manager's yet in the row table: the first-offer case.
                memory.push_back({1, scenario.r32(manager + 0x9a0) + person * 10 + 1, 0});
            }
            Case one{.entry = 0x103e25d0, .registers = with_toc({{3, manager}, {4, at}, {5, offers[i + 8]}}), .memory = memory,
                     .label = label("add offer table", unsigned(i), shape)};
            one.answers.push_back({0x1014c2e0, shape % 3 == 0 ? 1u : 0u});
            cases.push_back(one);
        }
    }
    // A source whose person and club match an entry at the very end of the table, and whose list is real.
    for (std::size_t i = 0; i < 3; ++i) {
        const auto base = offers[10 + i];
        const auto at = scratch + 0x3000 + std::uint32_t(i) * 0x200;
        Writes memory;
        std::size_t block = 14;
        scratch_offer(scenario, memory, at, base);
        const auto list = make_list(offers, block, {{{scenario.r32(base + 0xc), 0}}, {{(scenario.r32(base + 0xc) + 5) % people, 0}}}, memory);
        memory.push_back({4, at + 4, list});
        cases.push_back({.entry = 0x103e25d0, .registers = with_toc({{3, manager}, {4, at}, {5, base}}), .memory = memory,
                         .label = label("add offer list replaced", unsigned(i))});
    }

    // ---- 0x103e2db0: an approach table with entries at its start (the free slot comes later) and an occupied
    // slot, the table being a real heap block (the snapshot has none of its own).
    for (std::size_t i = 0; i < 3; ++i) {
        const auto base = offers[i], block = offers[19];
        const auto at = scratch + 0x4000 + std::uint32_t(i) * 0x200, copy_at = at + 0x100;
        Writes memory;
        scratch_offer(scenario, memory, at, base, {{4, 0x30, i == 2 ? 1u : 0xffffffffu}});
        scratch_offer(scenario, memory, copy_at, base);
        for (std::uint32_t word = 0; word < 21; ++word) memory.push_back({4, block + word * 4, 0});
        memory.push_back({4, block, offers[17]});
        memory.push_back({4, block + 4, offers[16]});
        memory.push_back({4, manager + 0xc, block});
        memory.push_back({4, manager + 0x10, 20});
        memory.push_back({4, manager + 0x14, i == 1 ? 20u : 2u});
        cases.push_back({.entry = 0x103e2db0, .registers = with_toc({{3, manager}, {4, at}, {5, copy_at}, {6, i % 2}}), .memory = memory,
                         .label = label("add approach occupied", unsigned(i))});
    }

    // ---- 0x1040d5c0 and 0x103f3bf0: contracts answered (a scratch contract for the club chosen), a person
    // without a contract, a club level for each of the two lookups, the person's data and the preference.
    {
        const auto contract_at = scratch + 0x8000, row_at = scratch + 0x8100, person_data = scratch + 0x8200;
        const auto pool = scenario.toc(-0x4ba0);
        const auto table = scenario.r32(scenario.toc(-0x7b9c));
        const auto out = scratch + 0x8300;
        unsigned n = 0;
        for (unsigned index = 5; index < people && n < 120; index += people / 60 + 1) {
            ++n;
            const auto person = scenario.person(index);
            for (const std::uint32_t shape : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u}) {
                Writes memory;
                const auto club_index = (index * 7 + shape) % 100;
                const auto club = scenario.club(club_index);
                const std::uint32_t preferences[] = {0x10, 0x00, 0x80, 0x7f, 0x9c, 0x64, 0x0a, 0xf6};
                // The contract: club index at +4, status byte +0x38 (not 5 in the upper six bits).
                memory.push_back({4, contract_at + 4, club_index});
                memory.push_back({1, contract_at + 0x38, shape == 7 ? 0x14u : 0x08u});
                memory.push_back({1, row_at + 0x44, preferences[shape]});
                memory.push_back({2, person_data + 6, shape % 4 == 3 ? 0x40u : 0x200u});
                memory.push_back({4, person + 0x14, person_data});
                memory.push_back({2, person + 0x5c, shape % 5 == 4 ? 0x1au : 0x03u});
                memory.push_back({2, club + 0x1ec, shape % 2 ? 0x7fffu : 0u});
                if (table) memory.push_back({1, table + scenario.r32(person) * 100 + 0xc, shape % 6 == 2 ? 0x30u : 0x20u});
                // The comparison constant: the shortlist value (answered, f1 as it stands) against it.
                write_double(memory, pool + 0xc0, shape % 3 == 0 ? 0xfff0000000000000ull : shape % 3 == 1 ? 0x7ff0000000000000ull : 0x7ff8000000000000ull);
                write_double(memory, pool + 0x140, shape % 2 ? 0x4059000000000000ull : shape % 4 == 2 ? 0x7ff0000000000000ull : 0x4000000000000000ull);
                Writes edited = memory;
                scratch_offer(scenario, edited, scratch + 0x8400, offers[(n + shape) % offers.size()],
                              {{4, 0xc, index}, {4, 8, (club_index + 3) % 100}, {4, 4, 0}, {1, 0x11, 1 + shape % 3}});
                Writes with_list = edited;
                std::size_t block = 6;
                const auto list = make_list(offers, block, {{{index, 0}}}, with_list);
                with_list.push_back({4, scratch + 0x8400 + 4, list});
                for (const bool listed : {false, true}) {
                    Case one{.entry = 0x103f3bf0, .registers = with_toc({{3, manager}, {4, scratch + 0x8400}, {5, out}, {6, out + 4}}),
                             .memory = listed ? with_list : edited, .label = label("first status answered", index, shape, listed)};
                    one.answers.push_back({0x10082f20, contract_at});
                    one.answers.push_back({0x10082f90, row_at});
                    one.answers.push_back({0x100c7a20, 1});
                    one.answers.push_back({0x105c5030, 12});
                    if (shape % 2) one.nth_answers.push_back({0x105c5030, 2, 20u + shape});
                    cases.push_back(one);
                    if (shape % 3 == 0) {
                        Case no_contract = one;
                        no_contract.answers.clear();
                        no_contract.answers.push_back({0x10082f20, 0});
                        no_contract.answers.push_back({0x100c7a20, 1});
                        no_contract.answers.push_back({0x105c5030, 12});
                        no_contract.label += " none";
                        cases.push_back(no_contract);
                    }
                }
                Case news{.entry = 0x1040d5c0, .registers = with_toc({{3, manager}, {4, scratch + 0x8400}, {5, offers[n % offers.size()]}}),
                          .memory = edited, .label = label("offer news answered", index, shape)};
                news.answers.push_back({0x10082f20, shape % 4 == 3 ? 0u : contract_at});
                cases.push_back(news);
                if (shape == 0 && n < 6) {
                    // The second lookup of the person's contract fails (which dispatch it is depends on the
                    // lookups 0x103fb840 makes first).
                    for (unsigned nth = 2; nth < 14; ++nth) {
                        Case failing{.entry = 0x1040d5c0, .registers = with_toc({{3, manager}, {4, offers[(n + nth) % offers.size()]}, {5, offers[n % offers.size()]}}),
                                     .label = label("offer news lookup fails", index, nth)};
                        failing.nth_answers.push_back({0x10082f20, nth, 0});
                        cases.push_back(failing);
                    }
                }
            }
        }
    }

    // ---- 0x103e3de0 and 0x103e3410: the loan club, the squad answer and the date shapes.
    {
        const auto from_at = scratch + 0x9000, to_at = scratch + 0x9100;
        unsigned n = 0;
        for (unsigned index = 3; index < people && n < 60; index += people / 50 + 1, ++n) {
            const auto person = scenario.person(index);
            const auto club = scenario.club((index * 7) % 100);
            for (unsigned shape = 0; shape < 12; ++shape) {
                Writes memory;
                const auto from_year = year, from_day = day;  // not past today
                const auto to_year = year + 1, to_day = day + 3;
                const auto relation = shape % 3;               // recorded date: earlier year, same year/earlier day, same
                const auto recorded_year = relation == 0 ? to_year - 1 : relation == 1 ? to_year + 1 : to_year;
                memory.push_back({2, from_at, from_year});
                memory.push_back({2, from_at + 2, from_day});
                memory.push_back({2, to_at, to_year});
                memory.push_back({2, to_at + 2, to_day});
                memory.push_back({2, person + 0x54, recorded_year});
                memory.push_back({2, person + 0x56, shape % 4 == 1 ? to_day : to_day - (shape % 2)});
                memory.push_back({1, person + 0x6d, shape % 2 ? 2u : 0u});
                memory.push_back({1, caller_sp(scenario) - 0xf0 + 0x38, shape % 5 == 4 ? 1u : 0u});
                Case one{.entry = 0x103e3de0, .registers = with_toc({{3, manager}, {4, person}, {5, club}, {6, from_at}, {7, to_at}}), .memory = memory,
                         .label = label("sign person more", index, shape)};
                one.answers.push_back({0x100c9070, 0});
                one.answers.push_back({0x10083110, 0});
                one.answers.push_back({0x10083e50, 0});
                one.answers.push_back({0x100ce320, shape % 3 == 2 ? scenario.club(1 + shape) : shape % 3 == 1 ? club : 0u});
                one.answers.push_back({0x103ecef0, shape % 2});
                one.answers.push_back({0x104ae570, shape % 4 < 2 ? 1u : 0u});
                cases.push_back(one);
            }
        }
        // 0x103e3410 with the contract answered in its last branch.
        const auto date = scratch + 0x9200;
        for (unsigned index = 2, count = 0; index < people && count < 20; index += people / 20 + 1, ++count) {
            Writes memory;
            memory.push_back({2, date, year});
            memory.push_back({2, date + 2, day});
            for (const std::uint32_t answer : {0u, 1u}) {
                if (answer == 0 && count < 3)
                    for (unsigned nth = 2; nth < 40; ++nth) {
                        Case failing{.entry = 0x103e3410,
                                     .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, scenario.club(count % 100)}, {6, date}, {7, 0}, {8, 0xffffffff}, {9, 2}}),
                                     .memory = memory, .label = label("regen offer lookup fails", index, nth)};
                        failing.nth_answers.push_back({0x10082f20, nth, 0});
                        cases.push_back(failing);
                    }
                Case one{.entry = 0x103e3410,
                         .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, scenario.club(count % 100)}, {6, date}, {7, 0}, {8, 0xffffffff}, {9, 2}}),
                         .memory = memory, .label = label("regen offer answered", index, answer)};
                one.answers.push_back({0x100d33d0, answer});
                cases.push_back(one);
            }
        }
    }
    (void)size;
}
}
